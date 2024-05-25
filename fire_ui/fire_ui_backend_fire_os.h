// This file is part of the Fire UI library, see "fire_ui.h"

//static UI_InputStates UI_OS_TranslateInputStates(OS_InputStateFlags flags) {
//	UI_InputStates states = 0;
//	if (flags & OS_InputStateFlag_WasPressed)         states |= UI_InputState_WasPressed;
//	if (flags & OS_InputStateFlag_WasPressedOrRepeat) states |= UI_InputState_WasPressedOrRepeat;
//	if (flags & OS_InputStateFlag_IsDown)             states |= UI_InputState_IsDown;
//	if (flags & OS_InputStateFlag_WasReleased)        states |= UI_InputState_WasReleased;
//	return states;
//}

static STR UI_OS_GetClipboardString(void* user_data) {
	STR string = OS_ClipboardGetText((OS_Arena*)user_data);
	return string;
}

static void UI_OS_SetClipboardString(STR string, void* user_data) {
	OS_ClipboardSetText(string);
}

static void UI_OS_ResetFrameInputs(OS_Window* window, UI_Inputs* ui_inputs) {
	memset(ui_inputs, 0, sizeof(*ui_inputs));
	
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
}

#if 0
// We might want to get the inputs
// let's imagine our fire-os implementation is super basic, like SDL. fire-UI should accomodate.
static void UI_OS_TranslateInputs(UI_Inputs* ui_inputs, const OS_Inputs* os_inputs, OS_Arena* frame_arena) {
	ui_inputs->input_states[UI_Input_MouseLeft]   = UI_OS_TranslateInputStates(os_inputs->input_states[OS_Input_MouseLeft]);
	ui_inputs->input_states[UI_Input_MouseRight]  = UI_OS_TranslateInputStates(os_inputs->input_states[OS_Input_MouseRight]);
	ui_inputs->input_states[UI_Input_MouseMiddle] = UI_OS_TranslateInputStates(os_inputs->input_states[OS_Input_MouseMiddle]);
	ui_inputs->input_states[UI_Input_Shift]       = UI_OS_TranslateInputStates(os_inputs->input_states[OS_Input_Shift]);
	ui_inputs->input_states[UI_Input_Control]     = UI_OS_TranslateInputStates(os_inputs->input_states[OS_Input_Control]);
	ui_inputs->input_states[UI_Input_Alt]         = UI_OS_TranslateInputStates(os_inputs->input_states[OS_Input_Alt]);
	ui_inputs->input_states[UI_Input_Tab]         = UI_OS_TranslateInputStates(os_inputs->input_states[OS_Input_Tab]);
	ui_inputs->input_states[UI_Input_Escape]      = UI_OS_TranslateInputStates(os_inputs->input_states[OS_Input_Escape]);
	ui_inputs->input_states[UI_Input_Enter]       = UI_OS_TranslateInputStates(os_inputs->input_states[OS_Input_Enter]);
	ui_inputs->input_states[UI_Input_Delete]      = UI_OS_TranslateInputStates(os_inputs->input_states[OS_Input_Delete]);
	ui_inputs->input_states[UI_Input_Backspace]   = UI_OS_TranslateInputStates(os_inputs->input_states[OS_Input_Backspace]);
	ui_inputs->input_states[UI_Input_A]           = UI_OS_TranslateInputStates(os_inputs->input_states[OS_Input_A]);
	ui_inputs->input_states[UI_Input_C]           = UI_OS_TranslateInputStates(os_inputs->input_states[OS_Input_C]);
	ui_inputs->input_states[UI_Input_V]           = UI_OS_TranslateInputStates(os_inputs->input_states[OS_Input_V]);
	ui_inputs->input_states[UI_Input_X]           = UI_OS_TranslateInputStates(os_inputs->input_states[OS_Input_X]);
	ui_inputs->input_states[UI_Input_Y]           = UI_OS_TranslateInputStates(os_inputs->input_states[OS_Input_Y]);
	ui_inputs->input_states[UI_Input_Z]           = UI_OS_TranslateInputStates(os_inputs->input_states[OS_Input_Z]);
	ui_inputs->input_states[UI_Input_Home]        = UI_OS_TranslateInputStates(os_inputs->input_states[OS_Input_Home]);
	ui_inputs->input_states[UI_Input_End]         = UI_OS_TranslateInputStates(os_inputs->input_states[OS_Input_End]);
	ui_inputs->input_states[UI_Input_Left]        = UI_OS_TranslateInputStates(os_inputs->input_states[OS_Input_Left]);
	ui_inputs->input_states[UI_Input_Right]       = UI_OS_TranslateInputStates(os_inputs->input_states[OS_Input_Right]);
	ui_inputs->input_states[UI_Input_Up]          = UI_OS_TranslateInputStates(os_inputs->input_states[OS_Input_Up]);
	ui_inputs->input_states[UI_Input_Down]        = UI_OS_TranslateInputStates(os_inputs->input_states[OS_Input_Down]);

	ui_inputs->mouse_position = UI_VEC2{(float)os_inputs->mouse_x, (float)os_inputs->mouse_y};
	ui_inputs->mouse_raw_delta = UI_VEC2{(float)os_inputs->mouse_raw_dx, (float)os_inputs->mouse_raw_dy};
	ui_inputs->mouse_wheel_delta = os_inputs->mouse_wheel_delta;
	ui_inputs->text_input_utf32 = os_inputs->text_input_utf32;
	ui_inputs->text_input_utf32_length = os_inputs->text_input_utf32_length;

	ui_inputs->get_clipboard_string_fn = UI_OS_GetClipboardString;
	ui_inputs->set_clipboard_string_fn = UI_OS_SetClipboardString;

	ui_inputs->user_data = frame_arena;
}
#endif

static void UI_OS_ApplyOutputs(const UI_Outputs* outputs) {
	switch (outputs->cursor) {
	case UI_MouseCursor_Default: OS_SetMouseCursor(OS_MouseCursor_Arrow); break;
	case UI_MouseCursor_ResizeH: OS_SetMouseCursor(OS_MouseCursor_ResizeH); break;
	case UI_MouseCursor_ResizeV: OS_SetMouseCursor(OS_MouseCursor_ResizeV); break;
	case UI_MouseCursor_I_beam:  OS_SetMouseCursor(OS_MouseCursor_I_Beam); break;
	}

	if (outputs->lock_and_hide_cursor) {
		OS_LockAndHideMouseCursor();
	}

}
