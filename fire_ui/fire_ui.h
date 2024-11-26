// Fire UI - by Eero Mutka (https://eeromutka.github.io/)
// 
// Immediate mode user interface library.
//
// This code is released under the MIT license (https://opensource.org/licenses/MIT).
//

#pragma once
#define FIRE_UI_INCLUDED

#include "../fire_ds.h"
#include "../fire_string.h"

#ifndef UI_ASSERT_OVERRIDE
#include <assert.h>
#define UI_ASSERT(x) assert(x)
#endif

#ifndef UI_PROFILER_MACROS_OVERRIDE
// Function-level profiler scope. A single function may only have one of these, and it should span the entire function.
// As a convention, we put this to every function in this library that does a significant amount of work. It's a bit subjective.
#define UI_ProfEnter()
#define UI_ProfExit()
#endif

#ifdef __cplusplus
#define UI_LangAgnosticLiteral(T) T   // in C++, struct and union literals are of the form MyStructType{...}
#else
#define UI_LangAgnosticLiteral(T) (T) // in C, struct and union literals are of the form (MyStructType){...}
#endif

// * Retrieve an unique key for the macro usage site.
#define UI_KEY()            UI_LangAgnosticLiteral(UI_Key){(uintptr_t)__FILE__ ^ (uintptr_t)__COUNTER__}
#define UI_KKEY(OTHER_KEY)  UI_HashKey(UI_KEY(), (OTHER_KEY))
#define UI_BKEY(OTHER_BOX)  UI_HashKey(UI_KEY(), (OTHER_BOX)->key)

#define UI_BOX()            UI_GetOrAddBox(UI_KEY(), true)
#define UI_BBOX(OTHER_BOX)  UI_GetOrAddBox(UI_BKEY(OTHER_BOX), true)
#define UI_KBOX(OTHER_KEY)  UI_GetOrAddBox(UI_KKEY(OTHER_KEY), true)

#define UI_Max(A, B) ((A) > (B) ? (A) : (B))
#define UI_Min(A, B) ((A) < (B) ? (A) : (B))
#define UI_Abs(X) ((X) < 0 ? -(X) : (X))

#define UI_INFINITE 10000000.f

#ifndef UI_CUSTOM_VEC2
typedef union UI_Vec2 {
	struct { float x, y; };
	float _[2];
} UI_Vec2;
#endif
#define UI_VEC2  UI_LangAgnosticLiteral(UI_Vec2)

// -100 to -99 is unexpanded size fit, take as much parent space as the ratio
// -200 to -199 is unexpanded size 0, take as much parent space as the ratio
typedef float UI_Size;
#define UI_SizeFit()        (-100.f)
#define UI_SizeFlex(WEIGHT) ((WEIGHT) - 100.f)

typedef uint64_t UI_Key; // 0 is invalid value
#define UI_INVALID_KEY (UI_Key)0

typedef int UI_Axis;
enum {
	UI_Axis_X,
	UI_Axis_Y,
};

typedef int UI_BoxFlags;
typedef enum UI_BoxFlagBits {
	UI_BoxFlag_HasText = 1 << 1, // Note: A box which has text must not have children
	UI_BoxFlag_Clickable = 1 << 2, // Note: When set, this will make 'pressed' input come to this box instead of the parent box
	UI_BoxFlag_PressingStaysWithoutHover = 1 << 3,
	UI_BoxFlag_Horizontal = 1 << 4, // Children are placed horizontally rather than vertically
	UI_BoxFlag_ReverseLayoutX = 1 << 5, // Reverse the layout direction for children.
	UI_BoxFlag_ReverseLayoutY = 1 << 6, // Reverse the layout direction for children.
	UI_BoxFlag_Selectable = 1 << 7,   // When set, the keyboard navigator can move to this UI box.
	UI_BoxFlag_NoAutoOffset = 1 << 8, // When set, this box won't use the layout system to determine its position, it'll be fully controlled by the `offset` field instead.
	UI_BoxFlag_NoScissor = 1 << 9,
	UI_BoxFlag_NoHover = 1 << 10, // Propagated from parent during AddBox
	UI_BoxFlag_NoFlexDownX = 1 << 11,
	UI_BoxFlag_NoFlexDownY = 1 << 12,
	
	// Draw flags, used in UI_DrawBoxDefault
	UI_BoxFlag_DrawBorder = 1 << 13,
	UI_BoxFlag_DrawTransparentBackground = 1 << 14,
	UI_BoxFlag_DrawOpaqueBackground = 1 << 15,
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
	// TODO: we could add function pointers here to let you use your own text data-structure instead of this.
} UI_Text;

#define UI_TextToStr(TEXT) UI_LangAgnosticLiteral(STR_View){(const char*)(TEXT).text.data, (size_t)(TEXT).text.count}

typedef struct UI_CachedGlyphKey {
	// !!! memcmp is used on this struct, so it must not have any compiler-generated padding.
	uint32_t codepoint;
	uint16_t font_index;
	uint16_t font_size;
} UI_CachedGlyphKey;

typedef struct UI_CachedGlyph {
	UI_Vec2 uv_min; // in normalized UV coordinates
	UI_Vec2 uv_max; // in normalized UV coordinates
	UI_Vec2 offset; // in pixel coordinates
	UI_Vec2 size;   // in pixel coordinates
	float advance; // X-advance to the next character in pixel coordinates
} UI_CachedGlyph;

typedef uint16_t UI_FontID;

typedef struct UI_Font {
	UI_FontID id;
	uint16_t size;
} UI_Font;

typedef struct UI_ArrangerSet {
	struct UI_Box* dragging_elem; // may be NULL
} UI_ArrangerSet;

typedef struct UI_ArrangersRequest {
	// If there's nothing to move, both of these will be set to 0
	int move_from; // index of the moved element before the move
	int move_to; // index of the moved element after the move
} UI_ArrangersRequest;

typedef struct UI_BoxVariableHeader UI_BoxVariableHeader;
struct UI_BoxVariableHeader {
	UI_Key key;
	UI_BoxVariableHeader* next;
	int debug_size;
};

typedef struct UI_DrawBoxDefaultArgs {
	const UI_Color* text_color;
	const UI_Color* transparent_bg_color;
	const UI_Color* opaque_bg_color;
	const UI_Color* border_color;
} UI_DrawBoxDefaultArgs;

typedef struct UI_Box UI_Box;

typedef struct UI_VarHolderBox {
	UI_BoxVariableHeader* variables; // linked list of variables
	UI_Box* prev_frame; // NULL if a box with the same key didn't exist (wasn't added to the tree) during the previous frame
} UI_VarHolderBox;

// You may always pass NULL to parameters that take in an "optional arguments" struct.
// Additionally, any member of an "optional arguments" struct may be NULL.
// IMPORTANT: When using optional argument structs, all non-null pointers
// MUST point to data which persists until the drawing phase, i.e. copied into a temp arena!!
typedef struct UI_BoxDrawOptArgs {
	const UI_Color* text_color;
	const UI_Color* transparent_bg_color;
	const UI_Color* opaque_bg_color;
	const UI_Color* border_color;
} UI_BoxDrawOptArgs;

struct UI_Box {
	UI_BoxVariableHeader* variables; // linked list of variables
	UI_Box* prev_frame; // NULL if a box with the same key didn't exist (wasn't added to the tree) during the previous frame

	UI_Key key;
	UI_Box* parent;
	UI_Box* prev;
	UI_Box* next;
	UI_Box* first_child;
	UI_Box* last_child;

	union {
		UI_BoxFlags flags;
		UI_BoxFlagBits flags_bits; // for debugger visualization
	};

	UI_Size size[2];
	UI_Vec2 offset;
	UI_Vec2 inner_padding; // applied to text or children
	UI_Font font;

	STR_View text;

	void (*compute_unexpanded_size)(UI_Box* box, UI_Axis axis, int pass, bool* request_second_pass);

	// These will be computed in UI_ComputeBox
	UI_Vec2 computed_position;
	UI_Vec2 computed_unexpanded_size;
	UI_Vec2 computed_expanded_size;
	UI_Rect computed_rect; // final rectangle including clipping
	
	// Drawing
	void (*draw)(UI_Box* box);
	UI_BoxDrawOptArgs* draw_opts;
	
	void* extended_data; // This will soon replace the box variable system
};

typedef struct UI_Mark {
	int line;
	int col;
} UI_Mark;
#define UI_MARK  UI_LangAgnosticLiteral(UI_Mark)

typedef struct UI_Selection {
	UI_Mark _[2]; // These must be sorted so that range[0] represents a mark before range[1]. You can sort them using `UI_SelectionFixOrder`.
	uint32_t cursor;
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

typedef struct UI_Texture UI_Texture; // user-defined structure

typedef struct UI_DrawCommand {
	UI_Texture* texture; // NULL means the atlas texture
	UI_Rect scissor_rect;
	uint32_t first_index;
	uint32_t index_count;
} UI_DrawCommand;

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
	UI_InputEvent_DoubleClick = 1 << 3,
} UI_InputEvent;

typedef struct UI_Backend {
	// Rendering
	UI_DrawVertex* (*ResizeAndMapVertexBuffer)(int num_vertices);
	uint32_t* (*ResizeAndMapIndexBuffer)(int num_indices);
	
	// Text rendering
	UI_CachedGlyph (*GetCachedGlyph)(uint32_t codepoint, UI_Font font);
} UI_Backend;

typedef struct UI_Inputs {
	UI_InputEvents input_events[UI_Input_COUNT];

	UI_Vec2 mouse_position;
	UI_Vec2 mouse_raw_delta;
	float mouse_wheel_delta; // +1.0 means the wheel was rotated forward by one scroll step

	uint32_t text_input_utf32[16];
	int text_input_utf32_length;

	// TODO: get rid of these two
	STR_View (*get_clipboard_string_fn)(void* user_data); // The returned string must stay valid for the rest of the frame
	void (*set_clipboard_string_fn)(STR_View string, void* user_data);

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

	UI_DrawCommand* draw_commands;
	int draw_commands_count;
} UI_Outputs;

typedef DS_Map(UI_Key, void*) UI_PtrFromKeyMap;

typedef struct UI_State {
	DS_Allocator* allocator;
	
	UI_Backend backend;

	DS_Arena _prev_frame_arena;
	DS_Arena _frame_arena;

	UI_PtrFromKeyMap prev_frame_data_from_key;
	UI_PtrFromKeyMap data_from_key;

	bool selection_is_visible; // The selected box can be hidden, i.e. when clicking a button with your mouse. Then, when pressing an arrow key, it becomes visible again.

	UI_Inputs inputs;
	UI_Outputs outputs;

	bool input_is_down[UI_Input_COUNT];

	float time_since_pressed_lmb;
	
	// Mouse position in screen space coordinates, snapped to the pixel center. Placing it at the pixel center means we don't
	// need to worry about degenerate cases where the mouse is exactly at the edge of one or many rectangles when testing for overlap.
	UI_Vec2 mouse_pos;
	
	UI_Vec2 last_released_mouse_pos; // Cleanup: remove from this struct
	UI_Vec2 mouse_travel_distance_after_press; // NOTE: holding alt/shift will modify the speed at which this value changes. Cleanup: remove from this struct

	UI_Key mouse_clicking_down_box;
	UI_Key mouse_clicking_down_box_new;
	UI_Key keyboard_clicking_down_box;
	UI_Key keyboard_clicking_down_box_new;
	UI_Key selected_box;
	UI_Key selected_box_new;
	
	float scrollbar_origin_before_press; // Cleanup: remove from this struct
	
	// -- Builder state -----------------------
	
	UI_Font default_font;
	UI_Font icons_font;
	DS_DynArray(UI_Box*) box_stack;

	// -- Draw state --------------------------
	
	uint32_t* index_buffer;
	int index_buffer_count;
	int index_buffer_capacity;
	
	UI_DrawVertex* vertex_buffer;
	int vertex_buffer_count;
	int vertex_buffer_capacity;
	
	UI_Rect active_scissor_rect;
	UI_Texture* active_texture; // NULL means the atlas texture
	DS_DynArray(UI_DrawCommand) draw_commands;
} UI_State;

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

#ifndef UI_EXTERN
#ifdef __cplusplus
#define UI_EXTERN extern "C"
#else
#define UI_EXTERN extern
#endif
#endif

#ifndef UI_API
#define UI_API UI_EXTERN
#endif

typedef const UI_Rect* UI_ScissorRect; // may be NULL for no scissor

typedef struct UI_ValTextState {
	bool is_editing;
	UI_Selection selection;
} UI_ValTextState;

// You may always pass NULL to parameters that take in an "optional arguments" struct.
// Additionally, any member of an "optional arguments" struct may be NULL.
// IMPORTANT: When using optional argument structs, all non-null pointers
// MUST point to data which persists until the drawing phase, i.e. copied into a temp arena!!
typedef struct UI_ValTextOptArgs {
	const UI_Color* text_color;
} UI_ValTextOptArgs;

typedef struct UI_ValNumericState {
	double value_before_press;
	bool is_dragging;
	bool is_editing_text;
	STR_View text;
} UI_ValNumericState;

typedef struct UI_SplittersState {
	int holding_splitter;  // one-based index
	int hovering_splitter; // one-based index
	float* panel_end_offsets;
	int panel_count;
} UI_SplittersState;

static const UI_Vec2 UI_DEFAULT_TEXT_PADDING = { 10.f, 5.f };

// -- Global state -------
UI_EXTERN UI_State UI_STATE;
UI_EXTERN DS_Arena* UI_TEMP; // Temporary arena for per-frame allocations
// -----------------------

static inline bool UI_InputIsDown(UI_Input input)               { return UI_STATE.input_is_down[input]; }
static inline bool UI_InputWasPressed(UI_Input input)           { return UI_STATE.inputs.input_events[input] & UI_InputEvent_Press; }
static inline bool UI_InputWasPressedOrRepeated(UI_Input input) { return (UI_STATE.inputs.input_events[input] & UI_InputEvent_PressOrRepeat) != 0; }
static inline bool UI_InputWasReleased(UI_Input input)          { return (UI_STATE.inputs.input_events[input] & UI_InputEvent_Release) != 0; }
static inline bool UI_DoubleClickedAnywhere(void)               { return (UI_STATE.inputs.input_events[UI_Input_MouseLeft] & UI_InputEvent_DoubleClick) != 0; }

static inline bool UI_MarkEquals(UI_Mark a, UI_Mark b) { return a.line == b.line && a.col == b.col; }

static inline UI_Vec2 UI_AddV2(UI_Vec2 a, UI_Vec2 b)            { return UI_VEC2{ a.x + b.x, a.y + b.y }; }
static inline UI_Vec2 UI_SubV2(UI_Vec2 a, UI_Vec2 b)            { return UI_VEC2{ a.x - b.x, a.y - b.y }; }
static inline UI_Vec2 UI_MulV2(UI_Vec2 a, UI_Vec2 b)            { return UI_VEC2{ a.x * b.x, a.y * b.y }; }
static inline UI_Vec2 UI_MulV2F(UI_Vec2 a, float b)             { return UI_VEC2{ a.x * b, a.y * b }; }
static inline float UI_Lerp(float a, float b, float t)          { return (1.f - t) * a + t * b; }
static inline UI_Vec2 UI_LerpV2(UI_Vec2 a, UI_Vec2 b, float t)  { return UI_VEC2{ (1.f - t) * a.x + t * b.x, (1.f - t) * a.y + t * b.y }; }

static inline bool UI_PointIsInRect(UI_Rect rect, UI_Vec2 p)    { return p.x >= rect.min.x && p.y >= rect.min.y && p.x <= rect.max.x && p.y <= rect.max.y; }
static inline UI_Vec2 UI_RectSize(UI_Rect rect)                 { return UI_VEC2 { rect.max.x - rect.min.x, rect.max.y - rect.min.y }; }

UI_API UI_Key UI_HashKey(UI_Key a, UI_Key b);
UI_API UI_Key UI_HashPtr(UI_Key a, void* b);
UI_API UI_Key UI_HashInt(UI_Key a, int b);

UI_API void UI_SelectionFixOrder(UI_Selection* sel);

UI_API void UI_RectPad(UI_Rect* rect, float pad);

/*
 `resources_directory` is the path of the `resources` folder that is shipped with FUI.
 FUI will load the default fonts from this folder during `UI_Init`, as well as the shader.
*/
UI_API void UI_Init(DS_Allocator* allocator);
UI_API void UI_Deinit(void);

UI_API void UI_BeginFrame(const UI_Inputs* inputs, UI_Font default_font, UI_Font icons_font);
UI_API void UI_EndFrame(UI_Outputs* outputs);

/*
 The way customization is meant to work with FUI is that if you want to add a new feature, then you simply
 copy code inside the corresponding code (i.e. in UI_draw_box_default) into your own new function and disregard the
 FUI-provided function. FUI tries to break the code down into reusable pieces of code so that it's easy to customize
 the entire tree of UI features that you want to support.
*/
UI_API void UI_DrawBox(UI_Box* box);

UI_API void UI_DrawBoxDefault(UI_Box* box);

// -- Tree builder API with implicit context -------

UI_API UI_Box* UI_GetOrAddBox(UI_Key key, bool assert_newly_added);

UI_API void UI_BoxComputeExpandedSizes(UI_Box* box);
UI_API void UI_BoxComputeRects(UI_Box* box, UI_Vec2 box_position);

UI_API void UI_BoxComputeUnexpandedSizeDefault(UI_Box* box, UI_Axis axis, int pass, bool* request_second_pass);
UI_API void UI_BoxComputeExpandedSize(UI_Box* box, UI_Axis axis, float size);
UI_API void UI_BoxComputeRectsStep(UI_Box* box, UI_Axis axis, float position, UI_ScissorRect scissor);

UI_API void UI_InitRootBox(UI_Box* box, UI_Size w, UI_Size h, UI_BoxFlags flags);
UI_API void UI_PushBox(UI_Box* box);
UI_API void UI_PopBox(UI_Box* box);
UI_API void UI_PopBoxN(UI_Box* box, int n);

UI_API void UI_AddBox(UI_Box* box, UI_Size w, UI_Size h, UI_BoxFlags flags);

UI_API void UI_AddLabel(UI_Box* box, UI_Size w, UI_Size h, UI_BoxFlags flags, STR_View string);

UI_API void UI_AddButton(UI_Box* box, UI_Size w, UI_Size h, UI_BoxFlags flags, STR_View string);

UI_API void UI_AddCheckbox(UI_Box* box, bool* value);

UI_API UI_ValTextState* UI_AddValText(UI_Box* box, UI_Size w, UI_Size h, UI_Text* text, const UI_ValTextOptArgs* optional);

UI_API UI_ValNumericState* UI_AddValInt(UI_Box* box, UI_Size w, UI_Size h, int32_t* value);
UI_API UI_ValNumericState* UI_AddValInt64(UI_Box* box, UI_Size w, UI_Size h, int64_t* value);
UI_API UI_ValNumericState* UI_AddValUInt64(UI_Box* box, UI_Size w, UI_Size h, uint64_t* value);
UI_API UI_ValNumericState* UI_AddValFloat(UI_Box* box, UI_Size w, UI_Size h, float* value);
UI_API UI_ValNumericState* UI_AddValFloat64(UI_Box* box, UI_Size w, UI_Size h, double* value);
UI_API UI_ValNumericState* UI_AddValNumeric(UI_Box* box, UI_Size w, UI_Size h, void* value_64_bit, bool is_signed, bool is_float);

// - Returns false if the collapsable header is closed.
// TODO: draw a custom icon without relying on an icons font.
UI_API bool UI_PushCollapsing(UI_Box* box, UI_Size w, UI_Size h, UI_Size indent, UI_BoxFlags flags, STR_View text);
UI_API void UI_PopCollapsing(UI_Box* box);

UI_API void UI_TextInit(DS_Allocator* allocator, UI_Text* text, STR_View initial_value);
UI_API void UI_TextDeinit(UI_Text* text);
UI_API void UI_TextSet(UI_Text* text, STR_View value);

// - `anchor_x` / `anchor_y` can be 0 or 1: A value of 0 means anchoring the scrollbar to left / top, 1 means anchoring it to right / bottom.
// Returns the scroll offset
UI_API UI_Vec2 UI_PushScrollArea(UI_Box* box, UI_Size w, UI_Size h, UI_BoxFlags flags, int anchor_x, int anchor_y);
UI_API void UI_PopScrollArea(UI_Box* box);

UI_API void UI_PushArrangerSet(UI_Box* box, UI_Size w, UI_Size h);
UI_API void UI_PopArrangerSet(UI_Box* box, UI_ArrangersRequest* out_edit_request);
UI_API void UI_AddArranger(UI_Box* box, UI_Size w, UI_Size h);

// --------------------------------------

#define UI_BoxAddVar(BOX, KEY, VALUE)            UI_BoxAddVarData(BOX, KEY, VALUE, sizeof(*VALUE))

// Returns true if the variable already existed. Otherwise the OUT_VALUE is not touched.
#define UI_BoxGetVar(BOX, KEY, OUT_VALUE)        UI_BoxGetVarData(BOX, KEY, OUT_VALUE, sizeof(*OUT_VALUE))

// If the variable doesn't exist, OUT_PTR will be set to NULL.
#define UI_BoxGetVarPtr(BOX, KEY, OUT_PTR)       UI_BoxGetVarPtrData(BOX, KEY, (void**)(OUT_PTR), sizeof(**(OUT_PTR)))

// Returns true if the variable already existed. New variables get zero-initialized.
#define UI_BoxGetRetainedVar(BOX, KEY, OUT_PTR)  UI_BoxGetRetainedVarData(BOX, KEY, (void**)(OUT_PTR), sizeof(**(OUT_PTR)))

UI_API void UI_BoxAddVarData(UI_Box* box, UI_Key key, void* ptr, int size);
UI_API bool UI_BoxGetVarData(UI_Box* box, UI_Key key, void* out_value, int size);
UI_API void UI_BoxGetVarPtrData(UI_Box* box, UI_Key key, void** out_ptr, int size);
UI_API bool UI_BoxGetRetainedVarData(UI_Box* box, UI_Key key, void** out_ptr, int size);

UI_API bool UI_BoxIsAParentOf(UI_Box* box, UI_Box* child);

UI_API bool UI_Clicked(UI_Box* box);
UI_API bool UI_Pressed(UI_Box* box);
UI_API bool UI_PressedIdle(UI_Box* box); // Differs from UI_Pressed by calling UI_IsHoveredIdle instead of UI_IsHovered
UI_API bool UI_PressedEx(UI_Box* box, UI_Input mouse_button);
UI_API bool UI_PressedIdleEx(UI_Box* box, UI_Input mouse_button); // Differs from UI_Pressed by calling UI_IsHoveredIdle instead of UI_IsHovered
UI_API bool UI_DoubleClicked(UI_Box* box);
UI_API bool UI_DoubleClickedIdle(UI_Box* box); // Differs from UI_DoubleClicked by calling UI_IsHoveredIdle instead of UI_IsHovered

UI_API bool UI_IsHovered(UI_Box* box);
UI_API bool UI_IsHoveredIdle(UI_Box* box); // Differs from UI_IsHovered in that if a clickable child box is also hovered, this will return false.
UI_API bool UI_IsMouseInsideOf(UI_Box* box); // like IsHovered, except ignores the NoHover box flag

UI_API bool UI_IsSelected(UI_Box* box);

UI_API bool UI_IsClickingDown(UI_Box* box);
UI_API bool UI_IsClickingDownAndHovered(UI_Box* box);

UI_API void UI_EditTextSelectAll(const UI_Text* text, UI_Selection* selection);

UI_API UI_SplittersState* UI_Splitters(UI_Key key, UI_Rect area, UI_Axis X, int panel_count, float panel_min_size);
UI_API void UI_SplittersNormalizeToTotalSize(UI_SplittersState* splitters, float total_size);

UI_API float UI_GlyphAdvance(uint32_t codepoint, UI_Font font);
UI_API float UI_TextWidth(STR_View text, UI_Font font);

// -- Drawing ----------------------

// returns "true" if the rect is fully clipped, "false" if there is still some area left
UI_API bool UI_ClipRect(UI_Rect* rect, const UI_Rect* scissor);
UI_API bool UI_ClipRectEx(UI_Rect* rect, UI_Rect* uv_rect, const UI_Rect* scissor);

// At the beginning of each frame, the scissor rect is reset such that it covers the entire screen.
// Try to minimize calls to UI_SetActiveScissorRect, as calling it internally splits the current draw
// batch into a new one, and having too many draw batches / draw calls can be expensive for the GPU.
UI_API void UI_SetActiveScissorRect(UI_Rect rect);
UI_API UI_Rect UI_GetActiveScissorRect();

UI_API uint32_t UI_AddVertices(UI_DrawVertex* vertices, int count); // Returns the index of the first new vertex
UI_API UI_DrawVertex* UI_AddVerticesUnsafe(int count, uint32_t* out_first_vertex); // WARNING: Returned pointer may be invalid on the next call to UI_AddVerticesUnsafe
UI_API void UI_AddIndices(uint32_t* indices, int count, UI_Texture* texture);
UI_API void UI_AddTriangleIndices(uint32_t a, uint32_t b, uint32_t c, UI_Texture* texture);
UI_API void UI_AddQuadIndices(uint32_t a, uint32_t b, uint32_t c, uint32_t d, UI_Texture* texture);

UI_API void UI_DrawRect(UI_Rect rect, UI_Color color);
UI_API void UI_DrawRectRounded(UI_Rect rect, float roundness, UI_Color color, int num_corner_segments);
UI_API void UI_DrawRectRounded2(UI_Rect rect, float roundness, UI_Color inner_color, UI_Color outer_color, int num_corner_segments);
UI_API void UI_DrawRectEx(UI_Rect rect, const UI_DrawRectCorners* corners, int num_corner_segments);
UI_API void UI_DrawRectLines(UI_Rect rect, float thickness, UI_Color color);
UI_API void UI_DrawRectLinesRounded(UI_Rect rect, float thickness, float roundness, UI_Color color);
UI_API void UI_DrawRectLinesEx(UI_Rect rect, const UI_DrawRectCorners* corners, float thickness);
UI_API void UI_DrawTriangle(UI_Vec2 a, UI_Vec2 b, UI_Vec2 c, UI_Color color);
UI_API void UI_DrawQuad(UI_Vec2 a, UI_Vec2 b, UI_Vec2 c, UI_Vec2 d, UI_Color color);

UI_API void UI_DrawText(STR_View text, UI_Font font, UI_Vec2 pos, UI_AlignH align, UI_Color color, UI_ScissorRect scissor);

UI_API void UI_DrawSprite(UI_Rect rect, UI_Color color, UI_Rect uv_rect, UI_Texture* texture, UI_ScissorRect scissor);

UI_API void UI_DrawCircle(UI_Vec2 p, float radius, int segments, UI_Color color);

UI_API void UI_DrawConvexPolygon(const UI_Vec2* points, int points_count, UI_Color color);

UI_API void UI_DrawPoint(UI_Vec2 p, float thickness, UI_Color color);

UI_API void UI_DrawLine(UI_Vec2 a, UI_Vec2 b, float thickness, UI_Color color);
UI_API void UI_DrawLineEx(UI_Vec2 a, UI_Vec2 b, float thickness, UI_Color a_color, UI_Color b_color);
UI_API void UI_DrawPolyline(const UI_Vec2* points, const UI_Color* colors, int points_count, float thickness);
UI_API void UI_DrawPolylineLoop(const UI_Vec2* points, const UI_Color* colors, int points_count, float thickness);
UI_API void UI_DrawPolylineEx(const UI_Vec2* points, const UI_Color* colors, int points_count, float thickness, bool loop, float split_miter_threshold);

// -- fire_ui_color_pickers.c ------------------

UI_API void UI_AddColorPicker(UI_Box* box, float* hue, float* saturation, float* value, float* alpha);

UI_API void UI_AddHueSaturationCircle(UI_Box* box, float diameter, float* hue, float* saturation);

// -- fire_ui_extras.c -------------------------

typedef void (*UI_ArrayEditElemFn)(UI_Key key, void* elem, int index, void* user_data);

typedef struct UI_ValueEditArrayModify {
	bool append_to_end;
	bool clear;
	int remove_elem; // -1 if none
} UI_ValueEditArrayModify;

UI_API void UI_AddValArray(UI_Box* box, const char* name, void* array, int array_count, int elem_size, UI_ArrayEditElemFn edit_elem, void* user_data, UI_ValueEditArrayModify* out_modify);

UI_API void UI_AddLabelWrapped(UI_Box* box, UI_Size w, UI_Size h, UI_BoxFlags flags, STR_View string);

#define UI_AddValDSArray(BOX, NAME, ARRAY, DEFAULT_VALUE, EDIT_ELEM) \
	UI_AddValDSArray_((BOX), (NAME), (DS_DynArrayRaw*)(ARRAY), sizeof((ARRAY)->data[0]), (DEFAULT_VALUE), (UI_ArrayEditElemFn)EDIT_ELEM, NULL)
#define UI_AddValDSArrayEx(BOX, NAME, ARRAY, DEFAULT_VALUE, EDIT_ELEM, USER_DATA) \
	UI_AddValDSArray_((BOX), (NAME), (DS_DynArrayRaw*)(ARRAY), sizeof((ARRAY)->data[0]), (DEFAULT_VALUE), (UI_ArrayEditElemFn)EDIT_ELEM, USER_DATA)

// Formatting rules / expected types:
// 
// %hhd : int8_t
// %hd  : int16_t
// %d   : int32_t
// %lld : int64_t
// %hhu : uint8_t
// %hu  : uint16_t
// %u   : uint32_t
// (TODO) %llu : uint64_t
// %b   : bool
// %f   : float
// %lf  : double
// %t   : UI_Text, passed by pointer
// %%   is an escape that turns to %
// 
// By default, the values are read-only.    (TODO: implement !) You may add the ! specifier for editable values. In that case,
// the passed value must be a pointer to the value.
// Example:
//    
//    int foo = 50;
//    UI_AddFmt(UI_KEY(), "Editable value: %!d", &foo); // "foo" may be edited by the user
//    
UI_API void UI_AddFmt(UI_Box* box, const char* fmt, ...);
