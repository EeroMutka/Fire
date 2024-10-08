// TODO:
// - it'd be nice to have menus which display some internal variables of fire-UI / things you can use, such as hover state
// - data table editor

typedef struct {
	UI_Key key;
	const char* name;
	UI_Text text;
	bool show;
} UIDemoTreeSpecie;

typedef struct UIDemoState {
	DS_Arena* persist;

	UI_Key deepest_hovered_root;
	UI_Key deepest_hovered_root_new;

	UI_Text dummy_text;

	DS_DynArray(UIDemoTreeSpecie) trees;
} UIDemoState;

static void UIDemoAddTreeSpecie(UIDemoState* state, UI_Key key, const char* name, const char* information) {
	UIDemoTreeSpecie tree = {0};
	tree.name = name;
	tree.show = false;
	tree.key = key;
	UI_TextInitC(state->persist, &tree.text, information);
	DS_ArrPush(&state->trees, tree);
}

static UI_Box* UIDemoTopBarButton(UI_Key key, UI_Size w, UI_Size h, const char* string) {
	return UI_AddBoxWithTextC(key, w, h, UI_BoxFlag_Clickable | UI_BoxFlag_Selectable, string);
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

	UI_TextInitC(persist, &state->dummy_text, "Strawberry");
}

static bool UIDemoOrderedDropdownShouldClose(UIDemoState* state, UI_Key dropdown_key) {
	if (UI_PrevFrameBoxFromKey(dropdown_key) != NULL) {
		// If we click anywhere in the UI that has been created already during this frame, we want to close this dropdown.
		if (UI_InputWasPressed(UI_Input_MouseLeft) && UI_BoxFromKey(state->deepest_hovered_root) != NULL) {
			return true;
		}
	}
	return false;
}

static void UIDemoRegisterOrderedRoot(UIDemoState* state, UI_Box* dropdown) {
	if (UI_IsMouseInsideOf(dropdown->key)) {
		state->deepest_hovered_root_new = dropdown->key;
	}
	if (state->deepest_hovered_root != dropdown->key) {
		dropdown->flags |= UI_BoxFlag_NoHover;
	}
}

static void UIDemoBuild(UIDemoState* state, UI_Vec2 window_size) {
	const UI_Vec2 area_child_padding = {12.f, 12.f};

	UI_Box* top_bar = NULL;
	UI_Box* file_button = NULL;
	static bool file_dropdown_is_open = false;

	state->deepest_hovered_root = state->deepest_hovered_root_new;
	state->deepest_hovered_root_new = UI_INVALID_KEY;

	//// Top bar ///////////////////////////////////////////////

	UI_Box* top_bar_root = UI_MakeRootBox(UI_KEY(), window_size.x, UI_SizeFit(), 0);
	UIDemoRegisterOrderedRoot(state, top_bar_root);
	
	UI_PushBox(top_bar_root);

	{
		top_bar = UI_AddBox(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Horizontal | UI_BoxFlag_DrawTransparentBackground);
		UI_PushBox(top_bar);

		file_button = UIDemoTopBarButton(UI_KEY(), UI_SizeFit(), UI_SizeFit(), "File");
		if (UI_Pressed(file_button->key)) {
			file_dropdown_is_open = true;
		}

		if (UI_Clicked(UIDemoTopBarButton(UI_KEY(), UI_SizeFit(), UI_SizeFit(), "Help")->key)) {
			printf("No help for you today, sorry.\n");
		}

		UI_PopBox(top_bar);
	}

	UI_PopBox(top_bar_root);
	UI_BoxComputeRects(top_bar_root, UI_VEC2{ 0, 0 });
	UI_DrawBox(top_bar_root);

	//// Main area /////////////////////////////////////////////

	UI_Vec2 main_area_size = { window_size.x, window_size.y - top_bar_root->computed_expanded_size.y };
	UI_Box* main_area = UI_MakeRootBox(UI_KEY(), main_area_size.x, main_area_size.y, UI_BoxFlag_DrawBorder);
	main_area->inner_padding = area_child_padding;
	
	if (UI_IsMouseInsideOf(main_area->key)) state->deepest_hovered_root_new = main_area->key;
	if (state->deepest_hovered_root != main_area->key) main_area->flags |= UI_BoxFlag_NoHover;
	
	UI_PushBox(main_area);
	UI_Box* main_scroll_area = UI_PushScrollArea(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFlex(1.f), 0, 0, 0);

	UI_Box* click_me = UI_AddButtonC(UI_KEY(), UI_SizeFit(), UI_SizeFit(), 0, "Click me!");
	if (UI_Clicked(click_me->key)) {
		printf("Button says thanks you!\n");
	}

	UI_AddBox(UI_KEY(), 0.f, 5.f, 0); // padding

	UI_Box* another_button = UI_AddButtonC(UI_KEY(), UI_SizeFit(), UI_SizeFit(), 0, "Another button");

	UI_AddBox(UI_KEY(), 0.f, 5.f, 0); // padding

	UI_AddBoxWithTextWrapped(UI_KEY(), UI_SizeFlex(1.f), 20.f, 0, STR_V("The quick brown fox jumped over the lazy dog. The quick brown fox jumped over the lazy dog. The quick brown fox jumped over the lazy dog."));

	UI_AddBox(UI_KEY(), 0.f, 5.f, 0); // padding

	{
		UI_Box* row = UI_AddBox(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Horizontal);
		UI_PushBox(row);
		static int my_int = 8281;
		UI_AddBoxWithTextC(UI_KEY(), UI_SizeFit(), UI_SizeFit(), 0, "Edit int: ");
		UI_AddValInt(UI_KEY(), UI_SizeFit(), UI_SizeFit(), &my_int);
		UI_PopBox(row);
	}

	UI_AddBox(UI_KEY(), 0.f, 5.f, 0); // padding

	{
		UI_Box* row = UI_AddBox(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Horizontal);
		UI_PushBox(row);
		static float my_float = 320.5f;
		UI_AddBoxWithTextC(UI_KEY(), UI_SizeFit(), UI_SizeFit(), 0, "Edit float: ");
		UI_AddValFloat(UI_KEY(), UI_SizeFit(), UI_SizeFit(), &my_float);
		UI_PopBox(row);
	}

	UI_AddBox(UI_KEY(), 0.f, 5.f, 0); // padding

	{
		UI_Box* row = UI_AddBox(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Horizontal);
		UI_PushBox(row);
		UI_AddBoxWithTextC(UI_KEY(), UI_SizeFit(), UI_SizeFit(), 0, "Enter text: ");
		UI_AddValText(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit(), &state->dummy_text);
		UI_PopBox(row);
	}

	UI_AddBox(UI_KEY(), 0.f, 5.f, 0); // padding

	{
		UI_Box* row = UI_AddBox(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Horizontal);
		UI_PushBox(row);
		UI_AddBoxWithTextC(UI_KEY(), UI_SizeFit(), UI_SizeFit(), 0, "A bunch of checkboxes:");

		static bool checkboxes[5];
		for (int i = 0; i < 5; i++) UI_AddCheckbox(UI_KEY1(i), &checkboxes[i]);

		UI_PopBox(row);
	}

	UI_AddBox(UI_KEY(), 0.f, 5.f, 0); // padding

	{
		UI_Box* row = UI_AddBox(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Horizontal | UI_BoxFlag_DrawBorder);
		UI_PushBox(row);
		if (UI_Clicked(UI_AddButtonC(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit(), 0, "Flex button")->key))          printf("Pressed flexible button!\n");
		if (UI_Clicked(UI_AddButtonC(UI_KEY(), UI_SizeFit(), UI_SizeFit(), 0, "Fit button")->key))               printf("Pressed fitting button!\n");
		if (UI_Clicked(UI_AddButtonC(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit(), 0, "Another flex button")->key))  printf("Pressed another flexible button!\n");
		if (UI_Clicked(UI_AddButtonC(UI_KEY(), UI_SizeFit(), UI_SizeFit(), 0, "Another fit button")->key))       printf("Pressed another fitting button!\n");
		UI_PopBox(row);
	}

	UI_AddBox(UI_KEY(), 0.f, 5.f, 0); // padding

	UI_Box* color_picker_section = UI_PushCollapsingC(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit(), 0.f, UI_BoxFlag_DrawBorder, "Color editing");
	if (color_picker_section) {
		color_picker_section->inner_padding = area_child_padding;

		static float hue = 0.4f, saturation = 1.f, value = 0.8f, alpha = 1.f;
		UI_ColorPicker(UI_KEY(), &hue, &saturation, &value, &alpha);

		UI_AddBox(UI_KEY(), 0.f, 20.f, 0); // padding

		UI_Box* colorful_text = UI_AddBoxWithTextC(UI_KEY(), UI_SizeFit(), UI_SizeFit(), 0, "This text is very colorful!");
		colorful_text->draw_args = UI_DrawBoxDefaultArgsInit();
		colorful_text->draw_args->text_color = UI_HSVToColor(hue, saturation, value, alpha);

		UI_PopCollapsing(color_picker_section);
	}

	UI_AddBox(UI_KEY(), 0.f, 5.f, 0); // padding

	UI_Box* arrangers_section = UI_PushCollapsingC(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit(), 0.f, UI_BoxFlag_DrawBorder, "Rearrangeable elements");
	if (arrangers_section) {
		arrangers_section->inner_padding = area_child_padding;

		UI_AddBoxWithTextC(UI_KEY(), UI_SizeFit(), UI_SizeFit(), 0, "And here we have some useful tree facts.");

		UI_Box* arrangers = UI_PushArrangerSet(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit());

		for (int i = 0; i < state->trees.count; i++) {
			UIDemoTreeSpecie* tree = &state->trees.data[i];

			UI_Box* tree_box = UI_AddBox(UI_KEY1(tree->key),
				UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Horizontal | UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawOpaqueBackground);
			UI_PushBox(tree_box);

			UI_AddArranger(UI_KEY1(tree->key), UI_SizeFit(), UI_SizeFit());

			UI_AddCheckbox(UI_KEY1(tree->key), &tree->show);
			UI_AddBoxWithTextC(UI_KEY1(tree->key), UI_SizeFit(), UI_SizeFit(), 0, tree->name);

			if (tree->show) {
				UI_Box* box = UI_AddBox(UI_KEY1(tree->key),
					UI_SizeFit(), UI_SizeFit(), UI_BoxFlag_DrawBorder | UI_BoxFlag_DrawOpaqueBackground);
				UI_PushBox(box);

				UI_AddBoxWithTextC(UI_KEY1(tree->key), UI_SizeFit(), UI_SizeFit(), 0, "Tree fact:");

				//static UI_Key editing_text = UI_INVALID_KEY;
				//static UI_Selection edit_text_selection;

				UI_Key key = UI_KEY1(tree->key);
				//bool was_editing_this = editing_text == key;

				//UI_EditTextRequest edit_request;
				//bool editing_this = was_editing_this;
				UI_AddValText(key, UI_SizeFit(), UI_SizeFit(), &tree->text);
				//UI_ApplyEditTextRequest(&tree->text, &edit_request);

				//if (editing_this) editing_text = key;
				//else if (was_editing_this) editing_text = UI_INVALID_KEY;

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

	UI_AddBox(UI_KEY(), 0.f, 5.f, 0); // padding

	UI_Box* scrollable_section = UI_PushCollapsingC(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit(), 0.f, UI_BoxFlag_DrawBorder, "Scrollable areas");
	if (scrollable_section) {
		scrollable_section->inner_padding = area_child_padding;

		static int button_count = 10;
		static float area_size = 100.f;
		UI_AddFmt(UI_KEY(), "Button count: %!d", &button_count);
		UI_AddFmt(UI_KEY(), "Area size: %!f", &area_size);
		
		UI_AddBox(UI_KEY(), 0.f, 5.f, 0); // padding

		UI_Box* scroll_area = UI_PushScrollArea(UI_KEY(), UI_SizeFlex(1.f), area_size, UI_BoxFlag_DrawBorder, 0, 0);

		//const char* lorem_ipsum_lines[] = {
		//	"Lorem ipsum dolor sit amet, consectetur adipiscing elit.",
		//	"In sagittis in enim a aliquam.",
		//	"Curabitur congue metus volutpat mi accumsan, ac dapibus augue euismod.",
		//	"Praesent nec est mollis quam feugiat tincidunt.",
		//	"Mauris ut ipsum tristique, commodo enim eu, consectetur odio.",
		//	"Mauris eget consectetur risus.",
		//	"Curabitur aliquam orci laoreet tortor varius feugiat.",
		//	"Phasellus dapibus laoreet imperdiet.",
		//	"Pellentesque at molestie lectus.",
		//};

		for (int i = 0; i < button_count; i++) {
			STR_View button_text = STR_Form(UI_FrameArena(), "Button %d", i);
			UI_Box* button = UI_AddButton(UI_KEY1(i), UI_SizeFit(), UI_SizeFit(), 0, button_text);
			if (UI_Clicked(button->key)) {
				printf("Button %d was clicked!\n", i);
			}
		}

		UI_PopScrollArea(scroll_area);
		UI_PopCollapsing(scrollable_section);
	}

	UI_AddBox(UI_KEY(), 0.f, 5.f, 0); // padding

	UI_AddBoxWithTextC(UI_KEY(), UI_SizeFit(), UI_SizeFit(), 0, "Some more text.");

	UI_AddBox(UI_KEY(), 0.f, UI_SizeFlex(1.f), 0); // padding

	UI_AddBoxWithTextC(UI_KEY(), UI_SizeFit(), UI_SizeFit(), 0, "Above this text is some flexy padding. Try resizing the window!");

	UI_PopScrollArea(main_scroll_area);
	UI_PopBox(main_area);
	UI_BoxComputeRects(main_area, UI_VEC2{ 0.f, top_bar_root->computed_expanded_size.y });
	UI_DrawBox(main_area);
	
	//// Dropdown menus ////////////////////////////////////////

	UI_Key file_dropdown_key = UI_KEY();
	file_dropdown_is_open = file_dropdown_is_open && !UIDemoOrderedDropdownShouldClose(state, file_dropdown_key);

	if (file_dropdown_is_open) {
		static bool nested_dropdown_is_open = false;

		UI_BoxFlags dropdown_window_flags = UI_BoxFlag_DrawOpaqueBackground | UI_BoxFlag_DrawBorder;

		UI_Box* dropdown = UI_MakeRootBox(file_dropdown_key, UI_SizeFit(), UI_SizeFit(), dropdown_window_flags);
		UIDemoRegisterOrderedRoot(state, dropdown);
		
		UI_PushBox(dropdown);

		if (UI_Clicked(UI_AddButtonC(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit(), 0, "Say Hello!")->key)) {
			printf("Hello!\n");
		}

		static bool curious_checkbox_active;
		static int curious_checkbox_state = 3;

		bool curious_checkbox_active_before = curious_checkbox_active;
		{
			UI_Box* row = UI_AddBox(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_Horizontal);
			UI_PushBox(row);
			UI_AddBoxWithTextC(UI_KEY(), UI_SizeFit(), UI_SizeFit(), 0, "Curious checkbox");

			UI_AddCheckbox(UI_KEY(), &curious_checkbox_active);

			UI_PopBox(row);
		}

		if (curious_checkbox_active) {
			const char* states[] = { ":|", ":)", ":|", ":(" };

			UI_AddBoxWithTextC(UI_KEY(), UI_SizeFit(), UI_SizeFit(), 0, states[curious_checkbox_state]);
			if (!curious_checkbox_active_before) {
				curious_checkbox_state = (curious_checkbox_state + 1) % DS_ArrayCount(states);
			}
		}

		UI_Box* nested_dropdown_button = UI_AddButtonC(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit(), 0, "Open nested dropdown:");
		if (UI_Pressed(nested_dropdown_button->key)) {
			nested_dropdown_is_open = true;
		}

		if (UI_Clicked(UI_AddButtonC(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFlex(1.f), 0, "Exit Program")->key)) {
			exit(0);
		}

		UI_PopBox(dropdown);

		UI_BoxComputeRects(dropdown, UI_VEC2{ file_button->computed_position.x, file_button->computed_rect.max.y });
		UI_DrawBox(dropdown);

		UI_Key nested_dropdown_key = UI_KEY();
		nested_dropdown_is_open = nested_dropdown_is_open && !UIDemoOrderedDropdownShouldClose(state, nested_dropdown_key);
		
		if (nested_dropdown_is_open) {
			UI_Box* nested_dropdown = UI_MakeRootBox(nested_dropdown_key, UI_SizeFit(), UI_SizeFit(), dropdown_window_flags);
			UIDemoRegisterOrderedRoot(state, nested_dropdown);

			UI_PushBox(nested_dropdown);
			UI_AddButtonC(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit(), 0, "Do nothing (1)");
			UI_AddButtonC(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit(), 0, "Do nothing (2)");
			UI_AddButtonC(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit(), 0, "Do nothing (3)");
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
