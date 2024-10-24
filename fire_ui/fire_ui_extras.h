// This file contains some experimental extra utilities

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

#ifdef /********************/ UI_IMPLEMENTATION /********************/

static void UI_ValEditArrayScrollAreaComputeUnexpandedSize(UI_Box* box, UI_Axis axis, int pass, bool* request_second_pass) {
	UI_BoxComputeUnexpandedSizeDefault(box, axis, pass, request_second_pass);
	if (axis == UI_Axis_Y) {
		box->computed_unexpanded_size.y = UI_Min(box->computed_unexpanded_size.y, 200.f);
	}
}

UI_API void UI_AddValArray(UI_Box* box, const char* name, void* array, int array_count, int elem_size, UI_ArrayEditElemFn edit_elem, void* user_data, UI_ValueEditArrayModify* out_modify)
{
	UI_AddBox(box, UI_SizeFlex(1.f), UI_SizeFit(), 0);
	UI_PushBox(box);

	UI_Box* scroll_area = UI_BBOX(box); //UI_Key child_box_key = UI_KEY1(key);
	UI_BoxFlags flags = UI_BoxFlag_Horizontal| UI_BoxFlag_Clickable | UI_BoxFlag_Selectable | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawTransparentBackground;
	UI_Box* header = UI_BBOX(box);
	UI_AddBox(header, UI_SizeFlex(1.f), UI_SizeFit(), flags);
	UI_PushBox(header);

	bool is_open = scroll_area->prev_frame != NULL;
	if (UI_PressedIdle(header)) {
		is_open = !is_open;
	}

	UI_Box* label = UI_BBOX(box);
	UI_AddLabel(label, 20.f, UI_SizeFit(), 0, is_open ? STR_V("\x44") : STR_V("\x46"));
	label->font = UI_STATE.icons_font;

	UI_AddLabel(UI_BBOX(box), UI_SizeFlex(1.f), UI_SizeFit(), 0, name);
	UI_Box* add_button = UI_BBOX(box);
	UI_AddButton(add_button, UI_SizeFit(), UI_SizeFlex(1.f), 0, STR_V("\x48"));
	add_button->font = UI_STATE.icons_font;
	add_button->font.size = (UI_STATE.icons_font.size * 8) / 10;
	add_button->inner_padding.y += 2.f;

	UI_Box* clear_button = UI_BBOX(box);
	UI_AddButton(clear_button, UI_SizeFit(), UI_SizeFlex(1.f), 0, STR_V("\x54"));
	clear_button->font = UI_STATE.icons_font;
	clear_button->font.size = (UI_STATE.icons_font.size * 8) / 10;
	clear_button->inner_padding.y += 2.f;

	UI_PopBox(header);

	if (UI_Clicked(add_button)) is_open = true;

	int should_remove_elem = -1;

	if (is_open) {
		UI_PushScrollArea(scroll_area, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_DrawBorder, 0, 0);
		scroll_area->compute_unexpanded_size = UI_ValEditArrayScrollAreaComputeUnexpandedSize;

		UI_Box* source_files_arrangers = UI_BBOX(box);
		UI_PushArrangerSet(source_files_arrangers, UI_SizeFlex(1.f), UI_SizeFit());

		for (int i = 0; i < array_count; i++) {
			UI_Box* elem = UI_KBOX(UI_HashInt(UI_BKEY(box), i));
			UI_AddBox(elem, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Horizontal|UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawOpaqueBackground);
			UI_PushBox(elem);

			UI_Box* arranger = UI_BBOX(elem);
			UI_AddArranger(arranger, UI_SizeFit(), UI_SizeFit());
			arranger->text = STR_Form(UI_FrameArena(), "%d.", i);

			UI_Box* user_box = UI_BBOX(elem);
			UI_AddBox(user_box, UI_SizeFlex(1.f), UI_SizeFit(), 0);
			UI_PushBox(user_box);
			edit_elem(UI_BKEY(elem), (char*)array + i*elem_size, i, user_data);
			UI_PopBox(user_box);

			UI_Box* remove_button = UI_BBOX(elem);
			UI_AddButton(remove_button, UI_SizeFit(), UI_SizeFit(), 0, STR_V("\x4a"));
			remove_button->font = UI_STATE.icons_font;
			remove_button->font.size = (UI_STATE.icons_font.size * 8) / 10;
			remove_button->inner_padding.y += 2.f;

			if (UI_Clicked(remove_button)) should_remove_elem = i;

			UI_PopBox(elem);
		}

		UI_ArrangersRequest edit_request;
		UI_PopArrangerSet(source_files_arrangers, &edit_request);

		UI_PopScrollArea(scroll_area);
	}

	if (out_modify) {
		out_modify->append_to_end = UI_Clicked(add_button);
		out_modify->clear = UI_Clicked(clear_button);
		out_modify->remove_elem = should_remove_elem;
	}

	UI_PopBox(box);
}

// default_value may be NULL, in which case the element is zero-initialized
static void UI_AddValDSArray_(UI_Box* box, const char* name, DS_DynArrayRaw* array, int elem_size,
	const void* default_value, UI_ArrayEditElemFn edit_elem, void* user_data)
{
	UI_ValueEditArrayModify modify;
	UI_AddValArray(box, name, array->data, array->count, elem_size, edit_elem, user_data, &modify);
	
	if (modify.append_to_end) {
		DS_ArrResizeRaw(array, array->count + 1, NULL, elem_size);
		if (default_value) {
			memcpy((char*)array->data + (array->count-1) * elem_size, default_value, elem_size);
		} else {
			memset((char*)array->data + (array->count-1) * elem_size, 0, elem_size);
		}
	}
	if (modify.clear) array->count = 0;
	if (modify.remove_elem != -1) {
		DS_ArrRemoveRaw(array, modify.remove_elem, elem_size);
	}
}

static void UI_InfoFmtFinishCurrent_(UI_Box* box, STR_Builder* current_string, int* section_index) {
	if (current_string->str.size > 0) {
		UI_Box* label = UI_KBOX(UI_HashInt(box->key, *section_index));
		UI_AddLabel(label, UI_SizeFit(), UI_SizeFit(), 0, current_string->str);
	}
	current_string->str.size = 0;
	*section_index += 1;
}

UI_API void UI_AddFmt(UI_Box* box, const char* fmt, ...) {
	va_list args; va_start(args, fmt);

	UI_AddBox(box, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Horizontal);
	UI_PushBox(box);

	STR_Builder current_string = {UI_FrameArena()};

	int section_index = 0;
	for (const char* c = fmt; *c; c++) {
		if (*c == '%') {
			c++;
			
			bool editable = false;
			if (*c == '!') { c++; editable = true; }

			// Parse size sub-specifiers
			int h = 0, l = 0;
			for (; *c == 'h'; ) { c++; h++; }
			for (; *c == 'l'; ) { c++; l++; }

			bool is_signed = false;
			int radix = 10;

			// Parse specifier
			switch (*c) {
			default: UI_ASSERT(0);
				//case 's': {
				//	STR_Print(s, va_arg(args, char*));
				//} break;
				//case 'v': {
				//	STR_PrintV(s, va_arg(args, STR_View));
				//} break;
			case '%': {
				STR_PrintC(&current_string, "%");
			} break;
			case 't': {
				UI_InfoFmtFinishCurrent_(box, &current_string, &section_index);

				UI_Box* val_box = UI_KBOX(UI_HashInt(box->key, section_index));
				if (editable) {
					UI_Text* val = va_arg(args, UI_Text*);
					UI_AddValText(val_box, UI_SizeFlex(1.f), UI_SizeFit(), val);
				} else {
					UI_Text val = va_arg(args, UI_Text);
					__debugbreak(); // TODO: read-only text
					//UI_EditTextModify modify;
					//UI_AddValText(val_key, UI_SizeFlex(1.f), UI_SizeFit(), &val);
				}

				section_index++;
			} break;
			case 'b': {
				UI_InfoFmtFinishCurrent_(box, &current_string, &section_index);

				UI_Box* val_box = UI_KBOX(UI_HashInt(box->key, section_index));
				if (editable) {
					bool* val = va_arg(args, bool*);
					UI_AddCheckbox(val_box, val);
				} else {
					bool val = va_arg(args, bool);
					UI_AddCheckbox(val_box, &val);
				}

				section_index++;
			} break;
			case 'f': {
				UI_InfoFmtFinishCurrent_(box, &current_string, &section_index);

				UI_Box* val_box = UI_KBOX(UI_HashInt(box->key, section_index));
				if (editable) {
					if (l == 1) {
						double* val = va_arg(args, double*);
						UI_AddValFloat64(val_box, UI_SizeFlex(1.f), UI_SizeFit(), val);
					} else {
						float* val = va_arg(args, float*);
						UI_AddValFloat(val_box, UI_SizeFlex(1.f), UI_SizeFit(), val);
					}
				} else {
					double val = va_arg(args, double);
					UI_AddValFloat64(val_box, UI_SizeFlex(1.f), UI_SizeFit(), &val);
				}
				section_index++;
			} break;
				//case 'x': { radix = 16;       goto add_int; }
			case 'd': { is_signed = true; goto add_int; }
			case 'i': { is_signed = true; goto add_int; }
			case 'u': {
			add_int:;
				UI_InfoFmtFinishCurrent_(box, &current_string, &section_index);
				
				UI_Box* val_box = UI_KBOX(UI_HashInt(box->key, section_index));
				if (editable) {
					void* ptr = va_arg(args, void*);
					uint64_t val = 0;
					int size;
					if (h == 2) {
						size = 1;
						val = is_signed ? (uint64_t)(int64_t)*(int8_t*)ptr : (uint64_t)*(uint8_t*)ptr;
					} else if (h == 1) {
						size = 2;
						val = is_signed ? (uint64_t)(int64_t)*(int16_t*)ptr : (uint64_t)*(uint16_t*)ptr;
					} else if (l == 2) {
						size = 8;
						val = *(uint64_t*)ptr;
					} else {
						size = 4;
						val = is_signed ? (uint64_t)(int64_t)*(int32_t*)ptr : (uint64_t)*(uint32_t*)ptr;
					}

					UI_AddValNumeric(val_box, UI_SizeFlex(1.f), UI_SizeFit(), &val, is_signed, false);
					memcpy(ptr, &val, size);
				}
				else {
					uint64_t val;
					if (h == 2) {
						val = is_signed ? (uint64_t)(int64_t)va_arg(args, int8_t) : (uint64_t)va_arg(args, uint8_t);
					} else if (h == 1) {
						val = is_signed ? (uint64_t)(int64_t)va_arg(args, int16_t) : (uint64_t)va_arg(args, uint16_t);
					} else if (l == 2) {
						val = va_arg(args, uint64_t);
					} else {
						val = is_signed ? (uint64_t)(int64_t)va_arg(args, int32_t) : (uint64_t)va_arg(args, uint32_t);
					}

					UI_AddValNumeric(val_box, UI_SizeFlex(1.f), UI_SizeFit(), &val, is_signed, false);
				}
				section_index++;
			} break;
			}
		}
		else {
			STR_View character_str = { c, 1 };
			STR_PrintV(&current_string, character_str);
		}
	}

	UI_InfoFmtFinishCurrent_(box, &current_string, &section_index);
	UI_PopBox(box);

	va_end(args);
}


typedef struct UI_BoxWithTextWrappedData {
	float line_height;
	DS_DynArray(STR_View) line_strings;
} UI_BoxWithTextWrappedData;

static UI_Key UI_BoxWithTextWrappedDataKey() { return UI_KEY(); }

static void UI_AddLabelWrappedComputeUnexpandedSize(UI_Box* box, UI_Axis axis, int pass, bool* request_second_pass) {
	box->computed_unexpanded_size._[axis] = 0.f;
	if (axis == UI_Axis_Y) {
		float line_width = box->computed_expanded_size.x - 2.f * box->inner_padding.x;

		float space_width = UI_TextWidth(STR_V(" "), box->font);

		UI_BoxWithTextWrappedData data = {0};
		data.line_height = (float)box->font.size;
		DS_ArrInit(&data.line_strings, UI_FrameArena());

		STR_View remaining = box->text;
		char* text_end = (char*)box->text.data + box->text.size;
		float line_remaining = line_width;
		char* line_start = (char*)box->text.data;
		bool line_has_content = false;

		for (;;) {
			STR_View remaining_before = remaining;
			STR_ParseUntilAndSkip(&remaining, ' ');

			STR_View word = { remaining_before.data, (int)(remaining.data - remaining_before.data) };
			float word_width = UI_TextWidth(word, box->font);

			bool break_line = word.size == 0 || (line_remaining < word_width && line_has_content);
			if (break_line) {
				STR_View line_string = {line_start, (int)(word.data - line_start)};
				DS_ArrPush(&data.line_strings, line_string);

				line_start = (char*)word.data;
				remaining = { word.data, (int)(text_end - word.data) };
				line_remaining = line_width;
				line_has_content = false;
			}
			else {
				line_remaining -= word_width;
				line_has_content = true;
			}

			if (word.size == 0) break;
		}

		UI_BoxAddVar(box, UI_BoxWithTextWrappedDataKey(), &data);

		box->computed_unexpanded_size.y = data.line_height * (float)data.line_strings.count + 2.f * box->inner_padding.y;
	}
}

static void UI_DrawBoxWithTextWrapped(UI_Box* box) {
	UI_DrawBoxDefault(box);

	UI_BoxWithTextWrappedData* data;
	UI_BoxGetVarPtr(box, UI_BoxWithTextWrappedDataKey(), &data);

	for (int i = 0; i < data->line_strings.count; i++) {
		STR_View line_string = data->line_strings.data[i];

		UI_Vec2 text_pos = {
			box->computed_position.x + box->inner_padding.x,
			box->computed_position.y + box->inner_padding.y + (float)i * data->line_height,
		};
		UI_DrawText(line_string, box->font, text_pos, UI_AlignH_Left, UI_AlignV_Upper, UI_WHITE, &box->computed_rect);
	}
}

UI_API void UI_AddLabelWrapped(UI_Box* box, UI_Size w, UI_Size h, UI_BoxFlags flags, STR_View string) {
	UI_AddBox(box, w, h, flags);
	box->text = string;
	box->font = UI_STATE.base_font;
	box->inner_padding = UI_DEFAULT_TEXT_PADDING;
	box->compute_unexpanded_size = UI_AddLabelWrappedComputeUnexpandedSize;
	box->draw = UI_DrawBoxWithTextWrapped;
}

#endif // UI_IMPLEMENTATION