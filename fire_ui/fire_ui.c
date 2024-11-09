#include "fire_ui.h"

// -- Global state ---------
UI_API UI_State UI_STATE;
// -------------------------

#define UI_TODO() __debugbreak()

UI_API bool UI_MarkGreaterThan(UI_Mark a, UI_Mark b) { return a.line > b.line || (a.line == b.line && a.col > b.col); }
UI_API bool UI_MarkLessThan(UI_Mark a, UI_Mark b) { return a.line < b.line || (a.line == b.line && a.col < b.col); }

UI_API void UI_AddButton(UI_Box* box, UI_Size w, UI_Size h, UI_BoxFlags flags, STR_View string) {
	UI_ProfEnter();
	flags |= UI_BoxFlag_Clickable | UI_BoxFlag_Selectable | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawTransparentBackground;
	UI_AddLabel(box, w, h, flags, string);
	UI_ProfExit();
}

static int UI_ColumnFromXOffset(float x, STR_View line, UI_Font font) {
	UI_ProfEnter();
	int column = 0;
	float start_x = 0.f;
	
	size_t i = 0;
	for (uint32_t r; r = STR_NextCodepoint(line, &i);) {
		float glyph_advance = UI_GlyphAdvance(r, font);
		float mid_x = start_x + 0.5f * glyph_advance;
		if (x >= mid_x) column++;
		start_x += glyph_advance;
	}
	UI_ProfExit();
	return column;
}

static float UI_XOffsetFromColumn(int col, STR_View line, UI_Font font) {
	UI_ProfEnter();
	float x = 0.f;

	size_t i = 0, i_next = 0;
	for (uint32_t r; r = STR_NextCodepoint(line, &i_next); i = i_next) {
		if (i == col) break;
		x += UI_GlyphAdvance(r, font);
		i++;
	}

	UI_ProfExit();
	return x;
}

static STR_View UI_GetLineString(int line, const UI_Text* text) {
	UI_ProfEnter();
	int lo = line == 0 ? 0 : DS_ArrGet(text->line_offsets, line - 1);
	int hi = line == text->line_offsets.count ? text->text.count : DS_ArrGet(text->line_offsets, line);
	STR_View result = { text->text.data, hi - lo };
	UI_ProfExit();
	return result;
}

static bool UI_MarkIsValid(UI_Mark mark, const UI_Text* text) {
	UI_ProfEnter();
	bool result = false;
	if (mark.line > 0 && mark.line < text->line_offsets.count) {
		STR_View line_str = UI_GetLineString(mark.line, text);
		result = mark.col >= 0 && mark.col <= STR_CodepointCount(line_str);
	}
	UI_ProfExit();
	return result;
}

static int UI_MarkToByteOffset(UI_Mark mark, const UI_Text* text) {
	UI_ProfEnter();
	int line_start = mark.line > 0 ? DS_ArrGet(text->line_offsets, mark.line - 1) : 0;
	STR_View after = STR_SliceAfter(UI_TextToStr(*text), line_start);

	int col = 0;
	int result = text->text.count;
	
	size_t i = 0, i_next = 0;
	for (uint32_t r; r = STR_NextCodepoint(after, &i_next); i = i_next) {
		if (col == mark.col) {
			result = line_start + (int)i;
			break;
		}
		col++;
	}

	UI_ProfExit();
	return result;
}

static UI_Vec2 UI_XYOffsetFromMark(const UI_Text* text, UI_Mark mark, UI_Font font) {
	UI_ProfEnter();
	float x = UI_XOffsetFromColumn(mark.col, UI_GetLineString(mark.line, text), font);
	UI_Vec2 result = { x, (float)font.size * mark.line };
	UI_ProfExit();
	return result;
}

static void UI_DrawTextRangeHighlight(UI_Mark min, UI_Mark max, UI_Vec2 text_origin, STR_View text, UI_Font font, UI_Color color, UI_ScissorRect scissor) {
	UI_ProfEnter();
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
			UI_ASSERT(false);
		}

		rect.min.x -= 1.f;
		rect.max.x += 1.f;
		rect.max.y += font.size;
		
		UI_Rect _uv_rect;
		if (!UI_ClipRectEx(&rect, &_uv_rect, scissor)) {
			UI_DrawRect(rect, color);
		}
	}
	UI_ProfExit();
}

UI_API UI_Key UI_HashKey(UI_Key a, UI_Key b) {
	UI_ProfEnter();
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
	UI_ProfExit();
	return h;
}

UI_API UI_Key UI_HashPtr(UI_Key a, void* b) {
	return UI_HashKey(a, (UI_Key)b);
}

UI_API UI_Key UI_HashInt(UI_Key a, int b) {
	return UI_HashKey(a, (UI_Key)b);
}

UI_API void UI_SelectionFixOrder(UI_Selection* sel) {
	UI_ProfEnter();
	if (UI_MarkGreaterThan(sel->_[0], sel->_[1])) {
		UI_Mark tmp = sel->_[1];
		sel->_[1] = sel->_[0];
		sel->_[0] = tmp;
		sel->cursor = 1 - sel->cursor;
	}
	UI_ProfExit();
}

UI_API void UI_RectPad(UI_Rect* rect, float pad) {
	rect->min.x += pad;
	rect->min.y += pad;
	rect->max.x -= pad;
	rect->max.y -= pad;
}

static void UI_MoveMarkByWord(UI_Mark* mark, const UI_Text* text, int dir) {
	UI_ProfEnter();
	size_t byteoffset = UI_MarkToByteOffset(*mark, text);

	bool was_whitespace = false;
	bool was_alnum = false;
	int i = 0;
	uint32_t r;

	uint32_t (*next_codepoint_fn)(STR_View, size_t*) = dir > 0 ? STR_NextCodepoint : STR_PrevCodepoint;

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
	UI_ProfExit();
}

static void UI_MoveMarkH(UI_Mark* mark, const UI_Text* text, int dir, bool ctrl) {
	UI_ProfEnter();
	if (ctrl) {
		UI_MoveMarkByWord(mark, text, dir);
	}
	else if (mark->col + dir < 0) {
		if (mark->line > 0) {
			mark->line -= 1;
			mark->col = (int)STR_CodepointCount(UI_GetLineString(mark->line, text));
		}
	}
	else {
		int end_col = (int)STR_CodepointCount(UI_GetLineString(mark->line, text));

		if (mark->col + dir > end_col) {
			if (mark->line < text->line_offsets.count) {
				mark->line += 1;
				mark->col = 0;
			}
		}
		else {
			mark->col += dir;
		}
	}
	UI_ProfExit();
}

static void UI_EditTextArrowKeyInputX(int dir, const UI_Text* text, UI_Selection* selection, UI_Font font) {
	UI_ProfEnter();

	bool shift = UI_InputIsDown(UI_Input_Shift);
	if (!shift && !UI_MarkEquals(selection->_[0], selection->_[1])) {
		if (dir > 0) {
			selection->_[0] = selection->_[1];
		}
		else {
			selection->_[1] = selection->_[0];
		}
	}
	else {
		UI_Mark* cursor = &selection->_[selection->cursor];
		UI_MoveMarkH(cursor, text, dir, UI_InputIsDown(UI_Input_Control));

		selection->cursor_x = UI_XYOffsetFromMark(text, *cursor, font).x;

		if (!shift) {
			selection->_[1 - selection->cursor] = *cursor;
		}

		UI_SelectionFixOrder(selection);
	}

	UI_ProfExit();
}

UI_API UI_Selection UI_TextReplaceRange(UI_Text* text, UI_Mark from, UI_Mark to, STR_View replace_with) {
	UI_ProfEnter();

	UI_Selection result = {0};
	result._[0] = from;
	result._[1] = from;
	result.cursor = 1;
	
	// I think this function could be optimized by combining the erase and insert steps

	{ // First erase selected range
		int start_byteoffset = UI_MarkToByteOffset(from, text);
		int end_byteoffset = UI_MarkToByteOffset(to, text);

		int remove_n = end_byteoffset - start_byteoffset;
		DS_ArrRemoveN(&text->text, start_byteoffset, remove_n);

		DS_ArrRemoveN(&text->line_offsets, from.line, to.line - from.line);

		DS_ForArrEach(int, &text->line_offsets, it) {
			*it.ptr -= remove_n;
		}
	}

	{ // Then insert the text
		STR_View insertion = replace_with;
		UI_Mark mark = from;
		int byteoffset = UI_MarkToByteOffset(mark, text);

		DS_ArrInsertN(&text->text, byteoffset, insertion.data, (int)insertion.size);

		int lines_count = 0;
		for (STR_View remaining = insertion;;) {
			STR_View line_str = STR_ParseUntilAndSkip(&remaining, '\n');
			lines_count++;
			if (remaining.size == 0) break;
		}

		if (lines_count > 1) UI_TODO();
		mark.col += (int)STR_CodepointCount(insertion);
		result._[1] = mark;
	}

	UI_ProfExit();
	return result;
}

UI_API void UI_EditTextSelectAll(const UI_Text* text, UI_Selection* selection) {
	UI_ProfEnter();
	selection->_[0] = UI_MARK{0};
	selection->_[1] = UI_MARK{
		text->line_offsets.count,
		(int)STR_CodepointCount(UI_GetLineString(text->line_offsets.count, text)),
	};
	selection->cursor = 1;
	UI_ProfExit();
}

UI_API UI_ValNumericState* UI_AddValNumeric(UI_Box* box, UI_Size w, UI_Size h, void* value_64_bit, bool is_signed, bool is_float) {
	UI_ProfEnter();
	
	UI_ValNumericState data = {0};
	UI_Key data_key = UI_KEY();
	if (box->prev_frame) UI_BoxGetVar(box->prev_frame, data_key, &data);

	STR_View value_str;
	if (is_float) {
		value_str = STR_FloatToStr(UI_FrameArena(), *(double*)value_64_bit, 1);
	} else {
		value_str = STR_IntToStrEx(UI_FrameArena(), *(uint64_t*)value_64_bit, is_signed, 10);
	}

	bool should_enable_text_edit = UI_STATE.selected_box_new == box->key && UI_STATE.selected_box != box->key;

	if (data.is_dragging && !UI_InputIsDown(UI_Input_MouseLeft)) {
		data.is_dragging = false;
		if (UI_Abs(UI_STATE.mouse_travel_distance_after_press.x) <= 2.f) {
			should_enable_text_edit = true;
		}
	}

	if (should_enable_text_edit) {
		data.text = STR_Clone(UI_FrameArena(), value_str);
		data.is_editing_text = true;
	}
	
	if (data.is_editing_text) {
		UI_Text new_text;
		UI_TextInit(UI_FrameArena(), &new_text, data.text);
		
		UI_AddValText(box, w, h, &new_text);

		data.text = UI_TextToStr(new_text);

		if (is_float) {
			STR_ParseFloat(data.text, (double*)value_64_bit);
		} else if (is_signed) {
			STR_ParseI64(data.text, (int64_t*)value_64_bit);
		} else {
			STR_ParseU64Ex(data.text, 10, (uint64_t*)value_64_bit);
		}

		if (!should_enable_text_edit) {
			if (UI_STATE.selected_box != box->key) {
				data.is_editing_text = false;
			}
			if (UI_InputWasPressed(UI_Input_MouseLeft) && !UI_Pressed(box)) {
				data.is_editing_text = false;
			}
			if (UI_InputWasPressed(UI_Input_Enter) || UI_InputWasPressed(UI_Input_Escape)) {
				data.is_editing_text = false;
			}
		}
	}
	else {
		UI_AddLabel(box, w, h,
			UI_BoxFlag_Clickable | UI_BoxFlag_Selectable | UI_BoxFlag_DrawBorder | UI_BoxFlag_PressingStaysWithoutHover, value_str);

		if (UI_Pressed(box)) {
			UI_STATE.outputs.lock_and_hide_cursor = true;

			if (is_float) {
				data.value_before_press = *(double*)value_64_bit;
			} else if (is_signed) {
				data.value_before_press = (double)*(int64_t*)value_64_bit;
			} else {
				data.value_before_press = (double)*(uint64_t*)value_64_bit;
			}

			data.is_dragging = true;
		}

		if (UI_IsHovered(box)) {
			UI_STATE.outputs.cursor = UI_MouseCursor_ResizeH;
		}

		if (data.is_dragging) {
			UI_STATE.outputs.lock_and_hide_cursor = true;

			double new_value = data.value_before_press + UI_STATE.mouse_travel_distance_after_press.x * 0.05f;

			if (is_float) {
				*(double*)value_64_bit = new_value;
			} else if (is_signed) {
				*(int64_t*)value_64_bit = (int64_t)new_value;
			} else {
				*(uint64_t*)value_64_bit = (uint64_t)new_value;
			}
		}
	}
	
	UI_ValNumericState* new_data;
	UI_BoxGetRetainedVar(box, data_key, &new_data);
	*new_data = data;

	UI_ProfExit();
	return new_data;
}

UI_API UI_ValNumericState* UI_AddValInt(UI_Box* box, UI_Size w, UI_Size h, int32_t* value) {
	int64_t value_i64 = *value;
	UI_ValNumericState* data = UI_AddValNumeric(box, w, h, &value_i64, true, false);
	*value = (int32_t)value_i64;
	return data;
}

UI_API UI_ValNumericState* UI_AddValInt64(UI_Box* box, UI_Size w, UI_Size h, int64_t* value) {
	return UI_AddValNumeric(box, w, h, value, true, false);
}

UI_API UI_ValNumericState* UI_AddValUInt64(UI_Box* box, UI_Size w, UI_Size h, uint64_t* value) {
	return UI_AddValNumeric(box, w, h, value, false, false);
}

UI_API UI_ValNumericState* UI_AddValFloat(UI_Box* box, UI_Size w, UI_Size h, float* value) {
	double value_f64 = (double)*value;
	UI_ValNumericState* data = UI_AddValNumeric(box, w, h, &value_f64, false, true);
	*value = (float)value_f64;
	return data;
}

UI_API UI_ValNumericState* UI_AddValFloat64(UI_Box* box, UI_Size w, UI_Size h, double* value) {
	return UI_AddValNumeric(box, w, h, value, false, true);
}

UI_API void UI_TextInit(DS_Allocator* allocator, UI_Text* text, STR_View initial_value) {
	UI_ProfEnter();
	memset(text, 0, sizeof(*text));
	DS_ArrInit(&text->text, allocator);
	DS_ArrInit(&text->line_offsets, allocator);
	UI_TextSet(text, initial_value);
	UI_ProfExit();
}

UI_API void UI_TextDeinit(UI_Text* text) {
	UI_ProfEnter();
	DS_ArrDeinit(&text->line_offsets);
	DS_ArrDeinit(&text->text);
	UI_ProfExit();
}

UI_API void UI_TextSet(UI_Text* text, STR_View value) {
	UI_ProfEnter();
	DS_ArrClear(&text->text);
	DS_ArrPushN(&text->text, value.data, (int)value.size);
	UI_ProfExit();
}

static UI_Key UI_AddValTextDataKey() { return UI_KEY(); }

static void UI_DrawValTextInnerBox(UI_Box* box) {
	UI_DrawBoxDefault(box);

	UI_ValTextState* state;
	UI_BoxGetRetainedVar(box->parent, UI_AddValTextDataKey(), &state);

	if (state->is_editing) {
		UI_Selection sel = state->selection;
		UI_DrawTextRangeHighlight(sel._[0], sel._[1], UI_AddV2(box->computed_position, box->inner_padding), box->text, box->font, UI_COLOR{255, 255, 255, 50}, &box->computed_rect);
	
		UI_Mark cursor = sel._[sel.cursor];
		UI_DrawTextRangeHighlight(cursor, cursor, UI_AddV2(box->computed_position, box->inner_padding), box->text, box->font, UI_COLOR{255, 255, 255, 255}, &box->computed_rect);
	}
}

UI_API UI_ValTextState* UI_AddValText(UI_Box* box, UI_Size w, UI_Size h, UI_Text* text) {
	UI_ProfEnter();

	UI_Font font = UI_STATE.default_font;

	UI_AddBox(box, w, h, UI_BoxFlag_Selectable | UI_BoxFlag_DrawBorder | UI_BoxFlag_Clickable);
	UI_PushBox(box);

	UI_Box* inner = UI_BBOX(box);
	UI_AddLabel(inner, UI_SizeFit(), UI_SizeFit(), 0, STR_V(""));
	inner->draw = UI_DrawValTextInnerBox;

	UI_ValTextState* state;
	bool had_text_edit_state = UI_BoxGetRetainedVar(box, UI_AddValTextDataKey(), &state);

	bool pressed_this = UI_Pressed(box);

	if (state->is_editing) {
		if (UI_InputWasPressed(UI_Input_MouseLeft) && !pressed_this) state->is_editing = false;
		if (UI_InputWasPressed(UI_Input_Enter))   state->is_editing = false;
		if (UI_InputWasPressed(UI_Input_Escape))  state->is_editing = false;
		if (UI_STATE.selected_box_new != box->key)     state->is_editing = false;
	}
	else {
		bool became_selected =
			(UI_STATE.selected_box_new == box->key && UI_STATE.selected_box != box->key) ||
			(UI_STATE.selected_box_new == box->key && !had_text_edit_state);

		if (pressed_this || became_selected) {
			state->is_editing = true;
			UI_EditTextSelectAll(text, &state->selection);
		}
	}

	if (state->is_editing) {
		UI_Selection* selection = &state->selection;

		if (UI_InputWasPressedOrRepeated(UI_Input_A) && UI_InputIsDown(UI_Input_Control)) {
			UI_EditTextSelectAll(text, selection);
		}

		if (UI_InputWasPressedOrRepeated(UI_Input_Right)) {
			UI_EditTextArrowKeyInputX(1, text, selection, font);
		}

		if (UI_InputWasPressedOrRepeated(UI_Input_Left)) {
			UI_EditTextArrowKeyInputX(-1, text, selection, font);
		}

		bool pressed_with_mouse = UI_InputWasPressed(UI_Input_MouseLeft) && UI_IsHovered(box);
		if (pressed_with_mouse || UI_STATE.mouse_clicking_down_box == box->key) {
			if (inner->prev_frame) {
				float origin = inner->prev_frame->computed_position.x + inner->prev_frame->inner_padding.x;
			
				int col = UI_ColumnFromXOffset(UI_STATE.mouse_pos.x - origin, UI_GetLineString(0, text), font);
				selection->_[selection->cursor].col = col;
				if (pressed_this) selection->_[1 - selection->cursor].col = col;
				UI_SelectionFixOrder(selection);
			}
		}
		
		if (UI_STATE.inputs.text_input_utf32_length > 0) {
			STR_Builder text_input = { UI_FrameArena() };
			for (int i = 0; i < UI_STATE.inputs.text_input_utf32_length; i++) {
				STR_PrintU(&text_input, UI_STATE.inputs.text_input_utf32[i]);
			}
			UI_Selection inserted_sel = UI_TextReplaceRange(text, selection->_[0], selection->_[1], text_input.str);
			selection->_[0] = inserted_sel._[1];
			selection->_[1] = inserted_sel._[1];
		}

		//if (UI_InputWasPressedOrRepeated(UI_Input_Home)) {
		//	UI_Mark* end = &selection->_[selection->end];
		//	end->col = 0;
		//	if (!UI_InputIsDown(UI_Input_Shift)) {
		//		selection->_[1 - selection->end] = *end;
		//	}
		//	UI_SelectionFixOrder(selection);
		//}

		//if (UI_InputWasPressedOrRepeated(UI_Input_End)) {
		//	UI_TODO(); // UI_Mark *end = &selection->_[selection->end];
		//	// end->col = StrRuneCount(UI_GetLineString(end->line, text));
		//	// if (!UI_InputIsDown(UI_Input_Shift)) {
		//	// 	selection->_[1 - selection->end] = *end;
		//	// }
		//	// UI_SelectionFixOrder(selection);
		//}

		// Ctrl X
		if (UI_InputWasPressedOrRepeated(UI_Input_X) && UI_InputIsDown(UI_Input_Control)) {
			if (UI_STATE.inputs.set_clipboard_string_fn) {
				int min = UI_MarkToByteOffset(selection->_[0], text);
				int max = UI_MarkToByteOffset(selection->_[1], text);
				UI_STATE.inputs.set_clipboard_string_fn(STR_Slice(UI_TextToStr(*text), min, max), UI_STATE.inputs.user_data);
			}
			*selection = UI_TextReplaceRange(text, selection->_[0], selection->_[1], STR_V(""));
		}
		
		// Ctrl C
		if ((UI_InputWasPressedOrRepeated(UI_Input_C) && UI_InputIsDown(UI_Input_Control))) {
			if (UI_STATE.inputs.set_clipboard_string_fn) {
				int min = UI_MarkToByteOffset(selection->_[0], text);
				int max = UI_MarkToByteOffset(selection->_[1], text);
				UI_STATE.inputs.set_clipboard_string_fn(STR_Slice(UI_TextToStr(*text), min, max), UI_STATE.inputs.user_data);
			}
		}

		// Ctrl V
		if (UI_InputWasPressedOrRepeated(UI_Input_V) && UI_InputIsDown(UI_Input_Control)) {
			if (UI_STATE.inputs.get_clipboard_string_fn) {
				STR_View str = UI_STATE.inputs.get_clipboard_string_fn(UI_STATE.inputs.user_data);
				*selection = UI_TextReplaceRange(text, selection->_[0], selection->_[1], str);
			}
		}

		if (UI_InputWasPressedOrRepeated(UI_Input_Backspace)) {
			if (UI_MarkEquals(selection->_[0], selection->_[1])) {
				UI_MoveMarkH(&selection->_[0], text, -1, UI_InputIsDown(UI_Input_Control));
			}
			*selection = UI_TextReplaceRange(text, selection->_[0], selection->_[1], STR_V(""));
		}
	}

	UI_PopBox(box);

	inner->text = STR_Clone(&UI_STATE.frame_arena, UI_TextToStr(*text)); // set text after possibile modifications

	if (UI_IsHovered(box)) {
		UI_STATE.outputs.cursor = UI_MouseCursor_I_beam;
	}

	UI_ProfExit();

	return state;
}

UI_API void UI_AddCheckbox(UI_Box* box, bool* value) {
	UI_ProfEnter();

	__debugbreak();
	/*
	float h = UI_STATE.default_font.size + 2.f * UI_DEFAULT_TEXT_PADDING.y;

	UI_AddBox(box, h, h, 0);
	box->inner_padding = UI_VEC2{ 5.f, 5.f };
	UI_PushBox(box);

	UI_BoxFlags inner_flags = UI_BoxFlag_Clickable | UI_BoxFlag_Selectable | UI_BoxFlag_DrawBorder;
	UI_Box* inner = UI_BBOX(box);
	if (*value) {
		UI_AddLabel(inner, UI_SizeFlex(1.f), UI_SizeFlex(1.f), inner_flags, STR_V("A"));
	}
	else {
		UI_AddBox(inner, UI_SizeFlex(1.f), UI_SizeFlex(1.f), inner_flags);
	}
	inner->font = UI_STATE.icons_font;
	inner->inner_padding = UI_VEC2{ 5.f, 2.f };
	
	UI_PopBox(box);

	if (UI_Pressed(inner)) *value = !*value;
	*/
	UI_ProfExit();
}

UI_API void UI_AddLabel(UI_Box* box, UI_Size w, UI_Size h, UI_BoxFlags flags, STR_View string) {
	UI_ProfEnter();
	UI_AddBox(box, w, h, flags | UI_BoxFlag_HasText);
	box->text = STR_Clone(&UI_STATE.frame_arena, string);
	box->inner_padding = UI_DEFAULT_TEXT_PADDING;
	UI_ProfExit();
}

/*UI_API bool UI_PushCollapsing(UI_Box* box, UI_Size w, UI_Size h, UI_Size indent, UI_BoxFlags flags, STR_View text, UI_Font icons_font) {
	UI_ProfEnter();
	
	UI_Box* header = UI_BBOX(box);
	UI_BoxFlags box_flags = UI_BoxFlag_Horizontal | UI_BoxFlag_Clickable | UI_BoxFlag_Selectable | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawTransparentBackground;
	UI_AddBox(header, UI_SizeFlex(1.f), h, box_flags);

	bool is_open = box->prev_frame != NULL && box->prev_frame->parent != NULL;
	if (UI_Pressed(header)) {
		is_open = !is_open;
	}

	UI_PushBox(header);
	UI_Box* icon = UI_BBOX(box);
	UI_AddLabel(icon, 20.f, h, 0, is_open ? STR_V("\x44") : STR_V("\x46"));
	icon->font = UI_STATE.icons_font;
	UI_AddLabel(UI_BBOX(box), w, h, 0, text);
	UI_PopBox(header);

	if (is_open) {
		UI_AddBox(box, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Horizontal);
		UI_PushBox(box);
		UI_AddBox(UI_BBOX(box), indent, UI_SizeFit(), 0);
		
		UI_Box* inner_child_box = UI_BBOX(box);
		UI_AddBox(inner_child_box, UI_SizeFlex(1.f), UI_SizeFit(), flags);
		UI_PushBox(inner_child_box);
	}
	UI_ProfExit();
	return is_open;
}

UI_API void UI_PopCollapsing(UI_Box* box) {
	UI_ProfEnter();
	UI_PopBox(box->last_child);
	UI_PopBox(box);
	UI_ProfExit();
}*/

UI_API void UI_PushScrollArea(UI_Box* box, UI_Size w, UI_Size h, UI_BoxFlags flags, int anchor_x, int anchor_y) {
	UI_ProfEnter();

	UI_Box* content = UI_BBOX(box);
	UI_Box* temp_boxes[2] = { UI_BBOX(box), UI_BBOX(box) };
	int anchors[] = { anchor_x, anchor_y };

	UI_Box* content_prev_frame = content->prev_frame; // may be NULL
	UI_Box* deepest_temp_box_prev_frame = temp_boxes[0]->prev_frame; // may return NULL

	UI_AddBox(box, w, h, UI_BoxFlag_Horizontal | flags);
	UI_PushBox(box);

	UI_Vec2 offset = { 0.f, 0.f };

	// TODO: turn this into "PushScrollArea2D" and have a separate function for PushScrollArea. The main difference is that with
	// a 1D scroll area, it should be possible to do, say, flex child boxes in X while scrolling in Y. Or maybe that should be
	// possible with 2D scroll area too, idk.

	// We should automatically add either X or Y scrollbars
	for (int y = 1; y >= 0; y--) { // y is the direction of the scroll bar
		int x = 1 - y;
		UI_Key y_key = UI_HashInt(UI_BKEY(box), y);

		UI_Size size[2];
		size[x] = UI_SizeFlex(1.f);
		size[y] = UI_SizeFlex(1.f);
		UI_Box* temp_box = temp_boxes[y];
		UI_AddBox(temp_box, size[0], size[1], 0);
		
		// Use the content size from the previous frame
		float content_length_px = content_prev_frame ? content_prev_frame->computed_unexpanded_size._[y] : 0.f;
		
		float visible_area_length_px = deepest_temp_box_prev_frame ? deepest_temp_box_prev_frame->computed_expanded_size._[y] : 0.f;
		float rail_line_length_px = temp_box->prev_frame ? temp_box->prev_frame->computed_expanded_size._[y] : 0.f;
		
		if (content_length_px > visible_area_length_px) {
			size[x] = 18.f;
			size[y] = UI_SizeFlex(1.f);

			UI_BoxFlags layout_dir_flag = y == UI_Axis_X ? UI_BoxFlag_Horizontal : 0;
			
			UI_Box* rail_line_box = UI_KBOX(y_key);
			UI_AddBox(rail_line_box, size[0], size[1], layout_dir_flag | UI_BoxFlag_DrawBorder);
			if (anchors[y] == 1) {
				rail_line_box->flags |= (y == 1 ? UI_BoxFlag_ReverseLayoutY : UI_BoxFlag_ReverseLayoutX);
			}

			UI_PushBox(rail_line_box);

			// TODO: we should pass a key parameter to attach the storage of the scroll wheel offset to.
			// That way e.g. if we have a collapsible and a scroll bar inside, the scroll bar offset wouldn't be lost on close/reopen.

			offset._[y] = content_prev_frame ? content_prev_frame->offset._[y] : 0.f;

			float scrollbar_distance_ratio = (anchors[y] == 1 ? 1.f : -1.f) * (offset._[y] / content_length_px);
			float scrollbar_length_ratio = visible_area_length_px / content_length_px; // between 0 and 1

			size[x] = 18.f;
			size[y] = scrollbar_distance_ratio * rail_line_length_px;
			UI_Box* pad_before_bar = UI_KBOX(y_key);
			UI_AddBox(pad_before_bar, size[0], size[1], UI_BoxFlag_PressingStaysWithoutHover);

			size[x] = 18.f;
			size[y] = rail_line_length_px * scrollbar_length_ratio;
			UI_Box* scrollbar = UI_KBOX(y_key);
			UI_AddBox(scrollbar, size[0], size[1],
				UI_BoxFlag_PressingStaysWithoutHover | UI_BoxFlag_Clickable | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawTransparentBackground);

			size[x] = 18.f;
			size[y] = UI_SizeFlex(1.f);
			UI_Box* pad_after_bar = UI_KBOX(y_key);
			UI_AddBox(pad_after_bar, size[0], size[1], UI_BoxFlag_PressingStaysWithoutHover);

			UI_PopBox(rail_line_box);

			// scrollbar inputs

			if (UI_Pressed(scrollbar)) {
				UI_STATE.scrollbar_origin_before_press = offset._[y];
			}

			if (UI_IsClickingDown(scrollbar)) {
				float mouse_delta = UI_STATE.mouse_pos._[y] - UI_STATE.last_released_mouse_pos._[y];
				float ratio_of_scrollbar_moved = mouse_delta / rail_line_length_px;
				offset._[y] = UI_STATE.scrollbar_origin_before_press - ratio_of_scrollbar_moved * content_length_px;
			}

			if (UI_IsClickingDown(pad_before_bar) || UI_IsClickingDown(pad_after_bar)) {
				float origin = box->prev_frame ? box->prev_frame->computed_position._[y] : 0.f;
				float scroll_ratio = (UI_STATE.mouse_pos._[y] - origin) / rail_line_length_px - 0.5f * scrollbar_length_ratio;

				offset._[y] = -scroll_ratio * content_length_px;
				if (anchors[y] == 1) {
					offset._[y] += content_length_px - visible_area_length_px;
				}
			}

			// idea: hold middle click and move mouse could scroll in X and Y

			if (UI_IsHovered(box) && y == UI_Axis_Y) {
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

	UI_AddBox(content, UI_SizeFlex(1.f), UI_SizeFlex(1.f), UI_BoxFlag_NoFlexDownX|UI_BoxFlag_NoFlexDownY);
	content->offset = offset;
	UI_PushBox(content);

	if (anchor_x == 1) temp_boxes[0]->flags |= UI_BoxFlag_ReverseLayoutX;
	if (anchor_y == 1) temp_boxes[0]->flags |= UI_BoxFlag_ReverseLayoutY;

	UI_ProfExit();
}

UI_API void UI_PopScrollArea(UI_Box* box) {
	UI_ProfEnter();
	UI_PopBoxN(box, 4);
	UI_ProfExit();
}

UI_API bool UI_ClipRect(UI_Rect* rect, const UI_Rect* scissor) {
	UI_Rect _;
	return UI_ClipRectEx(rect, &_, scissor);
}

// returns "true" if the rect is fully clipped, "false" if there is still some area left
UI_API bool UI_ClipRectEx(UI_Rect* rect, UI_Rect* uv_rect, const UI_Rect* scissor) {
	UI_ProfEnter();
	bool fully_clipped =
		rect->max.x < scissor->min.x ||
		rect->min.x > scissor->max.x ||
		rect->max.y < scissor->min.y ||
		rect->min.y > scissor->max.y;
	if (!fully_clipped) {
		float rect_w = rect->max.x - rect->min.x;
		float rect_h = rect->max.y - rect->min.y;
		float uv_rect_w = uv_rect->max.x - uv_rect->min.x;
		float uv_rect_h = uv_rect->max.y - uv_rect->min.y;

		float offset_min_x = scissor->min.x - rect->min.x;
		float offset_max_x = scissor->max.x - rect->max.x;
		float offset_min_y = scissor->min.y - rect->min.y;
		float offset_max_y = scissor->max.y - rect->max.y;

		if (offset_min_x > 0) {
			rect->min.x = scissor->min.x;
			uv_rect->min.x += offset_min_x * (uv_rect_w / rect_w);
		}
		if (offset_max_x < 0) {
			rect->max.x = scissor->max.x;
			uv_rect->max.x += offset_max_x * (uv_rect_w / rect_w);
		}
		if (offset_min_y > 0) {
			rect->min.y = scissor->min.y;
			uv_rect->min.y += offset_min_y * (uv_rect_h / rect_h);
		}
		if (offset_max_y < 0) {
			rect->max.y = scissor->max.y;
			uv_rect->max.y += offset_max_y * (uv_rect_h / rect_h);
		}
	}
	UI_ProfExit();
	return fully_clipped;
}

UI_API void UI_DrawBox(UI_Box* box) {
	UI_ProfEnter();
	box->draw(box);
	UI_ProfExit();
}

static UI_DrawBoxDefaultArgs* UI_DrawBoxDefaultArgsConstDefaults() {
	static const UI_DrawBoxDefaultArgs args = {
		{ 250, 255, 255, 255 }, // text_color
		{ 255, 255, 255, 50 },  // transparent_bg_color
		{ 50, 50, 50, 255 },    // opaque_bg_color
		{ 0, 0, 0, 128 },       // border_color
	};
	return (UI_DrawBoxDefaultArgs*)&args;
}

UI_API UI_DrawBoxDefaultArgs* UI_DrawBoxDefaultArgsInit() {
	return DS_Clone(UI_DrawBoxDefaultArgs, &UI_STATE.frame_arena, *UI_DrawBoxDefaultArgsConstDefaults());
}

UI_API void UI_DrawBoxDefault(UI_Box* box) {
	UI_ProfEnter();

	const UI_DrawBoxDefaultArgs* args = box->draw_args;

	if (box->flags & UI_BoxFlag_DrawOpaqueBackground) {
		const float shadow_distance = 10.f;
		UI_Rect rect = box->computed_rect;
		rect.min = UI_SubV2(rect.min, UI_VEC2{ 0.5f * shadow_distance, 0.5f * shadow_distance });
		rect.max = UI_AddV2(rect.max, UI_VEC2{ shadow_distance, shadow_distance });
		UI_DrawRectRounded2(rect, 2.f * shadow_distance, UI_COLOR{ 0, 0, 0, 50 }, UI_COLOR{ 0, 0, 0, 0 }, 2);
	}

	UI_Rect box_rect = box->computed_rect;

	if (box->flags & UI_BoxFlag_DrawTransparentBackground) {
		UI_DrawRectRounded(box_rect, 4.f, args->transparent_bg_color, 2);
	}

	if (box->flags & UI_BoxFlag_DrawOpaqueBackground) {
		UI_DrawRectRounded(box_rect, 4.f, args->opaque_bg_color, 2);
	}

	if (box->flags & UI_BoxFlag_DrawBorder) {
		UI_DrawRectLinesRounded(box_rect, 2.f, 4.f, args->border_color);
	}

	if (box->flags & UI_BoxFlag_Clickable) {
		typedef struct {
			float lazy_is_hovered;
			float lazy_is_holding_down;
		} AnimState;

		AnimState* anim_state;
		UI_BoxGetRetainedVar(box, UI_KEY(), &anim_state);

		float is_clicking = (float)UI_IsClickingDownAndHovered(box);
		anim_state->lazy_is_hovered = (float)UI_IsHoveredIdle(box);
		anim_state->lazy_is_holding_down = is_clicking > anim_state->lazy_is_holding_down ?
			is_clicking : UI_Lerp(anim_state->lazy_is_holding_down, is_clicking, 0.2f);

		float r = 4.f;
		{
			float hovered = anim_state->lazy_is_hovered * (1.f - anim_state->lazy_is_holding_down); // We don't want to show the hover highlight when holding down
			const UI_Color top = UI_COLOR{ 255, 255, 255, (uint8_t)(hovered * 20.f) };
			const UI_Color bot = UI_COLOR{ 255, 255, 255, (uint8_t)(hovered * 10.f) };
			UI_DrawRectCorners corners = {
				{top, top, bot, bot},
				{top, top, bot, bot},
				{r, r, r, r} };
			UI_DrawRectEx(box_rect, &corners, 2);
		}
		{
			const UI_Color top = UI_COLOR{ 0, 0, 0, (uint8_t)(anim_state->lazy_is_holding_down * 100.f) };
			const UI_Color bot = UI_COLOR{ 0, 0, 0, (uint8_t)(anim_state->lazy_is_holding_down * 20.f) };
			UI_DrawRectCorners corners = {
				{top, top, bot, bot},
				{top, top, bot, bot},
				{r, r, r, r} };
			UI_DrawRectEx(box_rect, &corners, 2);
		}
	}

	if (UI_IsSelected(box) && UI_STATE.selection_is_visible) {
		//UI_Rect box_rect = box->computed_rect;
		UI_Color selection_color = UI_COLOR{ 250, 200, 85, 240 };
		UI_DrawRectLinesRounded(box_rect, 2.f, 4.f, selection_color);
	}

	for (UI_Box* child = box->first_child; child; child = child->next) {
		UI_DrawBox(child);
	}

	if (box->flags & UI_BoxFlag_HasText) {
		UI_Vec2 text_pos = UI_AddV2(box->computed_position, box->inner_padding);
		UI_DrawText(box->text, box->font, text_pos, UI_AlignH_Left, args->text_color, &box_rect);
	}

	UI_ProfExit();
}

UI_API bool UI_Pressed(UI_Box* box) {
	return UI_PressedEx(box, UI_Input_MouseLeft);
}

UI_API bool UI_PressedEx(UI_Box* box, UI_Input mouse_button) {
	bool pressed = UI_InputWasPressed(mouse_button) && UI_IsHovered(box);
	pressed = pressed || (UI_STATE.selected_box == box->key && UI_STATE.selection_is_visible && UI_InputWasPressed(UI_Input_Enter));
	return pressed;
}

UI_API bool UI_PressedIdleEx(UI_Box* box, UI_Input mouse_button) {
	bool pressed = UI_InputWasPressed(mouse_button) && UI_IsHoveredIdle(box);
	pressed = pressed || (UI_STATE.selected_box == box->key && UI_STATE.selection_is_visible && UI_InputWasPressed(UI_Input_Enter));
	return pressed;
}

UI_API bool UI_PressedIdle(UI_Box* box) {
	return UI_PressedIdleEx(box, UI_Input_MouseLeft);
}

UI_API bool UI_Clicked(UI_Box* box) {
	bool clicked = UI_IsClickingDownAndHovered(box) &&
		(UI_InputWasReleased(UI_Input_MouseLeft) || (UI_STATE.selection_is_visible && UI_InputWasReleased(UI_Input_Enter)));
	return clicked;
}

UI_API bool UI_DoubleClicked(UI_Box* box) {
	bool result = UI_Pressed(box) && UI_DoubleClickedAnywhere();
	return result;
}

UI_API bool UI_DoubleClickedIdle(UI_Box* box) {
	bool result = UI_PressedIdle(box) && UI_DoubleClickedAnywhere();
	return result;
}

UI_API bool UI_IsSelected(UI_Box* box) {
	return UI_STATE.selected_box == box->key;
}

UI_API bool UI_IsClickingDown(UI_Box* box) {
	return UI_STATE.mouse_clicking_down_box == box->key || UI_STATE.keyboard_clicking_down_box == box->key;
}

UI_API bool UI_IsClickingDownAndHovered(UI_Box* box) {
	return (UI_STATE.keyboard_clicking_down_box == box->key && UI_STATE.selected_box == box->key) ||
		(UI_STATE.mouse_clicking_down_box == box->key && UI_IsHovered(box));
}

UI_API bool UI_IsHovered(UI_Box* box) {
	bool result = box->prev_frame && !(box->prev_frame->flags & UI_BoxFlag_NoHover) && UI_PointIsInRect(box->prev_frame->computed_rect, UI_STATE.mouse_pos);
	return result;
}

UI_API bool UI_IsMouseInsideOf(UI_Box* box) {
	bool result = box->prev_frame && UI_PointIsInRect(box->prev_frame->computed_rect, UI_STATE.mouse_pos);
	return result;
}

static bool UI_HasAnyHoveredClickableChild_(UI_Box* box) {
	UI_ProfEnter();
	bool result = false;
	for (UI_Box* child = box->first_child; child; child = child->next) {
		if (UI_PointIsInRect(child->computed_rect, UI_STATE.mouse_pos)) {
			if (child->flags & UI_BoxFlag_Clickable) {
				result = true;
				break;
			}
			if (UI_HasAnyHoveredClickableChild_(child)) {
				result = true;
				break;
			}
		}
	}
	UI_ProfExit();
	return result;
}

UI_API bool UI_IsHoveredIdle(UI_Box* box) {
	bool result = box->prev_frame && !(box->prev_frame->flags & UI_BoxFlag_NoHover) && UI_PointIsInRect(box->prev_frame->computed_rect, UI_STATE.mouse_pos);
	result = result && !UI_HasAnyHoveredClickableChild_(box->prev_frame);
	return result;
}

//UI_API UI_Box* UI_BoxFromKey(UI_Key key) {
//	UI_Box* box = NULL;
//	DS_MapFind(&UI_STATE.data_from_key, key, &box);
//	return box;
//}

//UI_API UI_Box* UI_PrevFrameBoxFromKey(UI_Key key) {
//	UI_Box* box = NULL;
//	DS_MapFind(&UI_STATE.prev_frame_data_from_key, key, &box);
//	return box;
//}

UI_API void UI_BoxAddVarData(UI_Box* box, UI_Key key, void* ptr, int size) {
	UI_BoxVariableHeader* header = (UI_BoxVariableHeader*)DS_ArenaPush(&UI_STATE.frame_arena, sizeof(UI_BoxVariableHeader) + size);
	header->key = key;
	header->next = box->variables;
	header->debug_size = size;
	box->variables = header;
	memcpy(header + 1, ptr, size);
}

UI_API bool UI_BoxGetRetainedVarData(UI_Box* box, UI_Key key, void** out_ptr, int size) {
	void* existing_this_frame = NULL;
	UI_BoxGetVarPtrData(box, key, &existing_this_frame, size);
	if (existing_this_frame) {
		*out_ptr = existing_this_frame;
		return true;
	}
	else {
		// Keep the variable alive by adding it to this frame's box
		UI_BoxVariableHeader* header = (UI_BoxVariableHeader*)DS_ArenaPush(&UI_STATE.frame_arena, sizeof(UI_BoxVariableHeader) + size);
		header->key = key;
		header->next = box->variables;
		header->debug_size = size;
		box->variables = header;
		
		void* existing = NULL;
		if (box->prev_frame) {
			UI_BoxGetVarPtrData(box->prev_frame, key, &existing, size);
		}
		if (existing) memcpy(header + 1, existing, size);
		else memset(header + 1, 0, size);

		*out_ptr = header + 1;
		return existing != NULL;
	}
}

UI_API void UI_BoxGetVarPtrData(UI_Box* box, UI_Key key, void** out_ptr, int size) {
	*out_ptr = NULL;
	for (UI_BoxVariableHeader* it = box->variables; it; it = it->next) {
		if (it->key == key) {
			UI_ASSERT(size == it->debug_size);
			*out_ptr = it + 1;
			break;
		}
	}
}

UI_API bool UI_BoxGetVarData(UI_Box* box, UI_Key key, void* out_value, int size) {
	void* ptr = NULL;
	UI_BoxGetVarPtrData(box, key, &ptr, size);
	memcpy(out_value, ptr, size);
	return ptr != NULL;
}

UI_API bool UI_SelectionMovementInput(UI_Box* box, UI_Key* out_new_selected_box) {
	UI_ProfEnter();

	// There are two strategies for tab-navigation. At any point (except the first step), stop if the node is selectable.

	// Strategy 1. (going down):
	//  - Recurse as far down as possible to the first child
	//  - go to the next node
	//  - repeat

	// Strategy 2. This is the reverse of strategy 1. (up/down should do exactly the opposite actions)

	// When going down, go from top-down and stop when a selectable node is found
	// When going up, go from bottom-up and stop when a selectable node is found

	bool result = false;
	if (UI_IsSelected(box) && box->parent && box->flags & UI_BoxFlag_Selectable) { // UI_BoxFlag_Selectable might have been removed for this new frame
		if (UI_InputWasPressedOrRepeated(UI_Input_Down) || (UI_InputWasPressedOrRepeated(UI_Input_Tab) && !UI_InputIsDown(UI_Input_Shift))) {
			UI_Box* n = box;
			for (;;) {
				if (n->first_child) {
					n = n->first_child;
				}
				else {
					for (;;) {
						if (n->next) {
							n = n->next;
							break;
						}
						else if (n->parent) {
							n = n->parent;
						}
						else {
							n = n->first_child;
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
			UI_Box* n = box;
			for (;;) {
				// go to the previous node
				if (n->prev) {
					n = n->prev;
					for (; n->last_child;) {
						n = n->last_child;
					}
				}
				else if (n->parent) {
					n = n->parent;
				}
				else {
					n = n->last_child;
					for (; n->last_child;) {
						n = n->last_child;
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

	for (UI_Box* child = box->first_child; child; child = child->next) {
		if (UI_SelectionMovementInput(child, out_new_selected_box)) {
			result = true;
			break;
		}
	}

end:;
	UI_ProfExit();
	return result;
}

UI_API UI_Box* UI_GetOrAddBox(UI_Key key, bool assert_newly_added) {
	UI_ProfEnter();
	
	UI_Box* box = DS_New(UI_Box, UI_FrameArena());
	void* box_voidptr = box; // this is annoying...
	bool newly_added = DS_MapInsert(&UI_STATE.data_from_key, key, box_voidptr);
	if (assert_newly_added) {
		UI_ASSERT(newly_added); // If this fails, then a box with the same key has already been added during this frame!
	}

	box->key = key;
	
	void* prev_frame_box = NULL;
	DS_MapFind(&UI_STATE.prev_frame_data_from_key, key, &prev_frame_box);
	box->prev_frame = (UI_Box*)prev_frame_box;

	UI_ProfExit();
	return box;
}

UI_API UI_Box* UI_InitBox(UI_Box* box, UI_Size w, UI_Size h, UI_BoxFlags flags) {
	UI_ProfEnter();

	box->font = UI_STATE.default_font;
	box->size[0] = w;
	box->size[1] = h;
	box->flags = flags;
	box->draw = UI_DrawBoxDefault;
	box->draw_args = UI_DrawBoxDefaultArgsConstDefaults();
	box->compute_unexpanded_size = UI_BoxComputeUnexpandedSizeDefault;

	if (UI_STATE.mouse_clicking_down_box == box->key && UI_InputIsDown(UI_Input_MouseLeft)) {
		UI_STATE.mouse_clicking_down_box_new = box->key;
	}

	if (UI_STATE.keyboard_clicking_down_box == box->key && (UI_STATE.selection_is_visible && UI_InputIsDown(UI_Input_Enter))) {
		UI_STATE.keyboard_clicking_down_box_new = box->key;
	}

	if (flags & UI_BoxFlag_Clickable) {
		bool pressed_with_mouse = UI_InputWasPressed(UI_Input_MouseLeft) && UI_IsHovered(box);
		bool pressed_with_keyboard = UI_STATE.selected_box == box->key && UI_STATE.selection_is_visible && UI_InputWasPressed(UI_Input_Enter);
		if (pressed_with_mouse) {
			UI_STATE.mouse_clicking_down_box_new = box->key;
			if (box->flags & UI_BoxFlag_Selectable) UI_STATE.selected_box_new = box->key;
		}
		if (pressed_with_keyboard) {
			UI_STATE.keyboard_clicking_down_box_new = box->key;
			if (box->flags & UI_BoxFlag_Selectable) UI_STATE.selected_box_new = box->key;
		}
	}

	// Keep the currently selected box selected unless overwritten by pressing some other box
	if (UI_STATE.selected_box == box->key && UI_STATE.selected_box_new == UI_INVALID_KEY) {
		UI_STATE.selected_box_new = box->key;
	}

	UI_ProfExit();
	return box;
}

UI_API void UI_InitRootBox(UI_Box* box, UI_Size w, UI_Size h, UI_BoxFlags flags) {
	UI_InitBox(box, w, h, flags);
	if (box->prev_frame) {
		// Update keyboard input on root boxes
		UI_Key new_selected_box;
		if (UI_SelectionMovementInput(box->prev_frame, &new_selected_box)) {
			if (UI_STATE.selection_is_visible) { // only move if selection is already visible; otherwise first make it visible
				UI_STATE.selected_box_new = new_selected_box;
			}
			UI_STATE.selection_is_visible = true;
		}
	}
}

UI_API void UI_AddBox(UI_Box* box, UI_Size w, UI_Size h, UI_BoxFlags flags) {
	UI_InitBox(box, w, h, flags);
	
	UI_Box* parent = DS_ArrPeek(UI_STATE.box_stack);
	UI_ASSERT(parent != NULL); // AddBox creates a new box under the currently pushed box. If this fails, no box is currently pushed
	UI_ASSERT(box->parent == NULL); // Has UI_AddBox been called once this frame already?

	box->parent = parent;
	if (parent->flags & UI_BoxFlag_NoHover) box->flags |= UI_BoxFlag_NoHover;

	if (parent->last_child) parent->last_child->next = box;
	else parent->first_child = box;
	box->prev = parent->last_child;
	parent->last_child = box;
}

UI_API void UI_PushBox(UI_Box* box) {
	UI_ProfEnter();
	DS_ArrPush(&UI_STATE.box_stack, box);
	UI_ProfExit();
}

UI_API void UI_PopBox(UI_Box* box) {
	UI_ProfEnter();
	UI_Box* popped = DS_ArrPop(&UI_STATE.box_stack);
	UI_ASSERT(popped == box);
	UI_ProfExit();
}

UI_API void UI_PopBoxN(UI_Box* box, int n) {
	UI_ASSERT(UI_STATE.box_stack.count >= n);
	UI_STATE.box_stack.count -= n - 1;
	UI_Box* last_popped = DS_ArrPop(&UI_STATE.box_stack);
	UI_ASSERT(last_popped == box);
}

UI_API void UI_BeginFrame(const UI_Inputs* inputs, UI_Font default_font) {
	UI_ProfEnter();

	const int max_vertices_default = 4096;
	const int max_indices_default = 4096*4;
	
	UI_STATE.vertex_buffer_count = 0;
	UI_STATE.vertex_buffer_capacity = max_vertices_default;
	UI_STATE.vertex_buffer = UI_STATE.backend.ResizeAndMapVertexBuffer(max_vertices_default);
	UI_ASSERT(UI_STATE.vertex_buffer != NULL);
	
	UI_STATE.index_buffer_count = 0;
	UI_STATE.index_buffer_capacity = max_indices_default;
	UI_STATE.index_buffer = UI_STATE.backend.ResizeAndMapIndexBuffer(max_indices_default);
	UI_ASSERT(UI_STATE.index_buffer != NULL);
	
	UI_STATE.active_texture = NULL;
	DS_ArrInit(&UI_STATE.draw_commands, &UI_STATE.frame_arena);
	
	UI_STATE.inputs = *inputs;
	memset(&UI_STATE.outputs, 0, sizeof(UI_STATE.outputs));
	
	UI_ASSERT(UI_STATE.box_stack.count == 1);
	UI_STATE.mouse_pos = UI_AddV2(UI_STATE.inputs.mouse_position, UI_VEC2{ 0.5f, 0.5f });
	UI_STATE.default_font = default_font;
	
	DS_Arena prev_frame_arena = UI_STATE.prev_frame_arena;
	UI_STATE.prev_frame_arena = UI_STATE.frame_arena;
	UI_STATE.frame_arena = prev_frame_arena;
	DS_ArenaReset(&UI_STATE.frame_arena);

	UI_STATE.prev_frame_data_from_key = UI_STATE.data_from_key;
	DS_MapInit(&UI_STATE.data_from_key, &UI_STATE.frame_arena);
	
	UI_STATE.mouse_clicking_down_box = UI_STATE.mouse_clicking_down_box_new;
	UI_STATE.mouse_clicking_down_box_new = UI_INVALID_KEY;
	UI_STATE.keyboard_clicking_down_box = UI_STATE.keyboard_clicking_down_box_new;
	UI_STATE.keyboard_clicking_down_box_new = UI_INVALID_KEY;
	UI_STATE.selected_box = UI_STATE.selected_box_new;
	UI_STATE.selected_box_new = UI_INVALID_KEY;

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

	UI_ProfExit();
}

UI_API void UI_Deinit(void) {
	UI_ProfEnter();
	
	DS_ArenaDeinit(&UI_STATE.frame_arena);
	DS_ArenaDeinit(&UI_STATE.prev_frame_arena);
	
	UI_ProfExit();
}

UI_API void UI_Init(DS_Allocator* allocator) {
	UI_ProfEnter();

	memset(&UI_STATE, 0, sizeof(UI_STATE));
	UI_STATE.allocator = allocator;
	
	DS_ArenaInit(&UI_STATE.prev_frame_arena, DS_KIB(4), allocator);
	DS_ArenaInit(&UI_STATE.frame_arena, DS_KIB(4), allocator);
	
	DS_ArrInit(&UI_STATE.box_stack, allocator);
	DS_ArrPush(&UI_STATE.box_stack, NULL);

	UI_ProfExit();
}

UI_API void UI_BoxComputeExpandedSize(UI_Box* box, UI_Axis axis, float size) {
	UI_ProfEnter();
	box->computed_expanded_size._[axis] = size;

	float child_area_size = size;
	child_area_size -= 2.f * box->inner_padding._[axis];

	UI_Axis layout_axis = box->flags & UI_BoxFlag_Horizontal ? UI_Axis_X : UI_Axis_Y;
	UI_BoxFlags no_flex_down_flag = axis == UI_Axis_X ? UI_BoxFlag_NoFlexDownX : UI_BoxFlag_NoFlexDownY;

	if (axis == layout_axis) {
		float total_leftover = child_area_size;
		float total_flex = 0.f;

		for (UI_Box* child = box->first_child; child; child = child->next) {
			total_leftover -= child->computed_unexpanded_size._[axis];
		}
		
		for (UI_Box* child = box->first_child; child; child = child->next) {
			if (total_leftover > 0) {
				total_flex += child->size[axis] < 0.f ? child->size[axis] + 100.f : 0.f; // flex up
			} else {
				total_flex += child->size[axis] < 0.f && !(child->flags & no_flex_down_flag) ? child->size[axis] + 100.f : 0.f; // flex down
			}
		}

		for (UI_Box* child = box->first_child; child; child = child->next) {
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
		for (UI_Box* child = box->first_child; child; child = child->next) {
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

	UI_ProfExit();
}

UI_API void UI_BoxComputeRectsStep(UI_Box* box, UI_Axis axis, float position, UI_ScissorRect scissor) {
	UI_ProfEnter();
	box->computed_position._[axis] = position + box->offset._[axis];

	float min = box->computed_position._[axis];
	float max = min + box->computed_expanded_size._[axis];
	float min_clipped = min;
	float max_clipped = max;

	if (scissor) {
		min_clipped = UI_Max(min, scissor->min._[axis]);
		max_clipped = UI_Min(max, scissor->max._[axis]);
	}

	box->computed_rect.min._[axis] = min_clipped;
	box->computed_rect.max._[axis] = max_clipped;

	bool layout_from_end = axis == UI_Axis_X ? box->flags & UI_BoxFlag_ReverseLayoutX : box->flags & UI_BoxFlag_ReverseLayoutY;
	float direction = layout_from_end ? -1.f : 1.f;

	float cursor_base = layout_from_end ? max : min;
	float cursor = cursor_base + direction * box->inner_padding._[axis];

	UI_ScissorRect child_scissor = (box->flags & UI_BoxFlag_NoScissor) ? scissor : &box->computed_rect;

	UI_Axis layout_axis = box->flags & UI_BoxFlag_Horizontal ? UI_Axis_X : UI_Axis_Y;

	for (UI_Box* child = box->first_child; child; child = child->next) {
		float child_position = (child->flags & UI_BoxFlag_NoAutoOffset ? cursor_base : cursor);
		if (layout_from_end) child_position -= child->computed_expanded_size._[axis];

		UI_BoxComputeRectsStep(child, axis, child_position, child_scissor);

		if (axis == layout_axis) {
			cursor += direction * child->computed_expanded_size._[axis];
		}
	}
	UI_ProfExit();
}

UI_API void UI_BoxComputeUnexpandedSizeDefault(UI_Box* box, UI_Axis axis, int pass, bool* request_second_pass) {
	UI_ProfEnter();
	
	for (UI_Box* child = box->first_child; child; child = child->next) {
		child->compute_unexpanded_size(child, axis, pass, request_second_pass);
	}

	float fitting_size = 0.f;

	if (box->flags & UI_BoxFlag_HasText) {
		UI_ASSERT(box->first_child == NULL); // A box which has text must not have children

		float text_size = axis == 0 ? UI_TextWidth(box->text, box->font) : (float)box->font.size;
		fitting_size = (float)(int)(text_size + 0.5f) + 2.f * box->inner_padding._[axis];
	}

	if (box->first_child) {
		UI_Axis layout_axis = box->flags & UI_BoxFlag_Horizontal ? UI_Axis_X : UI_Axis_Y;

		for (UI_Box* child = box->first_child; child; child = child->next) {
			if (layout_axis == axis) {
				fitting_size += child->computed_unexpanded_size._[axis];
			}
			else {
				fitting_size = UI_Max(fitting_size, child->computed_unexpanded_size._[axis]);
			}
		}

		fitting_size += 2 * box->inner_padding._[axis];
	}

	float unexpanded_size = box->size[axis] < 0.f ? fitting_size : box->size[axis];
	box->computed_unexpanded_size._[axis] = unexpanded_size;
	UI_ProfExit();
}

UI_API void UI_BoxComputeExpandedSizes(UI_Box* box) {
	UI_ProfEnter();

	for (int pass = 0; pass < 2; pass++) {
		bool request_second_pass = false;

		box->compute_unexpanded_size(box, UI_Axis_X, pass, &request_second_pass);
		UI_BoxComputeExpandedSize(box, UI_Axis_X, box->computed_unexpanded_size.x);
	
		box->compute_unexpanded_size(box, UI_Axis_Y, pass, &request_second_pass);
		UI_BoxComputeExpandedSize(box, UI_Axis_Y, box->computed_unexpanded_size.y);
		
		if (!request_second_pass) break;
	}

	UI_ProfExit();
}

UI_API void UI_BoxComputeRects(UI_Box* box, UI_Vec2 box_position) {
	UI_ProfEnter();
	UI_BoxComputeExpandedSizes(box);
	UI_BoxComputeRectsStep(box, UI_Axis_X, box_position.x, NULL);
	UI_BoxComputeRectsStep(box, UI_Axis_Y, box_position.y, NULL);
	UI_ProfExit();
}

static void UI_FinalizeDrawBatch() {
	UI_ProfEnter();
	uint32_t first_index = 0;
	if (UI_STATE.draw_commands.count > 0) {
		UI_DrawCommand last = DS_ArrPeek(UI_STATE.draw_commands);
		first_index = last.first_index + last.index_count;
	}

	uint32_t index_count = UI_STATE.index_buffer_count - first_index;
	if (index_count > 0) {
		UI_DrawCommand draw_call = {0};
		draw_call.texture = UI_STATE.active_texture;
		draw_call.first_index = first_index;
		draw_call.index_count = index_count;
		DS_ArrPush(&UI_STATE.draw_commands, draw_call);
	}
	UI_ProfExit();
}

UI_API void UI_EndFrame(UI_Outputs* outputs) {
	UI_ProfEnter();

	// We reset these at the end of the frame so that we can still check for `was_key_released` and have these not be reset yet
	if (UI_InputIsDown(UI_Input_MouseLeft)) {
		UI_Vec2 delta = UI_STATE.inputs.mouse_raw_delta;
		float scale = 1.f;
		if (UI_InputIsDown(UI_Input_Alt)) scale /= 50.f;
		if (UI_InputIsDown(UI_Input_Shift)) scale *= 50.f;
		UI_STATE.mouse_travel_distance_after_press = UI_AddV2(UI_STATE.mouse_travel_distance_after_press, UI_MulV2F(delta, scale));
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

	UI_STATE.outputs.draw_commands = UI_STATE.draw_commands.data;
	UI_STATE.outputs.draw_commands_count = UI_STATE.draw_commands.count;
	*outputs = UI_STATE.outputs;

	UI_ProfExit();
}

UI_API void UI_SplittersNormalizeToTotalSize(UI_SplittersState* splitters, float total_size) {
	float normalize_factor = total_size / splitters->panel_end_offsets[splitters->panel_count - 1];
	for (int i = 0; i < splitters->panel_count; i++) {
		splitters->panel_end_offsets[i] = splitters->panel_end_offsets[i] * normalize_factor;
	}
}

UI_API UI_SplittersState* UI_Splitters(UI_Key key, UI_Rect area, UI_Axis X, int panel_count, float panel_min_width)
{
	UI_ProfEnter();
	UI_ASSERT(panel_count > 0);

	UI_SplittersState* data;
	UI_BoxGetRetainedVar(UI_KBOX(key), 0, &data); // TODO: go directly from key to data and not through a box

	float* panel_end_offsets = (float*)DS_ArenaPushZero(&UI_STATE.frame_arena, sizeof(float) * panel_count);
	if (data->panel_end_offsets) {
		int panel_copy_count = UI_Min(data->panel_count, panel_count);
		for (int i = 0; i < panel_copy_count; i++) {
			panel_end_offsets[i] = data->panel_end_offsets[i]; // Copy over from the previous frame
		}
	}
	data->panel_end_offsets = panel_end_offsets;
	data->panel_count = panel_count;

	// Sanitize positions
	float offset = 0.f;
	for (int i = 0; i < panel_count; i++) {
		panel_end_offsets[i] = UI_Max(panel_end_offsets[i], offset + panel_min_width);
		offset = panel_end_offsets[i];
	}

	float rect_width = area.max._[X] - area.min._[X];
	UI_SplittersNormalizeToTotalSize(data, rect_width);
	
	if (data->holding_splitter && !UI_InputIsDown(UI_Input_MouseLeft)) {
		data->holding_splitter = 0;
	}

	if (data->holding_splitter) {
		int holding_splitter = data->holding_splitter - 1; // zero-based index
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

	data->hovering_splitter = 0;
	for (int i = 0; i < panel_count - 1; i++) {
		const float SPLITTER_HALF_WIDTH = 2.f; // this could use the current DPI
		float end_x = area.min._[X] + panel_end_offsets[i];
		UI_Rect end_splitter_rect = area;
		end_splitter_rect.min._[X] = end_x - SPLITTER_HALF_WIDTH;
		end_splitter_rect.max._[X] = end_x + SPLITTER_HALF_WIDTH;

		if (UI_PointIsInRect(end_splitter_rect, UI_STATE.mouse_pos)) {
			data->hovering_splitter = i + 1;
			break;
		}
	}
	
	if (data->hovering_splitter) {
		UI_STATE.outputs.cursor = X == UI_Axis_X ? UI_MouseCursor_ResizeH : UI_MouseCursor_ResizeV;

		if (UI_InputWasPressed(UI_Input_MouseLeft)) {
			data->holding_splitter = data->hovering_splitter;
		}
	}

	UI_ProfExit();
	return data;
}

UI_API float UI_GlyphAdvance(uint32_t codepoint, UI_Font font) {
	UI_CachedGlyph glyph = UI_STATE.backend.GetCachedGlyph(codepoint, font);
	return glyph.advance;
}

UI_API float UI_TextWidth(STR_View text, UI_Font font) {
	UI_ProfEnter();
	float w = 0.f;
	size_t i = 0;
	for (uint32_t r; r = STR_NextCodepoint(text, &i);) {
		w += UI_GlyphAdvance(r, font);
	}
	UI_ProfExit();
	return w;
}

UI_API UI_DrawVertex* UI_AddVerticesUnsafe(int count, uint32_t* out_first_vertex) {
	UI_ProfEnter();
	
	*out_first_vertex = UI_STATE.vertex_buffer_count;
	
	// Grow vertex buffer if needed
	int new_count = UI_STATE.vertex_buffer_count + count;
	if (new_count > UI_STATE.vertex_buffer_capacity) {
		while (new_count > UI_STATE.vertex_buffer_capacity) {
			UI_STATE.vertex_buffer_capacity *= 2;
		}
		UI_STATE.vertex_buffer = UI_STATE.backend.ResizeAndMapVertexBuffer(UI_STATE.vertex_buffer_capacity);
	}
	
	UI_DrawVertex* result_data = &UI_STATE.vertex_buffer[UI_STATE.vertex_buffer_count];
	UI_STATE.vertex_buffer_count = new_count;
	
	UI_ProfExit();
	return result_data;
}

UI_API uint32_t UI_AddVertices(UI_DrawVertex* vertices, int count) {
	UI_ProfEnter();
	
	uint32_t first_index = UI_STATE.vertex_buffer_count;
	
	// Grow vertex buffer if needed
	int new_count = UI_STATE.vertex_buffer_count + count;
	if (new_count > UI_STATE.vertex_buffer_capacity) {
		while (new_count > UI_STATE.vertex_buffer_capacity) {
			UI_STATE.vertex_buffer_capacity *= 2;
		}
		UI_STATE.vertex_buffer = UI_STATE.backend.ResizeAndMapVertexBuffer(UI_STATE.vertex_buffer_capacity);
	}
	
	memcpy(&UI_STATE.vertex_buffer[UI_STATE.vertex_buffer_count], vertices, count * sizeof(UI_DrawVertex));
	UI_STATE.vertex_buffer_count = new_count;
	
	UI_ProfExit();
	return first_index;
}

UI_API void UI_AddIndices(uint32_t* indices, int count, UI_Texture* texture) {
	UI_ProfEnter();
	
	// Set active texture
	if (texture != UI_STATE.active_texture) {
		if (UI_STATE.active_texture != NULL) {
			UI_FinalizeDrawBatch();
		}
		UI_STATE.active_texture = texture;
	}
	
	// Grow index buffer if needed
	int new_count = UI_STATE.index_buffer_count + count;
	if (new_count > UI_STATE.index_buffer_capacity) {
		while (new_count > UI_STATE.index_buffer_capacity) {
			UI_STATE.index_buffer_capacity *= 2;
		}
		UI_STATE.index_buffer = UI_STATE.backend.ResizeAndMapIndexBuffer(UI_STATE.index_buffer_capacity);
	}
	
	memcpy(&UI_STATE.index_buffer[UI_STATE.index_buffer_count], indices, count * sizeof(uint32_t));
	UI_STATE.index_buffer_count = new_count;
	
	UI_ProfExit();
}

UI_API void UI_AddTriangleIndices(uint32_t a, uint32_t b, uint32_t c, UI_Texture* texture) {
	UI_ProfEnter();
	uint32_t indices[3] = {a, b, c};
	UI_AddIndices(indices, 3, texture);
	UI_ProfExit();
}

UI_API void UI_AddQuadIndices(uint32_t a, uint32_t b, uint32_t c, uint32_t d, UI_Texture* texture) {
	UI_ProfEnter();
	uint32_t indices[6] = {a, b, c, a, c, d};
	UI_AddIndices(indices, 6, texture);
	UI_ProfExit();
}

UI_API void UI_DrawConvexPolygon(const UI_Vec2* points, int points_count, UI_Color color) {
	UI_ProfEnter();
	uint32_t first_vertex;
	UI_DrawVertex* vertices = UI_AddVerticesUnsafe(points_count, &first_vertex);
	for (int i = 0; i < points_count; i++) {
		UI_Vec2 p = points[i];
		vertices[i] = UI_DRAW_VERTEX{ {p.x, p.y}, {0, 0}, color };
	}
	for (int i = 2; i < points_count; i++) {
		UI_AddTriangleIndices(first_vertex, first_vertex + i - 1, first_vertex + i, NULL);
	}
	UI_ProfExit();
}

UI_API void UI_DrawSprite(UI_Rect rect, UI_Color color, UI_Rect uv_rect, UI_Texture* texture, UI_ScissorRect scissor) {
	UI_ProfEnter();
	if (scissor == NULL || !UI_ClipRectEx(&rect, &uv_rect, scissor)) {
		UI_DrawVertex vertices[4] = {
			{ {rect.min.x, rect.min.y}, uv_rect.min,                    color },
			{ {rect.max.x, rect.min.y}, {uv_rect.max.x, uv_rect.min.y}, color },
			{ {rect.max.x, rect.max.y}, uv_rect.max,                    color },
			{ {rect.min.x, rect.max.y}, {uv_rect.min.x, uv_rect.max.y}, color },
		};
		uint32_t first_vertex = UI_AddVertices(vertices, 4);
		UI_AddQuadIndices(first_vertex, first_vertex + 1, first_vertex + 2, first_vertex + 3, texture);
	}
	UI_ProfExit();
}

static void UI_LimitRectPadding(const UI_Rect* rect, float* padding) {
	float size_x = (rect->max.x - rect->min.x) * 0.5f;
	float size_y = (rect->max.y - rect->min.y) * 0.5f;
	if (size_x < *padding) *padding = size_x;
	if (size_y < *padding) *padding = size_y;
}

UI_API void UI_DrawRectLines(UI_Rect rect, float thickness, UI_Color color) {
	UI_ProfEnter();
	UI_LimitRectPadding(&rect, &thickness);
	UI_DrawRectCorners corners = { {color, color, color, color}, {0}, {0.f, 0.f, 0.f, 0.f} };
	UI_DrawRectLinesEx(rect, &corners, thickness);
	UI_ProfExit();
}

UI_API void UI_DrawRectLinesRounded(UI_Rect rect, float thickness, float roundness, UI_Color color) {
	UI_ProfEnter();
	UI_LimitRectPadding(&rect, &roundness);
	UI_LimitRectPadding(&rect, &thickness);
	UI_DrawRectCorners corners = { {color, color, color, color}, {0}, {roundness, roundness, roundness, roundness} };
	UI_DrawRectLinesEx(rect, &corners, thickness);
	UI_ProfExit();
}

static UI_Vec2 UI_PointOnRoundedCorner(int corner_index, int vertex_index, int end_vertex_index) {
	UI_ProfEnter();
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
	UI_ProfExit();
	return c_rotated;
}

UI_API void UI_DrawRectLinesEx(UI_Rect rect, const UI_DrawRectCorners* corners, float thickness) {
	UI_ProfEnter();
	if (rect.max.x > rect.min.x && rect.max.y > rect.min.y) {
		UI_Vec2 inset_corners[4];
		inset_corners[0] = UI_AddV2(rect.min, UI_VEC2{ corners->roundness[0], corners->roundness[0] });
		inset_corners[1] = UI_AddV2(UI_VEC2{ rect.max.x, rect.min.y }, UI_VEC2{ -corners->roundness[1], corners->roundness[1] });
		inset_corners[2] = UI_AddV2(rect.max, UI_VEC2{ -corners->roundness[2], -corners->roundness[2] });
		inset_corners[3] = UI_AddV2(UI_VEC2{ rect.min.x, rect.max.y }, UI_VEC2{ corners->roundness[3], -corners->roundness[3] });

		// Per each edge (top, right, bottom, left), we add 4 vertices: first outer, first inner, last outer, last inner
		UI_DrawVertex v[16] = {
			{ {inset_corners[0].x, rect.min.y}, {0.f, 0.f}, corners->color[0] },
			{ {inset_corners[0].x, rect.min.y + thickness}, {0.f, 0.f}, corners->color[0] },
			{ {inset_corners[1].x, rect.min.y}, {0.f, 0.f}, corners->color[1] },
			{ {inset_corners[1].x, rect.min.y + thickness}, {0.f, 0.f}, corners->color[1] },

			{ {rect.max.x, inset_corners[1].y}, {0.f, 0.f}, corners->color[1] },
			{ {rect.max.x - thickness, inset_corners[1].y}, {0.f, 0.f}, corners->color[1] },
			{ {rect.max.x, inset_corners[2].y}, {0.f, 0.f}, corners->color[2] },
			{ {rect.max.x - thickness, inset_corners[2].y}, {0.f, 0.f}, corners->color[2] },

			{ {inset_corners[2].x, rect.max.y}, {0.f, 0.f}, corners->color[2] },
			{ {inset_corners[2].x, rect.max.y - thickness}, {0.f, 0.f}, corners->color[2] },
			{ {inset_corners[3].x, rect.max.y}, {0.f, 0.f}, corners->color[3] },
			{ {inset_corners[3].x, rect.max.y - thickness}, {0.f, 0.f}, corners->color[3] },

			{ {rect.min.x, inset_corners[3].y}, {0.f, 0.f}, corners->color[3] },
			{ {rect.min.x + thickness, inset_corners[3].y}, {0.f, 0.f}, corners->color[3] },
			{ {rect.min.x, inset_corners[0].y}, {0.f, 0.f}, corners->color[0] },
			{ {rect.min.x + thickness, inset_corners[0].y}, {0.f, 0.f}, corners->color[0] },
		};
		uint32_t edge_verts = UI_AddVertices(v, 16);

		// Generate edge quads
		for (uint32_t base = edge_verts; base < edge_verts + 16; base += 4) {
			UI_AddTriangleIndices(base + 0, base + 2, base + 3, NULL);
			UI_AddTriangleIndices(base + 0, base + 3, base + 1, NULL);
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

				UI_DrawVertex new_verts[2] = {
					{ outer_pos, {0.f, 0.f}, corners->color[corner] },
					{ mid_pos, {0.f, 0.f}, corners->color[corner] },
				};
				uint32_t new_verts_first = UI_AddVertices(new_verts, 2);

				UI_AddTriangleIndices(prev_verts_first + 0, new_verts_first + 0, new_verts_first + 1, NULL);
				UI_AddTriangleIndices(prev_verts_first + 0, new_verts_first + 1, prev_verts_first + 1, NULL);
				prev_verts_first = new_verts_first;
			}

			uint32_t new_verts_first = edge_verts + 4 * corner;
			UI_AddTriangleIndices(prev_verts_first + 0, new_verts_first + 0, new_verts_first + 1, NULL);
			UI_AddTriangleIndices(prev_verts_first + 0, new_verts_first + 1, prev_verts_first + 1, NULL);
		}
	}
	UI_ProfExit();
}

UI_API void UI_DrawCircle(UI_Vec2 p, float radius, int segments, UI_Color color) {
	UI_ProfEnter();
	uint32_t first_vertex;
	UI_DrawVertex* vertices = UI_AddVerticesUnsafe(segments, &first_vertex);
	for (int i = 0; i < segments; i++) {
		float theta = ((float)i / (float)segments) * (2.f * 3.141592f);
		UI_Vec2 v = { p.x + radius * cosf(theta), p.y + radius * sinf(theta) };
		vertices[i] = UI_DRAW_VERTEX{ {v.x, v.y}, {0, 0}, color };
	}
	for (int i = 2; i < segments; i++) {
		UI_AddTriangleIndices(first_vertex, first_vertex + i - 1, first_vertex + i, NULL);
	}
	UI_ProfExit();
}

UI_API void UI_DrawTriangle(UI_Vec2 a, UI_Vec2 b, UI_Vec2 c, UI_Color color) {
	UI_ProfEnter();
	UI_DrawVertex v[3] = {
		{a, {0, 0}, color},
		{b, {0, 0}, color},
		{c, {0, 0}, color},
	};
	uint32_t i = UI_AddVertices(v, 3);
	UI_AddTriangleIndices(i, i + 1, i + 2, NULL);
	UI_ProfExit();
}

UI_API void UI_DrawQuad(UI_Vec2 a, UI_Vec2 b, UI_Vec2 c, UI_Vec2 d, UI_Color color) {
	UI_ProfEnter();
	UI_DrawVertex v[4] = {
		{a, {0, 0}, color},
		{b, {0, 0}, color},
		{c, {0, 0}, color},
		{d, {0, 0}, color},
	};
	uint32_t i = UI_AddVertices(v, 4);
	UI_AddQuadIndices(i, i + 1, i + 2, i + 3, NULL);
	UI_ProfExit();
}

UI_API void UI_DrawRect(UI_Rect rect, UI_Color color) {
	UI_ProfEnter();
	if (rect.max.x > rect.min.x && rect.max.y > rect.min.y) {
		UI_DrawVertex v[4] = {
			{{rect.min.x, rect.min.y}, {0, 0}, color},
			{{rect.max.x, rect.min.y}, {0, 0}, color},
			{{rect.max.x, rect.max.y}, {0, 0}, color},
			{{rect.min.x, rect.max.y}, {0, 0}, color},
		};
		uint32_t i = UI_AddVertices(v, 4);
		UI_AddQuadIndices(i, i + 1, i + 2, i + 3, NULL);
	}
	UI_ProfExit();
}

UI_API void UI_DrawRectRounded(UI_Rect rect, float roundness, UI_Color color, int num_corner_segments) {
	UI_ProfEnter();
	UI_LimitRectPadding(&rect, &roundness);
	UI_DrawRectCorners corners = { {color, color, color, color}, {color, color, color, color}, {roundness, roundness, roundness, roundness} };
	UI_DrawRectEx(rect, &corners, num_corner_segments);
	UI_ProfExit();
}

UI_API void UI_DrawRectRounded2(UI_Rect rect, float roundness, UI_Color inner_color, UI_Color outer_color, int num_corner_segments) {
	UI_ProfEnter();
	UI_LimitRectPadding(&rect, &roundness);
	UI_DrawRectCorners corners = {
		{inner_color, inner_color, inner_color, inner_color},
		{outer_color, outer_color, outer_color, outer_color},
		{roundness, roundness, roundness, roundness} };
	UI_DrawRectEx(rect, &corners, num_corner_segments);
	UI_ProfExit();
}

UI_API void UI_DrawRectEx(UI_Rect rect, const UI_DrawRectCorners* corners, int num_corner_segments) {
	UI_ProfEnter();
	if (rect.max.x > rect.min.x && rect.max.y > rect.min.y) {
		UI_Vec2 inset_corners[4];
		inset_corners[0] = UI_AddV2(rect.min, UI_VEC2{ corners->roundness[0], corners->roundness[0] });
		inset_corners[1] = UI_AddV2(UI_VEC2{ rect.max.x, rect.min.y }, UI_VEC2{ -corners->roundness[1], corners->roundness[1] });
		inset_corners[2] = UI_AddV2(rect.max, UI_VEC2{ -corners->roundness[2], -corners->roundness[2] });
		inset_corners[3] = UI_AddV2(UI_VEC2{ rect.min.x, rect.max.y }, UI_VEC2{ corners->roundness[3], -corners->roundness[3] });
		
		UI_DrawVertex v[12] = {
			{ inset_corners[0], {0, 0}, corners->color[0] },
			{ inset_corners[1], {0, 0}, corners->color[1] },
			{ inset_corners[2], {0, 0}, corners->color[2] },
			{ inset_corners[3], {0, 0}, corners->color[3] },

			{ {rect.min.x, inset_corners[0].y}, {0, 0}, corners->outer_color[0] },
			{ {inset_corners[0].x, rect.min.y}, {0, 0}, corners->outer_color[0] },
			{ {inset_corners[1].x, rect.min.y}, {0, 0}, corners->outer_color[1] },
			{ {rect.max.x, inset_corners[1].y}, {0, 0}, corners->outer_color[1] },
			{ {rect.max.x, inset_corners[2].y}, {0, 0}, corners->outer_color[2] },
			{ {inset_corners[2].x, rect.max.y}, {0, 0}, corners->outer_color[2] },
			{ {inset_corners[3].x, rect.max.y}, {0, 0}, corners->outer_color[3] },
			{ {rect.min.x, inset_corners[3].y}, {0, 0}, corners->outer_color[3] },
		};
		uint32_t inset_corner_verts = UI_AddVertices(v, 12);
		uint32_t border_verts = inset_corner_verts + 4;

		// edge quads
		UI_AddQuadIndices(border_verts + 1, border_verts + 2, inset_corner_verts + 1, inset_corner_verts + 0, NULL); // top edge
		UI_AddQuadIndices(border_verts + 3, border_verts + 4, inset_corner_verts + 2, inset_corner_verts + 1, NULL); // right edge
		UI_AddQuadIndices(border_verts + 5, border_verts + 6, inset_corner_verts + 3, inset_corner_verts + 2, NULL); // bottom edge
		UI_AddQuadIndices(border_verts + 7, border_verts + 0, inset_corner_verts + 0, inset_corner_verts + 3, NULL); // left edge

		for (int corner_i = 0; corner_i < 4; corner_i++) {
			float r = -corners->roundness[corner_i];

			uint32_t prev_vert_idx = border_verts + corner_i * 2;

			// Generate corner triangles
			for (int i = 1; i < num_corner_segments; i++) {
				UI_Vec2 c = UI_PointOnRoundedCorner(corner_i, i, num_corner_segments);
				
				UI_DrawVertex new_vert = { {inset_corners[corner_i].x + r*c.x, inset_corners[corner_i].y + r*c.y}, {0, 0}, corners->outer_color[corner_i] };
				uint32_t new_vert_idx = UI_AddVertices(&new_vert, 1);

				UI_AddTriangleIndices(inset_corner_verts + corner_i, prev_vert_idx, new_vert_idx, NULL);
				prev_vert_idx = new_vert_idx;
			}

			UI_AddTriangleIndices(inset_corner_verts + corner_i, prev_vert_idx, border_verts + corner_i * 2 + 1, NULL);
		}

		UI_AddQuadIndices(inset_corner_verts, inset_corner_verts + 1, inset_corner_verts + 2, inset_corner_verts + 3, NULL);
	}
	UI_ProfExit();
}

UI_API void UI_DrawPoint(UI_Vec2 p, float thickness, UI_Color color) {
	UI_ProfEnter();
	UI_Vec2 extent = { 0.5f * thickness, 0.5f * thickness };
	UI_Rect rect = { UI_SubV2(p, extent), UI_AddV2(p, extent) };
	UI_DrawRect(rect, color);
	UI_ProfExit();
}

UI_API void UI_DrawLine(UI_Vec2 a, UI_Vec2 b, float thickness, UI_Color color) {
	UI_ProfEnter();
	UI_Vec2 points[] = { a, b };
	UI_Color colors[] = { color, color };
	UI_DrawPolyline(points, colors, 2, thickness);
	UI_ProfExit();
}

UI_API void UI_DrawLineEx(UI_Vec2 a, UI_Vec2 b, float thickness, UI_Color a_color, UI_Color b_color) {
	UI_ProfEnter();
	UI_Vec2 points[] = { a, b };
	UI_Color colors[] = { a_color, b_color };
	UI_DrawPolyline(points, colors, 2, thickness);
	UI_ProfExit();
}

UI_API void UI_DrawPolylineEx(const UI_Vec2* points, const UI_Color* colors, int points_count, float thickness, bool loop,
	float split_miter_threshold)
{
	UI_ProfEnter();
	if (points_count >= 2) {
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
				UI_DrawVertex v[4] = {
					{{p.x + n_pre.x*half_thickness, p.y + n_pre.y*half_thickness}, {0.f, 0.f}, color},
					{{p.x - n_pre.x*half_thickness, p.y - n_pre.y*half_thickness}, {0.f, 0.f}, color},
					{{p.x + n_post.x*half_thickness, p.y + n_post.y*half_thickness}, {0.f, 0.f}, color},
					{{p.x - n_post.x*half_thickness, p.y - n_post.y*half_thickness}, {0.f, 0.f}, color},
				};
				uint32_t new_vertices = UI_AddVertices(v, 4);

				if (loop || (i != 0 && i != last)) {
					UI_AddQuadIndices(new_vertices+0, new_vertices+1, new_vertices+3, new_vertices+2, NULL);
				}

				if (i > 0) {
					UI_AddQuadIndices(prev_idx[0], prev_idx[1], new_vertices + 1, new_vertices + 0, NULL);
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

				UI_DrawVertex v[2] = {
					{{p.x + n.x*t, p.y + n.y*t}, {0.f, 0.f}, color},
					{{p.x - n.x*t, p.y - n.y*t}, {0.f, 0.f}, color},
				};
				uint32_t new_vertices = UI_AddVertices(v, 2);

				if (i > 0) {
					UI_AddQuadIndices(prev_idx[0], prev_idx[1], new_vertices + 1, new_vertices + 0, NULL);
				} else {
					first_idx[0] = new_vertices + 0;
					first_idx[1] = new_vertices + 1;
				}
				prev_idx[0] = new_vertices + 0;
				prev_idx[1] = new_vertices + 1;
			}
		}

		if (loop) {
			UI_AddQuadIndices(prev_idx[0], prev_idx[1], first_idx[1], first_idx[0], NULL);
		}
	}
	UI_ProfExit();
}

UI_API void UI_DrawPolyline(const UI_Vec2* points, const UI_Color* colors, int points_count, float thickness) {
	UI_DrawPolylineEx(points, colors, points_count, thickness, false, 0.7f);
}

UI_API void UI_DrawPolylineLoop(const UI_Vec2* points, const UI_Color* colors, int points_count, float thickness) {
	UI_DrawPolylineEx(points, colors, points_count, thickness, true, 0.7f);
}

UI_API void UI_DrawText(STR_View text, UI_Font font, UI_Vec2 pos, UI_AlignH align, UI_Color color, UI_ScissorRect scissor) {
	UI_ProfEnter();
	
	if (align == UI_AlignH_Middle) {
		pos.x -= UI_TextWidth(text, font) * 0.5f;
	}
	else if (align == UI_AlignH_Right) {
		pos.x -= UI_TextWidth(text, font);
	}

	pos.x = (float)(int)(pos.x + 0.5f); // round to integer
	pos.y = (float)(int)(pos.y + 0.5f); // round to integer

	size_t i = 0;
	for (uint32_t r; r = STR_NextCodepoint(text, &i);) {
		UI_CachedGlyph glyph = UI_STATE.backend.GetCachedGlyph(r, font);

		UI_Rect glyph_rect;
		glyph_rect.min.x = pos.x + glyph.offset.x;
		glyph_rect.min.y = pos.y + glyph.offset.y;
		glyph_rect.max.x = glyph_rect.min.x + glyph.size.x;
		glyph_rect.max.y = glyph_rect.min.y + glyph.size.y;

		UI_Rect glyph_uv_rect = {glyph.uv_min, glyph.uv_max};
		UI_DrawSprite(glyph_rect, color, glyph_uv_rect, NULL, scissor);
		pos.x += glyph.advance;
	}

	UI_ProfExit();
}

static UI_Key UI_ArrangerSetVarKey() { return UI_KEY(); }

UI_API void UI_PushArrangerSet(UI_Box* box, UI_Size w, UI_Size h) {
	UI_ProfEnter();

	UI_AddBox(box, w, h, /*UI_BoxFlag_NoScissor*/0);
	UI_PushBox(box);
	
	UI_ArrangerSet arranger_set = {0};
	UI_BoxAddVar(box, UI_ArrangerSetVarKey(), &arranger_set);

	UI_ProfExit();
}

#define UI_DoublyListRemove(LIST, ELEM) \
	if (ELEM->prev) ELEM->prev->next = ELEM->next; \
	else LIST->first_child = ELEM->next; \
	if (ELEM->next) ELEM->next->prev = ELEM->prev; \
	else LIST->last_child = ELEM->prev;

#define UI_DoublyListPushBack(LIST, ELEM) \
	ELEM->prev = LIST->last_child; \
	ELEM->next = NULL; \
	if (LIST->last_child) LIST->last_child->next = ELEM; \
	else LIST->first_child = ELEM; \
	LIST->last_child = ELEM;

UI_API void UI_PopArrangerSet(UI_Box* box, UI_ArrangersRequest* out_edit_request) {
	UI_ProfEnter();
	UI_PopBox(box);

	UI_ArrangerSet* arranger_set;
	UI_BoxGetVarPtr(box, UI_ArrangerSetVarKey(), &arranger_set);
	UI_ASSERT(arranger_set);

	float box_origin_prev_frame = box->prev_frame ? box->prev_frame->computed_position.y : 0.f;
	float mouse_rel_y = UI_STATE.mouse_pos.y - box_origin_prev_frame;

	UI_Box* dragging = arranger_set->dragging_elem; // may be NULL

	// Compute position for each box using relative coordinates
	UI_BoxComputeRects(box, UI_VEC2{ 0, 0 });

	int dragging_index = 0;
	int target_index = 0;
	if (dragging) {
		int elem_count = 0;
		for (UI_Box* elem = box->first_child; elem; elem = elem->next) {
			if (elem == dragging) {
				dragging_index = elem_count;
			}
			elem_count++;
		}

		for (UI_Box* elem = box->first_child; elem; elem = elem->next) {
			if (elem->computed_position.y > mouse_rel_y) {
				break;
			}
			target_index++;
		}

		target_index = UI_Min(UI_Max(target_index - 1, 0), elem_count - 1);

		// When dragging an arranger, bring the dragged element to the END so that it will be drawn on top of the other things!
		UI_DoublyListRemove(dragging->parent, dragging);
		UI_DoublyListPushBack(dragging->parent, dragging);
	}

	// Now we have the original positions and sizes for each box.
	// Then we just need to figure out their target positions and provide them as offsets

	int i = 0;
	for (UI_Box* elem = box->first_child; elem; elem = elem->next) {
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
					offset += dragging->computed_expanded_size.y;
				}
				if (i >= dragging_index && i < target_index) {
					offset -= dragging->computed_expanded_size.y;
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
	UI_ProfExit();
}

UI_API void UI_AddArranger(UI_Box* box, UI_Size w, UI_Size h) {
	UI_ProfEnter();
	UI_AddLabel(box, w, h, UI_BoxFlag_Clickable, STR_V(":"));

	bool holding_down = UI_IsClickingDown(box);
	if (holding_down || UI_IsHovered(box)) {
		UI_STATE.outputs.cursor = UI_MouseCursor_ResizeV;
	}

	if (holding_down) {
		// find the element

		UI_Box* elem = box;
		for (; elem; elem = elem->parent) {
			if (elem->parent) {
				UI_ArrangerSet* arranger_set;
				UI_BoxGetVarPtr(elem->parent, UI_ArrangerSetVarKey(), &arranger_set);
				if (arranger_set) {
					arranger_set->dragging_elem = elem;
					break;
				}
			}
		}

		// This arranger is not inside of an arranger set. Did you call UI_ArrangersPush?
		UI_ASSERT(elem);
	}
	UI_ProfExit();
}
