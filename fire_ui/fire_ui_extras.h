
typedef void (*UI_ArrayEditElemFn)(UI_Key key, void* array, int index);

typedef struct UI_ValueEditArrayModify {
	bool append_to_end;
	bool clear;
	int remove_elem; // -1 if none
} UI_ValueEditArrayModify;

static void UI_ValueEditArrayScrollAreaComputeUnexpandedSize(UI_Box* box, UI_Axis axis) {
	UI_BoxComputeUnexpandedSizeDefault(box, axis);
	if (axis == UI_Axis_Y) {
		box->computed_unexpanded_size.y = UI_Min(box->computed_unexpanded_size.y, 200.f);
	}
}

static void UI_AddValueEditArray(UI_Key key, STR name, void* array, int array_count, UI_ArrayEditElemFn edit_elem, UI_ValueEditArrayModify* out_modify) {
	UI_Key child_box_key = UI_KEY1(key);
	UI_BoxFlags flags = UI_BoxFlag_LayoutInX | UI_BoxFlag_Clickable | UI_BoxFlag_Selectable | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawTransparentBackground;
	UI_Box* header = UI_AddBox(key, UI_SizeFlex(1.f), UI_SizeFit(), flags);
	UI_PushBox(header);

	bool is_open = UI_PrevFrameBoxFromKey(child_box_key) != NULL;
	if (UI_PressedIdle(header->key)) {
		is_open = !is_open;
	}

	UI_Box* label = UI_AddBoxWithText(UI_KEY1(key), UI_SizePx(20.f), UI_SizeFit(), 0, is_open ? STR_("\x44") : STR_("\x46"));
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
		scroll_area->compute_unexpanded_size_override = UI_ValueEditArrayScrollAreaComputeUnexpandedSize;

		UI_Box* source_files_arrangers = UI_PushArrangerSet(UI_KEY1(key), UI_SizeFlex(1.f), UI_SizeFit());

		UI_Key elems_key = UI_KEY1(key);
		for (int i = 0; i < array_count; i++) {
			UI_Key elem_key = UI_HashInt(elems_key, i);
			UI_Box* elem_box = UI_AddBox(UI_KEY1(elem_key), UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_LayoutInX|UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawOpaqueBackground);
			UI_PushBox(elem_box);

			UI_Box* arranger = UI_AddArranger(UI_KEY1(elem_key), UI_SizeFit(), UI_SizeFit());
			arranger->text = STR_FormV(UI_FrameArena(), "%d.", i);

			edit_elem(UI_KEY1(elem_key), array, i);
		
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
}

static void UI_AddValueEditDSArray(UI_Key key, STR name, DS_DynArrayRaw* array, UI_ArrayEditElemFn edit_elem) {
	UI_ValueEditArrayModify modify;
	UI_AddValueEditArray(key, name, array->data, array->length, edit_elem, &modify);
}