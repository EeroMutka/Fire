// fire_os_window.h - by Eero Mutka (https://eeromutka.github.io/)
// 
// Clipboard utility library. Only Windows is supported for time being.
//
// This code is released under the MIT license (https://opensource.org/licenses/MIT).
//
// If you wish to use a different prefix than OS_CLIPBOARD_, simply do a find and replace in this file.
//

#ifndef FIRE_OS_CLIPBOARD_INCLUDED
#define FIRE_OS_CLIPBOARD_INCLUDED

#ifndef OS_CLIPBOARD_API
#define OS_CLIPBOARD_API static
#endif

typedef void* (*OS_CLIPBOARD_AllocFn)(int size, void* alloc_data);

// Assumes UTF-8
OS_CLIPBOARD_API bool OS_CLIPBOARD_GetText(char** out_data, int* out_size, OS_CLIPBOARD_AllocFn alloc, void* alloc_data);

// Assumes UTF-8
OS_CLIPBOARD_API bool OS_CLIPBOARD_SetText(const char* data, int size);


#ifdef /**********/ FIRE_OS_CLIPBOARD_IMPLEMENTATION /**********/

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

OS_CLIPBOARD_API bool OS_CLIPBOARD_GetText(char** out_data, int* out_size, OS_CLIPBOARD_AllocFn alloc, void* alloc_data) {
	char* data = NULL;
	int size = 0;
	bool ok = OpenClipboard(NULL);
	if (ok) {
		HANDLE hData = GetClipboardData(CF_UNICODETEXT);
		wchar_t* data_wide = (wchar_t*)GlobalLock(hData);

		size = WideCharToMultiByte(CP_UTF8, 0, data_wide, -1, NULL, 0, NULL, NULL);
		ok = size > 0;
		if (ok) {
			data = (char*)alloc(size, alloc_data);
			WideCharToMultiByte(CP_UTF8, 0, data_wide, -1, data, size, NULL, NULL);
			size -= 1; // `WideCharToMultiByte` returned size includes null termination
		}

		GlobalUnlock(hData);
		CloseClipboard();
	}

	*out_data = data;
	*out_size = size;
	return ok;
}

OS_CLIPBOARD_API bool OS_CLIPBOARD_SetText(const char* data, int size) {
	bool ok = OpenClipboard(NULL);
	if (ok) {
		int size_wide = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, data, size, NULL, 0);
		
		HANDLE clipbuffer = GlobalAlloc(GMEM_MOVEABLE, size_wide*2 + 2);
		wchar_t* buffer = (wchar_t*)GlobalLock(clipbuffer);
		
		MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, data, size, buffer, size_wide);
		buffer[size_wide] = 0;
		
		GlobalUnlock(clipbuffer);
		
		EmptyClipboard();
		ok = SetClipboardData(CF_UNICODETEXT, clipbuffer) != 0;
		CloseClipboard();
	}
	return ok;
}

#endif // FIRE_OS_CLIPBOARD_IMPLEMENTATION
#endif // FIRE_OS_CLIPBOARD_INCLUDED