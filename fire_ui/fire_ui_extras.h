// This file contains some experimental extra utilities

typedef void (*UI_ArrayEditElemFn)(UI_Key key, void* elem, int index, void* user_data);

typedef struct UI_ValueEditArrayModify {
	bool append_to_end;
	bool clear;
	int remove_elem; // -1 if none
} UI_ValueEditArrayModify;

UI_API UI_Box* UI_AddValArray(UI_Key key, STR name, void* array, int array_count, int elem_size, UI_ArrayEditElemFn edit_elem, void* user_data, UI_ValueEditArrayModify* out_modify);

#define UI_AddValDSArray(KEY, NAME, ARRAY, DEFAULT_VALUE, EDIT_ELEM) \
	UI_AddValDSArray_((KEY), (NAME), (DS_DynArrayRaw*)(ARRAY), sizeof((ARRAY)->data[0]), (DEFAULT_VALUE), (UI_ArrayEditElemFn)EDIT_ELEM, NULL)
#define UI_AddValDSArrayEx(KEY, NAME, ARRAY, DEFAULT_VALUE, EDIT_ELEM, USER_DATA) \
	UI_AddValDSArray_((KEY), (NAME), (DS_DynArrayRaw*)(ARRAY), sizeof((ARRAY)->data[0]), (DEFAULT_VALUE), (UI_ArrayEditElemFn)EDIT_ELEM, USER_DATA)

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
UI_API void UI_AddFmt(UI_Key key, const char* fmt, ...);

#ifdef /********************/ UI_IMPLEMENTATION /********************/

static void UI_ValEditArrayScrollAreaComputeUnexpandedSize(UI_Box* box, UI_Axis axis) {
	UI_BoxComputeUnexpandedSizeDefault(box, axis);
	if (axis == UI_Axis_Y) {
		box->computed_unexpanded_size.y = UI_Min(box->computed_unexpanded_size.y, 200.f);
	}
}

UI_API UI_Box* UI_AddValArray(UI_Key key, STR name, void* array, int array_count, int elem_size, UI_ArrayEditElemFn edit_elem, void* user_data, UI_ValueEditArrayModify* out_modify)
{
	UI_Box* root_box = UI_AddBox(key, UI_SizeFlex(1.f), UI_SizeFit(), 0);
	UI_PushBox(root_box);

	UI_Key child_box_key = UI_KEY1(key);
	UI_BoxFlags flags = UI_BoxFlag_LayoutInX | UI_BoxFlag_Clickable | UI_BoxFlag_Selectable | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawTransparentBackground;
	UI_Box* header = UI_AddBox(UI_KEY1(key), UI_SizeFlex(1.f), UI_SizeFit(), flags);
	UI_PushBox(header);

	bool is_open = UI_PrevFrameBoxFromKey(child_box_key) != NULL;
	if (UI_PressedIdle(header->key)) {
		is_open = !is_open;
	}

	UI_Box* label = UI_AddBoxWithText(UI_KEY1(key), 20.f, UI_SizeFit(), 0, is_open ? STR_("\x44") : STR_("\x46"));
	label->style = UI_MakeStyle();
	label->style->font.font = UI_STATE.icons_font;

	UI_AddBoxWithText(UI_KEY1(key), UI_SizeFlex(1.f), UI_SizeFit(), 0, name);
	UI_Box* add_button = UI_AddButton(UI_KEY1(key), UI_SizeFit(), UI_SizeFlex(1.f), 0, STR_("\x48"));

	UI_Box* clear_button = UI_AddButton(UI_KEY1(key), UI_SizeFit(), UI_SizeFlex(1.f), 0, STR_("\x54"));
	clear_button->style = UI_MakeStyle();
	clear_button->style->font.font = UI_STATE.icons_font;
	clear_button->style->font.size *= 0.8f;
	clear_button->style->text_padding.y += 2.f;
	add_button->style = clear_button->style;

	UI_PopBox(header);

	if (UI_Clicked(add_button->key)) is_open = true;

	int should_remove_elem = -1;

	if (is_open) {
		UI_Box* scroll_area = UI_PushScrollArea(child_box_key, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_DrawBorder, 0, 0);
		scroll_area->compute_unexpanded_size_override = UI_ValEditArrayScrollAreaComputeUnexpandedSize;

		UI_Box* source_files_arrangers = UI_PushArrangerSet(UI_KEY1(key), UI_SizeFlex(1.f), UI_SizeFit());

		UI_Key elems_key = UI_KEY1(key);
		for (int i = 0; i < array_count; i++) {
			UI_Key elem_key = UI_HashInt(elems_key, i);
			UI_Box* elem_box = UI_AddBox(UI_KEY1(elem_key), UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_LayoutInX|UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawOpaqueBackground);
			UI_PushBox(elem_box);

			UI_Box* arranger = UI_AddArranger(UI_KEY1(elem_key), UI_SizeFit(), UI_SizeFit());
			arranger->text = STR_FormV(UI_FrameArena(), "%d.", i);

			UI_Box* user_box = UI_AddBox(UI_KEY1(elem_key), UI_SizeFlex(1.f), UI_SizeFit(), 0);
			UI_PushBox(user_box);
			edit_elem(UI_KEY1(elem_key), (char*)array + i*elem_size, i, user_data);
			UI_PopBox(user_box);

			UI_Box* remove_button = UI_AddButton(UI_KEY1(elem_key), UI_SizeFit(), UI_SizeFit(), 0, STR_("\x4a"));
			remove_button->style = clear_button->style;
			if (UI_Clicked(remove_button->key)) should_remove_elem = i;

			UI_PopBox(elem_box);
		}

		UI_ArrangersRequest edit_request;
		UI_PopArrangerSet(source_files_arrangers, &edit_request);

		UI_PopScrollArea(scroll_area);
	}

	if (out_modify) {
		out_modify->append_to_end = UI_Clicked(add_button->key);
		out_modify->clear = UI_Clicked(clear_button->key);
		out_modify->remove_elem = should_remove_elem;
	}

	UI_PopBox(root_box);
	return root_box;
}

// default_value may be NULL, in which case the element is zero-initialized
static void UI_AddValDSArray_(UI_Key key, STR name, DS_DynArrayRaw* array, int elem_size,
	const void* default_value, UI_ArrayEditElemFn edit_elem, void* user_data)
{
	UI_ValueEditArrayModify modify;
	UI_AddValArray(key, name, array->data, array->length, elem_size, edit_elem, user_data, &modify);
	
	if (modify.append_to_end) {
		DS_ArrResizeRaw(array, array->length + 1, NULL, elem_size);
		if (default_value) {
			memcpy((char*)array->data + (array->length-1) * elem_size, default_value, elem_size);
		} else {
			memset((char*)array->data + (array->length-1) * elem_size, 0, elem_size);
		}
	}
	if (modify.clear) array->length = 0;
	if (modify.remove_elem != -1) {
		DS_ArrRemoveRaw(array, modify.remove_elem, elem_size);
	}
}

static void UI_InfoFmtFinishCurrent_(UI_Key key, STR_Builder* current_string, int* section_index) {
	if (current_string->str.size > 0) {
		UI_AddBoxWithText(UI_HashInt(key, *section_index), UI_SizeFit(), UI_SizeFit(), 0, current_string->str);
	}
	current_string->str.size = 0;
	*section_index += 1;
}

UI_API void UI_AddFmt(UI_Key key, const char* fmt, ...) {
	va_list args; va_start(args, fmt);

	UI_Box* row = UI_AddBox(UI_KEY1(key), UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_LayoutInX);
	UI_PushBox(row);

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
			default: STR_CHECK(0);
				//case 's': {
				//	STR_Print(s, va_arg(args, char*));
				//} break;
				//case 'v': {
				//	STR_PrintV(s, va_arg(args, STR));
				//} break;
			case '%': {
				STR_Print(&current_string, "%");
			} break;
			case 't': {
				UI_InfoFmtFinishCurrent_(key, &current_string, &section_index);

				UI_Key val_key = UI_HashInt(key, section_index);
				if (editable) {
					UI_Text* val = va_arg(args, UI_Text*);
					UI_AddValText(val_key, UI_SizeFlex(1.f), UI_SizeFit(), val, NULL);
				} else {
					UI_Text val = va_arg(args, UI_Text);
					UI_EditTextModify modify;
					UI_AddValText(val_key, UI_SizeFlex(1.f), UI_SizeFit(), &val, &modify);
				}

				section_index++;
			} break;
			case 'b': {
				UI_InfoFmtFinishCurrent_(key, &current_string, &section_index);

				UI_Key val_key = UI_HashInt(key, section_index);
				if (editable) {
					bool* val = va_arg(args, bool*);
					UI_AddCheckbox(val_key, val);
				} else {
					bool val = va_arg(args, bool);
					UI_AddCheckbox(val_key, &val);
				}

				section_index++;
			} break;
			case 'f': {
				UI_InfoFmtFinishCurrent_(key, &current_string, &section_index);

				UI_Key val_key = UI_HashInt(key, section_index);
				if (editable) {
					if (l == 1) {
						double* val = va_arg(args, double*);
						UI_AddValFloat64(val_key, UI_SizeFlex(1.f), UI_SizeFit(), val);
					} else {
						float* val = va_arg(args, float*);
						UI_AddValFloat(val_key, UI_SizeFlex(1.f), UI_SizeFit(), val);
					}
				} else {
					double val = va_arg(args, double);
					UI_AddValFloat64(val_key, UI_SizeFlex(1.f), UI_SizeFit(), &val);
				}
				section_index++;
			} break;
				//case 'x': { radix = 16;       goto add_int; }
			case 'd': { is_signed = true; goto add_int; }
			case 'i': { is_signed = true; goto add_int; }
			case 'u': {
			add_int:;
				UI_InfoFmtFinishCurrent_(key, &current_string, &section_index);
				
				UI_Key val_key = UI_HashInt(key, section_index);
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

					UI_AddValNumeric(val_key, UI_SizeFlex(1.f), UI_SizeFit(), &val, is_signed, false);
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

					UI_AddValNumeric(val_key, UI_SizeFlex(1.f), UI_SizeFit(), &val, is_signed, false);
				}
				section_index++;
			} break;
			}
		}
		else {
			STR character_str = { c, 1 };
			STR_PrintV(&current_string, character_str);
		}
	}

	UI_InfoFmtFinishCurrent_(UI_KEY1(key), &current_string, &section_index);
	UI_PopBox(row);

	va_end(args);
}

#endif // UI_IMPLEMENTATION