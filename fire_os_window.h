// fire_os_window.h - by Eero Mutka (https://eeromutka.github.io/)
//
// OS window creation and keyboard/mouse input. Only Windows is supported for time being.
// 
// This code is released under the MIT license (https://opensource.org/licenses/MIT).
//
// If you wish to use a different prefix than OS_, simply do a find and replace in this file.
//

#ifndef FIRE_OS_WINDOW_INCLUDED
#define FIRE_OS_WINDOW_INCLUDED

#ifndef OS_WINDOW_API
#define OS_WINDOW_API
#endif

#include <stdint.h>
#include <stdbool.h>
#include <assert.h> // TODO: get rid of this

typedef enum OS_Key {
	OS_Key_Invalid = 0,

	OS_Key_Space = 32,
	OS_Key_Apostrophe = 39,   /* ' */
	OS_Key_Comma = 44,        /* , */
	OS_Key_Minus = 45,        /* - */
	OS_Key_Period = 46,       /* . */
	OS_Key_Slash = 47,        /* / */

	OS_Key_0 = 48,
	OS_Key_1 = 49,
	OS_Key_2 = 50,
	OS_Key_3 = 51,
	OS_Key_4 = 52,
	OS_Key_5 = 53,
	OS_Key_6 = 54,
	OS_Key_7 = 55,
	OS_Key_8 = 56,
	OS_Key_9 = 57,

	OS_Key_Semicolon = 59,    /* ; */
	OS_Key_Equal = 61,        /* = */
	OS_Key_LeftBracket = 91,  /* [ */
	OS_Key_Backslash = 92,    /* \ */
	OS_Key_RightBracket = 93, /* ] */
	OS_Key_GraveAccent = 96,  /* ` */

	OS_Key_A = 65,
	OS_Key_B = 66,
	OS_Key_C = 67,
	OS_Key_D = 68,
	OS_Key_E = 69,
	OS_Key_F = 70,
	OS_Key_G = 71,
	OS_Key_H = 72,
	OS_Key_I = 73,
	OS_Key_J = 74,
	OS_Key_K = 75,
	OS_Key_L = 76,
	OS_Key_M = 77,
	OS_Key_N = 78,
	OS_Key_O = 79,
	OS_Key_P = 80,
	OS_Key_Q = 81,
	OS_Key_R = 82,
	OS_Key_S = 83,
	OS_Key_T = 84,
	OS_Key_U = 85,
	OS_Key_V = 86,
	OS_Key_W = 87,
	OS_Key_X = 88,
	OS_Key_Y = 89,
	OS_Key_Z = 90,

	OS_Key_Escape = 256,
	OS_Key_Enter = 257,
	OS_Key_Tab = 258,
	OS_Key_Backspace = 259,
	OS_Key_Insert = 260,
	OS_Key_Delete = 261,
	OS_Key_Right = 262,
	OS_Key_Left = 263,
	OS_Key_Down = 264,
	OS_Key_Up = 265,
	OS_Key_PageUp = 266,
	OS_Key_PageDown = 267,
	OS_Key_Home = 268,
	OS_Key_End = 269,
	OS_Key_CapsLock = 280,
	OS_Key_ScrollLock = 281,
	OS_Key_NumLock = 282,
	OS_Key_PrintScreen = 283,
	OS_Key_Pause = 284,

	OS_Key_F1 = 290,
	OS_Key_F2 = 291,
	OS_Key_F3 = 292,
	OS_Key_F4 = 293,
	OS_Key_F5 = 294,
	OS_Key_F6 = 295,
	OS_Key_F7 = 296,
	OS_Key_F8 = 297,
	OS_Key_F9 = 298,
	OS_Key_F10 = 299,
	OS_Key_F11 = 300,
	OS_Key_F12 = 301,
	OS_Key_F13 = 302,
	OS_Key_F14 = 303,
	OS_Key_F15 = 304,
	OS_Key_F16 = 305,
	OS_Key_F17 = 306,
	OS_Key_F18 = 307,
	OS_Key_F19 = 308,
	OS_Key_F20 = 309,
	OS_Key_F21 = 310,
	OS_Key_F22 = 311,
	OS_Key_F23 = 312,
	OS_Key_F24 = 313,
	OS_Key_F25 = 314,

	//OS_Key_KP_0 = 320,
	//OS_Key_KP_1 = 321,
	//OS_Key_KP_2 = 322,
	//OS_Key_KP_3 = 323,
	//OS_Key_KP_4 = 324,
	//OS_Key_KP_5 = 325,
	//OS_Key_KP_6 = 326,
	//OS_Key_KP_7 = 327,
	//OS_Key_KP_8 = 328,
	//OS_Key_KP_9 = 329,

	//OS_Key_KP_Decimal = 330,
	//OS_Key_KP_Divide = 331,
	//OS_Key_KP_Multiply = 332,
	//OS_Key_KP_Subtract = 333,
	//OS_Key_KP_Add = 334,
	//OS_Key_KP_Enter = 335,
	//OS_Key_KP_Equal = 336,

	OS_Key_LeftShift = 340,
	OS_Key_LeftControl = 341,
	OS_Key_LeftAlt = 342,
	OS_Key_LeftSuper = 343,
	OS_Key_RightShift = 344,
	OS_Key_RightControl = 345,
	OS_Key_RightAlt = 346,
	OS_Key_RightSuper = 347,
	//OS_Key_Menu = 348,

	OS_Key_MouseLeft = 353,
	OS_Key_MouseRight = 354,
	OS_Key_MouseMiddle = 355,
	//OS_Key_Mouse_4 = 356,
	//OS_Key_Mouse_5 = 357,
	//OS_Key_Mouse_6 = 358,
	//OS_Key_Mouse_7 = 359,
	//OS_Key_Mouse_8 = 360,

	OS_Key_COUNT,
} OS_Key;

typedef enum OS_MouseCursor {
	OS_MouseCursor_Arrow,
	OS_MouseCursor_Hand,
	OS_MouseCursor_I_Beam,
	OS_MouseCursor_Crosshair,
	OS_MouseCursor_ResizeH,
	OS_MouseCursor_ResizeV,
	OS_MouseCursor_ResizeNESW,
	OS_MouseCursor_ResizeNWSE,
	OS_MouseCursor_ResizeAll,
	OS_MouseCursor_COUNT
} OS_MouseCursor;

typedef struct OS_Window {
	void* handle; // OS_specific window handle. On windows (which is currently the only supported target), it's represented as HWND.
	
	OS_MouseCursor current_cursor;
	void* current_cursor_handle; // HCURSOR on windows

	bool mouse_is_hidden;
	int32_t mouse_hidden_pos[2];

	bool should_close;

	bool key_is_down[OS_Key_COUNT];
	
	bool queue_release_next_key;
	int queue_release_next_key_idx;

	struct {
		uint32_t left, top, right, bottom;
	} pre_fullscreen_state; // When calling OS_SetWindowFullscreen, some information about the previous window state needs to be saved in these fields
} OS_Window;

typedef enum OS_EventKind {
	OS_EventKind_Press,
	OS_EventKind_Release,
	OS_EventKind_TextCharacter,
	OS_EventKind_MouseWheel,
	OS_EventKind_RawMouseInput,
} OS_EventKind;

typedef struct OS_Event {
	OS_EventKind kind;
	OS_Key key; // for Press and Release events
	bool is_repeat; // for Press events
	uint8_t mouse_click_index; // 0 by default. 1 for double-click, 2 for triple-click
	uint32_t text_character; // Unicode character
	float mouse_wheel; // for MouseWheel event; +1.0 means the wheel was rotated forward by one detent (scroll step)
	float raw_mouse_input[2]; // for RawMouseInput event
} OS_Event;

typedef void(*OS_OnResizeFn)(uint32_t width, uint32_t height, void* user_data);

// After calling `OS_CreateWindowHidden`, the window will remain hidden.
// To show the window, you must explicitly call `OS_ShowWindow`. This separation is there so that you
// can get rid of the initial flicker that normally happens when you create a window, then do some work
// such as initialize a graphics API, and finally present a frame. If you instead first call `create`,
// then initialize the graphics API, and only then call `show`, there won't be any flicker as the
// window doesn't have to wait for the initialization.
OS_WINDOW_API OS_Window OS_CreateWindow(uint32_t width, uint32_t height, const char* name);
OS_WINDOW_API OS_Window OS_CreateWindowHidden(uint32_t width, uint32_t height, const char* name);
OS_WINDOW_API void OS_ShowWindow(OS_Window* window);

OS_WINDOW_API bool OS_SetWindowFullscreen(OS_Window* window, bool fullscreen);

// * Returns false when the window is closed.
// * The reason for having `on_resize` as a callback is that when resizing, Windows doesn't
//   let us exit the event loop until the user lets go of the mouse. This means that if you want to draw into the window
//   while resizing to make it feel smooth, you have to do it from a callback internally.
// * on_resize may be NULL
// * inputs is a stateful data structure and should be initialized and kept around across multiple frames. TODO: refactor this and return an array of events instead.
OS_WINDOW_API bool OS_PollEvent(OS_Window* window, OS_Event* event, OS_OnResizeFn on_resize, void* user_data);

OS_WINDOW_API bool OS_WindowShouldClose(OS_Window* window);

OS_WINDOW_API void OS_GetMousePosition(OS_Window* window, float* x, float* y);

OS_WINDOW_API void OS_SetMouseCursor(OS_Window* window, OS_MouseCursor cursor);
OS_WINDOW_API void OS_SetMouseCursorLockAndHide(OS_Window* window, bool lock_and_hide);

#ifdef /********************/ FIRE_OS_WINDOW_IMPLEMENTATION /********************/

#pragma comment(lib, "User32.lib")

#include <Windows.h>

OS_WINDOW_API OS_Window OS_CreateWindow(uint32_t width, uint32_t height, const char* name) {
	OS_Window window = OS_CreateWindowHidden(width, height, name);
	OS_ShowWindow(&window);
	return window;
}

#define OS_WINDOW_CLASS_NAME L"OS_WindowModuleClassName"

typedef struct OS_WindowProcUserData {
	OS_Window* window;
	OS_Event* event;
	OS_OnResizeFn on_resize;
	void* user_data;

	bool has_event;
	bool got_kill_focus;
} OS_WindowProcUserData;

static OS_Key OS_KeyFromVK(uint64_t vk, uint16_t scancode) {
	OS_Key key = OS_Key_Invalid;
	if ((vk >= OS_Key_0 && vk <= OS_Key_9) || (vk >= OS_Key_A && vk <= OS_Key_Z)) {
		key = (OS_Key)vk;
	}
	else if (vk >= VK_F1 && vk <= VK_F24) {
		key = (OS_Key)(OS_Key_F1 + (vk - VK_F1));
	}
	else switch (vk) {
	case VK_SPACE: key = OS_Key_Space; break;
	case VK_OEM_7: key = OS_Key_Apostrophe; break;
	case VK_OEM_COMMA: key = OS_Key_Comma; break;
	case VK_OEM_MINUS: key = OS_Key_Minus; break;
	case VK_OEM_PERIOD: key = OS_Key_Period; break;
	case VK_OEM_2: key = OS_Key_Slash; break;
	case VK_OEM_1: key = OS_Key_Semicolon; break;
	case VK_OEM_PLUS: key = OS_Key_Equal; break;
	case VK_OEM_4: key = OS_Key_LeftBracket; break;
	case VK_OEM_5: key = OS_Key_Backslash; break;
	case VK_OEM_6: key = OS_Key_RightBracket; break;
	case VK_OEM_3: key = OS_Key_GraveAccent; break;
	case VK_ESCAPE: key = OS_Key_Escape; break;
	case VK_RETURN: key = OS_Key_Enter; break;
	case VK_TAB: key = OS_Key_Tab; break;
	case VK_BACK: key = OS_Key_Backspace; break;
	case VK_INSERT: key = OS_Key_Insert; break;
	case VK_DELETE: key = OS_Key_Delete; break;
	case VK_RIGHT: key = OS_Key_Right; break;
	case VK_LEFT: key = OS_Key_Left; break;
	case VK_DOWN: key = OS_Key_Down; break;
	case VK_UP: key = OS_Key_Up; break;
	case VK_PRIOR: key = OS_Key_PageUp; break;
	case VK_NEXT: key = OS_Key_PageDown; break;
	case VK_HOME: key = OS_Key_Home; break;
	case VK_END: key = OS_Key_End; break;
	case VK_CAPITAL: key = OS_Key_CapsLock; break;
	case VK_NUMLOCK: key = OS_Key_NumLock; break;
	case VK_SNAPSHOT: key = OS_Key_PrintScreen; break;
	case VK_PAUSE: key = OS_Key_Pause; break;
	case VK_LWIN: key = OS_Key_LeftSuper; break;
	case VK_RWIN: key = OS_Key_RightSuper; break;
	case VK_SHIFT: {
		key = (uint16_t)MapVirtualKeyW(scancode, MAPVK_VSC_TO_VK_EX) == VK_LSHIFT ? OS_Key_LeftShift : OS_Key_RightShift;
	} break;
	case VK_MENU: {
		key = (uint16_t)MapVirtualKeyW(scancode, MAPVK_VSC_TO_VK_EX) == VK_LMENU ? OS_Key_LeftAlt : OS_Key_RightAlt;
	} break;
	case VK_CONTROL: {
		key = (uint16_t)MapVirtualKeyW(scancode, MAPVK_VSC_TO_VK_EX) == VK_LCONTROL ? OS_Key_LeftControl : OS_Key_RightControl;
	} break;

	default: break;
	}
	return key;
}

static bool OS_AddKeyEvent(OS_Window* window, OS_Event* event, OS_EventKind kind, bool is_repeat, uint8_t mouse_click_index, OS_Key key) {
	bool press = kind == OS_EventKind_Press;

	bool generate_event = false;
	if (key != OS_Key_Invalid) {
		// Only generate the event if it matches our expected state
		if (press) {
			generate_event = !window->key_is_down[key] || is_repeat;
		} else {
			generate_event = window->key_is_down[key];
		}

		event->kind = kind;
		event->key = key;
		event->is_repeat = is_repeat;
		event->mouse_click_index = mouse_click_index;
		window->key_is_down[key] = press;
	}

	return generate_event;
}

static LRESULT __stdcall OS_WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	OS_WindowProcUserData* passed = (OS_WindowProcUserData*)GetWindowLongPtrW(hWnd, GWLP_USERDATA);

	LRESULT result = 0;
	if (passed) {
		OS_Event* event = passed->event;
		OS_Window* window = passed->window;

		switch (uMsg) {
		default: {
			result = DefWindowProcW(hWnd, uMsg, wParam, lParam);
		} break;
			//case WM_SETFOCUS:
		case WM_CLOSE: // fallthrough
		case WM_QUIT: {
			window->should_close = true;
		} break;
		case WM_KILLFOCUS: {
			passed->got_kill_focus = true;
		} break;
		case WM_SYSKEYDOWN: // fallthrough
		case WM_SYSKEYUP: // fallthrough
		case WM_KEYDOWN: // fallthrough
		case WM_KEYUP: {
			bool is_repeat = (lParam & (1 << 30)) != 0;
			OS_EventKind kind = uMsg == WM_KEYUP || uMsg == WM_SYSKEYUP ? OS_EventKind_Release : OS_EventKind_Press;

			// https://learn.microsoft.com/en-us/windows/win32/inputdev/about-keyboard-input#keystroke-message-flags
			uint16_t scancode = LOBYTE(HIWORD(lParam));
			bool is_extended_key = (HIWORD(lParam) & KF_EXTENDED) == KF_EXTENDED;
			if (is_extended_key) scancode |= 0xE000;

			OS_Key key = OS_KeyFromVK(wParam, scancode);
			passed->has_event = OS_AddKeyEvent(window, event, kind, is_repeat, 0, key);

			result = DefWindowProcW(hWnd, uMsg, wParam, lParam);
		} break;

		case WM_LBUTTONDOWN: // fallthrough
		case WM_RBUTTONDOWN: // fallthrough
		case WM_LBUTTONDBLCLK: // fallthrough
		case WM_MBUTTONDOWN: {
			OS_Key key = OS_Key_Invalid;
			uint8_t mouse_click_index = 0;
			if (uMsg == WM_LBUTTONDOWN) key = OS_Key_MouseLeft;
			if (uMsg == WM_RBUTTONDOWN) key = OS_Key_MouseRight;
			if (uMsg == WM_MBUTTONDOWN) key = OS_Key_MouseMiddle;
			if (uMsg == WM_LBUTTONDBLCLK) {
				key = OS_Key_MouseLeft;
				mouse_click_index = 1;
			}
			passed->has_event = OS_AddKeyEvent(window, event, OS_EventKind_Press, false, mouse_click_index, key);

			SetCapture(hWnd);

			result = DefWindowProcW(hWnd, uMsg, wParam, lParam);
		} break;
		case WM_LBUTTONUP: // fallthrough
		case WM_RBUTTONUP: // fallthrough
		case WM_MBUTTONUP: {
			OS_Key key = OS_Key_Invalid;
			if (uMsg == WM_LBUTTONUP) key = OS_Key_MouseLeft;
			if (uMsg == WM_RBUTTONUP) key = OS_Key_MouseRight;
			if (uMsg == WM_MBUTTONUP) key = OS_Key_MouseMiddle;
			passed->has_event = OS_AddKeyEvent(window, event, OS_EventKind_Release, false, 0, key);

			if (!window->key_is_down[OS_Key_MouseLeft] &&
				!window->key_is_down[OS_Key_MouseRight] &&
				!window->key_is_down[OS_Key_MouseMiddle])
			{
				ReleaseCapture();
			}
		} break;
		case WM_CHAR: {
			// wParam contains the character as a 32-bit unicode codepoint.
			if (wParam >= 32 && wParam != 127 /* ignore the DEL character on ctrl-backspace */) {
				passed->has_event = true;
				event->kind = OS_EventKind_TextCharacter;
				event->text_character = (uint32_t)wParam;
			}
		} break;
		case WM_SIZE: {
			// "The low-order word of lParam specifies the new width of the client area."
			// "The high-order word of lParam specifies the new height of the client area."
			uint32_t width = (uint32_t)(lParam) & 0xFFFF;
			uint32_t height = (uint32_t)(lParam >> 16);
			if (passed->on_resize) {
				passed->on_resize(width, height, passed->user_data);
			}
		} break;
		case WM_MOUSEWHEEL: {
			passed->has_event = true;
			event->kind = OS_EventKind_MouseWheel;

			int16_t wheel = (int16_t)(wParam >> 16);
			event->mouse_wheel = (float)wheel / (float)WHEEL_DELTA;
		} break;
		case WM_INPUT: {
			RAWINPUT raw_input = {0};
			uint32_t dwSize = sizeof(RAWINPUT);
			GetRawInputData((HRAWINPUT)lParam, RID_INPUT, &raw_input, &dwSize, sizeof(RAWINPUTHEADER));

			if (raw_input.header.dwType == RIM_TYPEMOUSE &&
				!(raw_input.data.mouse.usFlags & MOUSE_MOVE_RELATIVE) &&
				!(raw_input.data.mouse.usFlags & MOUSE_MOVE_ABSOLUTE /* Drawing tables may give absolute mouse inputs which we want to ignore */ ))
			{
				passed->has_event = true;
				event->kind = OS_EventKind_RawMouseInput;
				event->raw_mouse_input[0] = (float)raw_input.data.mouse.lLastX;
				event->raw_mouse_input[1] = (float)raw_input.data.mouse.lLastY;
			}
		} break;
		case WM_SETCURSOR: {
			if (window->current_cursor_handle && LOWORD(lParam) == HTCLIENT) {
				SetCursor((HCURSOR)window->current_cursor_handle);
			} else {
				result = DefWindowProcW(hWnd, uMsg, wParam, lParam);
			}
		} break;
		}
	}
	else {
		result = DefWindowProcW(hWnd, uMsg, wParam, lParam);
	}
	return result;
}

static void OS_RegisterWindowClass() {
	// Windows requires us to first register a "window class" with a given name,
	// which will be used in subsequent calls to CreateWindowExW()

	HINSTANCE hInst = GetModuleHandleW(NULL);

	// leave the background brush to NULL
	// https://stackoverflow.com/questions/6593014/how-to-draw-opengl-content-while-resizing-win32-window

	WNDCLASSEXW wnd_class = {0};
	wnd_class.cbSize = sizeof(WNDCLASSEXW);
	wnd_class.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS; // CS_OWNDC is required for OpenGL
	wnd_class.lpfnWndProc = OS_WindowProc;
	wnd_class.cbClsExtra = 0;
	wnd_class.cbWndExtra = 0;
	wnd_class.hInstance = hInst;
	wnd_class.hIcon = NULL;
	wnd_class.hCursor = LoadCursorW(0, (const wchar_t*)IDC_ARROW);
	wnd_class.hbrBackground = NULL;
	wnd_class.lpszMenuName = NULL;
	wnd_class.lpszClassName = OS_WINDOW_CLASS_NAME;
	wnd_class.hIconSm = NULL;

	uint64_t atom = RegisterClassExW(&wnd_class);
	assert(atom != 0);
}

OS_WINDOW_API OS_Window OS_CreateWindowHidden(uint32_t width, uint32_t height, const char* name) {
	// register raw input
	// https://learn.microsoft.com/en-us/windows/win32/inputdev/using-raw-input

	RAWINPUTDEVICE input_devices[1];
	input_devices[0].usUsagePage = 0x01;          // HID_USAGE_PAGE_GENERIC
	input_devices[0].usUsage = 0x02;              // HID_USAGE_GENERIC_MOUSE
	input_devices[0].dwFlags = 0/*RIDEV_NOLEGACY*/;    // adds mouse and also ignores legacy mouse messages
	input_devices[0].hwndTarget = 0;
	bool ok = RegisterRawInputDevices(input_devices, sizeof(input_devices) / sizeof(input_devices[0]), sizeof(RAWINPUTDEVICE)) == 1;
	assert(ok);

	// NOTE: When you use a DPI scale on windows that's not 1, the window that we get back from `CreateWindowExW`
	// has an incorrect size that's not what we ask for.
	// Calling `SetProcessDPIAware` seems to resolve this issue, at least for the single monitor case.
	// 
	// TODO: fix for multiple monitors
	// https://stackoverflow.com/questions/71300163/how-to-create-a-window-with-createwindowex-but-ignoring-the-scale-settings-on-wi
	ok = SetProcessDPIAware() == 1;
	assert(ok);

	// TODO: multiple windows?
	OS_RegisterWindowClass();

	int32_t x = 200;
	int32_t y = 200;
	RECT rect = { x, y, x + (int32_t)width, y + (int32_t)height };

	// AdjustWindowRect modifies the window rectangle: we give it the client area rectangle, it gives us back the entire window rectangle.
	AdjustWindowRectEx(&rect, WS_OVERLAPPEDWINDOW, 0, 0);

	wchar_t name_wide[256];
	MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, name, -1, name_wide, 256);

	HWND hwnd = CreateWindowExW(0,
		OS_WINDOW_CLASS_NAME,
		name_wide,
		WS_OVERLAPPEDWINDOW,
		rect.left,
		rect.top,
		rect.right - rect.left,
		rect.bottom - rect.top,
		0, 0, 0, 0);
	assert(hwnd != 0);

	OS_Window result = {0};
	result.handle = hwnd;
	return result;
}

OS_WINDOW_API void OS_ShowWindow(OS_Window* window) {
	bool ok = UpdateWindow((HWND)window->handle) != 0;
	assert(ok);
	ShowWindow((HWND)window->handle, SW_SHOW);
}

OS_WINDOW_API bool OS_SetWindowFullscreen(OS_Window* window, bool fullscreen) {
	// https://devblogs.microsoft.com/oldnewthing/20100412-00/?p=14353

	bool ok = true;
	int32_t style = GetWindowLongW((HWND)window->handle, GWL_STYLE);

	if (fullscreen) {
		HMONITOR monitor = MonitorFromWindow((HWND)window->handle, MONITOR_DEFAULTTONEAREST);

		MONITORINFO info;
		info.cbSize = sizeof(MONITORINFO);
		ok = GetMonitorInfoW(monitor, &info) == 1;
		if (ok) {
			SetWindowLongW((HWND)window->handle, GWL_STYLE, style & ~WS_OVERLAPPEDWINDOW);

			RECT old_rect;
			GetWindowRect((HWND)window->handle, &old_rect);

			window->pre_fullscreen_state.left = old_rect.left;
			window->pre_fullscreen_state.top = old_rect.top;
			window->pre_fullscreen_state.right = old_rect.right;
			window->pre_fullscreen_state.bottom = old_rect.bottom;

			int32_t x = info.rcMonitor.left;
			int32_t y = info.rcMonitor.top;
			int32_t w = info.rcMonitor.right - x;
			int32_t h = info.rcMonitor.bottom - y;
			SetWindowPos((HWND)window->handle, HWND_TOPMOST, x, y, w, h, SWP_NOZORDER | SWP_NOACTIVATE | SWP_FRAMECHANGED);
		}
	}
	else {
		SetWindowLongW((HWND)window->handle, GWL_STYLE, style | WS_OVERLAPPEDWINDOW);

		int32_t x = window->pre_fullscreen_state.left;
		int32_t y = window->pre_fullscreen_state.top;
		int32_t w = window->pre_fullscreen_state.right - x;
		int32_t h = window->pre_fullscreen_state.bottom - y;
		SetWindowPos((HWND)window->handle, HWND_TOP, x, y, w, h, SWP_FRAMECHANGED);
	}
	return ok;
}

// * Returns false when the window is closed.
// * The reason for having `on_resize` as a callback is that when resizing, Windows doesn't
//   let us exit the event loop until the user lets go of the mouse. This means that if you want to draw into the window
//   while resizing to make it feel smooth, you have to do it from a callback internally.
// * on_resize may be NULL
// * inputs is a stateful data structure and should be initialized and kept around across multiple frames. TODO: refactor this and return an array of events instead.
OS_WINDOW_API bool OS_PollEvent(OS_Window* window, OS_Event* event, OS_OnResizeFn on_resize, void* user_data) {
	if (window->queue_release_next_key) {
		bool found_held_key = false;
		for (int i = window->queue_release_next_key_idx; i < OS_Key_COUNT; i++) {
			if (window->key_is_down[i]) {
				window->key_is_down[i] = false;
				found_held_key = true;
				event->kind = OS_EventKind_Release;
				event->key = (OS_Key)i;
				window->queue_release_next_key_idx = i + 1;
				break;
			}
		}
		window->queue_release_next_key = found_held_key;
		if (found_held_key) return true;
	}

	OS_WindowProcUserData passed = {0};
	passed.window = window;
	passed.event = event;
	passed.on_resize = on_resize;
	passed.user_data = user_data;
	memset(event, 0, sizeof(*event));
	SetWindowLongPtrW((HWND)window->handle, GWLP_USERDATA, (int64_t)&passed);

	// Maybe-todo: investigate https://twitter.com/DefenceForceOrg/status/1719342491902583131

	MSG msg = {0};
	for (; PeekMessageW(&msg, (HWND)window->handle, 0, 0, PM_REMOVE) != 0;) {
		TranslateMessage(&msg);
		DispatchMessageW(&msg);
		//if (msg.message == WM_QUIT) {
		//	window->should_close = true;
		//	break;
		//}
		if (passed.got_kill_focus || passed.has_event) break;
	}

	if (passed.got_kill_focus) {
		window->queue_release_next_key = true;
		window->queue_release_next_key_idx = 0;
		passed.has_event = OS_PollEvent(window, event, on_resize, user_data);
	}

	SetWindowLongPtrW((HWND)window->handle, GWLP_USERDATA, 0);

	return passed.has_event;
}

OS_WINDOW_API bool OS_WindowShouldClose(OS_Window* window) {
	return window->should_close;
}

OS_WINDOW_API void OS_GetMousePosition(OS_Window* window, float* x, float* y) {
	POINT cursor_pos;
	GetCursorPos(&cursor_pos);
	ScreenToClient((HWND)window->handle, &cursor_pos);
	*x = (float)cursor_pos.x;
	*y = (float)cursor_pos.y;
}

OS_WINDOW_API void OS_SetMouseCursor(OS_Window* window, OS_MouseCursor cursor) {
	if (cursor != window->current_cursor) {
		wchar_t* cursor_name = NULL;
		switch (cursor) {
		case OS_MouseCursor_Arrow: cursor_name = (wchar_t*)IDC_ARROW; break;
		case OS_MouseCursor_Hand: cursor_name = (wchar_t*)IDC_HAND; break;
		case OS_MouseCursor_I_Beam: cursor_name = (wchar_t*)IDC_IBEAM; break;
		case OS_MouseCursor_Crosshair: cursor_name = (wchar_t*)IDC_CROSS; break;
		case OS_MouseCursor_ResizeH: cursor_name = (wchar_t*)IDC_SIZEWE; break;
		case OS_MouseCursor_ResizeV: cursor_name = (wchar_t*)IDC_SIZENS; break;
		case OS_MouseCursor_ResizeNESW: cursor_name = (wchar_t*)IDC_SIZENESW; break;
		case OS_MouseCursor_ResizeNWSE: cursor_name = (wchar_t*)IDC_SIZENWSE; break;
		case OS_MouseCursor_ResizeAll: cursor_name = (wchar_t*)IDC_SIZEALL; break;
		case OS_MouseCursor_COUNT: break;
		}
		window->current_cursor_handle = LoadCursorW(NULL, cursor_name);
		window->current_cursor = cursor;
	}
}

OS_WINDOW_API void OS_SetMouseCursorLockAndHide(OS_Window* window, bool lock_and_hide) {
	if (lock_and_hide) {
		if (!window->mouse_is_hidden) {
			POINT point;
			GetCursorPos(&point);
			window->mouse_hidden_pos[0] = point.x;
			window->mouse_hidden_pos[1] = point.y;
		}
		
		SetCursorPos(window->mouse_hidden_pos[0], window->mouse_hidden_pos[1]);
		if (!window->mouse_is_hidden) {
			ShowCursor(0);
			window->mouse_is_hidden = true;
		}
	}
	else {
		if (window->mouse_is_hidden) {
			ShowCursor(1);
			window->mouse_is_hidden = false;
		}
	}
}

#endif // FIRE_OS_WINDOW_IMPLEMENTATION
#endif // FIRE_OS_WINDOW_INCLUDED