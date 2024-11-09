// Text rendering backend for fire-ui implemented using stb_truetype

#define STB_TRUETYPE_IMPLEMENTATION
#define STBTT_STATIC
#include "stb_truetype.h"

typedef struct UI_FontData {
	const uint8_t* data; // NULL if the font is deinitialized
	float y_offset;
	stbtt_fontinfo font_info;
} UI_FontData;

typedef struct UI_STBTT_CachedGlyph {
	uint32_t atlas_slot_index : 31;
	uint32_t used_this_frame : 1;
	UI_CachedGlyph info;
} UI_STBTT_CachedGlyph;

typedef struct UI_STBTT_AtlasSlot {
	bool occupied;
} UI_STBTT_AtlasSlot;

typedef struct UI_STBTT_AtlasSlotInfo {
	int x0;
	int y0;
	int x1;
	int y1;
	int slot_index;
} UI_STBTT_AtlasSlotInfo;

typedef struct UI_STBTT_AtlasParams {
	int level0_num_slots_x;
	int level0_num_slots_y;
	int level0_slot_size;
	int level_count;
} UI_STBTT_AtlasParams;

typedef UI_Texture* (*UI_STBTT_CreateAtlasFn)(uint32_t width, uint32_t height);

// When mapping a resource, the returned pointer must stay valid until UI_EndFrame or resize_* is called.
// The map function may be called multiple times during a frame, subsequent calls should return the previously mapped pointer!
typedef void* (*UI_STBTT_MapAtlasFn)();

typedef struct UI_STBTT_State {
	UI_STBTT_CreateAtlasFn CreateAtlas;
	UI_STBTT_MapAtlasFn MapAtlas;
	
	DS_DynArray(UI_FontData) fonts;
	DS_Map(UI_CachedGlyphKey, UI_STBTT_CachedGlyph) glyph_map;
	
	UI_STBTT_AtlasParams atlas_params;
	
	UI_STBTT_AtlasSlot* atlas_slots; // first big atlas slots, then smaller slots, then smaller, etc...
	
	int atlas_width;
	int atlas_height;
	float inv_atlas_width;
	float inv_atlas_height;
} UI_STBTT_State;


static void UI_STBTT_Init(UI_STBTT_CreateAtlasFn CreateAtlas, UI_STBTT_MapAtlasFn MapAtlas);
static void UI_STBTT_Deinit();

// `ttf_data` should be a pointer to a TTF font file data. It is NOT copied internally,
// and must therefore remain valid across the whole lifetime of the font!
static UI_FontID UI_STBTT_FontInit(const void* ttf_data, float y_offset);
static void UI_STBTT_FontDeinit(UI_FontID font_idx);

static UI_STBTT_AtlasSlotInfo UI_STBTT_AtlasAllocateSlot(int required_width);
static void UI_STBTT_AtlasFreeSlot(int slot_index);

static void UI_STBTT_FreeUnusedGlyphs();

// -------------------------------------------------------------

UI_API UI_STBTT_State UI_STBTT_STATE;

// -------------------------------------------------------------

static UI_FontID UI_STBTT_FontInit(const void* ttf_data, float y_offset) {
	UI_ProfEnter();
	
	int idx = -1;
	for (int i = 0; i < UI_STBTT_STATE.fonts.count; i++) {
		if (UI_STBTT_STATE.fonts.data[i].data == NULL) { idx = i; break; }
	}
	if (idx == -1) {
		idx = UI_STBTT_STATE.fonts.count;
		DS_ArrResizeUndef(&UI_STBTT_STATE.fonts, UI_STBTT_STATE.fonts.count + 1);
	}

	UI_FontData* font = &UI_STBTT_STATE.fonts.data[idx];
	memset(font, 0, sizeof(*font));

	font->data = (const unsigned char*)ttf_data;
	font->y_offset = y_offset;

	int font_offset = stbtt_GetFontOffsetForIndex(font->data, 0);
	UI_ASSERT(stbtt_InitFont(&font->font_info, font->data, font_offset));
	UI_ProfExit();
	
	return (UI_FontID)idx;
}

static void UI_STBTT_FontDeinit(UI_FontID font_idx) {
	UI_FontData* font = DS_ArrGetPtr(UI_STBTT_STATE.fonts, font_idx);
	UI_ASSERT(font->data);
	font->data = NULL;
}

static UI_CachedGlyph UI_STBTT_GetCachedGlyph(uint32_t codepoint, UI_Font font) {
	UI_ProfEnter();
	UI_CachedGlyphKey key = { codepoint, font.id, (uint16_t)font.size };

	UI_STBTT_CachedGlyph* glyph = NULL;
	bool added_new = DS_MapGetOrAddPtr(&UI_STBTT_STATE.glyph_map, key, &glyph);
	if (added_new) {
		UI_FontData* font_data = DS_ArrGetPtr(UI_STBTT_STATE.fonts, font.id);

		int glyph_index = stbtt_FindGlyphIndex(&font_data->font_info, codepoint);
		float scale = stbtt_ScaleForPixelHeight(&font_data->font_info, (float)font.size);

		int x0, y0, x1, y1;
		stbtt_GetGlyphBitmapBox(&font_data->font_info, glyph_index, scale, scale, &x0, &y0, &x1, &y1);

		int glyph_w = x1 - x0;
		int glyph_h = y1 - y0;

		UI_STBTT_AtlasSlotInfo atlas_slot = UI_STBTT_AtlasAllocateSlot(UI_Max(glyph_w, glyph_h));

		uint8_t* glyph_data = (uint8_t*)DS_ArenaPush(&UI_STATE.frame_arena, glyph_w*glyph_h);
		memset(glyph_data, 0, glyph_w*glyph_h);
		stbtt_MakeGlyphBitmapSubpixel(&font_data->font_info, glyph_data, glyph_w, glyph_h, glyph_w, scale, scale, 0.f, 0.f, glyph_index);

		uint32_t* atlas_data = (uint32_t*)UI_STBTT_STATE.MapAtlas();
		UI_ASSERT(atlas_data);

		for (int y = 0; y < glyph_h; y++) {
			uint8_t* src_row = glyph_data + y*glyph_w;
			uint32_t* dst_row = atlas_data + (atlas_slot.y0 + y)*UI_STBTT_STATE.atlas_width + atlas_slot.x0;

			for (int x = 0; x < glyph_w; x++) {
				dst_row[x] = ((uint32_t)src_row[x] << 24) | 0x00FFFFFF;
			}
		}

		int x_advance;
		int left_side_bearing;
		stbtt_GetGlyphHMetrics(&font_data->font_info, glyph_index, &x_advance, &left_side_bearing);

		glyph->atlas_slot_index = (uint32_t)atlas_slot.slot_index;
		glyph->info.uv_min.x = (float)atlas_slot.x0 * (float)UI_STBTT_STATE.inv_atlas_width;
		glyph->info.uv_min.y = (float)atlas_slot.y0 * (float)UI_STBTT_STATE.inv_atlas_height;
		glyph->info.uv_max.x = glyph->info.uv_min.x + (float)glyph_w * (float)UI_STBTT_STATE.inv_atlas_width;
		glyph->info.uv_max.y = glyph->info.uv_min.y + (float)glyph_h * (float)UI_STBTT_STATE.inv_atlas_height;
		glyph->info.size.x = (float)glyph_w;
		glyph->info.size.y = (float)glyph_h;
		glyph->info.offset.x = (float)x0;
		glyph->info.offset.y = (float)y0 + (float)font.size + font_data->y_offset;
		glyph->info.advance = (float)(int)((float)x_advance*scale + 0.5f); // round to integer
	}

	glyph->used_this_frame = 1;

	UI_ProfExit();
	return glyph->info;
}

static void UI_STBTT_Init(UI_STBTT_CreateAtlasFn CreateAtlas, UI_STBTT_MapAtlasFn MapAtlas) {
	UI_STBTT_STATE.CreateAtlas = CreateAtlas;
	UI_STBTT_STATE.MapAtlas = MapAtlas;
	
	DS_MapInit(&UI_STBTT_STATE.glyph_map, UI_STATE.allocator);
	DS_ArrInit(&UI_STBTT_STATE.fonts, UI_STATE.allocator);

	// hard-code the parameters for now
	UI_STBTT_AtlasParams atlas_params;
	atlas_params.level0_num_slots_x = 16;
	atlas_params.level0_num_slots_y = 8;
	atlas_params.level0_slot_size = 128;
	atlas_params.level_count = 4;
	UI_STBTT_STATE.atlas_params = atlas_params;

	int width = atlas_params.level0_slot_size;
	int num_slots_y = atlas_params.level0_num_slots_y;
	int num_slots_x = atlas_params.level0_num_slots_x;
	int tex_size_y = 0;
	int num_slots_total = 0;
	
	for (int l = 0; l < atlas_params.level_count; l++) {
		tex_size_y += width * num_slots_y;
		width /= 2;
		num_slots_total += num_slots_x * num_slots_y;
		num_slots_x *= 2;
	}
	
	UI_STBTT_STATE.atlas_slots = (UI_STBTT_AtlasSlot*)DS_MemAlloc(UI_STATE.allocator, sizeof(UI_STBTT_AtlasSlot) * num_slots_total);
	memset(UI_STBTT_STATE.atlas_slots, 0, sizeof(UI_STBTT_AtlasSlot) * num_slots_total);
	
	UI_STBTT_STATE.atlas_width = atlas_params.level0_slot_size * atlas_params.level0_num_slots_x;
	UI_STBTT_STATE.atlas_height = tex_size_y;
	UI_STBTT_STATE.inv_atlas_width = 1.f / (float)UI_STBTT_STATE.atlas_width;
	UI_STBTT_STATE.inv_atlas_height = 1.f / (float)UI_STBTT_STATE.atlas_height;
	
	// Initialize atlas texture
	{
		UI_STBTT_STATE.CreateAtlas(UI_STBTT_STATE.atlas_width, UI_STBTT_STATE.atlas_height);

		// Allocate the first possible slot from the atlas allocator, which ends up being the slot in the top-left corner.
		// This lets us set the top-left corner pixel to be opaque and white. This makes sure that any vertices with an uv of {0, 0}
		// ends up sampling a white pixel from the atlas and will therefore act as if there was not texture.
		UI_STBTT_AtlasAllocateSlot(0);
		void* atlas_data = UI_STBTT_STATE.MapAtlas();
		*(uint32_t*)atlas_data = 0xFFFFFFFF;
	}

	UI_STATE.backend.GetCachedGlyph = UI_STBTT_GetCachedGlyph;
}

static void UI_STBTT_Deinit() {
	DS_MemFree(UI_STATE.allocator, UI_STBTT_STATE.atlas_slots);
	
	DS_MapDeinit(&UI_STBTT_STATE.glyph_map);
	DS_ArrDeinit(&UI_STBTT_STATE.fonts);
	
	DS_DebugFillGarbage(&UI_STBTT_STATE, sizeof(UI_STBTT_STATE));
}

static UI_STBTT_AtlasSlotInfo UI_STBTT_AtlasAllocateSlot(int required_width) {
	UI_ASSERT(required_width <= UI_STBTT_STATE.atlas_params.level0_slot_size);

	int width = UI_STBTT_STATE.atlas_params.level0_slot_size;
	int num_slots_x = UI_STBTT_STATE.atlas_params.level0_num_slots_x;
	for (int i = 1; i < UI_STBTT_STATE.atlas_params.level_count; i++) {
		width /= 2;
		num_slots_x *= 2;
	}
	
	int first_slot = 0;
	int num_slots_y = UI_STBTT_STATE.atlas_params.level0_num_slots_y;

	UI_STBTT_AtlasSlotInfo result = {0};

	int base_texspace_y = 0;

	for (int l = 0; l < UI_STBTT_STATE.atlas_params.level_count; l++) {
		
		if (required_width > width) {
			// Does not fit into this level
			base_texspace_y += width * num_slots_y;
			width *= 2;
			first_slot += num_slots_x * num_slots_y;
			num_slots_x /= 2;
			continue;
		}

		// Find a slot in this level.

		int i = first_slot;
		for (int y = 0; y < num_slots_y; y++) {
			for (int x = 0; x < num_slots_x; x++) {
				if (!UI_STBTT_STATE.atlas_slots[i].occupied) {
					// Take this slot!
					UI_STBTT_STATE.atlas_slots[i].occupied = true;
					
					result.x0 = x * width;
					result.y0 = base_texspace_y + y * width;
					result.x1 = result.x0 + width;
					result.y1 = result.y0 + width;
					result.slot_index = i;
					
					return result;
				}
				i++;
			}
		}
	}
	
	UI_ASSERT(0); // We didn't find a slot.
	return result;
}

static void UI_STBTT_AtlasFreeSlot(int slot_index) {
	UI_STBTT_STATE.atlas_slots[slot_index].occupied = false;
}

static void UI_STBTT_FreeUnusedGlyphs() {
	DS_DynArray(UI_CachedGlyphKey) unused_glyphs = {&UI_STATE.frame_arena};

	DS_ForMapEach(UI_CachedGlyphKey, UI_STBTT_CachedGlyph, &UI_STBTT_STATE.glyph_map, IT) {
		if (!IT.value->used_this_frame) {
			UI_STBTT_AtlasFreeSlot(IT.value->atlas_slot_index);
			DS_ArrPush(&unused_glyphs, *IT.key);
		}
		IT.value->used_this_frame = 0;
	}

	for (int i = 0; i < unused_glyphs.count; i++) {
		DS_MapRemove(&UI_STBTT_STATE.glyph_map, unused_glyphs.data[i]);
	}
}
