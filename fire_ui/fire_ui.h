// Fire UI - immediate mode user interface library
//
// Author: Eero Mutka
// Version: 0
// Date: 25 March, 2024
//
// This code is released under the MIT license (https://opensource.org/licenses/MIT).
//
// Headers that must have been included before this file:
// - "fire_ds.h"
// - "fire_string.h"
// - "stb_rect_pack.h"  (from https://github.com/nothings/stb)
// - "stb_truetype.h"   (from https://github.com/nothings/stb)
//

#ifndef FIRE_UI_INCLUDED
#define FIRE_UI_INCLUDED

#ifndef UI_CHECK_OVERRIDE
#include <assert.h>
#define UI_CHECK(x) assert(x)
#endif

typedef STR UI_String;

#ifdef __cplusplus
#define UI_LangAgnosticLiteral(T) T   // in C++, struct and union literals are of the form MyStructType{...}
#else
#define UI_LangAgnosticLiteral(T) (T) // in C, struct and union literals are of the form (MyStructType){...}
#endif

// * Retrieve an unique key for the macro usage site.
#define UI_KEY()      UI_LangAgnosticLiteral(UI_Key){(uintptr_t)__FILE__ ^ (uintptr_t)__COUNTER__}
#define UI_KEY1(A)    UI_HashKey(UI_KEY(), (A))
#define UI_KEY2(A, B) UI_HashKey(UI_KEY1(A), (B))

#define UI_ArrayCount(X) (sizeof(X) / sizeof(X[0]))
#define UI_Max(A, B) ((A) > (B) ? (A) : (B))
#define UI_Min(A, B) ((A) < (B) ? (A) : (B))
#define UI_Abs(X) ((X) < 0 ? -(X) : (X))

#define UI_INFINITE 10000000.f

typedef union {
	struct { float x, y; };
	float _[2];
} UI_Vec2;
#define UI_VEC2  UI_LangAgnosticLiteral(UI_Vec2)

typedef float UI_Size;

typedef uint64_t UI_Key; // 0 is invalid value
#define UI_INVALID_KEY (UI_Key)0

typedef enum UI_Axis {
	UI_Axis_X,
	UI_Axis_Y,
} UI_Axis;

typedef int UI_BoxFlags;
typedef enum UI_BoxFlagBits {
	UI_BoxFlag_DrawBorder = 1 << 0,
	UI_BoxFlag_DrawText = 1 << 1,
	UI_BoxFlag_DrawTransparentBackground = 1 << 2,
	UI_BoxFlag_DrawOpaqueBackground = 1 << 3,
	UI_BoxFlag_Clickable = 1 << 4, // NOTE: When set, this will make 'pressed' input come to this box instead of the parent box
	UI_BoxFlag_PressingStaysWithoutHover = 1 << 5,
	UI_BoxFlag_ChildPadding = 1 << 6,
	UI_BoxFlag_LayoutInX = 1 << 7,
	UI_BoxFlag_LayoutFromEndX = 1 << 8, // Reverse the layout direction for children.
	UI_BoxFlag_LayoutFromEndY = 1 << 9, // Reverse the layout direction for children.
	UI_BoxFlag_Selectable = 1 << 10, // When set, the keyboard navigator can move to this UI box.
	UI_BoxFlag_NoAutoOffset = 1 << 11, // When set, this box won't use the layout system to determine its position, it'll be fully controlled by the `offset` field instead.
	UI_BoxFlag_NoScissor = 1 << 12,
	UI_BoxFlag_NoHover = 1 << 13, // Propagated from parent during AddBox
	UI_BoxFlag_NoFlexDownX = 1 << 14,
	UI_BoxFlag_NoFlexDownY = 1 << 15,
	//UI_BoxFlag_HasCalledMakeBox = 1 << 13, // Internal flag
	//UI_BoxFlag_HasComputedUnexpandedSizes = 1 << 14, // Internal flag
	//UI_BoxFlag_HasComputedExpandedSizes = 1 << 15, // Internal flag
	UI_BoxFlag_HasComputedRects = 1 << 16, // Internal flag
} UI_BoxFlagBits;

typedef struct UI_Rect {
	UI_Vec2 min, max;
} UI_Rect;
#define UI_RECT  UI_LangAgnosticLiteral(UI_Rect)

typedef struct UI_Color {
	uint8_t r, g, b, a;
} UI_Color;
#define UI_COLOR  UI_LangAgnosticLiteral(UI_Color)

#define UI_MakeColor(R, G, B, A)  UI_COLOR{(uint8_t)(R), (uint8_t)(G), (uint8_t)(B), (uint8_t)(A)}
#define UI_MakeColorF(R, G, B, A) UI_COLOR{(uint8_t)((R) * 255.f), (uint8_t)((G) * 255.f), (uint8_t)((B) * 255.f), (uint8_t)((A) * 255.f)}

typedef struct UI_Text {
	DS_DynArray(char) text;
	DS_DynArray(int) line_offsets;
} UI_Text;

#define UI_TextToStr(TEXT) UI_LangAgnosticLiteral(STR){(const char*)(TEXT).text.data, (TEXT).text.length}

typedef struct UI_CachedGlyphKey {
	// !!! memcmp is used on this struct, so it must be manually padded for 0-initialized padding.
	uint32_t codepoint;
	int size;
} UI_CachedGlyphKey;

typedef struct UI_CachedGlyph {
	UI_Vec2 origin_uv;     // in UV coordinates
	UI_Vec2 size_pixels;   // in pixel coordinates
	UI_Vec2 offset_pixels; // in pixel coordinates
	float x_advance;      // advance to the next character in pixel coordinates
} UI_CachedGlyph;

typedef struct UI_Font {
	const uint8_t* data;
	float y_offset;

	DS_Map(UI_CachedGlyphKey, UI_CachedGlyph) glyph_map;
	DS_Map(UI_CachedGlyphKey, UI_CachedGlyph) glyph_map_old;

	stbtt_fontinfo font_info;
} UI_Font;

typedef struct UI_FontUsage {
	UI_Font* font;
	float size; // height in pixels
} UI_FontUsage;

typedef struct UI_ArrangerSet {
	struct UI_Box* dragging_elem; // may be NULL
} UI_ArrangerSet;

typedef struct UI_ArrangersRequest {
	// If there's nothing to move, both of these will be set to 0
	int move_from; // index of the moved element before the move
	int move_to; // index of the moved element after the move
} UI_ArrangersRequest;

typedef struct UI_Style {
	UI_FontUsage font;
	UI_Color border_color;
	UI_Color opaque_bg_color;
	UI_Color transparent_bg_color;
	UI_Color text_color;
	UI_Vec2 text_padding;  // in size units
	UI_Vec2 child_padding; // in size units
} UI_Style;

// If I have two functions, those function must be able to independently associate extra data to a box.
// I think either a hash table, or a linked list of "extra data" could be good.

typedef struct UI_BoxCustomDataHeader UI_BoxCustomDataHeader;
struct UI_BoxCustomDataHeader {
	UI_Key key;
	UI_BoxCustomDataHeader* next;
	int debug_size;
};

typedef struct UI_Box UI_Box;

struct UI_Box {
	UI_Key key;
	UI_Box* prev_frame; // NULL if a box with the same key didn't exist (wasn't added to the tree) during the previous frame
	UI_Box* parent;
	UI_Box* next[2]; // prev and next pointers
	UI_Box* first_child[2]; // first and last child

	union {
		UI_BoxFlags flags;
		UI_BoxFlagBits flags_bits; // for debugger visualization
	};

	UI_String text;
	UI_Size size[2];
	UI_Vec2 offset;

	UI_Style* style;
	
	void (*draw_override)(UI_Box* box);
	void (*compute_unexpanded_size_override)(UI_Box* box, UI_Axis axis);

	// Persistent / animated state
	float lazy_is_hovered;
	float lazy_is_holding_down;

	// These will be computed with UI_ComputeBox.
	UI_Vec2 computed_position; // in absolute screen space coordinates
	UI_Vec2 computed_unexpanded_size;
	UI_Vec2 computed_size;
	UI_Rect computed_rect_clipped;

	UI_BoxCustomDataHeader* custom_data_list;
};

typedef struct UI_Mark {
	int line;
	int col;
} UI_Mark;
#define UI_MARK  UI_LangAgnosticLiteral(UI_Mark)

typedef struct UI_Selection {
	UI_Mark range[2]; // These must be sorted so that range[0] represents a mark before range[1]. You can sort them using `UI_SelectionFixOrder`.
	uint32_t end;
	float cursor_x;
} UI_Selection;

typedef struct UI_DrawVertex {
	UI_Vec2 position;
	UI_Vec2 uv;
	UI_Color color;
} UI_DrawVertex;
#define UI_DRAW_VERTEX  UI_LangAgnosticLiteral(UI_DrawVertex)

typedef uint16_t UI_ScissorID;

typedef enum UI_AlignV { UI_AlignV_Upper, UI_AlignV_Middle, UI_AlignV_Lower } UI_AlignV;
typedef enum UI_AlignH { UI_AlignH_Left, UI_AlignH_Middle, UI_AlignH_Right } UI_AlignH;

#define UI_TEXTURE_ID_NIL ((UI_TextureID)0)
//#define UI_BUFFER_ID_NIL ((UI_BufferID)0)

#ifndef UI_MAX_BACKEND_BUFFERS
#define UI_MAX_BACKEND_BUFFERS 16
#endif

typedef uint64_t UI_TextureID; // UI_TEXTURE_ID_NIL is a reserved value
// typedef uint64_t UI_BufferID;  // UI_BUFFER_ID_NIL is a reserved value

typedef struct UI_DrawCall {
	int vertex_buffer_id;
	int index_buffer_id;
	UI_TextureID texture;
	uint32_t first_index;
	uint32_t index_count;
} UI_DrawCall;

typedef struct UI_DrawRectCorners {
	// The following order applies: top-left, top-right, bottom-right, bottom-left
	UI_Color color[4];
	UI_Color outer_color[4]; // only used for UI_DrawRectEx
	float roundness[4];
} UI_DrawRectCorners;

typedef enum UI_Input {
	UI_Input_Invalid,
	UI_Input_MouseLeft,
	UI_Input_MouseRight,
	UI_Input_MouseMiddle,
	UI_Input_Shift,
	UI_Input_Control,
	UI_Input_Alt,
	UI_Input_Tab,
	UI_Input_Escape,
	UI_Input_Enter,
	UI_Input_Delete,
	UI_Input_Backspace,
	UI_Input_A,
	UI_Input_C,
	UI_Input_V,
	UI_Input_X,
	UI_Input_Y,
	UI_Input_Z,
	UI_Input_Home,
	UI_Input_End,
	UI_Input_Left,
	UI_Input_Right,
	UI_Input_Up,
	UI_Input_Down,
	UI_Input_COUNT,
} UI_Input;

typedef uint8_t UI_InputEvents;
typedef enum UI_InputEvent {
	UI_InputEvent_PressOrRepeat = 1 << 0,
	UI_InputEvent_Press = 1 << 1,
	UI_InputEvent_Release = 1 << 2,
} UI_InputEvent;

//typedef uint8_t UI_InputStates;
//typedef enum UI_InputState {
//	UI_InputState_IsDown = 1 << 0,
//	UI_InputState_WasPressed = 1 << 1,
//	UI_InputState_WasPressedOrRepeated = 1 << 2,
//	UI_InputState_WasReleased = 1 << 3,
//} UI_InputState;

typedef struct UI_Backend {
	// `buffer_id` is a value between 0 and UI_MAX_BACKEND_BUFFERS - 1
	void (*create_vertex_buffer)(int buffer_id, uint32_t size_in_bytes);
	void (*create_index_buffer)(int buffer_id, uint32_t size_in_bytes);
	void (*destroy_buffer)(int buffer_id);

	// `atlas_id` is a value between 0 and 1
	UI_TextureID(*create_atlas)(int atlas_id, uint32_t width, uint32_t height);
	void (*destroy_atlas)(int atlas_id);

	// From these functions, the returned pointer must stay valid until UI_EndFrame or destroy_buffer/destroy_atlas is called.
	void* (*buffer_map_until_frame_end)(int buffer_id);
	void* (*atlas_map_until_frame_end)(int atlas_id);
} UI_Backend;

typedef struct UI_Inputs {
	UI_InputEvents input_events[UI_Input_COUNT];

	UI_Font* base_font;
	UI_Font* icons_font;

	UI_Vec2 mouse_position;
	UI_Vec2 mouse_raw_delta;
	float mouse_wheel_delta; // +1.0 means the wheel was rotated forward by one scroll step

	uint32_t text_input_utf32[16];
	int text_input_utf32_length;

	UI_String(*get_clipboard_string_fn)(void* user_data); // The returned string must stay valid for the rest of the frame
	void (*set_clipboard_string_fn)(UI_String string, void* user_data);

	float frame_delta_time;

	void* user_data; // Unused by the library; passed into `get_clipboard_string_fn` and `set_clipboard_string_fn`
} UI_Inputs;

typedef enum UI_MouseCursor {
	UI_MouseCursor_Default,
	UI_MouseCursor_ResizeH,
	UI_MouseCursor_ResizeV,
	UI_MouseCursor_I_beam,
} UI_MouseCursor;

typedef struct UI_Outputs {
	UI_MouseCursor cursor;
	bool lock_and_hide_cursor;

	UI_DrawCall* draw_calls;
	int draw_calls_count;

	// what if we need to update two atlases in one frame?
	// bool texture_atlas_was_updated; // new data was written into texture_atlas_cpu_local_data
} UI_Outputs;

typedef struct UI_EditTextModify {
	bool has_edit;
	UI_Box* box_with_text;
	UI_Mark replace_from; // TODO: add byteoffsets to these for maximum information to the user
	UI_Mark replace_to;
	UI_String replace_with;
} UI_EditTextModify;

typedef DS_Map(UI_Key, UI_Box*) UI_BoxFromKeyMap;

typedef struct UI_State {
	DS_Allocator* allocator;
	DS_Arena persistent_arena;
	UI_Backend backend;

	UI_Font* base_font;
	UI_Font* icons_font;

	DS_Arena prev_frame_arena;
	DS_Arena frame_arena;

	UI_BoxFromKeyMap prev_frame_box_from_key;
	UI_BoxFromKeyMap box_from_key;

	uint64_t frame_idx;

	// The selected box can be hidden, i.e. when clicking a button with your mouse. Then, when pressing an arrow key, it becomes visible again.
	bool selection_is_visible;

	UI_Inputs inputs;
	UI_Outputs outputs;

	bool input_is_down[UI_Input_COUNT];

	float time_since_pressed_lmb;

	// atlas
	stbtt_pack_context pack_context;
	//bool atlas_needs_reupload;
	UI_TextureID atlases[2]; // 0 is the current atlas, 1 is NULL or the old atlas
	uint8_t* atlas_buffer_grayscale; // stb rect pack works with grayscale, while we want to convert to RGBA8 on the fly.

	// Mouse position in screen space coordinates, snapped to the pixel center. Placing it at the pixel center means we don't
	// need to worry about dUI_enerate cases where the mouse is exactly at the edge of one or many rectangles when testing for overlap.
	UI_Vec2 mouse_pos;
	UI_Vec2 window_size;

	UI_Vec2 last_released_mouse_pos;
	UI_Vec2 last_pressed_mouse_pos;
	UI_Vec2 mouse_travel_distance_after_press; // NOTE: holding alt/shift will modify the speed at which this value changes

	UI_Key mouse_clicking_down_box;
	UI_Key mouse_clicking_down_box_new;
	UI_Key keyboard_clicking_down_box;
	UI_Key keyboard_clicking_down_box_new;
	UI_Key selected_box;
	UI_Key selected_box_new;
	
	DS_DynArray(UI_Box*) box_stack;
	DS_DynArray(UI_Style*) style_stack;

	float scrollbar_origin_before_press;

	// -- Draw state --
	uint32_t* draw_indices; // NULL by default
	UI_DrawVertex* draw_vertices; // NULL by default
	uint32_t draw_next_vertex;
	uint32_t draw_next_index;
	UI_TextureID draw_active_texture;
	DS_DynArray(UI_DrawCall) draw_calls;

	// -- Splitters state --
	UI_Key holding_splitter_key; // UI_INVALID_KEY when not holding anything
	int holding_splitter_index;

	// -- Edit number state --
	double edit_number_value_before_press;
	UI_Text edit_number_text;

	// -- Text editing state --
	UI_Key text_editing_box;
	UI_Key text_editing_box_new;
	bool edit_text_should_refresh; // implicit parameter to the next call to UI_AddValText
	UI_Selection edit_text_selection;
} UI_State;

// The color palette here is the same as in Raylib
#define UI_LIGHTGRAY  UI_COLOR{200, 200, 200, 255}
#define UI_GRAY       UI_COLOR{130, 130, 130, 255}
#define UI_DARKGRAY   UI_COLOR{80, 80, 80, 255}
#define UI_YELLOW     UI_COLOR{253, 249, 0, 255}
#define UI_GOLD       UI_COLOR{255, 203, 0, 255}
#define UI_ORANGE     UI_COLOR{255, 161, 0, 255}
#define UI_PINK       UI_COLOR{255, 109, 194, 255}
#define UI_RED        UI_COLOR{230, 41, 55, 255}
#define UI_MAROON     UI_COLOR{190, 33, 55, 255}
#define UI_GREEN      UI_COLOR{0, 228, 48, 255}
#define UI_LIME       UI_COLOR{0, 158, 47, 255}
#define UI_DARKGREEN  UI_COLOR{0, 117, 44, 255}
#define UI_SKYBLUE    UI_COLOR{102, 191, 255, 255}
#define UI_BLUE       UI_COLOR{0, 121, 241, 255}
#define UI_DARKBLUE   UI_COLOR{0, 82, 172, 255}
#define UI_PURPLE     UI_COLOR{200, 122, 255, 255}
#define UI_VIOLET     UI_COLOR{135, 60, 190, 255}
#define UI_DARKPURPLE UI_COLOR{112, 31, 126, 255}
#define UI_BEIGE      UI_COLOR{211, 176, 131, 255}
#define UI_BROWN      UI_COLOR{127, 106, 79, 255}
#define UI_DARKBROWN  UI_COLOR{76, 63, 47, 255}
#define UI_WHITE      UI_COLOR{255, 255, 255, 255}
#define UI_BLACK      UI_COLOR{0, 0, 0, 255}
#define UI_BLANK      UI_COLOR{0, 0, 0, 0}
#define UI_MAGENTA    UI_COLOR{255, 0, 255, 255}

#ifndef UI_API
#define UI_API static
#define UI_IMPLEMENTATION
#endif

typedef const UI_Rect* UI_ScissorRect; // may be NULL for no scissor

// -- Global state -------
extern UI_State UI_STATE;
// -----------------------

UI_API inline bool UI_InputIsDown(UI_Input input) { return UI_STATE.input_is_down[input]; }
UI_API inline bool UI_InputWasPressed(UI_Input input) { return UI_STATE.inputs.input_events[input] & UI_InputEvent_Press; }
UI_API inline bool UI_InputWasPressedOrRepeated(UI_Input input) { return (UI_STATE.inputs.input_events[input] & UI_InputEvent_PressOrRepeat) != 0; }
UI_API inline bool UI_InputWasReleased(UI_Input input) { return (UI_STATE.inputs.input_events[input] & UI_InputEvent_Release) != 0; }

UI_API inline DS_Arena* UI_FrameArena(void) { return &UI_STATE.frame_arena; }

UI_API inline bool UI_MarkEquals(UI_Mark a, UI_Mark b) { return a.line == b.line && a.col == b.col; }

// Mini math library
UI_API inline UI_Vec2 UI_AddV2(UI_Vec2 a, UI_Vec2 b) { return UI_VEC2{ a.x + b.x, a.y + b.y }; }
UI_API inline UI_Vec2 UI_SubV2(UI_Vec2 a, UI_Vec2 b) { return UI_VEC2{ a.x - b.x, a.y - b.y }; }
UI_API inline UI_Vec2 UI_MulV2(UI_Vec2 a, UI_Vec2 b) { return UI_VEC2{ a.x * b.x, a.y * b.y }; }
UI_API inline UI_Vec2 UI_MulV2F(UI_Vec2 a, float b) { return UI_VEC2{ a.x * b, a.y * b }; }
UI_API inline float UI_Lerp(float a, float b, float t) { return (1.f - t) * a + t * b; }
UI_API inline UI_Vec2 UI_LerpV2(UI_Vec2 a, UI_Vec2 b, float t) { return UI_VEC2{ (1.f - t) * a.x + t * b.x, (1.f - t) * a.y + t * b.y }; }

UI_API UI_Key UI_HashKey(UI_Key a, UI_Key b);
UI_API UI_Key UI_HashPtr(UI_Key a, void* b);
UI_API UI_Key UI_HashInt(UI_Key a, int b);
// UI_API UI_Key UI_KeyFromN(UI_Key *keys, int keys_count);

UI_API void UI_SelectionFixOrder(UI_Selection* sel);

UI_API bool UI_PointIsInRect(UI_Rect rect, UI_Vec2 p);
UI_API UI_Rect UI_RectIntersection(UI_Rect a, UI_Rect b);
UI_API inline UI_Vec2 UI_RectSize(UI_Rect rect) { UI_Vec2 x = { rect.max.x - rect.min.x, rect.max.y - rect.min.y }; return x; }

/*
 `resources_directory` is the path of the `resources` folder that is shipped with FUI.
 FUI will load the default fonts from this folder during `UI_Init`, as well as the shader.
*/
UI_API void UI_Init(DS_Allocator* allocator, const UI_Backend* backend);
UI_API void UI_Deinit(void);

UI_API void UI_BeginFrame(const UI_Inputs* inputs, UI_Vec2 window_size);
UI_API void UI_EndFrame(UI_Outputs* outputs/*, GPU_Graph *graph, GPU_DescriptorArena *descriptor_arena*/);

/*
 The way customization is meant to work with FUI is that if you want to add a new feature, then you simply
 copy code inside the corresponding code (i.e. in UI_draw_box_default) into your own new function and disregard the
 FUI-provided function. FUI tries to break the code down into reusable pieces of code so that it's easy to customize
 the entire tree of UI features that you want to support.
*/
UI_API void UI_DrawBox(UI_Box* box);
UI_API void UI_DrawBoxDefault(UI_Box* box);
UI_API void UI_DrawBoxBackdrop(UI_Box* box);

// -- Tree builder API with implicit context -------

// -100 to -99 is unexpanded size fit, take as much parent space as the ratio
// -200 to -199 is unexpanded size 0, take as much parent space as the ratio

#define UI_SizeFit()        (-100.f)
#define UI_SizeFlex(WEIGHT) ((WEIGHT) - 100.f)

UI_API void UI_BoxComputeUnexpandedSizes(UI_Box* box);
UI_API void UI_BoxComputeExpandedSizes(UI_Box* box);
UI_API void UI_BoxComputeRects(UI_Box* box, UI_Vec2 box_position);

UI_API void UI_BoxComputeUnexpandedSize(UI_Box* box, UI_Axis axis);
UI_API void UI_BoxComputeUnexpandedSizeDefault(UI_Box* box, UI_Axis axis);
UI_API void UI_BoxComputeExpandedSize(UI_Box* box, UI_Axis axis, float size);
UI_API void UI_BoxComputeRectsStep(UI_Box* box, UI_Axis axis, float position, UI_ScissorRect scissor);

UI_API UI_Box* UI_MakeRootBox(UI_Key key, UI_Size w, UI_Size h, UI_BoxFlags flags);
UI_API void UI_PushBox(UI_Box* box);
UI_API void UI_PopBox(UI_Box* box);
UI_API void UI_PopBoxN(UI_Box* box, int n);

UI_API UI_Box* UI_AddBox(UI_Key key, UI_Size w, UI_Size h, UI_BoxFlags flags);
UI_API UI_Box* UI_AddBoxWithText(UI_Key key, UI_Size w, UI_Size h, UI_BoxFlags flags, UI_String string);

UI_API UI_Box* UI_AddButton(UI_Key key, UI_Size w, UI_Size h, UI_BoxFlags flags, UI_String string);
UI_API UI_Box* UI_AddDropdownButton(UI_Key key, UI_Size w, UI_Size h, UI_BoxFlags flags, UI_String string);

UI_API void UI_AddCheckbox(UI_Key key, bool* value);

// You may pass NULL to `out_modify`, in which case the modification will be done automatically
UI_API UI_Box* UI_AddValFilepath(UI_Key key, UI_Size w, UI_Size h, UI_Text* text, UI_EditTextModify* out_modify);
UI_API UI_Box* UI_AddValText(UI_Key key, UI_Size w, UI_Size h, UI_Text* text, UI_EditTextModify* out_modify);
UI_API void UI_ApplyEditTextModify(UI_Text* text, const UI_EditTextModify* modify);

UI_API UI_Box* UI_AddValInt(UI_Key key, UI_Size w, UI_Size h, int* value);
UI_API UI_Box* UI_AddValInt64(UI_Key key, UI_Size w, UI_Size h, int64_t* value);
UI_API UI_Box* UI_AddValUInt64(UI_Key key, UI_Size w, UI_Size h, uint64_t* value);
UI_API UI_Box* UI_AddValFloat(UI_Key key, UI_Size w, UI_Size h, float* value);
UI_API UI_Box* UI_AddValFloat64(UI_Key key, UI_Size w, UI_Size h, double* value);
UI_API UI_Box* UI_AddValNumeric(UI_Key key, UI_Size w, UI_Size h, void* value_64_bit, bool is_signed, bool is_float);

// * may return NULL
UI_API UI_Box* UI_PushCollapsing(UI_Key key, UI_Size w, UI_Size h, UI_Size indent, UI_BoxFlags flags, UI_String text);
UI_API void UI_PopCollapsing(UI_Box* box);

UI_API void UI_TextInit(DS_Allocator* allocator, UI_Text* text, UI_String initial_value);
UI_API void UI_TextDeinit(UI_Text* text);
UI_API void UI_TextSet(UI_Text* text, UI_String value);

// * `anchor_x` / `anchor_y` can be 0 or 1: A value of 0 means anchoring the scrollbar to left / top, 1 means anchoring it to right / bottom.
UI_API UI_Box* UI_PushScrollArea(UI_Key key, UI_Size w, UI_Size h, UI_BoxFlags flags, int anchor_x, int anchor_y);
UI_API void UI_PopScrollArea(UI_Box* box);

UI_API UI_Box* UI_PushArrangerSet(UI_Key key, UI_Size w, UI_Size h);
UI_API void UI_PopArrangerSet(UI_Box* box, UI_ArrangersRequest* out_edit_request);
UI_API UI_Box* UI_AddArranger(UI_Key key, UI_Size w, UI_Size h);

// --------------------------------------

UI_API UI_Style* UI_PushStyle(void);
UI_API UI_Style* UI_MakeStyle(void);
UI_API void UI_PopStyle(UI_Style* style);
UI_API UI_Style* UI_PeekStyle(void);

UI_API UI_Box* UI_PrevFrameBoxFromKey(UI_Key key); // Returns NULL if a box with this key did not exist
UI_API UI_Box* UI_BoxFromKey(UI_Key key); // Returns NULL if a box with this key has not been created this frame so far

#define UI_BoxAddCustomVal(BOX, KEY, VALUE) UI_BoxAddCustomData(BOX, KEY, &(VALUE), sizeof(VALUE))
#define UI_BoxGetCustomVal(T, BOX, KEY)     (T*)UI_BoxGetCustomData(BOX, KEY, sizeof(T))
UI_API void UI_BoxAddCustomData(UI_Box* box, UI_Key key, void* ptr, int size);
UI_API void* UI_BoxGetCustomData(UI_Box* box, UI_Key key, int size); // NULL is returned if no custom data using the provided key is found

UI_API bool UI_BoxIsAParentOf(UI_Box* box, UI_Box* child);

UI_API bool UI_HasMovedMouseAfterPressed(void);

UI_API bool UI_ClickedAnywhere(void);
UI_API bool UI_DoubleClickedAnywhere(void);

// These input functions can be called even before a box with the given key has been created.
// You may, for example, implement a "close" button, which you can first ask if it has been pressed during this frame,
// and if so, decide to not create it still on the same frame.
// Internally, these functions look at the rectangle of the box of the previous frame and the mouse position
// of the current frame to determine overlap.

UI_API bool UI_Clicked(UI_Key key);
UI_API bool UI_Pressed(UI_Key key);
UI_API bool UI_PressedIdle(UI_Key key); // Differs from UI_Pressed by calling UI_IsHoveredIdle instead of UI_IsHovered
UI_API bool UI_PressedEx(UI_Key key, UI_Input mouse_button);
UI_API bool UI_PressedIdleEx(UI_Key key, UI_Input mouse_button); // Differs from UI_Pressed by calling UI_IsHoveredIdle instead of UI_IsHovered
UI_API bool UI_DoubleClicked(UI_Key key);
UI_API bool UI_DoubleClickedIdle(UI_Key key); // Differs from UI_DoubleClicked by calling UI_IsHoveredIdle instead of UI_IsHovered

UI_API bool UI_IsHovered(UI_Key key);
UI_API bool UI_IsHoveredIdle(UI_Key key); // Differs from UI_IsHovered in that if a clickable child box is also hovered, this will return false.
UI_API bool UI_IsMouseInsideOf(UI_Key key); // like IsHovered, except ignores the NoHover box flag

UI_API bool UI_IsSelected(UI_Key key);

UI_API bool UI_IsClickingDown(UI_Key key);
UI_API bool UI_IsClickingDownAndHovered(UI_Key key);

UI_API void UI_EditTextSelectAll(const UI_Text* text, UI_Selection* selection);

// Input-only / does not create any drawn elements
// `holding_splitter` is -1 when not holding anything, 0 when the first splitter, and so on.
UI_API void UI_Splitters(UI_Key key, UI_Rect area, UI_Axis X, int panel_count,
	float* panel_end_offsets, float panel_min_size);

UI_API bool UI_SplittersFindHoveredIndex(UI_Rect area, UI_Axis X, int panel_count, float* panel_end_offsets, int* out_index);

// UI_Splitters must be called before calling UI_SplittersGetHoldingIndex
UI_API bool UI_SplittersGetHoldingIndex(UI_Key key, int* out_index);

UI_API float UI_GlyphWidth(uint32_t codepoint, UI_FontUsage font);
UI_API float UI_TextWidth(UI_String text, UI_FontUsage font);

// Returns true when newly added, false when existing
// ... do we have the ability to upload new data to a texture? That would need to inform the graphics backend.
UI_API bool UI_CacheTextureArea(int64_t key, int width, int height, void** out_pixels, UI_Vec2* out_uv_min, UI_Vec2* out_uv_max);

UI_API void UI_FreeTextureArea(int64_t key);

// `ttf_data` should be a pointer to a TTF font file data. It is NOT copied internally,
// and must therefore remain valid across the whole lifetime of the font!
UI_API void UI_FontInit(UI_Font* font, const void* ttf_data, float y_offset);
UI_API void UI_FontDeinit(UI_Font* font);

// -- Drawing ----------------------

UI_API inline UI_DrawVertex* UI_AddVertices(int count, uint32_t* out_first_index);
UI_API inline uint32_t* UI_AddIndices(int count, UI_TextureID texture);
UI_API inline void UI_AddTriangleIndices(uint32_t a, uint32_t b, uint32_t c, UI_TextureID texture);
UI_API inline void UI_AddQuadIndices(uint32_t a, uint32_t b, uint32_t c, uint32_t d, UI_TextureID texture);
UI_API void UI_AddTriangleIndicesAndClip(uint32_t a, uint32_t b, uint32_t c, UI_TextureID texture, UI_ScissorRect scissor);
UI_API void UI_AddQuadIndicesAndClip(uint32_t a, uint32_t b, uint32_t c, uint32_t d, UI_TextureID texture, UI_ScissorRect scissor);

UI_API void UI_DrawRect(UI_Rect rect, UI_Color color, UI_ScissorRect scissor);
UI_API void UI_DrawRectRounded(UI_Rect rect, float roundness, UI_Color color, int num_corner_segments, UI_ScissorRect scissor);
UI_API void UI_DrawRectRounded2(UI_Rect rect, float roundness, UI_Color inner_color, UI_Color outer_color, int num_corner_segments, UI_ScissorRect scissor);
UI_API void UI_DrawRectEx(UI_Rect rect, const UI_DrawRectCorners* corners, int num_corner_segments, UI_ScissorRect scissor);
UI_API void UI_DrawRectLines(UI_Rect rect, float thickness, UI_Color color, UI_ScissorRect scissor);
UI_API void UI_DrawRectLinesRounded(UI_Rect rect, float thickness, float roundness, UI_Color color, UI_ScissorRect scissor);
UI_API void UI_DrawRectLinesEx(UI_Rect rect, const UI_DrawRectCorners* corners, float thickness, UI_ScissorRect scissor);
UI_API void UI_DrawTriangle(UI_Vec2 a, UI_Vec2 b, UI_Vec2 c, UI_Color color, UI_ScissorRect scissor);
UI_API void UI_DrawQuad(UI_Vec2 a, UI_Vec2 b, UI_Vec2 c, UI_Vec2 d, UI_Color color, UI_ScissorRect scissor);

UI_API UI_Vec2 UI_DrawText(UI_String text, UI_FontUsage font, UI_Vec2 origin, UI_AlignH align_h, UI_AlignV align_v, UI_Color color, UI_ScissorRect scissor);

UI_API void UI_DrawSprite(UI_Rect rect, UI_Color color, UI_Rect uv_rect, UI_TextureID texture, UI_ScissorRect scissor);

UI_API void UI_DrawCircle(UI_Vec2 p, float radius, int segments, UI_Color color, UI_ScissorRect scissor);

UI_API void UI_DrawConvexPolygon(const UI_Vec2* points, int points_count, UI_Color color, UI_ScissorRect scissor);

UI_API void UI_DrawPoint(UI_Vec2 p, float thickness, UI_Color color, UI_ScissorRect scissor);

UI_API void UI_DrawLine(UI_Vec2 a, UI_Vec2 b, float thickness, UI_Color color, UI_ScissorRect scissor);
UI_API void UI_DrawLineEx(UI_Vec2 a, UI_Vec2 b, float thickness, UI_Color a_color, UI_Color b_color, UI_ScissorRect scissor);
UI_API void UI_DrawPolyline(const UI_Vec2* points, const UI_Color* colors, int points_count, float thickness, UI_ScissorRect scissor);
UI_API void UI_DrawPolylineLoop(const UI_Vec2* points, const UI_Color* colors, int points_count, float thickness, UI_ScissorRect scissor);
UI_API void UI_DrawPolylineEx(const UI_Vec2* points, const UI_Color* colors, int points_count, float thickness, bool loop, float split_miter_threshold, UI_ScissorRect scissor);

#ifdef /********************/ UI_IMPLEMENTATION /********************/

// -- Global state ---------
UI_API UI_State UI_STATE;
// -------------------------

#define UI_TODO() __debugbreak()

#define UI_MAX_VERTEX_COUNT 65536*2
#define UI_MAX_INDEX_COUNT  UI_MAX_VERTEX_COUNT*2

#define UI_GLYPH_PADDING 1
#define UI_GLYPH_MAP_SIZE 1024

static const UI_Vec2 UI_WHITE_PIXEL_UV = { 0.5f / (float)UI_GLYPH_MAP_SIZE, 0.5f / (float)UI_GLYPH_MAP_SIZE };

UI_API inline bool UI_MarkGreaterThan(UI_Mark a, UI_Mark b) { return a.line > b.line || (a.line == b.line && a.col > b.col); }
UI_API inline bool UI_MarkLessThan(UI_Mark a, UI_Mark b) { return a.line < b.line || (a.line == b.line && a.col < b.col); }

UI_API UI_Box* UI_AddButton(UI_Key key, UI_Size w, UI_Size h, UI_BoxFlags flags, UI_String string) {
	flags |= UI_BoxFlag_Clickable | UI_BoxFlag_Selectable | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawTransparentBackground;
	UI_Box* box = UI_AddBoxWithText(key, w, h, flags, string);
	return box;
}

UI_API UI_Box* UI_AddDropdownButton(UI_Key key, UI_Size w, UI_Size h, UI_BoxFlags flags, UI_String string) {
	flags |= UI_BoxFlag_LayoutInX | UI_BoxFlag_Clickable | UI_BoxFlag_Selectable | UI_BoxFlag_DrawBorder |UI_BoxFlag_DrawTransparentBackground;
	UI_Box* box = UI_AddBox(key, w, h, flags);
	UI_PushBox(box);

	UI_AddBoxWithText(UI_KEY1(key), UI_SizeFlex(1.f), UI_SizeFit(), 0, string);

	UI_Box* icon_box = UI_AddBoxWithText(UI_KEY1(key), UI_SizeFit(), UI_SizeFit(), 0, STR_("\x44"));
	icon_box->style = UI_MakeStyle();
	icon_box->style->font.font = UI_STATE.icons_font;

	UI_PopBox(box);
	return box;
}

static int UI_ColumnFromXOffset(float x, UI_String line, UI_FontUsage font) {
	DS_ProfEnter();
	int column = 0;
	float start_x = 0.f;
	for STR_Each(line, r, offset) {
		float glyph_width = UI_GlyphWidth(r, font);
		float mid_x = start_x + 0.5f * glyph_width;
		if (x >= mid_x) column++;
		start_x += glyph_width;
	}
	DS_ProfExit();
	return column;
}

static float UI_XOffsetFromColumn(int col, UI_String line, UI_FontUsage font) {
	DS_ProfEnter();
	float x = 0.f;
	int i = 0;
	for STR_Each(line, r, offset) {
		if (i == col) break;
		x += UI_GlyphWidth(r, font);
		i++;
	}
	DS_ProfExit();
	return x;
}

static UI_String UI_GetLineString(int line, const UI_Text* text) {
	DS_ProfEnter();
	int lo = line == 0 ? 0 : DS_ArrGet(text->line_offsets, line - 1);
	int hi = line == text->line_offsets.length ? text->text.length : DS_ArrGet(text->line_offsets, line);
	UI_String result = { text->text.data, hi - lo };
	DS_ProfExit();
	return result;
}

static bool UI_MarkIsValid(UI_Mark mark, const UI_Text* text) {
	if (mark.line < 0 || mark.line > text->line_offsets.length) return false;

	UI_String line_str = UI_GetLineString(mark.line, text);
	return mark.col >= 0 && mark.col <= STR_CodepointCount(line_str);
}

static int UI_MarkToByteOffset(UI_Mark mark, const UI_Text* text) {
	DS_ProfEnter();
	int line_start = mark.line > 0 ? DS_ArrGet(text->line_offsets, mark.line - 1) : 0;
	UI_String after = STR_SliceAfter(UI_TextToStr(*text), line_start);

	int i = 0;
	int result = text->text.length;
	for STR_Each(after, r, offset) {
		if (i == mark.col) {
			result = line_start + offset;
			break;
		}
		i++;
	}

	DS_ProfExit();
	return result;
}

static UI_Vec2 UI_XYOffsetFromMark(const UI_Text* text, UI_Mark mark, UI_FontUsage font) {
	DS_ProfEnter();
	float x = UI_XOffsetFromColumn(mark.col, UI_GetLineString(mark.line, text), font);
	UI_Vec2 result = { x, font.size * mark.line };
	DS_ProfExit();
	return result;
}

static void UI_DrawTextRangeHighlight(UI_Mark min, UI_Mark max, UI_Vec2 text_origin, UI_String text, UI_FontUsage font, UI_Color color, UI_ScissorRect scissor) {
	DS_ProfEnter();
	float min_pos_x = UI_XOffsetFromColumn(min.col, text, font);
	float max_pos_x = UI_XOffsetFromColumn(max.col, text, font); // this is not OK with multiline!

	for (int line = min.line; line <= max.line; line++) {
		UI_Rect rect;
		rect.min = text_origin;
		rect.min.x += (float)line;
		if (line == min.line) {
			rect.min.x += min_pos_x;
		}

		rect.max = text_origin;
		if (line == max.line) {
			rect.max.x += max_pos_x;
		}
		else {
			UI_CHECK(false);
		}

		rect.min.x -= 1.f;
		rect.max.x += 1.f;
		rect.max.y += font.size;
		UI_DrawRect(rect, color, scissor);
	}
	DS_ProfExit();
}

UI_API UI_Key UI_HashKey(UI_Key a, UI_Key b) {
	// Computes MurmurHash64A for two 64-bit integer keys. We could probably use some simpler/faster hash function.
	const uint64_t m = 0xc6a4a7935bd1e995LLU;
	const int r = 47;
	uint64_t h = (sizeof(UI_Key) * 2 * m);
	a *= m;
	a ^= a >> r;
	a *= m;
	h ^= a;
	h *= m;
	b *= m;
	b ^= b >> r;
	b *= m;
	h ^= b;
	h *= m;
	h ^= h >> r;
	h *= m;
	h ^= h >> r;
	return h;
}

//UI_API UI_Key UI_KeyFromN(UI_Key *keys, int keys_count) {
//	return (UI_Key)DS_MurmurHash64A(keys, keys_count * sizeof(UI_Key), 3344);
//}

UI_API UI_Key UI_HashPtr(UI_Key a, void* b) {
	return UI_HashKey(a, (UI_Key)b);
}

UI_API UI_Key UI_HashInt(UI_Key a, int b) {
	return UI_HashKey(a, (UI_Key)b);
}

UI_API void UI_SelectionFixOrder(UI_Selection* sel) {
	DS_ProfEnter();
	if (UI_MarkGreaterThan(sel->range[0], sel->range[1])) {
		UI_Mark tmp = sel->range[1];
		sel->range[1] = sel->range[0];
		sel->range[0] = tmp;
		sel->end = 1 - sel->end;
	}
	DS_ProfExit();
}

UI_API bool UI_PointIsInRect(UI_Rect rect, UI_Vec2 p) {
	bool result = p.x >= rect.min.x && p.y >= rect.min.y && p.x <= rect.max.x && p.y <= rect.max.y;
	return result;
}

UI_API UI_Rect UI_RectIntersection(UI_Rect a, UI_Rect b) {
	return UI_RECT{ {UI_Max(a.min.x, b.min.x), UI_Max(a.min.y, b.min.y)}, {UI_Min(a.max.x, b.max.x), UI_Min(a.max.y, b.max.y)} };
}

static void UI_MoveMarkByWord(UI_Mark* mark, const UI_Text* text, int dir) {
	DS_ProfEnter();
	int byteoffset = UI_MarkToByteOffset(*mark, text);

	bool was_whitespace = false;
	bool was_alnum = false;
	int i = 0;
	uint32_t r;

	uint32_t (*next_codepoint_fn)(UI_String, int*) = dir > 0 ? STR_NextCodepoint : STR_PrevCodepoint;

	for (; r = next_codepoint_fn(UI_TextToStr(*text), &byteoffset); i++) {
		bool whitespace = r == ' ' || r == '\t';
		bool alnum = (r >= 'a' && r <= 'z') || (r >= 'A' && r <= 'Z') || (r >= '0' && r <= '9') || r == '_';

		if (i == 0 && r == '\n') break; // TODO: move to next line

		if (i > 0) {
			if (r == '\n') break;

			if (!alnum && !whitespace && was_alnum) break;
			if (alnum && !was_whitespace && !was_alnum) break;

			if (whitespace && !was_whitespace) break;
			if (!whitespace && was_whitespace && i > 1) break;
		}

		was_whitespace = whitespace;
		was_alnum = alnum;
		mark->col += dir;
	}
	DS_ProfExit();
}

static void UI_MoveMarkH(UI_Mark* mark, const UI_Text* text, int dir, bool ctrl) {
	DS_ProfEnter();
	if (ctrl) {
		UI_MoveMarkByWord(mark, text, dir);
	}
	else if (mark->col + dir < 0) {
		if (mark->line > 0) {
			mark->line -= 1;
			mark->col = STR_CodepointCount(UI_GetLineString(mark->line, text));
		}
	}
	else {
		int end_col = STR_CodepointCount(UI_GetLineString(mark->line, text));

		if (mark->col + dir > end_col) {
			if (mark->line < text->line_offsets.length) {
				mark->line += 1;
				mark->col = 0;
			}
		}
		else {
			mark->col += dir;
		}
	}
	DS_ProfExit();
}

static void UI_EditTextArrowKeyInputX(int dir, const UI_Text* text, UI_Selection* selection, UI_FontUsage font) {
	DS_ProfEnter();

	bool shift = UI_InputIsDown(UI_Input_Shift);
	if (!shift && !UI_MarkEquals(selection->range[0], selection->range[1])) {
		if (dir > 0) {
			selection->range[0] = selection->range[1];
		}
		else {
			selection->range[1] = selection->range[0];
		}
	}
	else {
		UI_Mark* end = &selection->range[selection->end];
		UI_MoveMarkH(end, text, dir, UI_InputIsDown(UI_Input_Control));

		selection->cursor_x = UI_XYOffsetFromMark(text, *end, font).x;

		if (!shift) {
			selection->range[1 - selection->end] = *end;
		}

		UI_SelectionFixOrder(selection);
	}
	DS_ProfExit();
}

//UI_API void UI_SetText(UI_Text *text, UI_String value) {
//	DS_ProfEnter();
//	UI_Selection selection;
//	UI_SelectAll(dst, &selection);
//	UI_EraseRange(dst, selection.range[0], selection.range[1]);
//	UI_InsertText(dst, &selection.range[0], text);
//	DS_ProfExit();
//}

UI_API void UI_ApplyEditTextModify(UI_Text* text, const UI_EditTextModify* request) {
	if (!request->has_edit) return;

	// @speed: I think we could optimize this function by combining the erase and insert steps

	{ // First erase selected range
		int start_byteoffset = UI_MarkToByteOffset(request->replace_from, text);
		int end_byteoffset = UI_MarkToByteOffset(request->replace_to, text);

		int remove_n = end_byteoffset - start_byteoffset;
		DS_ArrRemoveN(&text->text, start_byteoffset, remove_n);

		DS_ArrRemoveN(&text->line_offsets, request->replace_from.line, request->replace_to.line - request->replace_from.line);

		DS_ForArrEach(int, &text->line_offsets, it) {
			*it.ptr -= remove_n;
		}
	}

	{ // Then insert the text
		UI_String insertion = request->replace_with;
		UI_Mark mark = request->replace_from;
		int byteoffset = UI_MarkToByteOffset(mark, text);

		DS_ArrInsertN(&text->text, byteoffset, insertion.data, insertion.size);

		int lines_count = 0;
		for (UI_String remaining = insertion;;) {
			UI_String line_str = STR_ParseUntilAndSkip(&remaining, '\n');
			lines_count++;
			if (remaining.size == 0) break;
		}

		// Should we try to do a new kind of text editing - optionally without storing line offsets?
		if (lines_count > 1) UI_TODO();
		mark.col += STR_CodepointCount(insertion);

		/*
		StrRangeArray line_ranges = StrSplit(UI_FrameArena(), insertion, '\n');

		if (line_ranges.length > 1) {
			int inserted_lines = line_ranges.length - 1;
			DS_ArrResizeUndef(&text->line_offsets, text->line_offsets.length + inserted_lines);

			for (int i = 0; i < line_ranges.length; i++) {
				DS_ArrSet(text->line_offsets, mark.line + i + inserted_lines, DS_ArrGet(text->line_offsets, mark.line + 1));
				DS_ArrSet(text->line_offsets, mark.line + i, byteoffset + DS_ArrGet(line_ranges, i).min);
			}

			mark.col = 0;
			mark.line += inserted_lines;
		}

		StrRange last_line_range = DS_ArrGet(line_ranges, line_ranges.length - 1);
		mark.col += StrRuneCount(StrSlice(insertion, last_line_range.min, last_line_range.max));

		for (int i = mark.line; i < text->line_offsets.length; i++) {
			int *line_offset = DS_ArrGetPtr(text->line_offsets, i);
			*line_offset += insertion.length;
		}*/
	}

	// Update text for the box, otherwise we get 1-frame delay
	request->box_with_text->text = STR_Clone(UI_FrameArena(), UI_TextToStr(*text));
}

UI_API void UI_EditTextSelectAll(const UI_Text* text, UI_Selection* selection) {
	DS_ProfEnter();
	selection->range[0] = UI_MARK{0};
	selection->range[1] = UI_MARK{
		text->line_offsets.length,
		STR_CodepointCount(UI_GetLineString(text->line_offsets.length, text)),
	};
	selection->end = 1;
	DS_ProfExit();
}

UI_API UI_Box* UI_AddValNumeric(UI_Key key, UI_Size w, UI_Size h, void* value_64_bit, bool is_signed, bool is_float)
{
	DS_ProfEnter();

	UI_Box* box = NULL;
	bool dragging = UI_IsClickingDown(key) && UI_InputIsDown(UI_Input_MouseLeft);
	bool has_moved_mouse_after_press = UI_Abs(UI_STATE.mouse_travel_distance_after_press.x) >= 2.f;

	UI_String value_str;
	if (is_float) {
		value_str = STR_FloatToStr(UI_FrameArena(), *(double*)value_64_bit, 1 /*dragging && has_moved_mouse_after_press ? 1 : 1*/);
	} else {
		value_str = STR_IntToStrEx(UI_FrameArena(), *(uint64_t*)value_64_bit, is_signed, 10);
	}

	bool activate_by_enter = UI_Pressed(key) && UI_InputWasPressed(UI_Input_Enter);
	bool activate_by_click = UI_Clicked(key) && !has_moved_mouse_after_press && UI_InputWasReleased(UI_Input_MouseLeft);
	bool did_begin_selection = UI_STATE.selected_box_new == key && UI_STATE.selected_box != key;
	bool activate_by_keyboard_navigation = did_begin_selection && !UI_InputIsDown(UI_Input_MouseLeft); // this UI_InputIsDown for mouse is a bit of a dumb hack
	bool activate = activate_by_enter || activate_by_click || activate_by_keyboard_navigation;

	bool text_edit_was_activated = activate && UI_STATE.text_editing_box != key;

	if (text_edit_was_activated) {
		UI_STATE.edit_text_should_refresh = true;
		UI_STATE.selected_box_new = key;
		UI_TextSet(&UI_STATE.edit_number_text, value_str);
	}

	bool text_editing_this = UI_STATE.text_editing_box == key && UI_STATE.selected_box_new == UI_INVALID_KEY; // The `selected_box_new` check is here as keyboard navigation could have already selected a different box
	if (text_editing_this || text_edit_was_activated) {
		box = UI_AddValText(key, w, h, &UI_STATE.edit_number_text, NULL);

		if (is_float) {
			STR_ParseFloat(UI_TextToStr(UI_STATE.edit_number_text), (double*)value_64_bit);
		} else if (is_signed) {
			STR_ParseI64(UI_TextToStr(UI_STATE.edit_number_text), (int64_t*)value_64_bit);
		} else {
			STR_ParseU64Ex(UI_TextToStr(UI_STATE.edit_number_text), 10, (uint64_t*)value_64_bit);
		}
	}
	else {
		box = UI_AddBoxWithText(key, w, h,
			UI_BoxFlag_Clickable | UI_BoxFlag_Selectable | UI_BoxFlag_DrawBorder | UI_BoxFlag_PressingStaysWithoutHover, value_str);

		if (UI_Pressed(box->key)) {
			UI_STATE.outputs.lock_and_hide_cursor = true;
			
			if (is_float) {
				UI_STATE.edit_number_value_before_press = *(double*)value_64_bit;
			} else if (is_signed) {
				UI_STATE.edit_number_value_before_press = (double)*(int64_t*)value_64_bit;
			} else {
				UI_STATE.edit_number_value_before_press = (double)*(uint64_t*)value_64_bit;
			}
		}

		if (UI_IsHovered(box->key)) {
			UI_STATE.outputs.cursor = UI_MouseCursor_ResizeH;
		}

		if (dragging) {
			UI_STATE.outputs.lock_and_hide_cursor = true;

			double new_value = UI_STATE.edit_number_value_before_press + UI_STATE.mouse_travel_distance_after_press.x * 0.05f;

			if (is_float) {
				*(double*)value_64_bit = new_value;
			} else if (is_signed) {
				*(int64_t*)value_64_bit = (int64_t)new_value;
			} else {
				*(uint64_t*)value_64_bit = (uint64_t)new_value;
			}
		}
	}

	DS_ProfExit();
	return box;
}

UI_API UI_Box* UI_AddValInt(UI_Key key, UI_Size w, UI_Size h, int* value) {
	int64_t value_i64 = *value;
	UI_Box* box = UI_AddValNumeric(key, w, h, &value_i64, true, false);
	*value = (int)value_i64;
	return box;
}

UI_API UI_Box* UI_AddValInt64(UI_Key key, UI_Size w, UI_Size h, int64_t* value) {
	return UI_AddValNumeric(key, w, h, value, true, false);
}

UI_API UI_Box* UI_AddValUInt64(UI_Key key, UI_Size w, UI_Size h, uint64_t* value) {
	return UI_AddValNumeric(key, w, h, value, false, false);
}

UI_API UI_Box* UI_AddValFloat(UI_Key key, UI_Size w, UI_Size h, float* value) {
	double value_f64 = (double)*value;
	UI_Box* box = UI_AddValNumeric(key, w, h, &value_f64, false, true);
	*value = (float)value_f64;
	return box;
}

UI_API UI_Box* UI_AddValFloat64(UI_Key key, UI_Size w, UI_Size h, double* value) {
	return UI_AddValNumeric(key, w, h, value, false, true);
}

UI_API UI_Box* UI_AddValFilepath(UI_Key key, UI_Size w, UI_Size h, UI_Text* filepath, UI_EditTextModify* out_modify) {
	DS_ProfEnter();
	UI_Box* box = UI_AddBox(key, w, h, UI_BoxFlag_LayoutInX);
	UI_PushBox(box);

	UI_Key edit_text_key = UI_KEY1(key);
	UI_AddValText(edit_text_key, UI_SizeFlex(1.f), UI_SizeFlex(1.f), filepath, out_modify);

	UI_Style* button_style = UI_PushStyle();
	button_style->font.font = UI_STATE.icons_font;
	UI_Box* button = UI_AddButton(UI_KEY1(key), UI_SizeFit(), UI_SizeFlex(1.f), 0, STR_("\x42"));
	UI_PopStyle(button_style);

	if (UI_Clicked(button->key)) {
		UI_TODO();
		// UI_String pick_result;
		// if (OS_FilePicker(UI_FrameArena(), &pick_result)) {
		// 	UI_TODO();
		// 	// UI_Selection selection;
		// 	// UI_SelectAll(filepath, &selection);
		// 	// UI_ReplaceSelectionWithText(filepath, &selection, pick_result, box->style->font);
		// 	// result.edited = true;
		// }

		//OS_FilePicker(
		//if (file.length) {
		//	// reset scroll
		//	(*UI_.box_from_key[edit_text_key])->scroll_target = {};
		//
		//}
	}

	UI_PopBox(box);
	DS_ProfExit();
	return box;
}

static void UI_EditTextModifyReplaceRange(UI_Selection* selection, UI_Mark from, UI_Mark to, UI_String with, UI_EditTextModify* out_edit_request) {
	UI_CHECK(!out_edit_request->has_edit);
	out_edit_request->has_edit = true;
	out_edit_request->replace_from = selection->range[0];
	out_edit_request->replace_to = selection->range[1];
	out_edit_request->replace_with = with;

	// Move the selection
	UI_Mark mark = selection->range[0];

	int lines_count = 0;
	UI_String last_line_str = {0};
	for (UI_String remaining = with;;) {
		last_line_str = STR_ParseUntilAndSkip(&remaining, '\n');
		lines_count++;
		if (remaining.size == 0) break;
	}

	mark.line += lines_count - 1;
	if (lines_count > 1) mark.col = 0;

	mark.col += STR_CodepointCount(last_line_str);
	selection->range[0] = mark;
	selection->range[1] = mark;
}

UI_API void UI_TextInit(DS_Allocator* allocator, UI_Text* text, UI_String initial_value) {
	memset(text, 0, sizeof(*text));
	DS_ArrInit(&text->text, allocator);
	DS_ArrInit(&text->line_offsets, allocator);
	UI_TextSet(text, initial_value);
}

UI_API void UI_TextDeinit(UI_Text* text) {
	DS_ArrDeinit(&text->line_offsets);
	DS_ArrDeinit(&text->text);
}

UI_API void UI_TextSet(UI_Text* text, UI_String value) {
	DS_ArrClear(&text->text);
	DS_ArrPushN(&text->text, value.data, value.size);
}

UI_API UI_Box* UI_AddValText(UI_Key key, UI_Size w, UI_Size h, UI_Text* text, UI_EditTextModify* out_modify) {
	DS_ProfEnter();
	UI_Style* style = UI_PeekStyle();
	UI_FontUsage font = style->font;

	UI_EditTextModify default_modify;
	UI_EditTextModify* modify = out_modify ? out_modify : &default_modify;
	memset(modify, 0, sizeof(*modify));

	UI_Box* outer = UI_AddBox(key, w, h, UI_BoxFlag_Selectable | UI_BoxFlag_DrawBorder | UI_BoxFlag_Clickable);
	UI_PushBox(outer);

	UI_Box* inner = UI_AddBoxWithText(UI_KEY1(key), UI_SizeFit(), UI_SizeFit(), 0, UI_TextToStr(*text));
	modify->box_with_text = inner;

	bool editing = UI_STATE.text_editing_box == key;
	bool pressed_this = UI_Pressed(key);
	bool activate_by_becoming_selected = UI_STATE.selected_box_new == key && UI_STATE.selected_box != key || UI_STATE.edit_text_should_refresh;
	
	if (pressed_this || activate_by_becoming_selected) {
		editing = true;
	}

	if ((UI_InputWasPressed(UI_Input_MouseLeft) && !pressed_this) ||
		(UI_STATE.text_editing_box == key && UI_InputWasPressed(UI_Input_Enter)) ||
		UI_InputWasPressed(UI_Input_Escape) ||
		UI_STATE.selected_box_new != key)
	{
		editing = false;
	}

	if (editing) {
		UI_STATE.text_editing_box_new = key;

		UI_Selection* selection = &UI_STATE.edit_text_selection;

		if (activate_by_becoming_selected ||
			UI_STATE.edit_text_should_refresh ||
			(UI_InputWasPressedOrRepeated(UI_Input_A) && UI_InputIsDown(UI_Input_Control)))
		{
			UI_EditTextSelectAll(text, selection);
		}

		if (UI_InputWasPressedOrRepeated(UI_Input_Right)) {
			UI_EditTextArrowKeyInputX(1, text, selection, font);
		}

		if (UI_InputWasPressedOrRepeated(UI_Input_Left)) {
			UI_EditTextArrowKeyInputX(-1, text, selection, font);
		}

		if (pressed_this || UI_STATE.mouse_clicking_down_box == key) {
			if (inner->prev_frame) {
				float origin = inner->prev_frame->computed_position.x + style->text_padding.x;
			
				int col = UI_ColumnFromXOffset(UI_STATE.mouse_pos.x - origin, UI_GetLineString(0, text), font);
				selection->range[selection->end].col = col;
				if (pressed_this) selection->range[1 - selection->end].col = col;
				UI_SelectionFixOrder(selection);
			}
		}
		
		if (UI_STATE.inputs.text_input_utf32_length > 0) {
			STR_Builder text_input = { UI_FrameArena() };
			for (int i = 0; i < UI_STATE.inputs.text_input_utf32_length; i++) {
				STR_PrintC(&text_input, UI_STATE.inputs.text_input_utf32[i]);
			}
			UI_EditTextModifyReplaceRange(selection, selection->range[0], selection->range[1], text_input.str, modify);
		}

		if (UI_InputWasPressedOrRepeated(UI_Input_Home)) {
			UI_Mark* end = &selection->range[selection->end];
			end->col = 0;
			if (!UI_InputIsDown(UI_Input_Shift)) {
				selection->range[1 - selection->end] = *end;
			}
			UI_SelectionFixOrder(selection);
		}

		if (UI_InputWasPressedOrRepeated(UI_Input_End)) {
			UI_TODO(); // UI_Mark *end = &selection->range[selection->end];
			// end->col = StrRuneCount(UI_GetLineString(end->line, text));
			// if (!UI_InputIsDown(UI_Input_Shift)) {
			// 	selection->range[1 - selection->end] = *end;
			// }
			// UI_SelectionFixOrder(selection);
		}

		//if (UI_InputWasPressedOrRepeated(UI_Input_X) && UI_InputIsDown(UI_Input_Control)) {
		//	UI_TODO(); // UI_CopySelectionToClipboard(selection, text);
		//	UI_TODO(); //UI_EraseRangeSel(text, selection, font);
		//	// result.edited = true;
		//}

		// Ctrl C
		if (UI_InputWasPressedOrRepeated(UI_Input_C) && UI_InputIsDown(UI_Input_Control)) {
			if (UI_STATE.inputs.set_clipboard_string_fn) {
				int min = UI_MarkToByteOffset(selection->range[0], text);
				int max = UI_MarkToByteOffset(selection->range[1], text);
				UI_STATE.inputs.set_clipboard_string_fn(STR_Slice(UI_TextToStr(*text), min, max), UI_STATE.inputs.user_data);
			}
		}

		// Ctrl V
		if (UI_InputWasPressedOrRepeated(UI_Input_V) && UI_InputIsDown(UI_Input_Control)) {
			if (UI_STATE.inputs.get_clipboard_string_fn) {
				UI_String str = UI_STATE.inputs.get_clipboard_string_fn(UI_STATE.inputs.user_data);
				UI_EditTextModifyReplaceRange(selection, selection->range[0], selection->range[1], str, modify);
			}
		}

		if (UI_InputWasPressedOrRepeated(UI_Input_Backspace)) {
			if (UI_MarkEquals(selection->range[0], selection->range[1])) {
				UI_MoveMarkH(&selection->range[0], text, -1, UI_InputIsDown(UI_Input_Control));
			}
			UI_EditTextModifyReplaceRange(selection, selection->range[0], selection->range[1], STR_(""), modify);
		}

		// UI_STATE.edit_text.draw_selection_from_box = inner;
		// UI_STATE.edit_text.draw_selection_from_box_sel = *selection;
	}

	UI_STATE.edit_text_should_refresh = false;

	UI_PopBox(outer);

	if (UI_IsHovered(outer->key)) {
		UI_STATE.outputs.cursor = UI_MouseCursor_I_beam;
	}

	if (out_modify == NULL) {
		UI_ApplyEditTextModify(text, modify);
	}

	DS_ProfExit();
	return outer;
}

UI_API void UI_AddCheckbox(UI_Key key, bool* value) {
	DS_ProfEnter();
	UI_Style* style = UI_PeekStyle();
	float h = style->font.size + 2.f * style->text_padding.y;

	UI_Style* outer_style = UI_PushStyle();
	outer_style->child_padding = UI_VEC2{ 5.f, 5.f };
	UI_Box* box = UI_AddBox(UI_KEY1(key), h, h, UI_BoxFlag_ChildPadding);
	UI_PopStyle(outer_style);
	UI_PushBox(box);

	UI_Style* inner_style = UI_PushStyle();
	inner_style->text_padding = UI_VEC2{ 5.f, 2.f };
	inner_style->font.font = UI_STATE.icons_font;

	UI_Box* inner;
	UI_BoxFlags inner_flags = UI_BoxFlag_Clickable | UI_BoxFlag_Selectable | UI_BoxFlag_DrawBorder;
	UI_Key inner_key = UI_KEY1(key);
	if (*value) {
		inner = UI_AddBoxWithText(inner_key, UI_SizeFlex(1.f), UI_SizeFlex(1.f), inner_flags, STR_("A"));
	}
	else {
		inner = UI_AddBox(inner_key, UI_SizeFlex(1.f), UI_SizeFlex(1.f), inner_flags);
	}
	UI_PopStyle(inner_style);
	UI_PopBox(box);

	if (UI_Pressed(inner->key)) *value = !*value;
	DS_ProfExit();
}

UI_API UI_Box* UI_AddBoxWithText(UI_Key key, UI_Size w, UI_Size h, UI_BoxFlags flags, UI_String string) {
	UI_Box* box = UI_AddBox(key, w, h, flags | UI_BoxFlag_DrawText);
	box->text = STR_Clone(UI_FrameArena(), string);
	return box;
}

UI_API UI_Box* UI_PushCollapsing(UI_Key key, UI_Size w, UI_Size h, UI_Size indent, UI_BoxFlags flags, UI_String text) {
	DS_ProfEnter();
	UI_Key child_box_key = UI_KEY1(key);
	UI_BoxFlags box_flags = UI_BoxFlag_LayoutInX | UI_BoxFlag_Clickable | UI_BoxFlag_Selectable | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawTransparentBackground;
	UI_Box* box = UI_AddBox(key, UI_SizeFlex(1.f), h, box_flags);
	UI_PushBox(box);

	bool is_open = UI_PrevFrameBoxFromKey(child_box_key) != NULL;
	if (UI_Pressed(box->key)) {
		is_open = !is_open;
	}

	UI_Style* label_style = UI_PushStyle();
	label_style->font.font = UI_STATE.icons_font;
	UI_AddBoxWithText(UI_KEY1(key), 20.f, h, 0, is_open ? STR_("\x44") : STR_("\x46"));
	UI_PopStyle(label_style);

	UI_AddBoxWithText(UI_KEY1(key), w, h, 0, text);

	UI_PopBox(box);

	UI_Box* outer_child_box = NULL;
	UI_Box* inner_child_box = NULL;
	if (is_open) {
		outer_child_box = UI_AddBox(child_box_key, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_LayoutInX);
		UI_PushBox(outer_child_box);
		UI_AddBox(UI_KEY1(key), indent, UI_SizeFit(), 0);
		
		inner_child_box = UI_AddBox(UI_KEY1(key), UI_SizeFlex(1.f), UI_SizeFit(), flags);
		UI_PushBox(inner_child_box);
	}
	DS_ProfExit();
	return inner_child_box;
}

UI_API void UI_PopCollapsing(UI_Box* box) {
	UI_PopBox(box);
	UI_PopBox(box->parent);
}

UI_API UI_Box* UI_PushScrollArea(UI_Key key, UI_Size w, UI_Size h, UI_BoxFlags flags, int anchor_x, int anchor_y) {
	DS_ProfEnter();

	UI_Key content_box_key = UI_KEY1(key);
	UI_Key temp_box_keys[2] = { UI_KEY1(key), UI_KEY1(key) };
	int anchors[] = { anchor_x, anchor_y };

	UI_Box* content_prev_frame = UI_PrevFrameBoxFromKey(content_box_key); // may return NULL
	UI_Box* deepest_temp_box_prev_frame = UI_PrevFrameBoxFromKey(temp_box_keys[0]); // may return NULL
	
	UI_Box* temp_boxes[2] = {0};

	UI_Box* parent = UI_AddBox(key, w, h, UI_BoxFlag_LayoutInX | flags);
	UI_PushBox(parent);

	UI_Vec2 offset = { 0.f, 0.f };

	// TODO: turn this into "PushScrollArea2D" and have a separate function for PushScrollArea. The main difference is that with
	// a 1D scroll area, it should be possible to do, say, flex child boxes in X while scrolling in Y. Or maybe that should be
	// possible with 2D scroll area too, idk.

	// We should automatically add either X or Y scrollbars
	for (int y = 1; y >= 0; y--) { // y is the direction of the scroll bar
		int x = 1 - y;
		UI_Key y_key = UI_HashInt(UI_KEY1(key), y);

		UI_Size size[2];
		size[x] = UI_SizeFlex(1.f);
		size[y] = UI_SizeFlex(1.f);
		//size[x] = UI_SIZE{ 0.f, 1.f, 1.f, 1.f };
		//size[y] = UI_SIZE{ 0.f, 1.f, 1.f, 1.f };
		UI_Box* temp_box = UI_AddBox(temp_box_keys[y], size[0], size[1], 0/*UI_BoxFlag_NoScissor*/);
		temp_boxes[y] = temp_box;

		// Use the content size from the previous frame
		float content_length_px = content_prev_frame ? content_prev_frame->computed_unexpanded_size._[y] : 0.f;
		float visible_area_length_px = deepest_temp_box_prev_frame ? deepest_temp_box_prev_frame->computed_size._[y] : 0.f;
		float rail_line_length_px = temp_box->prev_frame ? temp_box->prev_frame->computed_size._[y] : 0.f;

		if (content_length_px > visible_area_length_px) {
			size[x] = 18.f;
			size[y] = UI_SizeFlex(1.f);

			UI_BoxFlags layout_dir_flag = y == UI_Axis_X ? UI_BoxFlag_LayoutInX : 0;
			UI_Box* rail_line_box = UI_AddBox(UI_KEY1(y_key), size[0], size[1], layout_dir_flag | UI_BoxFlag_DrawBorder);
			if (anchors[y] == 1) {
				rail_line_box->flags |= (y == 1 ? UI_BoxFlag_LayoutFromEndY : UI_BoxFlag_LayoutFromEndX);
			}

			UI_PushBox(rail_line_box);

			// TODO: we should pass a key parameter to attach the storage of the scroll wheel offset to.
			// That way e.g. if we have a collapsible and a scroll bar inside, the scroll bar offset wouldn't be lost on close/reopen.

			offset._[y] = content_prev_frame ? content_prev_frame->offset._[y] : 0.f;

			float scrollbar_distance_ratio = (anchors[y] == 1 ? 1.f : -1.f) * (offset._[y] / content_length_px);
			float scrollbar_length_ratio = visible_area_length_px / content_length_px; // between 0 and 1

			size[x] = 18.f;
			size[y] = scrollbar_distance_ratio * rail_line_length_px;
			UI_Box* pad_before_bar = UI_AddBox(UI_KEY1(y_key), size[0], size[1], UI_BoxFlag_PressingStaysWithoutHover);

			size[x] = 18.f;
			size[y] = rail_line_length_px * scrollbar_length_ratio;
			UI_Box* scrollbar = UI_AddBox(UI_KEY1(y_key), size[0], size[1],
				UI_BoxFlag_PressingStaysWithoutHover | UI_BoxFlag_Clickable | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawTransparentBackground);

			size[x] = 18.f;
			size[y] = UI_SizeFlex(1.f);
			UI_Box* pad_after_bar = UI_AddBox(UI_KEY1(y_key), size[0], size[1], UI_BoxFlag_PressingStaysWithoutHover);

			UI_PopBox(rail_line_box);

			// scrollbar inputs

			if (UI_Pressed(scrollbar->key)) {
				UI_STATE.scrollbar_origin_before_press = offset._[y];
			}

			if (UI_IsClickingDown(scrollbar->key)) {
				float mouse_delta = UI_STATE.mouse_pos._[y] - UI_STATE.last_released_mouse_pos._[y];
				float ratio_of_scrollbar_moved = mouse_delta / rail_line_length_px;
				offset._[y] = UI_STATE.scrollbar_origin_before_press - ratio_of_scrollbar_moved * content_length_px;
			}

			if (UI_IsClickingDown(pad_before_bar->key) || UI_IsClickingDown(pad_after_bar->key)) {
				float origin = parent->prev_frame ? parent->prev_frame->computed_position._[y] : 0.f;
				float scroll_ratio = (UI_STATE.mouse_pos._[y] - origin) / rail_line_length_px - 0.5f * scrollbar_length_ratio;

				offset._[y] = -scroll_ratio * content_length_px;
				if (anchors[y] == 1) {
					offset._[y] += content_length_px - visible_area_length_px;
				}
			}

			// idea: hold middle click and move mouse could scroll in X and Y

			if (UI_IsHovered(parent->key) && y == UI_Axis_Y) {
				offset._[y] += UI_STATE.inputs.mouse_wheel_delta * 30.f;
			}

			if (anchors[y] == 1) {
				offset._[y] = UI_Min(offset._[y], content_length_px - visible_area_length_px);
				offset._[y] = UI_Max(offset._[y], 0);
			}
			else {
				offset._[y] = UI_Max(offset._[y], visible_area_length_px - content_length_px);
				offset._[y] = UI_Min(offset._[y], 0);
			}
		}

		UI_PushBox(temp_box);
	}

	UI_Box* content = UI_AddBox(content_box_key, UI_SizeFlex(1.f), UI_SizeFlex(1.f), UI_BoxFlag_NoFlexDownX|UI_BoxFlag_NoFlexDownY);
	content->offset = offset;
	UI_PushBox(content);

	if (anchor_x == 1) temp_boxes[0]->flags |= UI_BoxFlag_LayoutFromEndX;
	if (anchor_y == 1) temp_boxes[0]->flags |= UI_BoxFlag_LayoutFromEndY;

	DS_ProfExit();
	return parent;
}

UI_API void UI_PopScrollArea(UI_Box* box) {
	UI_PopBoxN(box, 4);
}

// The "backdrop" of a box includes background, opaque background, border and hover/click highlighting
UI_API void UI_DrawBoxBackdrop(UI_Box* box) {
	DS_ProfEnter();

	//UI_Rect box_rect = box->computed_rect_clipped;
	UI_Rect box_rect = { box->computed_position, UI_AddV2(box->computed_position, box->computed_size) };
	UI_ScissorRect scissor = box->flags & UI_BoxFlag_NoScissor ? NULL : &box->computed_rect_clipped;

	if (box->flags & UI_BoxFlag_DrawTransparentBackground) {
		UI_DrawRectRounded(box_rect, 4.f, box->style->transparent_bg_color, 2, scissor);
	}

	if (box->flags & UI_BoxFlag_DrawOpaqueBackground) {
		UI_DrawRectRounded(box_rect, 4.f, box->style->opaque_bg_color, 2, scissor);
		//UI_DrawRectEx(box_rect, box->style->opaque_bg_color, 12.f, 1.f, UI_INFINITE, scissor);
	}

	if (box->flags & UI_BoxFlag_DrawBorder) {
		UI_DrawRectLinesRounded(box_rect, 2.f, 4.f, box->style->border_color, scissor);
	}

	if (box->flags & UI_BoxFlag_Clickable) {
		float r = 5.f;
		{
			float hovered = box->lazy_is_hovered * (1.f - box->lazy_is_holding_down); // We don't want to show the hover highlight when holding down
			const UI_Color top = UI_COLOR{ 255, 255, 255, (uint8_t)(hovered * 20.f) };
			const UI_Color bot = UI_COLOR{ 255, 255, 255, (uint8_t)(hovered * 10.f) };
			UI_DrawRectCorners corners = {
				{top, top, bot, bot},
				{top, top, bot, bot},
				{r, r, r, r} };
			UI_DrawRectEx(box_rect, &corners, 2, scissor);
		}
		{
			const UI_Color top = UI_COLOR{ 0, 0, 0, (uint8_t)(box->lazy_is_holding_down * 100.f) };
			const UI_Color bot = UI_COLOR{ 0, 0, 0, (uint8_t)(box->lazy_is_holding_down * 20.f) };
			UI_DrawRectCorners corners = {
				{top, top, bot, bot},
				{top, top, bot, bot},
				{r, r, r, r} };
			UI_DrawRectEx(box_rect, &corners, 2, scissor);
		}
	}
	DS_ProfExit();
}

UI_API void UI_DrawBox(UI_Box* box) {
	if (box->draw_override) {
		box->draw_override(box);
	} else {
		UI_DrawBoxDefault(box);
	}
}

UI_API void UI_DrawBoxDefault(UI_Box* box) {
	DS_ProfEnter();

	UI_CHECK(box->flags & UI_BoxFlag_HasComputedRects);
	UI_ScissorRect scissor = box->flags & UI_BoxFlag_NoScissor ? NULL : &box->computed_rect_clipped;

	if (box->flags & UI_BoxFlag_DrawOpaqueBackground) {
		// Draw drop shadow, BEFORE applying the clipping rect
		const float shadow_distance = 10.f;

		UI_Rect rect = box->computed_rect_clipped;
		rect.min = UI_SubV2(rect.min, UI_VEC2{ 0.5f * shadow_distance, 0.5f * shadow_distance });
		rect.max = UI_AddV2(rect.max, UI_VEC2{ shadow_distance, shadow_distance });
		UI_DrawRectRounded2(rect, 2.f * shadow_distance, UI_COLOR{ 0, 0, 0, 50 }, UI_COLOR{ 0, 0, 0, 0 }, 2, NULL);
		//UI_DrawRect(rect, UI_RED, NULL);

	//UI_DrawRectEx(rect, UI_COLOR{0, 0, 0, 100}, shadow_distance, 2.f*shadow_distance, UI_INFINITE);
	}

	UI_DrawBoxBackdrop(box);

	if (UI_IsSelected(box->key) && UI_STATE.selection_is_visible) {
		UI_Rect box_rect = box->computed_rect_clipped;
		UI_Color color = UI_COLOR{ 250, 200, 85, 240 };
		UI_DrawRectLinesRounded(box_rect, 2.f, 4.f, color, scissor);
	}

	if (box->flags & UI_BoxFlag_DrawText) {
		UI_Vec2 text_pos = UI_AddV2(box->computed_position, box->style->text_padding);
		UI_DrawText(box->text, box->style->font, text_pos, UI_AlignH_Left, UI_AlignV_Upper, box->style->text_color, scissor);
	}

	if (box->parent && UI_STATE.text_editing_box_new == box->parent->key) {
		//UI_Vec2 P = box->computed_position;
		//P.x += box->style->text_padding.x;
		//UI_DrawLine(P, UI_AddV2(P, UI_VEC2{0, 5.f}), 2.f, UI_BLUE, NULL);

		UI_Selection sel = UI_STATE.edit_text_selection;
		UI_DrawTextRangeHighlight(sel.range[0], sel.range[1], UI_AddV2(box->computed_position, box->style->text_padding), box->text, box->style->font, UI_COLOR{255, 255, 255, 50}, scissor);

		UI_Mark end = sel.range[sel.end];
		UI_DrawTextRangeHighlight(end, end, UI_AddV2(box->computed_position, box->style->text_padding), box->text, box->style->font, UI_COLOR{255, 255, 255, 255}, scissor);
	}
	
	for (UI_Box* child = box->first_child[0]; child; child = child->next[1]) {
		UI_DrawBox(child);
	}

	//if (!(box->flags & UI_BoxFlag_NoScissor)) {
	//	//UI_PopScissor();
	//}
	DS_ProfExit();
}

UI_API bool UI_HasMovedMouseAfterPressed(void) {
	UI_Vec2 delta = UI_SubV2(UI_STATE.mouse_pos, UI_STATE.last_pressed_mouse_pos);
	return delta.x * delta.x + delta.y * delta.y > 4.f;
}

UI_API bool UI_ClickedAnywhere(void) {
	bool clicked = UI_InputWasReleased(UI_Input_MouseLeft) || UI_InputWasReleased(UI_Input_Enter);
	return clicked;
}

UI_API bool UI_Pressed(UI_Key key) {
	return UI_PressedEx(key, UI_Input_MouseLeft);
}

UI_API bool UI_PressedEx(UI_Key key, UI_Input mouse_button) {
	bool pressed = UI_InputWasPressed(mouse_button) && UI_IsHovered(key);
	pressed = pressed || (UI_STATE.selected_box == key && UI_STATE.selection_is_visible && UI_InputWasPressed(UI_Input_Enter));
	return pressed;
}

UI_API bool UI_PressedIdleEx(UI_Key key, UI_Input mouse_button) {
	bool pressed = UI_InputWasPressed(mouse_button) && UI_IsHoveredIdle(key);
	pressed = pressed || (UI_STATE.selected_box == key && UI_STATE.selection_is_visible && UI_InputWasPressed(UI_Input_Enter));
	return pressed;
}

UI_API bool UI_PressedIdle(UI_Key key) {
	return UI_PressedIdleEx(key, UI_Input_MouseLeft);
}

UI_API bool UI_Clicked(UI_Key key) {
	bool clicked = UI_IsClickingDownAndHovered(key) &&
		(UI_InputWasReleased(UI_Input_MouseLeft) || (UI_STATE.selection_is_visible && UI_InputWasReleased(UI_Input_Enter)));
	return clicked;
}

UI_API bool UI_DoubleClickedAnywhere(void) {
	bool result = UI_InputWasPressed(UI_Input_MouseLeft);
	if (result && UI_InputIsDown(UI_Input_Shift)) UI_TODO();

	result = result && !UI_HasMovedMouseAfterPressed();
	result = result && UI_STATE.time_since_pressed_lmb < 0.2f;
	return result;
}

UI_API bool UI_DoubleClicked(UI_Key key) {
	bool result = UI_Pressed(key) && UI_DoubleClickedAnywhere();
	return result;
}

UI_API bool UI_DoubleClickedIdle(UI_Key key) {
	bool result = UI_PressedIdle(key) && UI_DoubleClickedAnywhere();
	return result;
}

UI_API bool UI_IsSelected(UI_Key key) {
	return key == UI_STATE.selected_box;
}

UI_API bool UI_IsClickingDown(UI_Key key) {
	return UI_STATE.mouse_clicking_down_box == key || UI_STATE.keyboard_clicking_down_box == key;
}

UI_API bool UI_IsClickingDownAndHovered(UI_Key key) {
	return (UI_STATE.keyboard_clicking_down_box == key && UI_STATE.selected_box == key) ||
		(UI_STATE.mouse_clicking_down_box == key && UI_IsHovered(key));
}

UI_API bool UI_IsHovered(UI_Key key) {
	UI_Box* box = UI_PrevFrameBoxFromKey(key);
	bool result = box != NULL && !(box->flags & UI_BoxFlag_NoHover) && UI_PointIsInRect(box->computed_rect_clipped, UI_STATE.mouse_pos);
	return result;
}

UI_API bool UI_IsMouseInsideOf(UI_Key key) {
	UI_Box* box = UI_PrevFrameBoxFromKey(key);
	bool result = box != NULL && UI_PointIsInRect(box->computed_rect_clipped, UI_STATE.mouse_pos);
	return result;
}

static bool UI_HasAnyHoveredClickableChild_(UI_Box* box) {
	for (UI_Box* child = box->first_child[0]; child; child = child->next[1]) {
		if (UI_PointIsInRect(child->computed_rect_clipped, UI_STATE.mouse_pos)) {
			if (child->flags & UI_BoxFlag_Clickable) {
				return true;
			}
			if (UI_HasAnyHoveredClickableChild_(child)) {
				return true;
			}
		}
	}
	return false;
}

UI_API bool UI_IsHoveredIdle(UI_Key key) {
	UI_Box* box = UI_PrevFrameBoxFromKey(key);
	bool result = box != NULL && !(box->flags & UI_BoxFlag_NoHover) && UI_PointIsInRect(box->computed_rect_clipped, UI_STATE.mouse_pos);
	result = result && !UI_HasAnyHoveredClickableChild_(box);
	return result;
}

UI_API UI_Style* UI_PeekStyle(void) {
	return DS_ArrPeek(UI_STATE.style_stack);
}

UI_API UI_Style* UI_MakeStyle(void) {
	return DS_Clone(UI_Style, UI_FrameArena(), *DS_ArrPeek(UI_STATE.style_stack));
}

UI_API UI_Style* UI_PushStyle(void) {
	UI_Style* style = DS_Clone(UI_Style, UI_FrameArena(), *DS_ArrPeek(UI_STATE.style_stack));
	DS_ArrPush(&UI_STATE.style_stack, style);
	return style;
}

UI_API void UI_PopStyle(UI_Style* style) {
	UI_CHECK(DS_ArrPeek(UI_STATE.style_stack) == style);
	DS_ArrPop(&UI_STATE.style_stack);
}

//UI_API UI_Data* UI_DataFromKey(UI_Key key) {
//	UI_Data* p_data = NULL;
//	bool newly_added = DS_MapGetOrAddPtr(&UI_STATE.imm_new.data_from_key, key, &p_data);
//	if (newly_added) {
//		UI_Data prev_frame_or_empty = {0};
//		DS_MapFind(&UI_STATE.imm_old.data_from_key, key, &prev_frame_or_empty);
//		*p_data = prev_frame_or_empty;
//	}
//	return p_data;
//}

UI_API UI_Box* UI_PrevFrameBoxFromKey(UI_Key key) {
	UI_Box* box = NULL;
	DS_MapFind(&UI_STATE.prev_frame_box_from_key, key, &box);
	return box;
}

UI_API void UI_BoxAddCustomData(UI_Box* box, UI_Key key, void* ptr, int size) {
	UI_BoxCustomDataHeader* header = (UI_BoxCustomDataHeader*)DS_ArenaPush(&UI_STATE.frame_arena, sizeof(UI_BoxCustomDataHeader) + size);
	header->key = key;
	header->next = box->custom_data_list;
	header->debug_size = size;
	box->custom_data_list = header;
	memcpy(header + 1, ptr, size);
}

UI_API void* UI_BoxGetCustomData(UI_Box* box, UI_Key key, int size) {
	void* result = NULL;
	for (UI_BoxCustomDataHeader* it = box->custom_data_list; it; it = it->next) {
		if (it->key == key) {
			UI_CHECK(size == it->debug_size);
			result = it + 1;
			break;
		}
	}
	return result;
}

UI_API UI_Box* UI_BoxFromKey(UI_Key key) {
	UI_Box* box = NULL;
	DS_MapFind(&UI_STATE.box_from_key, key, &box);
	return box;
}

UI_API bool UI_SelectionMovementInput(UI_Box* node, UI_Key* out_new_selected_box) {
	DS_ProfEnter();
	// There are two strategies for tab-navigation. At any point (except the first step), stop if the node is selectable.

	// Strategy 1. (going down):
	//  - Recurse as far down as possible to the first child
	//  - go to the next node
	//  - repeat

	// Strategy 2. This is the reverse of strategy 1. (up/down should do exactly the opposite actions)

	// When going down, go from top-down and stop when a selectable node is found
	// When going up, go from bottom-up and stop when a selectable node is found

	bool result = false;
	if (UI_IsSelected(node->key) && node->parent) {
		if (UI_InputWasPressedOrRepeated(UI_Input_Down) || (UI_InputWasPressedOrRepeated(UI_Input_Tab) && !UI_InputIsDown(UI_Input_Shift))) {
			UI_Box* n = node;
			for (;;) {
				if (n->first_child[0]) {
					n = n->first_child[0];
				}
				else {
					for (;;) {
						if (n->next[1]) {
							n = n->next[1];
							break;
						}
						else if (n->parent) {
							n = n->parent;
						}
						else {
							n = n->first_child[0];
							break;
						}
					}
				}
				if (n->flags & UI_BoxFlag_Selectable) {
					*out_new_selected_box = n->key;
					result = true;
					goto end;
				}
			}
		}
		if (UI_InputWasPressedOrRepeated(UI_Input_Up) || (UI_InputWasPressedOrRepeated(UI_Input_Tab) && UI_InputIsDown(UI_Input_Shift))) {
			UI_Box* n = node;
			for (;;) {
				// go to the previous node
				if (n->next[0]) {
					n = n->next[0];
					for (; n->first_child[1];) {
						n = n->first_child[1];
					}
				}
				else if (n->parent) {
					n = n->parent;
				}
				else {
					n = n->first_child[1];
					for (; n->first_child[1];) {
						n = n->first_child[1];
					}
				}

				if (n->flags & UI_BoxFlag_Selectable) {
					*out_new_selected_box = n->key;
					result = true;
					goto end;
				}
			}
		}
	}

	for (UI_Box* child = node->first_child[0]; child; child = child->next[1]) {
		if (UI_SelectionMovementInput(child, out_new_selected_box)) {
			result = true;
			break;
		}
	}

end:;
	DS_ProfExit();
	return result;
}

UI_API UI_Box* UI_MakeBox_(UI_Key key, UI_Size w, UI_Size h, UI_BoxFlags flags, bool is_a_root) {
	DS_ProfEnter();
	UI_CHECK(w >= 0.f || (w >= -100.f && w <= -99.f));
	UI_CHECK(h >= 0.f || (h >= -100.f && h <= -99.f));

	UI_Box* box = DS_New(UI_Box, UI_FrameArena());
	bool newly_added = DS_MapInsert(&UI_STATE.box_from_key, key, box);
	DS_CHECK(newly_added); // If this fails, then a box with the same key has already been added during this frame!

	box->key = key;
	box->prev_frame = UI_PrevFrameBoxFromKey(key);
	box->size[0] = w;
	box->size[1] = h;
	box->flags = flags;
	box->style = UI_PeekStyle();

	if (UI_STATE.mouse_clicking_down_box == key && UI_InputIsDown(UI_Input_MouseLeft)) {
		UI_STATE.mouse_clicking_down_box_new = key;
	}

	if (UI_STATE.keyboard_clicking_down_box == key && (UI_STATE.selection_is_visible && UI_InputIsDown(UI_Input_Enter))) {
		UI_STATE.keyboard_clicking_down_box_new = key;
	}

	if (flags & UI_BoxFlag_Clickable) {
		bool pressed_with_mouse = UI_InputWasPressed(UI_Input_MouseLeft) && UI_IsHovered(key);
		bool pressed_with_keyboard = UI_STATE.selected_box == key && UI_STATE.selection_is_visible && UI_InputWasPressed(UI_Input_Enter);
		if (pressed_with_mouse) {
			UI_STATE.mouse_clicking_down_box_new = key;
			if (box->flags & UI_BoxFlag_Selectable) UI_STATE.selected_box_new = key;
		}
		if (pressed_with_keyboard) {
			UI_STATE.keyboard_clicking_down_box_new = key;
			if (box->flags & UI_BoxFlag_Selectable) UI_STATE.selected_box_new = key;
		}
	}

	// Keep the currently selected box selected, unless overwritten by pressing some other box
	if (UI_STATE.selected_box == key && UI_STATE.selected_box_new == UI_INVALID_KEY) {
		UI_STATE.selected_box_new = key;
	}

	if (box->prev_frame) {
		if (is_a_root) {
			UI_Key new_selected_box;
			if (UI_SelectionMovementInput(box->prev_frame, &new_selected_box)) {
				if (UI_STATE.selection_is_visible) { // only move if selection is already visible; otherwise first make it visible
					UI_STATE.selected_box_new = new_selected_box;
				}
				UI_STATE.selection_is_visible = true;
			}
		}

		float is_clicking = (float)UI_IsClickingDownAndHovered(box->key);
		box->lazy_is_hovered = (float)UI_IsHoveredIdle(box->key);
		box->lazy_is_holding_down = is_clicking > box->prev_frame->lazy_is_holding_down ?
			is_clicking : UI_Lerp(box->prev_frame->lazy_is_holding_down, is_clicking, 0.2f);
	}

	DS_ProfExit();
	return box;
}

UI_API UI_Box* UI_MakeRootBox(UI_Key key, UI_Size w, UI_Size h, UI_BoxFlags flags) {
	return UI_MakeBox_(key, w, h, flags, true);
}

UI_API UI_Box* UI_AddBox(UI_Key key, UI_Size w, UI_Size h, UI_BoxFlags flags) {
	UI_Box* parent = DS_ArrPeek(UI_STATE.box_stack);
	UI_CHECK(parent != NULL); // AddBox creates a new box under the currently pushed box. If this fails, no box is currently pushed
	
	UI_Box* box = UI_MakeBox_(key, w, h, flags, false);
	box->parent = parent;
	if (parent->flags & UI_BoxFlag_NoHover) box->flags |= UI_BoxFlag_NoHover;

	if (parent->first_child[1]) parent->first_child[1]->next[1] = box;
	else parent->first_child[0] = box;
	box->next[0] = parent->first_child[1];
	parent->first_child[1] = box;

	return box;
}

UI_API void UI_PushBox(UI_Box* box) {
	DS_ProfEnter();
	DS_ArrPush(&UI_STATE.box_stack, box);
	DS_ProfExit();
}

UI_API void UI_PopBox(UI_Box* box) {
	DS_ProfEnter();
	UI_Box* popped = DS_ArrPop(&UI_STATE.box_stack);
	UI_CHECK(popped == box);
	DS_ProfExit();
}

UI_API void UI_PopBoxN(UI_Box* box, int n) {
	UI_CHECK(UI_STATE.box_stack.length >= n);
	UI_STATE.box_stack.length -= n - 1;
	UI_Box* last_popped = DS_ArrPop(&UI_STATE.box_stack);
	UI_CHECK(last_popped == box);
}

UI_API void UI_BeginFrame(const UI_Inputs* inputs, UI_Vec2 window_size) {
	DS_ProfEnter();

	UI_STATE.draw_next_vertex = 0;
	UI_STATE.draw_next_index = 0;
	UI_STATE.draw_vertices = (UI_DrawVertex*)UI_STATE.backend.buffer_map_until_frame_end(0);
	UI_STATE.draw_indices = (uint32_t*)UI_STATE.backend.buffer_map_until_frame_end(1);
	UI_STATE.draw_active_texture = UI_STATE.atlases[0];
	UI_CHECK(UI_STATE.draw_active_texture != UI_TEXTURE_ID_NIL);
	UI_CHECK(UI_STATE.draw_vertices != NULL);
	UI_CHECK(UI_STATE.draw_indices != NULL);

	UI_STATE.inputs = *inputs;
	memset(&UI_STATE.outputs, 0, sizeof(UI_STATE.outputs));
	UI_STATE.window_size = window_size;

	UI_CHECK(UI_STATE.box_stack.length == 1);
	UI_STATE.mouse_pos = UI_AddV2(UI_STATE.inputs.mouse_position, UI_VEC2{ 0.5f, 0.5f });
	UI_STATE.base_font = UI_STATE.inputs.base_font;
	UI_STATE.icons_font = UI_STATE.inputs.icons_font;
	UI_CHECK(UI_STATE.base_font != NULL && UI_STATE.icons_font != NULL);

	DS_Arena prev_frame_arena = UI_STATE.prev_frame_arena;
	UI_STATE.prev_frame_arena = UI_STATE.frame_arena;
	UI_STATE.frame_arena = prev_frame_arena;
	DS_ArenaReset(&UI_STATE.frame_arena);

	UI_STATE.prev_frame_box_from_key = UI_STATE.box_from_key;
	DS_MapInit(&UI_STATE.box_from_key, &UI_STATE.frame_arena);
	
	UI_STATE.mouse_clicking_down_box = UI_STATE.mouse_clicking_down_box_new;
	UI_STATE.mouse_clicking_down_box_new = UI_INVALID_KEY;
	UI_STATE.keyboard_clicking_down_box = UI_STATE.keyboard_clicking_down_box_new;
	UI_STATE.keyboard_clicking_down_box_new = UI_INVALID_KEY;
	UI_STATE.selected_box = UI_STATE.selected_box_new;
	UI_STATE.selected_box_new = UI_INVALID_KEY;
	UI_STATE.text_editing_box = UI_STATE.text_editing_box_new;
	UI_STATE.text_editing_box_new = UI_INVALID_KEY;

	UI_STATE.frame_idx += 1;

	DS_ArrInit(&UI_STATE.draw_calls, &UI_STATE.frame_arena);

	// When clicking somewhere or pressing escape, by default, hide the selection box
	if (UI_InputWasPressed(UI_Input_MouseLeft) || UI_InputWasPressed(UI_Input_Escape)) {
		UI_STATE.selection_is_visible = false;
	}

	for (int i = 0; i < UI_Input_COUNT; i++) {
		if (inputs->input_events[i] & UI_InputEvent_Press) {
			UI_STATE.input_is_down[i] = true;
		}
		if (inputs->input_events[i] & UI_InputEvent_Release) {
			UI_STATE.input_is_down[i] = false;
		}
	}

	// Push default style
	UI_Style style = {0};
	style.font.font = UI_STATE.base_font;
	style.font.size = 18.f;
	style.border_color = UI_COLOR{ 0, 0, 0, 128 };
	style.opaque_bg_color = UI_COLOR{ 50, 50, 50, 255 };
	style.transparent_bg_color = UI_COLOR{ 255, 255, 255, 50 };
	style.text_padding = UI_VEC2{ 10.f, 5.f };
	style.child_padding = UI_VEC2{ 12.f, 12.f };
	style.text_color = UI_COLOR{ 255, 255, 255, 255 };
	DS_ArrPush(&UI_STATE.style_stack, DS_Clone(UI_Style, &UI_STATE.persistent_arena, style));

	DS_ProfExit();
}

UI_API void UI_Deinit(void) {
	DS_ArenaDeinit(&UI_STATE.prev_frame_arena);
	DS_ArenaDeinit(&UI_STATE.frame_arena);
	DS_ArenaDeinit(&UI_STATE.persistent_arena);

	UI_STATE.backend.destroy_buffer(0);
	UI_STATE.backend.destroy_buffer(1);
	UI_STATE.backend.destroy_atlas(0);

	DS_MemFree(UI_STATE.allocator, UI_STATE.atlas_buffer_grayscale);

	UI_TextDeinit(&UI_STATE.edit_number_text);
}

UI_API void UI_Init(DS_Allocator* allocator, const UI_Backend* backend) {
	DS_ProfEnter();

	memset(&UI_STATE, 0, sizeof(UI_STATE));
	UI_STATE.allocator = allocator;
	UI_STATE.backend = *backend;
	DS_ArenaInit(&UI_STATE.persistent_arena, DS_KIB(1), allocator);
	DS_ArenaInit(&UI_STATE.prev_frame_arena, DS_KIB(4), allocator);
	DS_ArenaInit(&UI_STATE.frame_arena, DS_KIB(4), allocator);

	stbtt_PackBegin(&UI_STATE.pack_context, NULL, UI_GLYPH_MAP_SIZE, UI_GLYPH_MAP_SIZE, 0, UI_GLYPH_PADDING, NULL);

	// init atlas
	{
		UI_TextureID atlas = backend->create_atlas(0, UI_GLYPH_MAP_SIZE, UI_GLYPH_MAP_SIZE);
		UI_CHECK(atlas != UI_TEXTURE_ID_NIL);
		UI_STATE.atlases[0] = atlas;

		// pack a white rectangle into the atlas. See UI_WHITE_PIXEL_UV

		stbrp_rect rect = {0};
		rect.w = 1;
		rect.h = 1;
		stbtt_PackFontRangesPackRects(&UI_STATE.pack_context, &rect, 1);
		UI_CHECK(rect.was_packed);
		UI_CHECK(rect.x == 0 && rect.y == 0);

		uint32_t* data = (uint32_t*)backend->atlas_map_until_frame_end(0);
		data[0] = 0xFFFFFFFF;
	}

	backend->create_vertex_buffer(0, sizeof(UI_DrawVertex) * UI_MAX_VERTEX_COUNT);
	backend->create_index_buffer(1, sizeof(uint32_t) * UI_MAX_INDEX_COUNT);

	//UI_.atlases[0] = GPU_MakeTexture(GPU_Format_RGBA8UN, UI_GLYPH_MAP_SIZE, UI_GLYPH_MAP_SIZE, 1, 0, NULL);
	//UI_.atlas_staging_buffer = GPU_MakeBuffer(sizeof(uint32_t)*UI_GLYPH_MAP_SIZE*UI_GLYPH_MAP_SIZE, GPU_BufferFlag_CPU, NULL);
	//memset(UI_.atlas_staging_buffer->data, 0, UI_.atlas_staging_buffer->size);

	UI_STATE.atlas_buffer_grayscale = (uint8_t*)DS_MemAlloc(UI_STATE.allocator, sizeof(uint8_t) * UI_GLYPH_MAP_SIZE * UI_GLYPH_MAP_SIZE);
	memset(UI_STATE.atlas_buffer_grayscale, 0, UI_GLYPH_MAP_SIZE * UI_GLYPH_MAP_SIZE);
	UI_STATE.pack_context.pixels = UI_STATE.atlas_buffer_grayscale;

	{
		// What if I as the user want to use a default emgui font + some own font? I guess for now, let's
		// split this so that the EMGUI default fonts go into one atlas, and the user can create their own atlases as needed.

		// UI_.atlas = UI_InitFontAtlas(persistent_arena);

		UI_String roboto_mono_ttf, icons_ttf;
		// UI_TODO();
		// UI_CHECK(OS_ReadEntireFile(UI_.persistent_arena, OS_CWD, StrJoin(UI_FrameArena(), resources_directory, STR_("/roboto_mono.ttf")), &roboto_mono_ttf));
		// UI_CHECK(OS_ReadEntireFile(UI_.persistent_arena, OS_CWD, StrJoin(UI_FrameArena(), resources_directory, STR_("/fontello/font/fontello.ttf")), &icons_ttf));

		// UI_FontInit(&UI_.base_font, roboto_mono_ttf.data, -4.f);
		// UI_FontInit(&UI_.icons_font, icons_ttf.data, -2.f);
	}

	DS_ArrInit(&UI_STATE.style_stack, &UI_STATE.persistent_arena);
	DS_ArrInit(&UI_STATE.box_stack, &UI_STATE.persistent_arena);
	DS_ArrPush(&UI_STATE.box_stack, NULL);

	UI_TextInit(allocator, &UI_STATE.edit_number_text, STR_(""));

	DS_ProfExit();
}

//UI_API void UI_BoxComputeUnexpandedSize(UI_Box* box, UI_Axis axis) {
//	DS_ProfEnter();
//
//	// if we have a special unexpanded-size override, then do it here!
//
//	DS_ProfExit();
//}

UI_API void UI_BoxComputeExpandedSize(UI_Box* box, UI_Axis axis, float size) {
	DS_ProfEnter();
	box->computed_size._[axis] = size;

	float child_area_size = size;
	if (box->flags & UI_BoxFlag_ChildPadding) child_area_size -= 2.f * box->style->child_padding._[axis];

	UI_Axis layout_axis = box->flags & UI_BoxFlag_LayoutInX ? UI_Axis_X : UI_Axis_Y;
	UI_BoxFlags no_flex_down_flag = axis == UI_Axis_X ? UI_BoxFlag_NoFlexDownX : UI_BoxFlag_NoFlexDownY;

	if (axis == layout_axis) {
		float total_leftover = child_area_size;
		float total_flex = 0.f;

		for (UI_Box* child = box->first_child[0]; child; child = child->next[1]) {
			total_leftover -= child->computed_unexpanded_size._[axis];
		}

		
		for (UI_Box* child = box->first_child[0]; child; child = child->next[1]) {
			if (total_leftover > 0) {
				total_flex += child->size[axis] < 0.f ? child->size[axis] + 100.f : 0.f; // flex up
			} else {
				total_flex += child->size[axis] < 0.f && !(child->flags & no_flex_down_flag) ? child->size[axis] + 100.f : 0.f; // flex down
			}
			//float flex = total_leftover > 0 ?
			//	child->size[axis].flex_up :
			//	child->size[axis].flex_down;
		}

		for (UI_Box* child = box->first_child[0]; child; child = child->next[1]) {
			float child_size = child->computed_unexpanded_size._[axis];

			if (total_leftover > 0) {
				float flex_up = child->size[axis] < 0.f ? child->size[axis] + 100.f : 0.f;
				float leftover_distributed = flex_up == 0.f ? 0.f : total_leftover * flex_up / total_flex;
				float flex_px = UI_Min(leftover_distributed, total_leftover * flex_up);
				child_size += flex_px;
			}
			else {
				float flex_down = child->size[axis] < 0.f && !(child->flags & no_flex_down_flag) ? child->size[axis] + 100.f : 0.f;
				float leftover_distributed = flex_down == 0.f ? 0.f : total_leftover * flex_down / total_flex;
				float flex_px = UI_Min(-leftover_distributed, child_size * flex_down);
				child_size -= flex_px;
			}

			UI_BoxComputeExpandedSize(child, axis, child_size);
		}
	}
	else {
		for (UI_Box* child = box->first_child[0]; child; child = child->next[1]) {
			float child_size = child->computed_unexpanded_size._[axis];

			float leftover = child_area_size - child_size;
			if (leftover > 0) {
				float flex_up = child->size[axis] < 0.f ? child->size[axis] + 100.f : 0.f;
				child_size += leftover * flex_up;
			}
			else {
				float flex_down = child->size[axis] < 0.f && !(child->flags & no_flex_down_flag) ? child->size[axis] + 100.f : 0.f;
				float flex_px = UI_Min(-leftover, child_size * flex_down);
				child_size -= flex_px;
			}

			UI_BoxComputeExpandedSize(child, axis, child_size);
		}
	}

	DS_ProfExit();
}

UI_API void UI_BoxComputeRectsStep(UI_Box* box, UI_Axis axis, float position, UI_ScissorRect scissor) {
	DS_ProfEnter();
	box->flags |= UI_BoxFlag_HasComputedRects;
	box->computed_position._[axis] = position + box->offset._[axis];

	float min = box->computed_position._[axis];
	float max = min + box->computed_size._[axis];
	float min_clipped = min;
	float max_clipped = max;

	if (scissor) {
		min_clipped = UI_Max(min, scissor->min._[axis]);
		max_clipped = UI_Min(max, scissor->max._[axis]);
	}

	box->computed_rect_clipped.min._[axis] = min_clipped;
	box->computed_rect_clipped.max._[axis] = max_clipped;

	bool layout_from_end = axis == UI_Axis_X ? box->flags & UI_BoxFlag_LayoutFromEndX : box->flags & UI_BoxFlag_LayoutFromEndY;
	float direction = layout_from_end ? -1.f : 1.f;

	float cursor_base = layout_from_end ? max : min;
	float cursor = cursor_base;
	if (box->flags & UI_BoxFlag_ChildPadding) cursor += direction * box->style->child_padding._[axis];

	UI_ScissorRect child_scissor = (box->flags & UI_BoxFlag_NoScissor) ? scissor : &box->computed_rect_clipped;
	UI_Axis layout_axis = box->flags & UI_BoxFlag_LayoutInX ? UI_Axis_X : UI_Axis_Y;

	for (UI_Box* child = box->first_child[0]; child; child = child->next[1]) {
		float child_position = (child->flags & UI_BoxFlag_NoAutoOffset ? cursor_base : cursor);
		if (layout_from_end) child_position -= child->computed_size._[axis];

		UI_BoxComputeRectsStep(child, axis, child_position, child_scissor);

		if (axis == layout_axis) {
			cursor += direction * child->computed_size._[axis];
		}
	}
	DS_ProfExit();
}

UI_API void UI_BoxComputeUnexpandedSizes(UI_Box* box) {
	UI_BoxComputeUnexpandedSize(box, UI_Axis_X);
	UI_BoxComputeUnexpandedSize(box, UI_Axis_Y);
}

UI_API void UI_BoxComputeUnexpandedSize(UI_Box* box, UI_Axis axis) {
	if (box->compute_unexpanded_size_override) {
		box->compute_unexpanded_size_override(box, axis);
	} else {
		UI_BoxComputeUnexpandedSizeDefault(box, axis);
	}
}

UI_API void UI_BoxComputeUnexpandedSizeDefault(UI_Box* box, UI_Axis axis) {
	for (UI_Box* child = box->first_child[0]; child; child = child->next[1]) {
		UI_BoxComputeUnexpandedSize(child, axis);
	}

	float fitting_size = 0.f;

	if (box->flags & UI_BoxFlag_DrawText) {
		UI_CHECK(box->first_child[0] == NULL); // DrawText may only be used on leaf boxes

		float text_size = axis == 0 ? UI_TextWidth(box->text, box->style->font) : box->style->font.size;
		fitting_size = (float)(int)(text_size + 0.5f) + 2.f * box->style->text_padding._[axis];
	}

	if (box->first_child[0]) {
		UI_Axis layout_axis = box->flags & UI_BoxFlag_LayoutInX ? UI_Axis_X : UI_Axis_Y;

		for (UI_Box* child = box->first_child[0]; child; child = child->next[1]) {
			if (layout_axis == axis) {
				fitting_size += child->computed_unexpanded_size._[axis];
			}
			else {
				fitting_size = UI_Max(fitting_size, child->computed_unexpanded_size._[axis]);
			}
		}

		if (box->flags & UI_BoxFlag_ChildPadding) fitting_size += 2 * box->style->child_padding._[axis];
	}

	float unexpanded_size = box->size[axis] < 0.f ? fitting_size : box->size[axis];
	box->computed_unexpanded_size._[axis] = unexpanded_size;
}

UI_API void UI_BoxComputeExpandedSizes(UI_Box* box) {
	UI_BoxComputeUnexpandedSizes(box);
	UI_BoxComputeExpandedSize(box, UI_Axis_X, box->computed_unexpanded_size.x);
	UI_BoxComputeExpandedSize(box, UI_Axis_Y, box->computed_unexpanded_size.y);
}

UI_API void UI_BoxComputeRects(UI_Box* box, UI_Vec2 box_position) {
	UI_BoxComputeExpandedSizes(box);
	UI_BoxComputeRectsStep(box, UI_Axis_X, box_position.x, NULL);
	UI_BoxComputeRectsStep(box, UI_Axis_Y, box_position.y, NULL);
}

static void UI_FinalizeDrawBatch() {
	DS_ProfEnter();
	uint32_t first_index = 0;
	if (UI_STATE.draw_calls.length > 0) {
		UI_DrawCall last = DS_ArrPeek(UI_STATE.draw_calls);
		first_index = last.first_index + last.index_count;
	}

	uint32_t index_count = UI_STATE.draw_next_index - first_index;
	if (index_count > 0) {
		UI_DrawCall draw_call = {0};
		draw_call.texture = UI_STATE.draw_active_texture;
		draw_call.first_index = first_index;
		draw_call.index_count = index_count;
		draw_call.vertex_buffer_id = 0;
		draw_call.index_buffer_id = 1;
		DS_ArrPush(&UI_STATE.draw_calls, draw_call);
	}
	DS_ProfExit();
}

UI_API void UI_EndFrame(UI_Outputs* outputs/*, GPU_Graph *graph, GPU_DescriptorArena *descriptor_arena*/) {
	DS_ProfEnter();

	// we could say that `UI_compute_tree_rects` and `draw` is left to the user.
	//UI_compute_tree_rects(UI_.root);
	//UI_.root->on_draw(UI_.root);

	// We reset these at the end of the frame so that we can still check for `was_key_released` and have these not be reset yet
	if (UI_InputIsDown(UI_Input_MouseLeft)) {
		UI_Vec2 delta = UI_STATE.inputs.mouse_raw_delta;
		float scale = 1.f;
		if (UI_InputIsDown(UI_Input_Alt)) scale /= 50.f;
		if (UI_InputIsDown(UI_Input_Shift)) scale *= 50.f;
		UI_STATE.mouse_travel_distance_after_press = UI_AddV2(UI_STATE.mouse_travel_distance_after_press, UI_MulV2F(delta, scale));
		UI_STATE.last_pressed_mouse_pos = UI_STATE.mouse_pos;
	}
	else {
		UI_STATE.last_released_mouse_pos = UI_STATE.mouse_pos;
		UI_STATE.mouse_travel_distance_after_press = UI_VEC2{0};
	}

	UI_STATE.time_since_pressed_lmb += UI_STATE.inputs.frame_delta_time;
	if (UI_InputWasPressed(UI_Input_MouseLeft)) {
		UI_STATE.time_since_pressed_lmb = 0.f;
	}

	UI_FinalizeDrawBatch();

	UI_CHECK(UI_STATE.style_stack.length == 1);
	DS_ArrPop(&UI_STATE.style_stack);

	UI_STATE.outputs.draw_calls = UI_STATE.draw_calls.data;
	UI_STATE.outputs.draw_calls_count = UI_STATE.draw_calls.length;
	*outputs = UI_STATE.outputs;

	DS_ProfExit();
}

UI_API void UI_Splitters(UI_Key key, UI_Rect area, UI_Axis X, int panel_count,
	float* panel_end_offsets, float panel_min_width)
{
	UI_CHECK(panel_count > 0);
	DS_ProfEnter();

	// Sanitize positions
	float offset = 0.f;
	for (int i = 0; i < panel_count; i++) {
		panel_end_offsets[i] = UI_Max(panel_end_offsets[i], offset + panel_min_width);
		offset = panel_end_offsets[i];
	}

	float rect_width = area.max._[X] - area.min._[X];
	float normalize_factor = rect_width / panel_end_offsets[panel_count - 1];

	for (int i = 0; i < panel_count; i++) {
		// Normalize all positions so that the last position has the same value
		// as the width of the area rectangle.
		panel_end_offsets[i] = panel_end_offsets[i] * normalize_factor;
	}

	if (UI_STATE.holding_splitter_key == key) {
		int holding_splitter = UI_STATE.holding_splitter_index;
		float split_position = UI_STATE.mouse_pos._[X] - area.min._[X];

		if (UI_InputIsDown(UI_Input_Alt)) {
			// Reset position to default.
			split_position = (float)(holding_splitter + 1) * (rect_width / (float)panel_count);
		}

		int panels_left = holding_splitter + 1;
		int panels_right = panel_count - 1 - holding_splitter;

		float clamped_split_position = UI_Max(split_position, panel_min_width * (float)panels_left);
		clamped_split_position = UI_Min(clamped_split_position, rect_width - (float)panels_right * panel_min_width);
		panel_end_offsets[holding_splitter] = clamped_split_position;

		float head = clamped_split_position;
		for (int i = holding_splitter + 1; i < panel_count; i++) {
			if (panel_end_offsets[i] < head + panel_min_width) {
				panel_end_offsets[i] = head + panel_min_width;
				head = panel_end_offsets[i];
			}
		}

		head = clamped_split_position;
		for (int i = holding_splitter - 1; i >= 0; i--) {
			if (panel_end_offsets[i] > head - panel_min_width) {
				panel_end_offsets[i] = head - panel_min_width;
				head = panel_end_offsets[i];
			}
		}
	}

	if (!UI_InputIsDown(UI_Input_MouseLeft)) {
		UI_STATE.holding_splitter_key = UI_INVALID_KEY;
	}

	int hovering_splitter_index;
	if (UI_SplittersFindHoveredIndex(area, X, panel_count, panel_end_offsets, &hovering_splitter_index)) {
		UI_STATE.outputs.cursor = X == UI_Axis_X ? UI_MouseCursor_ResizeH : UI_MouseCursor_ResizeV;

		if (UI_InputWasPressed(UI_Input_MouseLeft)) {
			UI_STATE.holding_splitter_key = key;
			UI_STATE.holding_splitter_index = hovering_splitter_index;
		}
	}

	DS_ProfExit();
}

UI_API bool UI_SplittersGetHoldingIndex(UI_Key key, int* out_index) {
	*out_index = UI_STATE.holding_splitter_index;
	return UI_STATE.holding_splitter_key == key;
}

UI_API bool UI_SplittersFindHoveredIndex(UI_Rect area, UI_Axis X, int panel_count, float* panel_end_offsets, int* out_splitter_index) {
	bool hovering = false;
	for (int i = 0; i < panel_count - 1; i++) {
		const float SPLITTER_HALF_WIDTH = 2.f; // this could use the current DPI
		float end_x = area.min._[X] + panel_end_offsets[i];
		UI_Rect end_splitter_rect = area;
		end_splitter_rect.min._[X] = end_x - SPLITTER_HALF_WIDTH;
		end_splitter_rect.max._[X] = end_x + SPLITTER_HALF_WIDTH;

		if (UI_PointIsInRect(end_splitter_rect, UI_STATE.mouse_pos)) {
			*out_splitter_index = i;
			hovering = true;
			break;
		}
	}
	return hovering;
}

//  --------------------------------------------------------------------------------------------------
// |                                        Drawing API                                               |
//  --------------------------------------------------------------------------------------------------

// If a glyph is missing from an atlas, it will be replaced with:
#define INVALID_GLYPH '?'
#define INVALID_GLYPH_COLOR UI_MAGENTA

UI_API void UI_FontInit(UI_Font* font, const void* ttf_data, float y_offset) {
	memset(font, 0, sizeof(*font));
	DS_MapInit(&font->glyph_map, UI_STATE.allocator);

	font->data = (const unsigned char*)ttf_data;
	font->y_offset = y_offset;

	int font_offset = stbtt_GetFontOffsetForIndex(font->data, 0);
	UI_CHECK(stbtt_InitFont(&font->font_info, font->data, font_offset));
}

UI_API void UI_FontDeinit(UI_Font* font) {
	DS_MapDeinit(&font->glyph_map);
}

static UI_CachedGlyph UI_GetCachedGlyph(uint32_t codepoint, UI_FontUsage font, int* out_atlas_index) {
	int atlas_index = 0;
	UI_CachedGlyphKey key = { codepoint, (int)font.size };

	UI_CachedGlyph* glyph = NULL;
	if (DS_MapGetOrAddPtr(&font.font->glyph_map, key, &glyph)) {
		// if (UI_.frame_has_split_atlas) {
		// 
		// }
		UI_CachedGlyph glyph_obsolete;
		// if (DS_MapFind(&font.font->glyph_map_obsolete, &key, &glyph_obsolete)) {
		// 	// TODO: we could copy the pixels from the old glyph map instead of rendering
		// }

		stbrp_rect rect = {0};
		stbtt_packedchar packed_char;
		stbtt_pack_range pack_range = {0};
		pack_range.font_size = font.size;
		pack_range.first_unicode_codepoint_in_range = 0;
		pack_range.array_of_unicode_codepoints = (int*)&codepoint;
		pack_range.num_chars = 1;
		pack_range.chardata_for_range = &packed_char;

		// GatherRects fills width & height fields of `rect`
		if (stbtt_PackFontRangesGatherRects(&UI_STATE.pack_context, &font.font->font_info, &pack_range, 1, &rect) == 0) {
			UI_TODO(); // glyph is not found in the font
		}
		UI_CHECK(rect.h >= 0);

		stbtt_PackFontRangesPackRects(&UI_STATE.pack_context, &rect, 1);
		if (!rect.was_packed) {
			UI_TODO(); // ran out of space in the atlas, we need to start using a new atlas
		}

		UI_CHECK(stbtt_PackFontRangesRenderIntoRects(&UI_STATE.pack_context, &font.font->font_info, &pack_range, 1, &rect));

		// The glyph will be now rasterized into UI_.atlas_buffer_grayscale. Let's convert it into RGBA8.

		uint32_t* atlas_data = (uint32_t*)UI_STATE.backend.atlas_map_until_frame_end(0);
		UI_CHECK(atlas_data);

		for (int y = packed_char.y0; y < packed_char.y1; y++) {
			uint8_t* src_row = UI_STATE.atlas_buffer_grayscale + y * UI_GLYPH_MAP_SIZE;
			uint32_t* dst_row = atlas_data + y * UI_GLYPH_MAP_SIZE;

			for (int x = packed_char.x0; x < packed_char.x1; x++) {
				dst_row[x] = ((uint32_t)src_row[x] << 24) | 0x00FFFFFF;
				//dst_row[x] = ((uint32_t)src_row[x] << 24) | 0x00FFFFFF;
				// Aha! Blending modes!
				//dst_row[x] = 0x01010101;//((uint32_t)src_row[x] << 0) | 0xFFFFFF00;
			}
		}

		UI_CHECK(packed_char.x0 == rect.x);
		UI_CHECK(packed_char.y0 == rect.y);
		UI_CHECK(packed_char.y1 - packed_char.y0 == rect.h);
		UI_CHECK(packed_char.x1 - packed_char.x0 == rect.w);

		int w = packed_char.x1 - packed_char.x0;
		int h = packed_char.y1 - packed_char.y0;
		glyph->origin_uv.x = (float)packed_char.x0 / (float)UI_GLYPH_MAP_SIZE;
		glyph->origin_uv.y = (float)packed_char.y0 / (float)UI_GLYPH_MAP_SIZE;
		glyph->size_pixels.x = (float)w;
		glyph->size_pixels.y = (float)h;
		glyph->offset_pixels.x = packed_char.xoff;
		glyph->offset_pixels.y = packed_char.yoff + font.size + font.font->y_offset;
		glyph->x_advance = (float)(int)(packed_char.xadvance + 0.5f); // round to integer

		// UI_.atlas_needs_reupload = true;
	}

	if (out_atlas_index) *out_atlas_index = atlas_index;
	return *glyph;
}

UI_API float UI_GlyphWidth(uint32_t codepoint, UI_FontUsage font) {
	UI_CachedGlyph glyph = UI_GetCachedGlyph(codepoint, font, NULL);
	return glyph.x_advance;
	//bool ok = DS_MapFind(&font->atlas->glyphs, &((UI_FontAtlasKey){codepoint, font->id}), &val);
	//if (!ok) {
	//	DS_MapFind(&font->atlas->glyphs, &((UI_FontAtlasKey){INVALID_GLYPH, font->id}), &val);
	//}
	//return val.advance;
}

UI_API float UI_TextWidth(UI_String text, UI_FontUsage font) {
	DS_ProfEnter();
	float w = 0.f;
	for STR_Each(text, r, i) {
		w += UI_GlyphWidth(r, font);
	}
	DS_ProfExit();
	return w;
}

/*
DRAW CACHING PLAN:
  The drawing layer (e.g. DrawRect()) is responsible for detecting changes and dirty rectangles.
  Change detection strategy: per tile, generate a hash of the array of all draw commands that overlap that tile. If we don't want to use a hash triangle map,
  we could store the array of draw command parameters per tile, and then on a second pass generate the actual triangle data. I guess we just need to profile all options here.
*/

UI_API inline UI_DrawVertex* UI_AddVertices(int count, uint32_t* out_first_index) {
	*out_first_index = UI_STATE.draw_next_vertex;
	UI_DrawVertex* v = &UI_STATE.draw_vertices[UI_STATE.draw_next_vertex];
	UI_STATE.draw_next_vertex += count;
	UI_CHECK(UI_STATE.draw_next_vertex <= UI_MAX_VERTEX_COUNT);
	return v;
}

UI_API inline uint32_t* UI_AddIndices(int count, UI_TextureID texture) {
	// Set active texture
	if (texture != UI_TEXTURE_ID_NIL && texture != UI_STATE.draw_active_texture) {
		if (UI_STATE.draw_active_texture != UI_TEXTURE_ID_NIL) {
			UI_FinalizeDrawBatch();
		}
		UI_STATE.draw_active_texture = texture;
	}

	uint32_t* i = &UI_STATE.draw_indices[UI_STATE.draw_next_index];
	UI_STATE.draw_next_index += count;
	UI_CHECK(UI_STATE.draw_next_index <= UI_MAX_INDEX_COUNT);
	return i;
}

UI_API inline void UI_AddTriangleIndices(uint32_t a, uint32_t b, uint32_t c, UI_TextureID texture) {
	uint32_t* indices = UI_AddIndices(3, texture);
	indices[0] = a;
	indices[1] = b;
	indices[2] = c;
}

UI_API inline void UI_AddQuadIndices(uint32_t a, uint32_t b, uint32_t c, uint32_t d, UI_TextureID texture) {
	uint32_t* indices = UI_AddIndices(6, texture);
	indices[0] = a; indices[1] = b; indices[2] = c;
	indices[3] = a; indices[4] = c; indices[5] = d;
}

UI_API void UI_AddQuadIndicesAndClip(uint32_t a, uint32_t b, uint32_t c, uint32_t d, UI_TextureID texture, UI_ScissorRect scissor) {
	UI_AddTriangleIndicesAndClip(a, b, c, texture, scissor);
	UI_AddTriangleIndicesAndClip(a, c, d, texture, scissor);
}

static void ClipConvexPolygonToHalfSpace(UI_Axis X, bool flip, float line_x, UI_Vec2* vertices, int vertices_count, UI_Vec2* out_vertices, int* out_vertices_count) {
	*out_vertices_count = 0;
	for (int i = 0; i < vertices_count; i++) {
		UI_Vec2 v0 = vertices[i];
		UI_Vec2 v1 = vertices[(i + 1) % vertices_count];

		// NOTE: it's imporant that we always nudge `starts_inside` towards true. That way even degenerate edges that we delete get their start points added.
		bool starts_inside = ((v0._[X] < line_x) != flip) || v0._[X] == line_x;
		bool ends_inside = (v1._[X] < line_x) != flip;
		if (starts_inside) {
			out_vertices[*out_vertices_count] = v0;
			*out_vertices_count += 1;
			UI_CHECK(*out_vertices_count <= 7);
		}
		if (starts_inside != ends_inside) {
			float edge_size_x = v1._[X] - v0._[X];
			if (edge_size_x != 0.f) { // Delete degenerate edges
				float t = (line_x - v0._[X]) / edge_size_x;
				out_vertices[*out_vertices_count]._[X] = line_x;
				out_vertices[*out_vertices_count]._[1 - X] = t * v1._[1 - X] + (1.f - t) * v0._[1 - X]; // Lerp between v0.y and v1.y using t
				*out_vertices_count += 1;
			}
		}
	}
}

static void UI_FindBarycentricCoordinates(const UI_Vec2* a, const UI_Vec2* b, const UI_Vec2* c, const UI_Vec2* p, float* u, float* v, float* w) {
	// Described in the book "Real-Time Collision Detection" (chapter: Barycentric Coordinates)
	UI_Vec2 v0 = { b->x - a->x, b->y - a->y };
	UI_Vec2 v1 = { c->x - a->x, c->y - a->y };
	UI_Vec2 v2 = { p->x - a->x, p->y - a->y };
	float d00 = v0.x * v0.x + v0.y * v0.y;
	float d01 = v0.x * v1.x + v0.y * v1.y;
	float d11 = v1.x * v1.x + v1.y * v1.y;
	float d20 = v2.x * v0.x + v2.y * v0.y;
	float d21 = v2.x * v1.x + v2.y * v1.y;
	float denom = d00 * d11 - d01 * d01;
	*v = (d11 * d20 - d01 * d21) / denom;
	*w = (d00 * d21 - d01 * d20) / denom;
	*u = 1.0f - *v - *w;
}

UI_API void UI_AddTriangleIndicesAndClip(uint32_t a, uint32_t b, uint32_t c, UI_TextureID texture, UI_ScissorRect scissor) {
	if (scissor) {
		UI_DrawVertex* V[3] = { &UI_STATE.draw_vertices[a], &UI_STATE.draw_vertices[b], &UI_STATE.draw_vertices[c] };

		// Early return if fully outside the scissor rect
		float min_x = UI_Min(V[0]->position.x, V[1]->position.x); min_x = UI_Min(min_x, V[2]->position.x);
		if (min_x >= scissor->max.x) return;
		float min_y = UI_Min(V[0]->position.y, V[1]->position.y); min_y = UI_Min(min_y, V[2]->position.y);
		if (min_y >= scissor->max.y) return;
		float max_x = UI_Max(V[0]->position.x, V[1]->position.x); max_x = UI_Max(max_x, V[2]->position.x);
		if (max_x <= scissor->min.x) return;
		float max_y = UI_Max(V[0]->position.y, V[1]->position.y); max_y = UI_Max(max_y, V[2]->position.y);
		if (max_y <= scissor->min.y) return;

		// if fully inside the scissor_rect, just draw the triangle directly
		if (max_x <= scissor->max.x && max_y <= scissor->max.y && min_x >= scissor->min.x && min_y >= scissor->min.y) goto draw;

		int k = 0;
		UI_Vec2 buffers[2][7];
		int buffer_counts[2] = { 3, 0 };
		buffers[0][0] = V[0]->position;
		buffers[0][1] = V[1]->position;
		buffers[0][2] = V[2]->position;

		if (max_x > scissor->max.x) { // clip right
			ClipConvexPolygonToHalfSpace(UI_Axis_X, false, scissor->max.x, buffers[k], buffer_counts[k], &buffers[1 - k][0], &buffer_counts[1 - k]);
			k = 1 - k;
		}
		if (max_y > scissor->max.y) { // clip bottom
			ClipConvexPolygonToHalfSpace(UI_Axis_Y, false, scissor->max.y, buffers[k], buffer_counts[k], &buffers[1 - k][0], &buffer_counts[1 - k]);
			k = 1 - k;
		}
		if (min_x < scissor->min.x) { // clip left
			ClipConvexPolygonToHalfSpace(UI_Axis_X, true, scissor->min.x, buffers[k], buffer_counts[k], &buffers[1 - k][0], &buffer_counts[1 - k]);
			k = 1 - k;
		}
		if (min_y < scissor->min.y) { // clip top
			ClipConvexPolygonToHalfSpace(UI_Axis_Y, true, scissor->min.y, buffers[k], buffer_counts[k], &buffers[1 - k][0], &buffer_counts[1 - k]);
			k = 1 - k;
		}

		UI_Vec2* poly_verts = buffers[k];
		int poly_verts_count = buffer_counts[k];
		UI_CHECK(poly_verts_count <= 7); // Cutting a triangle by 4 lines means that at max it will be a 7-sided (3+4) polygon. That means that it can at max have 7 vertices.

		uint32_t first_vertex_idx;
		UI_DrawVertex* vertices = UI_AddVertices(poly_verts_count, &first_vertex_idx);
		uint32_t* indices = UI_AddIndices(3 * (poly_verts_count - 2), texture);

		for (int i = 0; i < poly_verts_count; i++) {
			float u, v, w;
			UI_FindBarycentricCoordinates(&V[0]->position, &V[1]->position, &V[2]->position, &poly_verts[i], &u, &v, &w);

			float r[3] = { (float)V[0]->color.r, (float)V[1]->color.r, (float)V[2]->color.r };
			float g[3] = { (float)V[0]->color.g, (float)V[1]->color.g, (float)V[2]->color.g };
			float b[3] = { (float)V[0]->color.b, (float)V[1]->color.b, (float)V[2]->color.b };
			float a[3] = { (float)V[0]->color.a, (float)V[1]->color.a, (float)V[2]->color.a };

			float mixed_r = r[0] * u + r[1] * v + r[2] * w;
			float mixed_g = g[0] * u + g[1] * v + g[2] * w;
			float mixed_b = b[0] * u + b[1] * v + b[2] * w;
			float mixed_a = a[0] * u + a[1] * v + a[2] * w;

			vertices[i] = UI_DRAW_VERTEX{ poly_verts[i], {0, 0}, {(uint8_t)mixed_r, (uint8_t)mixed_g, (uint8_t)mixed_b, (uint8_t)mixed_a} };
			//vertices[i] = UI_DRAW_VERTEX{poly_verts[i], {0, 0}, UI_RED};
		}
		for (int i = 2; i < poly_verts_count; i++) {
			int first_idx = (i - 2) * 3;
			indices[first_idx + 0] = first_vertex_idx;
			indices[first_idx + 1] = first_vertex_idx + i - 1;
			indices[first_idx + 2] = first_vertex_idx + i;
		}
		return;
	}
draw:;
	uint32_t* indices = UI_AddIndices(3, texture);
	indices[0] = a;
	indices[1] = b;
	indices[2] = c;
}

UI_API void UI_DrawConvexPolygon(const UI_Vec2* points, int points_count, UI_Color color, UI_ScissorRect scissor) {
	DS_ProfEnter();
	uint32_t first_vertex;
	UI_DrawVertex* vertices = UI_AddVertices(points_count, &first_vertex);
	for (int i = 0; i < points_count; i++) {
		UI_Vec2 p = points[i];
		vertices[i] = UI_DRAW_VERTEX{ {p.x, p.y}, {0, 0}, color };
	}
	for (int i = 2; i < points_count; i++) {
		UI_AddTriangleIndicesAndClip(first_vertex, first_vertex + i - 1, first_vertex + i, UI_TEXTURE_ID_NIL, scissor);
	}
	DS_ProfExit();
}

UI_API void UI_DrawSprite(UI_Rect rect, UI_Color color, UI_Rect uv_rect, UI_TextureID texture, UI_ScissorRect scissor) {
	if (scissor) {
		if (rect.max.x < scissor->min.x) return;
		if (rect.min.x > scissor->max.x) return;
		if (rect.max.y < scissor->min.y) return;
		if (rect.min.y > scissor->max.y) return;

		float rect_w = rect.max.x - rect.min.x;
		float rect_h = rect.max.y - rect.min.y;
		float uv_rect_w = uv_rect.max.x - uv_rect.min.x;
		float uv_rect_h = uv_rect.max.y - uv_rect.min.y;

		float offset_min_x = scissor->min.x - rect.min.x;
		float offset_max_x = scissor->max.x - rect.max.x;
		float offset_min_y = scissor->min.y - rect.min.y;
		float offset_max_y = scissor->max.y - rect.max.y;

		if (offset_min_x > 0) {
			rect.min.x = scissor->min.x;
			uv_rect.min.x += offset_min_x * (uv_rect_w / rect_w);
		}
		if (offset_max_x < 0) {
			rect.max.x = scissor->max.x;
			uv_rect.max.x += offset_max_x * (uv_rect_w / rect_w);
		}
		if (offset_min_y > 0) {
			rect.min.y = scissor->min.y;
			uv_rect.min.y += offset_min_y * (uv_rect_h / rect_h);
		}
		if (offset_max_y < 0) {
			rect.max.y = scissor->max.y;
			uv_rect.max.y += offset_max_y * (uv_rect_h / rect_h);
		}
	}
	DS_ProfEnter();

	uint32_t first_vertex;
	UI_DrawVertex* vertices = UI_AddVertices(4, &first_vertex);
	vertices[0] = UI_DRAW_VERTEX{ {rect.min.x, rect.min.y}, uv_rect.min,                    color };
	vertices[1] = UI_DRAW_VERTEX{ {rect.max.x, rect.min.y}, {uv_rect.max.x, uv_rect.min.y}, color };
	vertices[2] = UI_DRAW_VERTEX{ {rect.max.x, rect.max.y}, uv_rect.max,                    color };
	vertices[3] = UI_DRAW_VERTEX{ {rect.min.x, rect.max.y}, {uv_rect.min.x, uv_rect.max.y}, color };

	UI_AddQuadIndices(first_vertex, first_vertex + 1, first_vertex + 2, first_vertex + 3, texture);

	DS_ProfExit();
}

UI_API void UI_DrawRectLines(UI_Rect rect, float thickness, UI_Color color, UI_ScissorRect scissor) {
	UI_DrawRectCorners corners = { {color, color, color, color}, {0}, {0.f, 0.f, 0.f, 0.f} };
	UI_DrawRectLinesEx(rect, &corners, thickness, scissor);
}

UI_API void UI_DrawRectLinesRounded(UI_Rect rect, float thickness, float roundness, UI_Color color, UI_ScissorRect scissor) {
	UI_DrawRectCorners corners = { {color, color, color, color}, {0}, {roundness, roundness, roundness, roundness} };
	UI_DrawRectLinesEx(rect, &corners, thickness, scissor);
}

static UI_Vec2 UI_PointOnRoundedCorner(int corner_index, int vertex_index, int end_vertex_index) {
	// Precomputed cos/sin tables
	//for (int i = 0; i < 2; i++) {
	//	float theta = 3.1415926f * 0.5f * (float)i / (float)2;
	//	printf("{%ff, %ff}, ", cosf(theta), sinf(theta));
	//}
	static const UI_Vec2 p2[2] = {{1.f, 0.f}, {0.707107f, 0.707107f}};
	static const UI_Vec2 p3[3] = {{1.f, 0.f}, {0.866025f, 0.5f}, {0.5f, 0.866025f}};
	static const UI_Vec2 p4[4] = {{1.f, 0.f}, {0.92388f, 0.382683f}, {0.707107f, 0.707107f}, {0.382683f, 0.92388f}};
	static const UI_Vec2 p5[5] = {{1.f, 0.f}, {0.951057f, 0.309017f}, {0.809017f, 0.587785f}, {0.587785f, 0.809017f}, {0.309017f, 0.951056f}};
	static const UI_Vec2 p6[6] = {{1.f, 0.f}, {0.965926f, 0.258819f}, {0.866025f, 0.5f}, {0.707107f, 0.707107f}, {0.5f, 0.866025f}, {0.258819f, 0.965926f}};
	static const UI_Vec2 p7[7] = {{1.f, 0.f}, {0.974928f, 0.222521f}, {0.900969f, 0.433884f}, {0.781832f, 0.62349f}, {0.62349f, 0.781831f}, {0.433884f, 0.900969f}, {0.222521f, 0.974928f}};
	static const UI_Vec2* p[8] = {p2, p2, p2, p3, p4, p5, p6, p7};

	UI_Vec2 c;
	if (end_vertex_index <= 7) {
		c = p[end_vertex_index][vertex_index];
	} else {
		float theta = 3.1415926f * 0.5f * (float)vertex_index / (float)end_vertex_index;
		c = UI_VEC2{ cosf(theta), sinf(theta) };
	}

	UI_Vec2 c_rotated;
	switch (corner_index) {
	case 0: c_rotated = UI_VEC2{+c.x, +c.y}; break;
	case 1: c_rotated = UI_VEC2{-c.y, +c.x}; break;
	case 2: c_rotated = UI_VEC2{-c.x, -c.y}; break;
	case 3: c_rotated = UI_VEC2{+c.y, -c.x}; break;
	}
	return c_rotated;
}

UI_API void UI_DrawRectLinesEx(UI_Rect rect, const UI_DrawRectCorners* corners, float thickness, UI_ScissorRect scissor) {
	DS_ProfEnter();

	UI_Vec2 inset_corners[4];
	inset_corners[0] = UI_AddV2(rect.min, UI_VEC2{ corners->roundness[0], corners->roundness[0] });
	inset_corners[1] = UI_AddV2(UI_VEC2{ rect.max.x, rect.min.y }, UI_VEC2{ -corners->roundness[1], corners->roundness[1] });
	inset_corners[2] = UI_AddV2(rect.max, UI_VEC2{ -corners->roundness[2], -corners->roundness[2] });
	inset_corners[3] = UI_AddV2(UI_VEC2{ rect.min.x, rect.max.y }, UI_VEC2{ corners->roundness[3], -corners->roundness[3] });

	// Per each edge (top, right, bottom, left), we add 4 vertices: first outer, first inner, last outer, last inner
	uint32_t edge_verts;
	UI_DrawVertex* v = UI_AddVertices(4 * 4, &edge_verts);
	v[0] = UI_DRAW_VERTEX{ {inset_corners[0].x, rect.min.y}, {0.f, 0.f}, corners->color[0] };
	v[1] = UI_DRAW_VERTEX{ {inset_corners[0].x, rect.min.y + thickness}, {0.f, 0.f}, corners->color[0] };
	v[2] = UI_DRAW_VERTEX{ {inset_corners[1].x, rect.min.y}, {0.f, 0.f}, corners->color[1] };
	v[3] = UI_DRAW_VERTEX{ {inset_corners[1].x, rect.min.y + thickness}, {0.f, 0.f}, corners->color[1] };

	v[4] = UI_DRAW_VERTEX{ {rect.max.x, inset_corners[1].y}, {0.f, 0.f}, corners->color[1] };
	v[5] = UI_DRAW_VERTEX{ {rect.max.x - thickness, inset_corners[1].y}, {0.f, 0.f}, corners->color[1] };
	v[6] = UI_DRAW_VERTEX{ {rect.max.x, inset_corners[2].y}, {0.f, 0.f}, corners->color[2] };
	v[7] = UI_DRAW_VERTEX{ {rect.max.x - thickness, inset_corners[2].y}, {0.f, 0.f}, corners->color[2] };

	v[8] = UI_DRAW_VERTEX{ {inset_corners[2].x, rect.max.y}, {0.f, 0.f}, corners->color[2] };
	v[9] = UI_DRAW_VERTEX{ {inset_corners[2].x, rect.max.y - thickness}, {0.f, 0.f}, corners->color[2] };
	v[10] = UI_DRAW_VERTEX{ {inset_corners[3].x, rect.max.y}, {0.f, 0.f}, corners->color[3] };
	v[11] = UI_DRAW_VERTEX{ {inset_corners[3].x, rect.max.y - thickness}, {0.f, 0.f}, corners->color[3] };

	v[12] = UI_DRAW_VERTEX{ {rect.min.x, inset_corners[3].y}, {0.f, 0.f}, corners->color[3] };
	v[13] = UI_DRAW_VERTEX{ {rect.min.x + thickness, inset_corners[3].y}, {0.f, 0.f}, corners->color[3] };
	v[14] = UI_DRAW_VERTEX{ {rect.min.x, inset_corners[0].y}, {0.f, 0.f}, corners->color[0] };
	v[15] = UI_DRAW_VERTEX{ {rect.min.x + thickness, inset_corners[0].y}, {0.f, 0.f}, corners->color[0] };

	// Generate edge quads
	for (uint32_t base = edge_verts; base < edge_verts + 16; base += 4) {
		UI_AddTriangleIndicesAndClip(base + 0, base + 2, base + 3, UI_TEXTURE_ID_NIL, scissor);
		UI_AddTriangleIndicesAndClip(base + 0, base + 3, base + 1, UI_TEXTURE_ID_NIL, scissor);
	}

	const int end_corner_vertex = 2;

	for (uint32_t corner = 0; corner < 4; corner++) {
		float outer_radius_x = -corners->roundness[corner];
		float outer_radius_y = -corners->roundness[corner];
		float mid_radius_x = -(corners->roundness[corner] - thickness);
		float mid_radius_y = -(corners->roundness[corner] - thickness);

		//float start_theta = 3.1415926f * 0.5f * (float)(corner);

		uint32_t prev_verts_first = edge_verts + 2 + 4 * ((corner + 3) % 4);

		// Generate corner triangles
		for (int i = 1; i < end_corner_vertex; i++) {
			UI_Vec2 dir = UI_PointOnRoundedCorner(corner, i, end_corner_vertex);
			// float theta = start_theta + ((float)i / (float)(end_corner_vertex)) * (3.1415926f * 0.5f);
			// float dir_x = cosf(theta);
			// float dir_y = sinf(theta);

			UI_Vec2 outer_pos = UI_AddV2(inset_corners[corner], UI_VEC2{ dir.x * outer_radius_x, dir.y * outer_radius_y });
			UI_Vec2 mid_pos = UI_AddV2(inset_corners[corner], UI_VEC2{ dir.x * mid_radius_x, dir.y * mid_radius_y });

			uint32_t new_verts_first;
			UI_DrawVertex* new_verts = UI_AddVertices(2, &new_verts_first);
			new_verts[0] = UI_DRAW_VERTEX{ outer_pos, {0.f, 0.f}, corners->color[corner] };
			new_verts[1] = UI_DRAW_VERTEX{ mid_pos, {0.f, 0.f}, corners->color[corner] };

			UI_AddTriangleIndicesAndClip(prev_verts_first + 0, new_verts_first + 0, new_verts_first + 1, UI_TEXTURE_ID_NIL, scissor);
			UI_AddTriangleIndicesAndClip(prev_verts_first + 0, new_verts_first + 1, prev_verts_first + 1, UI_TEXTURE_ID_NIL, scissor);
			prev_verts_first = new_verts_first;
		}

		uint32_t new_verts_first = edge_verts + 4 * corner;
		UI_AddTriangleIndicesAndClip(prev_verts_first + 0, new_verts_first + 0, new_verts_first + 1, UI_TEXTURE_ID_NIL, scissor);
		UI_AddTriangleIndicesAndClip(prev_verts_first + 0, new_verts_first + 1, prev_verts_first + 1, UI_TEXTURE_ID_NIL, scissor);
	}

	DS_ProfExit();
}

UI_API void UI_DrawCircle(UI_Vec2 p, float radius, int segments, UI_Color color, UI_ScissorRect scissor) {
	uint32_t first_vertex;
	UI_DrawVertex* vertices = UI_AddVertices(segments, &first_vertex);
	for (int i = 0; i < segments; i++) {
		float theta = ((float)i / (float)segments) * (2.f * 3.141592f);
		UI_Vec2 v = { p.x + radius * cosf(theta), p.y + radius * sinf(theta) };
		vertices[i] = UI_DRAW_VERTEX{ {v.x, v.y}, {0, 0}, color };
	}
	for (int i = 2; i < segments; i++) {
		UI_AddTriangleIndicesAndClip(first_vertex, first_vertex + i - 1, first_vertex + i, UI_TEXTURE_ID_NIL, scissor);
	}
}

UI_API void UI_DrawTriangle(UI_Vec2 a, UI_Vec2 b, UI_Vec2 c, UI_Color color, UI_ScissorRect scissor) {
	uint32_t first_vert;
	UI_DrawVertex* v = UI_AddVertices(4, &first_vert);
	v[0] = UI_DRAW_VERTEX{a, UI_WHITE_PIXEL_UV, color};
	v[1] = UI_DRAW_VERTEX{b, UI_WHITE_PIXEL_UV, color};
	v[2] = UI_DRAW_VERTEX{c, UI_WHITE_PIXEL_UV, color};
	UI_AddTriangleIndicesAndClip(first_vert, first_vert + 1, first_vert + 2, UI_TEXTURE_ID_NIL, scissor);
}

UI_API void UI_DrawQuad(UI_Vec2 a, UI_Vec2 b, UI_Vec2 c, UI_Vec2 d, UI_Color color, UI_ScissorRect scissor) {
	uint32_t first_vert;
	UI_DrawVertex* v = UI_AddVertices(4, &first_vert);
	v[0] = UI_DRAW_VERTEX{a, UI_WHITE_PIXEL_UV, color};
	v[1] = UI_DRAW_VERTEX{b, UI_WHITE_PIXEL_UV, color};
	v[2] = UI_DRAW_VERTEX{c, UI_WHITE_PIXEL_UV, color};
	v[3] = UI_DRAW_VERTEX{d, UI_WHITE_PIXEL_UV, color};
	UI_AddQuadIndicesAndClip(first_vert, first_vert + 1, first_vert + 2, first_vert + 3, UI_TEXTURE_ID_NIL, scissor);
}

UI_API void UI_DrawRect(UI_Rect rect, UI_Color color, UI_ScissorRect scissor) {
	uint32_t first_vert;
	UI_DrawVertex* v = UI_AddVertices(4, &first_vert);
	v[0] = UI_DRAW_VERTEX{{rect.min.x, rect.min.y}, UI_WHITE_PIXEL_UV, color};
	v[1] = UI_DRAW_VERTEX{{rect.max.x, rect.min.y}, UI_WHITE_PIXEL_UV, color};
	v[2] = UI_DRAW_VERTEX{{rect.max.x, rect.max.y}, UI_WHITE_PIXEL_UV, color};
	v[3] = UI_DRAW_VERTEX{{rect.min.x, rect.max.y}, UI_WHITE_PIXEL_UV, color};
	UI_AddQuadIndicesAndClip(first_vert, first_vert + 1, first_vert + 2, first_vert + 3, UI_TEXTURE_ID_NIL, scissor);
}

UI_API void UI_DrawRectRounded(UI_Rect rect, float roundness, UI_Color color, int num_corner_segments, UI_ScissorRect scissor) {
	UI_DrawRectCorners corners = { {color, color, color, color}, {color, color, color, color}, {roundness, roundness, roundness, roundness} };
	UI_DrawRectEx(rect, &corners, num_corner_segments, scissor);
}

UI_API void UI_DrawRectRounded2(UI_Rect rect, float roundness, UI_Color inner_color, UI_Color outer_color, int num_corner_segments, UI_ScissorRect scissor) {
	UI_DrawRectCorners corners = {
		{inner_color, inner_color, inner_color, inner_color},
		{outer_color, outer_color, outer_color, outer_color},
		{roundness, roundness, roundness, roundness} };
	UI_DrawRectEx(rect, &corners, num_corner_segments, scissor);
}

UI_API void UI_DrawRectEx(UI_Rect rect, const UI_DrawRectCorners* corners, int num_corner_segments, UI_ScissorRect scissor) {
	UI_Vec2 inset_corners[4];
	inset_corners[0] = UI_AddV2(rect.min, UI_VEC2{ corners->roundness[0], corners->roundness[0] });
	inset_corners[1] = UI_AddV2(UI_VEC2{ rect.max.x, rect.min.y }, UI_VEC2{ -corners->roundness[1], corners->roundness[1] });
	inset_corners[2] = UI_AddV2(rect.max, UI_VEC2{ -corners->roundness[2], -corners->roundness[2] });
	inset_corners[3] = UI_AddV2(UI_VEC2{ rect.min.x, rect.max.y }, UI_VEC2{ corners->roundness[3], -corners->roundness[3] });
	if (inset_corners[0].x > inset_corners[2].x) return; // discard invalid rects
	if (inset_corners[0].y > inset_corners[2].y) return; // discard invalid rects
	
	uint32_t inset_corner_verts;
	UI_DrawVertex* v = UI_AddVertices(12, &inset_corner_verts);
	v[0] = UI_DRAW_VERTEX{ inset_corners[0], UI_WHITE_PIXEL_UV, corners->color[0] };
	v[1] = UI_DRAW_VERTEX{ inset_corners[1], UI_WHITE_PIXEL_UV, corners->color[1] };
	v[2] = UI_DRAW_VERTEX{ inset_corners[2], UI_WHITE_PIXEL_UV, corners->color[2] };
	v[3] = UI_DRAW_VERTEX{ inset_corners[3], UI_WHITE_PIXEL_UV, corners->color[3] };

	uint32_t border_verts = inset_corner_verts + 4;
	v[4] = UI_DRAW_VERTEX{ {rect.min.x, inset_corners[0].y}, UI_WHITE_PIXEL_UV, corners->outer_color[0] };
	v[5] = UI_DRAW_VERTEX{ {inset_corners[0].x, rect.min.y}, UI_WHITE_PIXEL_UV, corners->outer_color[0] };
	v[6] = UI_DRAW_VERTEX{ {inset_corners[1].x, rect.min.y}, UI_WHITE_PIXEL_UV, corners->outer_color[1] };
	v[7] = UI_DRAW_VERTEX{ {rect.max.x, inset_corners[1].y}, UI_WHITE_PIXEL_UV, corners->outer_color[1] };
	v[8] = UI_DRAW_VERTEX{ {rect.max.x, inset_corners[2].y}, UI_WHITE_PIXEL_UV, corners->outer_color[2] };
	v[9] = UI_DRAW_VERTEX{ {inset_corners[2].x, rect.max.y}, UI_WHITE_PIXEL_UV, corners->outer_color[2] };
	v[10] = UI_DRAW_VERTEX{ {inset_corners[3].x, rect.max.y}, UI_WHITE_PIXEL_UV, corners->outer_color[3] };
	v[11] = UI_DRAW_VERTEX{ {rect.min.x, inset_corners[3].y}, UI_WHITE_PIXEL_UV, corners->outer_color[3] };

	// edge quads
	UI_AddQuadIndicesAndClip(border_verts + 1, border_verts + 2, inset_corner_verts + 1, inset_corner_verts + 0, UI_TEXTURE_ID_NIL, scissor); // top edge
	UI_AddQuadIndicesAndClip(border_verts + 3, border_verts + 4, inset_corner_verts + 2, inset_corner_verts + 1, UI_TEXTURE_ID_NIL, scissor); // right edge
	UI_AddQuadIndicesAndClip(border_verts + 5, border_verts + 6, inset_corner_verts + 3, inset_corner_verts + 2, UI_TEXTURE_ID_NIL, scissor); // bottom edge
	UI_AddQuadIndicesAndClip(border_verts + 7, border_verts + 0, inset_corner_verts + 0, inset_corner_verts + 3, UI_TEXTURE_ID_NIL, scissor); // left edge

	for (int corner_i = 0; corner_i < 4; corner_i++) {
		float r = -corners->roundness[corner_i];
		//float radius_x = -corners->roundness[corner];
		//float radius_y = -corners->roundness[corner];

		//float start_theta = 3.1415926f * 0.5f * (float)corner;

		uint32_t prev_vert_idx = border_verts + corner_i * 2;

		// Generate corner triangles
		for (int i = 1; i < num_corner_segments; i++) {
			UI_Vec2 c = UI_PointOnRoundedCorner(corner_i, i, num_corner_segments);
			
			uint32_t new_vert_idx;
			UI_DrawVertex* new_vert = UI_AddVertices(1, &new_vert_idx);
			new_vert[0] = UI_DRAW_VERTEX{ {inset_corners[corner_i].x + r*c.x, inset_corners[corner_i].y + r*c.y}, UI_WHITE_PIXEL_UV, corners->outer_color[corner_i] };

			UI_AddTriangleIndicesAndClip(inset_corner_verts + corner_i, prev_vert_idx, new_vert_idx, UI_TEXTURE_ID_NIL, scissor);
			prev_vert_idx = new_vert_idx;
		}

		UI_AddTriangleIndicesAndClip(inset_corner_verts + corner_i, prev_vert_idx, border_verts + corner_i * 2 + 1, UI_TEXTURE_ID_NIL, scissor);
	}

	UI_AddQuadIndicesAndClip(inset_corner_verts, inset_corner_verts + 1, inset_corner_verts + 2, inset_corner_verts + 3, UI_TEXTURE_ID_NIL, scissor);
}

UI_API void UI_DrawPoint(UI_Vec2 p, float thickness, UI_Color color, UI_ScissorRect scissor) {
	DS_ProfEnter();
	UI_Vec2 extent = { 0.5f * thickness, 0.5f * thickness };
	UI_Rect rect = { UI_SubV2(p, extent), UI_AddV2(p, extent) };
	UI_DrawRect(rect, color, scissor);
	DS_ProfExit();
}

UI_API void UI_DrawLine(UI_Vec2 a, UI_Vec2 b, float thickness, UI_Color color, UI_ScissorRect scissor) {
	UI_Vec2 points[] = { a, b };
	UI_Color colors[] = { color, color };
	UI_DrawPolyline(points, colors, 2, thickness, scissor);
}

UI_API void UI_DrawLineEx(UI_Vec2 a, UI_Vec2 b, float thickness, UI_Color a_color, UI_Color b_color, UI_ScissorRect scissor) {
	UI_Vec2 points[] = { a, b };
	UI_Color colors[] = { a_color, b_color };
	UI_DrawPolyline(points, colors, 2, thickness, scissor);
}

UI_API void UI_DrawPolylineEx(const UI_Vec2* points, const UI_Color* colors, int points_count, float thickness, bool loop,
	float split_miter_threshold, UI_ScissorRect scissor)
{
	if (points_count < 2) return;
	DS_ProfEnter();

	float half_thickness = thickness * 0.5f;

	UI_Vec2 start_dir, end_dir;

	DS_DynArray(UI_Vec2) line_normals = { UI_FrameArena() };
	DS_ArrResizeUndef(&line_normals, points_count);

	int last = points_count - 1;
	for (int i = 0; i < points_count; i++) {
		UI_Vec2 p1 = points[i];
		UI_Vec2 p2 = points[i == last ? 0 : i + 1];

		UI_Vec2 dir = UI_SubV2(p2, p1);
		float length = sqrtf(dir.x * dir.x + dir.y * dir.y);
		dir = UI_MulV2F(dir, 1.f / length);

		UI_Vec2 dir_rotated = { -dir.y, dir.x };
		line_normals.data[i] = dir_rotated;

		if (i == 0) start_dir = dir;
		if (i == last-1) end_dir = dir;
	}

	uint32_t first_idx[2];
	uint32_t prev_idx[2];

	for (int i = 0; i < points_count; i++) {
		UI_Vec2 p = points[i];
		UI_Color color = colors[i];

		if (!loop) {
			// Extend start and end points outwards by half thickness
			if (i == 0)    p = UI_AddV2(p, UI_MulV2F(start_dir, -half_thickness));
			if (i == last) p = UI_AddV2(p, UI_MulV2F(end_dir, half_thickness));
		}

		UI_Vec2 n_pre, n_post;
		if (loop) {
			n_pre = line_normals.data[i == 0 ? last : i - 1];
			n_post = line_normals.data[i];
		} else {
			n_pre = line_normals.data[i == 0 ? 0 : i - 1];
			n_post = line_normals.data[i == last ? i - 1 : i];
		}

		if (n_pre.x*n_post.x + n_pre.y*n_post.y < split_miter_threshold) {
			uint32_t new_vertices;
			UI_DrawVertex* v = UI_AddVertices(4, &new_vertices);
			v[0] = UI_DRAW_VERTEX{{p.x + n_pre.x*half_thickness, p.y + n_pre.y*half_thickness}, {0.f, 0.f}, color};
			v[1] = UI_DRAW_VERTEX{{p.x - n_pre.x*half_thickness, p.y - n_pre.y*half_thickness}, {0.f, 0.f}, color};
			v[2] = UI_DRAW_VERTEX{{p.x + n_post.x*half_thickness, p.y + n_post.y*half_thickness}, {0.f, 0.f}, color};
			v[3] = UI_DRAW_VERTEX{{p.x - n_post.x*half_thickness, p.y - n_post.y*half_thickness}, {0.f, 0.f}, color};

			if (loop || (i != 0 && i != last)) {
				UI_AddQuadIndicesAndClip(new_vertices+0, new_vertices+1, new_vertices+3, new_vertices+2, UI_TEXTURE_ID_NIL, scissor);
			}

			if (i > 0) {
				UI_AddQuadIndicesAndClip(prev_idx[0], prev_idx[1], new_vertices + 1, new_vertices + 0, UI_TEXTURE_ID_NIL, scissor);
			} else {
				first_idx[0] = new_vertices + 0;
				first_idx[1] = new_vertices + 1;
			}
			prev_idx[0] = new_vertices + 2;
			prev_idx[1] = new_vertices + 3;
		}
		else {
			UI_Vec2 n = UI_AddV2(n_pre, n_post); // Note: This doesn't need to be normalized, the math works regardless
			float denom = n.x * n_pre.x + n.y * n_pre.y;
			float t = half_thickness / denom;

			uint32_t new_vertices;
			UI_DrawVertex* v = UI_AddVertices(2, &new_vertices);
			v[0] = UI_DRAW_VERTEX{{p.x + n.x*t, p.y + n.y*t}, {0.f, 0.f}, color};
			v[1] = UI_DRAW_VERTEX{{p.x - n.x*t, p.y - n.y*t}, {0.f, 0.f}, color};

			if (i > 0) {
				UI_AddQuadIndicesAndClip(prev_idx[0], prev_idx[1], new_vertices + 1, new_vertices + 0, UI_TEXTURE_ID_NIL, scissor);
			} else {
				first_idx[0] = new_vertices + 0;
				first_idx[1] = new_vertices + 1;
			}
			prev_idx[0] = new_vertices + 0;
			prev_idx[1] = new_vertices + 1;
		}
	}

	if (loop) {
		UI_AddQuadIndicesAndClip(prev_idx[0], prev_idx[1], first_idx[1], first_idx[0], UI_TEXTURE_ID_NIL, scissor);
	}

	DS_ProfExit();
}

UI_API void UI_DrawPolyline(const UI_Vec2* points, const UI_Color* colors, int points_count, float thickness, UI_ScissorRect scissor) {
	UI_DrawPolylineEx(points, colors, points_count, thickness, false, 0.7f, scissor);
}

UI_API void UI_DrawPolylineLoop(const UI_Vec2* points, const UI_Color* colors, int points_count, float thickness, UI_ScissorRect scissor) {
	UI_DrawPolylineEx(points, colors, points_count, thickness, true, 0.7f, scissor);
}

UI_API UI_Vec2 UI_DrawText(UI_String text, UI_FontUsage font, UI_Vec2 origin, UI_AlignH align_h, UI_AlignV align_v, UI_Color color, UI_ScissorRect scissor) {
	DS_ProfEnter();
	UI_Vec2 s = { UI_TextWidth(text, font), font.size };

	if (align_h == UI_AlignH_Middle) {
		origin.x -= s.x * 0.5f;
	}
	else if (align_h == UI_AlignH_Right) {
		origin.x -= s.x;
	}

	if (align_v == UI_AlignV_Middle) {
		origin.y -= s.y * 0.5f;
	}
	else if (align_v == UI_AlignV_Lower) {
		origin.y -= s.y;
	}

	origin.x = (float)(int)(origin.x + 0.5f); // round to integer
	origin.y = (float)(int)(origin.y + 0.5f); // round to integer

	for STR_Each(text, r, i) {
		int atlas_index = 0;
		UI_CachedGlyph glyph = UI_GetCachedGlyph(r, font, &atlas_index);

		UI_Rect glyph_rect;
		glyph_rect.min.x = origin.x + glyph.offset_pixels.x;
		glyph_rect.min.y = origin.y + glyph.offset_pixels.y;
		glyph_rect.max.x = glyph_rect.min.x + glyph.size_pixels.x;
		glyph_rect.max.y = glyph_rect.min.y + glyph.size_pixels.y;

		UI_Rect glyph_uv_rect;
		glyph_uv_rect.min = glyph.origin_uv;
		glyph_uv_rect.max = glyph_uv_rect.min;
		glyph_uv_rect.max.x += glyph.size_pixels.x / (float)UI_GLYPH_MAP_SIZE;
		glyph_uv_rect.max.y += glyph.size_pixels.y / (float)UI_GLYPH_MAP_SIZE;

		// Apply clipping rect
		UI_DrawSprite(glyph_rect, color, glyph_uv_rect, UI_STATE.atlases[atlas_index], scissor);
		origin.x += glyph.x_advance;
	}

	DS_ProfExit();
	return s;
}

static UI_Key UI_GetArrangerSetKey() { return UI_KEY(); }

UI_API UI_Box* UI_PushArrangerSet(UI_Key key, UI_Size w, UI_Size h) {
	DS_ProfEnter();
	UI_Box* box = UI_AddBox(key, w, h, /*UI_BoxFlag_NoScissor*/0);
	UI_PushBox(box);
	
	UI_ArrangerSet arranger_set = {0};
	UI_BoxAddCustomData(box, UI_GetArrangerSetKey(), &arranger_set, sizeof(UI_ArrangerSet));

	DS_ProfExit();
	return box;
}

UI_API void UI_PopArrangerSet(UI_Box* box, UI_ArrangersRequest* out_edit_request) {
	DS_ProfEnter();
	UI_PopBox(box);

	UI_ArrangerSet* arranger_set = (UI_ArrangerSet*)UI_BoxGetCustomData(box, UI_GetArrangerSetKey(), sizeof(UI_ArrangerSet));
	UI_CHECK(arranger_set);

	float box_origin_prev_frame = box->prev_frame ? box->prev_frame->computed_position.y : 0.f;
	float mouse_rel_y = UI_STATE.mouse_pos.y - box_origin_prev_frame;

	UI_Box* dragging = arranger_set->dragging_elem; // may be NULL

	// here we compute `computed_position` for each box using relative coordinates
	UI_BoxComputeRects(box, UI_VEC2{ 0, 0 });

	int dragging_index = 0;
	int target_index = 0;
	if (dragging) {
		int elem_count = 0;
		for (UI_Box* elem = box->first_child[0]; elem; elem = elem->next[1]) {
			if (elem == dragging) {
				dragging_index = elem_count;
			}
			elem_count++;
		}

		for (UI_Box* elem = box->first_child[0]; elem; elem = elem->next[1]) {
			if (elem->computed_position.y > mouse_rel_y) {
				break;
			}
			target_index++;
		}

		target_index = UI_Min(UI_Max(target_index - 1, 0), elem_count - 1);

		// When dragging an arranger, bring the dragged element to the END so that it will be drawn on top of the other things!
		DS_DoublyListRemove(dragging->parent->first_child, dragging, next);
		DS_DoublyListPushBack(dragging->parent->first_child, dragging, next);
	}

	// Now we have the original positions and sizes for each box.
	// Then we just need to figure out their target positions and provide them as offsets

	int i = 0;
	for (UI_Box* elem = box->first_child[0]; elem; elem = elem->next[1]) {
		elem->flags |= UI_BoxFlag_NoAutoOffset;

		float interp_amount = 0.2f;
		float offset = elem->computed_position.y;
		if (dragging) {
			if (elem == dragging) {
				offset += UI_STATE.mouse_pos.y - UI_STATE.last_released_mouse_pos.y;
				interp_amount = 1.f;
			}
			else {
				if (i >= target_index && i < dragging_index) {
					offset += dragging->computed_size.y;
				}
				if (i >= dragging_index && i < target_index) {
					offset -= dragging->computed_size.y;
				}
			}
		}

		if (elem->prev_frame) {
			offset = UI_Lerp(elem->prev_frame->offset.y, offset, interp_amount); // smooth animation
		}

		elem->offset.y = offset;
		i++;
	}

	UI_ArrangersRequest edit_request = {0};
	if (dragging && UI_InputWasReleased(UI_Input_MouseLeft)) {
		edit_request.move_from = dragging_index;
		edit_request.move_to = target_index;
	}
	*out_edit_request = edit_request;
	DS_ProfExit();
}

UI_API UI_Box* UI_AddArranger(UI_Key key, UI_Size w, UI_Size h) {
	DS_ProfEnter();
	UI_Box* box = UI_AddBoxWithText(key, w, h, UI_BoxFlag_Clickable, STR_(":"));

	bool holding_down = UI_IsClickingDown(box->key);
	if (holding_down || UI_IsHovered(box->key)) {
		UI_STATE.outputs.cursor = UI_MouseCursor_ResizeV;
	}

	if (holding_down) {
		// find the element

		UI_Box* elem = box;
		for (; elem; elem = elem->parent) {
			if (elem->parent) {
				UI_ArrangerSet* arranger_set = (UI_ArrangerSet*)UI_BoxGetCustomData(elem->parent, UI_GetArrangerSetKey(), sizeof(UI_ArrangerSet));
				if (arranger_set) {
					arranger_set->dragging_elem = elem;
					break;
				}
			}
		}

		// This arranger is not inside of an arranger set. Did you call UI_ArrangersPush?
		UI_CHECK(elem);
	}
	DS_ProfExit();
	return box;
}

#endif // UI_IMPLEMENTATION
#endif // FIRE_UI_INCLUDED




