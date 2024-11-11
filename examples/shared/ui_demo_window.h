
struct UIDemoTreeSpecie {
	UI_Key key;
	STR_View name;
	UI_Text text;
	bool show;
};

struct UIDemoState {
	DS_Arena* persist;

	UI_Key deepest_hovered_root;
	UI_Key deepest_hovered_root_new;
	bool has_added_deepest_hovered_root;

	UI_Text dummy_text;

	DS_DynArray(UIDemoTreeSpecie) trees;
};

static void UIDemoAddTreeSpecie(UIDemoState* state, UI_Key key, STR_View name, STR_View information) {
	UIDemoTreeSpecie tree = {0};
	tree.name = name;
	tree.show = false;
	tree.key = key;
	UI_TextInit(state->persist, &tree.text, information);
	DS_ArrPush(&state->trees, tree);
}

static void UIDemoTopBarButton(UI_Box* box, UI_Size w, UI_Size h, STR_View string) {
	return UI_AddLabel(box, w, h, UI_BoxFlag_Clickable | UI_BoxFlag_Selectable, string);
}

static void UIDemoInit(UIDemoState* state, DS_Arena* persist) {
	memset(state, 0, sizeof(*state));
	state->persist = persist;

	DS_ArrInit(&state->trees, persist);
	UIDemoAddTreeSpecie(state, UI_KEY(), "Pine", "Pine trees can live up to 1000 years.");
	UIDemoAddTreeSpecie(state, UI_KEY(), "Oak", "Oak is commonly used in construction and furniture.");
	UIDemoAddTreeSpecie(state, UI_KEY(), "Maple", "Maple trees are typically 10 to 45 meters tall.");
	UIDemoAddTreeSpecie(state, UI_KEY(), "Birch", "Birch is most commonly found in the Northern hemisphere.");
	UIDemoAddTreeSpecie(state, UI_KEY(), "Cedar", "Cedar wood is a natural repellent to moths.");

	UI_TextInit(persist, &state->dummy_text, "Strawberry");
}

static void UIDemoUpdateDropdownState(UIDemoState* state, bool* is_open, UI_Box* button) {
	if (*is_open) {
		// If we click anywhere in the UI that has been added during this frame, we want to close this dropdown.
		*is_open = !(UI_InputWasPressed(UI_Input_MouseLeft) && state->has_added_deepest_hovered_root);
	}
	else if (UI_Pressed(button)) {
		*is_open = true;
	}
}

static void UIDemoRegisterRoot(UIDemoState* state, UI_Box* area) {
	if (UI_IsMouseInsideOf(area)) {
		state->deepest_hovered_root_new = area->key;
	}
	if (state->deepest_hovered_root == area->key) {
		state->has_added_deepest_hovered_root = true;
	}
	else {
		area->flags |= UI_BoxFlag_NoHover;
	}
}

static void UIDemoBuild(UIDemoState* state, UI_Vec2 window_size) {
	const UI_Vec2 area_child_padding = {12.f, 12.f};

	UI_Box* top_bar = UI_BOX();
	UI_Box* file_button = UI_BOX();
	
	state->deepest_hovered_root = state->deepest_hovered_root_new;
	state->deepest_hovered_root_new = UI_INVALID_KEY;
	state->has_added_deepest_hovered_root = false;
	
	//// Top bar ///////////////////////////////////////////////

	UI_Box* top_bar_root = UI_BOX();
	UI_InitRootBox(top_bar_root, window_size.x, UI_SizeFit(), 0);
	UIDemoRegisterRoot(state, top_bar_root);

	UI_PushBox(top_bar_root);

	{
		UI_AddBox(top_bar, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Horizontal | UI_BoxFlag_DrawTransparentBackground);
		UI_PushBox(top_bar);

		UIDemoTopBarButton(file_button, UI_SizeFit(), UI_SizeFit(), "File");
		
		UI_Box* help_button = UI_BOX();
		UIDemoTopBarButton(help_button, UI_SizeFit(), UI_SizeFit(), "Help");
		if (UI_Clicked(help_button)) {
			printf("No help for you today, sorry.\n");
		}

		UI_PopBox(top_bar);
	}

	UI_PopBox(top_bar_root);
	UI_BoxComputeRects(top_bar_root, UI_VEC2{ 0, 0 });
	UI_DrawBox(top_bar_root);

	//// Main area /////////////////////////////////////////////

	UI_Vec2 main_area_size = { window_size.x, window_size.y - top_bar_root->computed_expanded_size.y };
	UI_Box* main_area = UI_BOX();
	UI_InitRootBox(main_area, main_area_size.x, main_area_size.y, UI_BoxFlag_DrawBorder);
	UIDemoRegisterRoot(state, main_area);
	main_area->inner_padding = area_child_padding;

	UI_PushBox(main_area);
	UI_Box* main_scroll_area = UI_BOX();
	UI_PushScrollArea(main_scroll_area, UI_SizeFlex(1.f), UI_SizeFlex(1.f), 0, 0, 0);

	UI_Box* click_me = UI_BOX();
	UI_AddButton(click_me, UI_SizeFit(), UI_SizeFit(), 0, "Button 1");
	if (UI_Clicked(click_me)) {
		printf("\"Yay, thanks for clicking me!\", said Button 1.\n");
	}

	UI_AddBox(UI_BOX(), 0.f, 5.f, 0); // padding

	UI_Box* another_button = UI_BOX();
	UI_AddButton(another_button, UI_SizeFit(), UI_SizeFit(), 0, "Button 2");
	if (UI_Clicked(another_button)) {
		printf("\"Ouch, that hurt.\", said Button 2\n");
	}

	UI_AddBox(UI_BOX(), 0.f, 5.f, 0); // padding

	UI_AddLabelWrapped(UI_BOX(), UI_SizeFlex(1.f), 20.f, 0, "The quick brown fox jumped over the lazy dog. The quick brown fox jumped over the lazy dog. The quick brown fox jumped over the lazy dog.");

	UI_AddBox(UI_BOX(), 0.f, 5.f, 0); // padding

	{
		UI_Box* row = UI_BOX();
		UI_AddBox(row, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Horizontal);
		UI_PushBox(row);
		static int my_int = 8281;
		UI_AddLabel(UI_BOX(), UI_SizeFit(), UI_SizeFit(), 0, "Edit int: ");
		UI_AddValInt(UI_BOX(), UI_SizeFit(), UI_SizeFit(), &my_int);
		UI_PopBox(row);
	}

	UI_AddBox(UI_BOX(), 0.f, 5.f, 0); // padding

	{
		UI_Box* row = UI_BOX();
		UI_AddBox(row, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Horizontal);
		UI_PushBox(row);
		static float my_float = 320.5f;
		UI_AddLabel(UI_BOX(), UI_SizeFit(), UI_SizeFit(), 0, "Edit float: ");
		UI_AddValFloat(UI_BOX(), UI_SizeFit(), UI_SizeFit(), &my_float);
		UI_PopBox(row);
	}

	UI_AddBox(UI_BOX(), 0.f, 5.f, 0); // padding

	{
		UI_Box* row = UI_BOX();
		UI_AddBox(row, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Horizontal);
		UI_PushBox(row);
		UI_AddLabel(UI_BOX(), UI_SizeFit(), UI_SizeFit(), 0, "Enter text: ");
		UI_AddValText(UI_BOX(), UI_SizeFlex(1.f), UI_SizeFit(), &state->dummy_text);
		UI_PopBox(row);
	}

	UI_AddBox(UI_BOX(), 0.f, 5.f, 0); // padding

	{
		UI_Box* row = UI_BOX();
		UI_AddBox(row, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Horizontal);
		UI_PushBox(row);
		UI_AddLabel(UI_BOX(), UI_SizeFit(), UI_SizeFit(), 0, "A bunch of checkboxes:");

		static bool checkboxes[5];
		for (int i = 0; i < 5; i++) {
			UI_Key key = UI_HashInt(UI_KEY(), i);
			UI_AddCheckbox(UI_KBOX(key), &checkboxes[i]);
		}

		UI_PopBox(row);
	}

	UI_AddBox(UI_BOX(), 0.f, 5.f, 0); // padding

	{
		UI_Box* row = UI_BOX();
		UI_AddBox(row, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Horizontal | UI_BoxFlag_DrawBorder);
		row->inner_padding.x = 10.f;
		row->inner_padding.y = 10.f;
		UI_PushBox(row);

		UI_Box* button1 = UI_BOX();
		UI_Box* button2 = UI_BOX();
		UI_Box* button3 = UI_BOX();
		UI_Box* button4 = UI_BOX();
		
		UI_AddButton(button1, UI_SizeFlex(1.f), UI_SizeFit(), 0, "Flex button");
		UI_AddButton(button2, UI_SizeFit(), UI_SizeFit(), 0, "Fit button");
		UI_AddButton(button3, UI_SizeFlex(1.f), UI_SizeFit(), 0, "Another flex button");
		UI_AddButton(button4, UI_SizeFit(), UI_SizeFit(), 0, "Another fit button");

		if (UI_Clicked(button1)) printf("Pressed flexible button!\n");
		if (UI_Clicked(button2)) printf("Pressed fitting button!\n");
		if (UI_Clicked(button3)) printf("Pressed another flexible button!\n");
		if (UI_Clicked(button4)) printf("Pressed another fitting button!\n");
		
		UI_PopBox(row);
	}

	UI_AddBox(UI_BOX(), 0.f, 5.f, 0); // padding

	UI_Box* color_picker_section = UI_BOX();
	if (UI_PushCollapsing(color_picker_section, UI_SizeFlex(1.f), UI_SizeFit(), 0.f, UI_BoxFlag_DrawBorder, "Color editing")) {
		color_picker_section->inner_padding = area_child_padding;

		static float hue = 0.4f, saturation = 1.f, value = 0.8f, alpha = 1.f;
		UI_AddColorPicker(UI_BOX(), &hue, &saturation, &value, &alpha);

		UI_AddBox(UI_BOX(), 0.f, 20.f, 0); // padding

		UI_Box* colorful_text = UI_BOX();
		UI_AddLabel(colorful_text, UI_SizeFit(), UI_SizeFit(), 0, "This text is very colorful!");
		colorful_text->draw_args = UI_DrawBoxDefaultArgsInit();
		colorful_text->draw_args->text_color = UI_HSVToColor(hue, saturation, value, alpha);

		UI_PopCollapsing(color_picker_section);
	}

	UI_AddBox(UI_BOX(), 0.f, 5.f, 0); // padding

	UI_Box* arrangers_section = UI_BOX();
	if (UI_PushCollapsing(arrangers_section, UI_SizeFlex(1.f), UI_SizeFit(), 0.f, UI_BoxFlag_DrawBorder, "Rearrangeable elements")) {
		arrangers_section->inner_padding = area_child_padding;

		UI_AddLabel(UI_BOX(), UI_SizeFit(), UI_SizeFit(), 0, "And here we have some useful tree facts.");

		UI_Box* arrangers = UI_BOX();
		UI_PushArrangerSet(arrangers, UI_SizeFlex(1.f), UI_SizeFit());

		for (int i = 0; i < state->trees.count; i++) {
			UIDemoTreeSpecie* tree = &state->trees.data[i];
			
			UI_Box* tree_box = UI_KBOX(tree->key);
			UI_AddBox(tree_box, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Horizontal | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawOpaqueBackground);
			UI_PushBox(tree_box);

			UI_AddArranger(UI_BBOX(tree_box), UI_SizeFit(), UI_SizeFit());

			UI_AddCheckbox(UI_BBOX(tree_box), &tree->show);
			UI_AddLabel(UI_BBOX(tree_box), UI_SizeFit(), UI_SizeFit(), 0, tree->name);

			if (tree->show) {
				UI_Box* box = UI_BBOX(tree_box);
				UI_AddBox(box, UI_SizeFit(), UI_SizeFit(), UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawOpaqueBackground);
				UI_PushBox(box);

				UI_AddLabel(UI_BBOX(tree_box), UI_SizeFit(), UI_SizeFit(), 0, "Tree fact:");
				UI_AddValText(UI_BBOX(tree_box), UI_SizeFit(), UI_SizeFit(), &tree->text);
				
				UI_PopBox(box);
			}

			UI_PopBox(tree_box);
		}

		UI_ArrangersRequest edit_request;
		UI_PopArrangerSet(arrangers, &edit_request);

		if (edit_request.move_from != edit_request.move_to) {
			UIDemoTreeSpecie moved = DS_ArrGet(state->trees, edit_request.move_from);
			DS_ArrRemove(&state->trees, edit_request.move_from);
			DS_ArrInsert(&state->trees, edit_request.move_to, moved);
		}

		UI_PopCollapsing(arrangers_section);
	}

	UI_AddBox(UI_BOX(), 0.f, 5.f, 0); // padding

	UI_Box* scrollable_section = UI_BOX();
	if (UI_PushCollapsing(scrollable_section, UI_SizeFlex(1.f), UI_SizeFit(), 0.f, UI_BoxFlag_DrawBorder, "Scrollable areas")) {
		scrollable_section->inner_padding = area_child_padding;

		static int button_count = 10;
		static float area_size = 100.f;
		UI_AddFmt(UI_BOX(), "Button count: %!d", &button_count);
		UI_AddFmt(UI_BOX(), "Area size: %!f", &area_size);

		UI_AddBox(UI_BOX(), 0.f, 5.f, 0); // padding

		UI_Box* scroll_area = UI_BOX();
		UI_PushScrollArea(scroll_area, UI_SizeFlex(1.f), area_size, UI_BoxFlag_DrawBorder, 0, 0);

		for (int i = 0; i < button_count; i++) {
			STR_View button_text = STR_Form(UI_FrameArena(), "Button %d", i);
			UI_Box* button = UI_KBOX(UI_HashInt(UI_KEY(), i));
			UI_AddButton(button, UI_SizeFit(), UI_SizeFit(), 0, button_text);
			if (UI_Clicked(button)) {
				printf("Button %d was clicked!\n", i);
			}
		}

		UI_PopScrollArea(scroll_area);
		UI_PopCollapsing(scrollable_section);
	}

	UI_AddBox(UI_BOX(), 0.f, 5.f, 0); // padding

	UI_AddLabel(UI_BOX(), UI_SizeFit(), UI_SizeFit(), 0, "Some more text.");

	UI_AddBox(UI_BOX(), 0.f, UI_SizeFlex(1.f), 0); // padding

	UI_AddLabel(UI_BOX(), UI_SizeFit(), UI_SizeFit(), 0, "Above this text is some flexy padding. Try resizing the window!");

	UI_PopScrollArea(main_scroll_area);
	UI_PopBox(main_area);
	UI_BoxComputeRects(main_area, UI_VEC2{ 0.f, top_bar_root->computed_expanded_size.y });
	UI_DrawBox(main_area);

	//// Dropdown menus ////////////////////////////////////////

	static bool file_dropdown_is_open = false;
	UIDemoUpdateDropdownState(state, &file_dropdown_is_open, file_button);

	if (file_dropdown_is_open) {
		UI_BoxFlags dropdown_window_flags = UI_BoxFlag_DrawOpaqueBackground | UI_BoxFlag_DrawBorder;

		UI_Box* file_dropdown = UI_BOX();
		UI_InitRootBox(file_dropdown, UI_SizeFit(), UI_SizeFit(), dropdown_window_flags);
		UIDemoRegisterRoot(state, file_dropdown);

		UI_PushBox(file_dropdown);

		UI_Box* hello_button = UI_BOX();
		UI_AddButton(hello_button, UI_SizeFlex(1.f), UI_SizeFit(), 0, "Say Hello!");
		if (UI_Clicked(hello_button)) {
			printf("Hello!\n");
		}

		static bool curious_checkbox_active;
		static int curious_checkbox_state = 3;
		
		bool curious_checkbox_active_before = curious_checkbox_active;
		{
			UI_Box* row = UI_BOX();
			UI_AddBox(row, UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Horizontal);
			UI_PushBox(row);
			UI_AddLabel(UI_BOX(), UI_SizeFit(), UI_SizeFit(), 0, "Curious checkbox");

			UI_AddCheckbox(UI_BOX(), &curious_checkbox_active);

			UI_PopBox(row);
		}

		if (curious_checkbox_active) {
			const char* states[] = { ":|", ":)", ":|", ":(" };

			UI_AddLabel(UI_BOX(), UI_SizeFit(), UI_SizeFit(), 0, states[curious_checkbox_state]);
			if (!curious_checkbox_active_before) {
				curious_checkbox_state = (curious_checkbox_state + 1) % DS_ArrayCount(states);
			}
		}
		
		UI_Box* nested_dropdown_button = UI_BOX();
		UI_AddButton(nested_dropdown_button, UI_SizeFlex(1.f), UI_SizeFit(), 0, "Open nested dropdown:");

		UI_Box* exit_program_button = UI_BOX();
		UI_AddButton(exit_program_button, UI_SizeFlex(1.f), UI_SizeFlex(1.f), 0, "Exit Program");
		if (UI_Clicked(exit_program_button)) {
			exit(0);
		}

		UI_PopBox(file_dropdown);

		UI_BoxComputeRects(file_dropdown, UI_VEC2{ file_button->computed_position.x, file_button->computed_rect.max.y });
		UI_DrawBox(file_dropdown);

		static bool nested_dropdown_is_open = false;
		UIDemoUpdateDropdownState(state, &nested_dropdown_is_open, nested_dropdown_button);

		if (nested_dropdown_is_open) {
			UI_Box* nested_dropdown = UI_BOX();
			UI_InitRootBox(nested_dropdown, UI_SizeFit(), UI_SizeFit(), dropdown_window_flags);
			UIDemoRegisterRoot(state, nested_dropdown);

			UI_PushBox(nested_dropdown);
			UI_AddButton(UI_BOX(), UI_SizeFlex(1.f), UI_SizeFit(), 0, "Do nothing (1)");
			UI_AddButton(UI_BOX(), UI_SizeFlex(1.f), UI_SizeFit(), 0, "Do nothing (2)");
			UI_AddButton(UI_BOX(), UI_SizeFlex(1.f), UI_SizeFit(), 0, "Do nothing (3)");
			UI_PopBox(nested_dropdown);

			UI_Vec2 pos = {
				nested_dropdown_button->computed_rect.max.x,
				nested_dropdown_button->computed_rect.min.y,
			};
			UI_BoxComputeRects(nested_dropdown, pos);
			UI_DrawBox(nested_dropdown);
		}
	}
}
