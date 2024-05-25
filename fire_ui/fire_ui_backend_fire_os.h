// This file is part of the Fire UI library, see "fire_ui.h"

static STR UI_OS_GetClipboardString(void* user_data) {
	STR string = OS_ClipboardGetText(UI_FrameArena());
	return string;
}

static void UI_OS_SetClipboardString(STR string, void* user_data) {
	OS_ClipboardSetText(string);
}

static void UI_OS_ResetFrameInputs(OS_Window* window, UI_Inputs* ui_inputs, UI_Font* base_font, UI_Font* icons_font) {
	memset(ui_inputs, 0, sizeof(*ui_inputs));
	ui_inputs->base_font = base_font;
	ui_inputs->icons_font = icons_font;
	ui_inputs->get_clipboard_string_fn = UI_OS_GetClipboardString;
	ui_inputs->set_clipboard_string_fn = UI_OS_SetClipboardString;
	
	float mouse_x, mouse_y;
	OS_WindowGetMousePosition(window, &mouse_x, &mouse_y);
	ui_inputs->mouse_position = UI_VEC2{mouse_x, mouse_y};
}

// let's imagine our fire-os implementation is super basic, like SDL. fire-UI should accomodate.
static void UI_OS_RegisterInputEvent(UI_Inputs* ui_inputs, const OS_Event* event) {
	if (event->kind == OS_EventKind_Press || event->kind == OS_EventKind_Release) {
		UI_Input ui_input = UI_Input_Invalid;
		switch (event->key) {
		default: break;
		case OS_Key_MouseLeft: { ui_input = UI_Input_MouseLeft; } break;
		case OS_Key_MouseRight: { ui_input = UI_Input_MouseRight; } break;
		case OS_Key_MouseMiddle: { ui_input = UI_Input_MouseMiddle; } break;
		case OS_Key_LeftShift: { ui_input = UI_Input_Shift; } break;
		case OS_Key_RightShift: { ui_input = UI_Input_Shift; } break;
		case OS_Key_LeftControl: { ui_input = UI_Input_Control; } break;
		case OS_Key_RightControl: { ui_input = UI_Input_Control; } break;
		case OS_Key_LeftAlt: { ui_input = UI_Input_Alt; } break;
		case OS_Key_RightAlt: { ui_input = UI_Input_Alt; } break;
		case OS_Key_Tab: { ui_input = UI_Input_Tab; } break;
		case OS_Key_Escape: { ui_input = UI_Input_Escape; } break;
		case OS_Key_Enter: { ui_input = UI_Input_Enter; } break;
		case OS_Key_Delete: { ui_input = UI_Input_Delete; } break;
		case OS_Key_Backspace: { ui_input = UI_Input_Backspace; } break;
		case OS_Key_A: { ui_input = UI_Input_A; } break;
		case OS_Key_C: { ui_input = UI_Input_C; } break;
		case OS_Key_V: { ui_input = UI_Input_V; } break;
		case OS_Key_X: { ui_input = UI_Input_X; } break;
		case OS_Key_Y: { ui_input = UI_Input_Y; } break;
		case OS_Key_Z: { ui_input = UI_Input_Z; } break;
		case OS_Key_Home: { ui_input = UI_Input_Home; } break;
		case OS_Key_End: { ui_input = UI_Input_End; } break;
		case OS_Key_Left: { ui_input = UI_Input_Left; } break;
		case OS_Key_Right: { ui_input = UI_Input_Right; } break;
		case OS_Key_Up: { ui_input = UI_Input_Up; } break;
		case OS_Key_Down: { ui_input = UI_Input_Down; } break;
		}
		if (ui_input) {
			if (event->kind == OS_EventKind_Press) {
				ui_inputs->input_events[ui_input] |= UI_InputEvent_PressOrRepeat;
				if (!event->is_repeat) ui_inputs->input_events[ui_input] |= UI_InputEvent_Press;
			}
			else {
				ui_inputs->input_events[ui_input] |= UI_InputEvent_Release;
			}
		}
	}
	if (event->kind == OS_EventKind_MouseWheel) {
		ui_inputs->mouse_wheel_delta += event->mouse_wheel;
	}
	if (event->kind == OS_EventKind_RawMouseInput) {
		ui_inputs->mouse_raw_delta.x += event->raw_mouse_input[0];
		ui_inputs->mouse_raw_delta.y += event->raw_mouse_input[1];
	}
	if (event->kind == OS_EventKind_TextCharacter) {
		UI_CHECK(ui_inputs->text_input_utf32_length < UI_ArrayCount(ui_inputs->text_input_utf32));
		ui_inputs->text_input_utf32[ui_inputs->text_input_utf32_length] = event->text_character;
		ui_inputs->text_input_utf32_length++;
	}
}

static void UI_OS_ApplyOutputs(const UI_Outputs* outputs) {
	OS_SetMouseCursorLockAndHide(outputs->lock_and_hide_cursor);

	switch (outputs->cursor) {
	case UI_MouseCursor_Default: OS_SetMouseCursor(OS_MouseCursor_Arrow); break;
	case UI_MouseCursor_ResizeH: OS_SetMouseCursor(OS_MouseCursor_ResizeH); break;
	case UI_MouseCursor_ResizeV: OS_SetMouseCursor(OS_MouseCursor_ResizeV); break;
	case UI_MouseCursor_I_beam:  OS_SetMouseCursor(OS_MouseCursor_I_Beam); break;
	}
}
