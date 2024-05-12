// fire_build.h - build C/C++ projects from code
//
// Author: Eero Mutka
// Version: 0
// Date: 25 March, 2024
//
// This code is released under the MIT license (https://opensource.org/licenses/MIT). Some code in this file
// are also adapted from "microsoft_craziness.h" by Jonathan Blow, which is released under the MIT license.
//
// fire_build is a library that lets you build C/C++ code or generate Visual Studio projects from code.
// The idea is, you make one build.c file, use this library in it, then compile and run it to build your code.
// Or, do it from anywhere you'd like from your programs as the control is in your hands now!
//

#ifndef BUILD_INCLUDED
#define BUILD_INCLUDED

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef BUILD_API
#define BUILD_IMPLEMENTATION
#define BUILD_API static
#endif

typedef enum BUILD_Target {
	BUILD_Target_Executable,
	BUILD_Target_DynamicLibrary,
	BUILD_Target_ObjectFileOnly, // linker won't be run with this option
	// TODO: BUILD_Target_StaticLibrary,
} BUILD_Target;

typedef struct BUILD_ProjectOptions {
	BUILD_Target target;

	bool debug_info; // in MSVC, this corresponds to the /Z7 argument.
	bool enable_optimizations;

	bool enable_warning_unused_variables;
	bool disable_warning_unhandled_switch_cases;
	bool disable_warning_shadowed_locals;

	bool disable_aslr; // Disable address-space layout randomization

	// By default, these are false, and thus will be set to /MT
	// https://learn.microsoft.com/en-us/cpp/build/reference/md-mt-ld-use-run-time-library
	bool c_runtime_library_debug;
	bool c_runtime_library_dll;

	struct {
		bool disable_console; // Corresponds to /SUBSYSTEM:WINDOWS
		bool use_msbuild; // This will generate VS project files and call the MSBuild command to build the project.
	} windows;
} BUILD_ProjectOptions;

typedef struct BUILD_Project BUILD_Project;

typedef struct BUILD_Log {
	void (*print)(struct BUILD_Log* self, const char* message);
} BUILD_Log;

// NOTE: the `project` pointer must remain stable as long as it's being used.
BUILD_API void BUILD_InitProject(BUILD_Project* project, const char* name, const BUILD_ProjectOptions* options);
BUILD_API void BUILD_DestroyProject(BUILD_Project* project);

// Paths should be relative to the project directory.
BUILD_API void BUILD_AddSourceFile(BUILD_Project* project, const char* source_file);
BUILD_API void BUILD_AddIncludeDir(BUILD_Project* project, const char* include_dir);
BUILD_API void BUILD_AddDefine(BUILD_Project* project, const char* define);
BUILD_API void BUILD_AddLinkerInput(BUILD_Project* project, const char* linker_input);
BUILD_API void BUILD_AddExtraLinkerArg(BUILD_Project* project, const char* arg_string);
BUILD_API void BUILD_AddExtraCompilerArg(BUILD_Project* project, const char* arg_string);
BUILD_API void BUILD_AddVisualStudioNatvisFile(BUILD_Project* project, const char* natvis_file);

// TODO:
// BUILD_API void BUILD_AddSourceDir(BUILD_Project *project, const char *source_dir); // Add all .c and .cpp files inside a directory as source files

BUILD_API BUILD_Log* BUILD_GetConsole();

// * All build outputs (object files, executables, etc) go into the build directory.
// * `relative_build_directory` is relative to `project_directory`
BUILD_API bool BUILD_CompileProject(BUILD_Project* project, const char* project_directory, const char* relative_build_directory,
	BUILD_Log* log_or_null);

// * All build outputs (object files, executables, etc) go into the build directory.
// * `relative_build_directory` is relative to `project_directory`
BUILD_API bool BUILD_CreateVisualStudioSolution(const char* project_directory, const char* relative_build_directory,
	const char* solution_name, BUILD_Project** projects, int projects_count, BUILD_Log* log_or_null);

// If the directory already exists or is successfully created, true is returned.
BUILD_API bool BUILD_CreateDirectory(const char* directory);

// String utilities; the returned strings will remain alive as long as the project remains alive
BUILD_API char* BUILD_Concat2(BUILD_Project* project, const char* a, const char* b);
BUILD_API char* BUILD_Concat3(BUILD_Project* project, const char* a, const char* b, const char* c);
BUILD_API char* BUILD_Concat4(BUILD_Project* project, const char* a, const char* b, const char* c, const char* d);

/* ---- end of the public API --------------------------------------------- */

typedef struct { int block_size; struct BUILD_ArenaBlockHeader* first_block; void* block; void* ptr; } BUILD_Arena;
#define BUILD_Array(T) struct { BUILD_Arena *arena; T *data; int length; int capacity; }

struct BUILD_Project {
	BUILD_Arena arena;
	const char* name;
	BUILD_ProjectOptions opts;
	BUILD_Array(const char*) source_files;
	BUILD_Array(const char*) natvis_files;
	BUILD_Array(const char*) source_dirs;
	BUILD_Array(const char*) include_dirs;
	BUILD_Array(const char*) defines;
	BUILD_Array(const char*) linker_inputs;
	BUILD_Array(const char*) extra_linker_args;
	BUILD_Array(const char*) extra_compiler_args;
};

#ifdef /* ---------------- */ BUILD_IMPLEMENTATION /* ---------------- */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <io.h>         // For _get_osfhandle
#include <string.h>
#include <assert.h>
#include <Windows.h>

#pragma comment(lib, "Ole32.lib") // for StringFromGUID2

typedef struct BUILD_ArenaBlockHeader {
	int size_including_header;
	struct BUILD_ArenaBlockHeader* next; // may be NULL
} BUILD_ArenaBlockHeader;

#define BUILD_AlignUpPow2(x, p) (((x) + (p) - 1) & ~((p) - 1)) // e.g. (x=30, p=16) -> 32

static void BUILD_InitArena(BUILD_Arena* arena, int block_size) {
	BUILD_Arena _arena = {0};
	_arena.block_size = block_size;
	*arena = _arena;
}

static void BUILD_DestroyArena(BUILD_Arena* arena) {
	for (BUILD_ArenaBlockHeader* block = arena->first_block; block;) {
		BUILD_ArenaBlockHeader* next = block->next;
		free(block);
		block = next;
	}
}

static void* BUILD_ArenaPush(BUILD_Arena* arena, int size) {
	int alignment = sizeof(void*);

	BUILD_ArenaBlockHeader* curr_block = (BUILD_ArenaBlockHeader*)arena->block; // may be NULL
	void* curr_ptr = arena->ptr;

	void* result_address = (void*)BUILD_AlignUpPow2((uintptr_t)curr_ptr, alignment);
	int remaining_space = curr_block ? curr_block->size_including_header - (int)((uintptr_t)result_address - (uintptr_t)curr_block) : 0;

	if (size > remaining_space) { // We need a new block!
		int result_offset = BUILD_AlignUpPow2(sizeof(BUILD_ArenaBlockHeader), alignment);
		int new_block_size = result_offset + size;
		if (arena->block_size > new_block_size) new_block_size = arena->block_size;

		BUILD_ArenaBlockHeader* new_block = malloc(new_block_size);
		new_block->size_including_header = new_block_size;
		new_block->next = NULL;

		if (curr_block) curr_block->next = new_block;
		else arena->first_block = new_block;

		arena->block = new_block;
		result_address = (char*)new_block + result_offset;
	}

	arena->ptr = (char*)result_address + size;
	return result_address;
}

static void BUILD_ArenaReset(BUILD_Arena* arena) {
	arena->block = arena->first_block;
	arena->ptr = (char*)arena->first_block + sizeof(BUILD_ArenaBlockHeader);
}

typedef BUILD_Array(char) BUILD_ArrayRaw;

#define BUILD_ArrayPush(ARR, ELEM) do { \
		BUILD_ArrayReserveRaw((BUILD_ArrayRaw*)(ARR), (ARR)->length + 1, sizeof(*(ARR)->data)); \
		(ARR)->data[(ARR)->length] = ELEM; \
		(ARR)->length++; \
	} while (0)

static inline void BUILD_ArrayReserveRaw(BUILD_ArrayRaw* array, int capacity, int elem_size) {
	int new_capacity = array->capacity;
	while (capacity > new_capacity) {
		new_capacity = new_capacity == 0 ? 8 : new_capacity * 2;
	}

	if (new_capacity != array->capacity) {
		void* new_data = BUILD_ArenaPush(array->arena, new_capacity * elem_size);
		memcpy(new_data, array->data, array->length * elem_size);
		array->data = new_data;
		array->capacity = new_capacity;
	}
}

static wchar_t* BUILD_UTF8ToWide(BUILD_Arena* arena, const char* str) {
	if (*str == 0) return L""; // MultiByteToWideChar does not accept 0-length strings

	int len = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, -1, NULL, 0);
	wchar_t* wstr = BUILD_ArenaPush(arena, (len + 1) * 2);

	int len2 = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, str, -1, wstr, len);
	assert(len != 0 && len2 == len);

	wstr[len] = 0; // null-termination
	return wstr;
}

static char* BUILD_WideToUTF8(BUILD_Arena* arena, const wchar_t* wstr) {
	if (*wstr == 0) return ""; // MultiByteToWideChar does not accept 0-length strings

	int len = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, NULL, 0, NULL, NULL);
	char* str = BUILD_ArenaPush(arena, len + 1);

	int len2 = WideCharToMultiByte(CP_UTF8, 0, wstr, -1, str, len, NULL, NULL);
	assert(len != 0 && len2 == len);

	str[len] = 0; // null-termination
	return str;
}

typedef struct {
	int windows_sdk_version; // Zero if no Windows SDK found.

	wchar_t* windows_sdk_root;
	wchar_t* windows_sdk_include_base;
	wchar_t* windows_sdk_um_library_path;
	wchar_t* windows_sdk_ucrt_library_path;

	wchar_t* vs_base_path;
	wchar_t* vs_exe_path;
	wchar_t* vs_include_path;
	wchar_t* vs_library_path;
} BUILD_FindVSResult;

static BUILD_FindVSResult BUILD_FindVSAndWindowsSDK(BUILD_Arena* arena);

// COM objects for the ridiculous Microsoft craziness.

#undef  INTERFACE
#define INTERFACE ISetupInstance
DECLARE_INTERFACE_(ISetupInstance, IUnknown)
{
	BEGIN_INTERFACE

		// IUnknown methods
		STDMETHOD(QueryInterface)   (THIS_  REFIID, void**) PURE;
	STDMETHOD_(ULONG, AddRef)    (THIS) PURE;
	STDMETHOD_(ULONG, Release)   (THIS) PURE;

	// ISetupInstance methods
	STDMETHOD(GetInstanceId)(THIS_ _Out_ BSTR * pbstrInstanceId) PURE;
	STDMETHOD(GetInstallDate)(THIS_ _Out_ LPFILETIME pInstallDate) PURE;
	STDMETHOD(GetInstallationName)(THIS_ _Out_ BSTR * pbstrInstallationName) PURE;
	STDMETHOD(GetInstallationPath)(THIS_ _Out_ BSTR * pbstrInstallationPath) PURE;
	STDMETHOD(GetInstallationVersion)(THIS_ _Out_ BSTR * pbstrInstallationVersion) PURE;
	STDMETHOD(GetDisplayName)(THIS_ _In_ LCID lcid, _Out_ BSTR * pbstrDisplayName) PURE;
	STDMETHOD(GetDescription)(THIS_ _In_ LCID lcid, _Out_ BSTR * pbstrDescription) PURE;
	STDMETHOD(ResolvePath)(THIS_ _In_opt_z_ LPCOLESTR pwszRelativePath, _Out_ BSTR * pbstrAbsolutePath) PURE;

	END_INTERFACE
};

#undef  INTERFACE
#define INTERFACE IEnumSetupInstances
DECLARE_INTERFACE_(IEnumSetupInstances, IUnknown)
{
	BEGIN_INTERFACE

		// IUnknown methods
		STDMETHOD(QueryInterface)   (THIS_  REFIID, void**) PURE;
	STDMETHOD_(ULONG, AddRef)    (THIS) PURE;
	STDMETHOD_(ULONG, Release)   (THIS) PURE;

	// IEnumSetupInstances methods
	STDMETHOD(Next)(THIS_ _In_ ULONG celt, _Out_writes_to_(celt, *pceltFetched) ISetupInstance * *rgelt, _Out_opt_ _Deref_out_range_(0, celt) ULONG * pceltFetched) PURE;
	STDMETHOD(Skip)(THIS_ _In_ ULONG celt) PURE;
	STDMETHOD(Reset)(THIS) PURE;
	STDMETHOD(Clone)(THIS_ _Deref_out_opt_ IEnumSetupInstances * *ppenum) PURE;

	END_INTERFACE
};

#undef  INTERFACE
#define INTERFACE ISetupConfiguration
DECLARE_INTERFACE_(ISetupConfiguration, IUnknown)
{
	BEGIN_INTERFACE

		// IUnknown methods
		STDMETHOD(QueryInterface)   (THIS_  REFIID, void**) PURE;
	STDMETHOD_(ULONG, AddRef)    (THIS) PURE;
	STDMETHOD_(ULONG, Release)   (THIS) PURE;

	// ISetupConfiguration methods
	STDMETHOD(EnumInstances)(THIS_ _Out_ IEnumSetupInstances * *ppEnumInstances) PURE;
	STDMETHOD(GetInstanceForCurrentProcess)(THIS_ _Out_ ISetupInstance * *ppInstance) PURE;
	STDMETHOD(GetInstanceForPath)(THIS_ _In_z_ LPCWSTR wzPath, _Out_ ISetupInstance * *ppInstance) PURE;

	END_INTERFACE
};

#ifdef __cplusplus
#define CALL_STDMETHOD(object, method, ...) object->method(__VA_ARGS__)
#define CALL_STDMETHOD_(object, method) object->method()
#else
#define CALL_STDMETHOD(object, method, ...) object->lpVtbl->method(object, __VA_ARGS__)
#define CALL_STDMETHOD_(object, method) object->lpVtbl->method(object)
#endif

// The beginning of the actual code that does things.

typedef struct {
	int32_t best_version[4];  // For Windows 8 versions, only two of these numbers are used.
	wchar_t* best_name;
} BUILD_FindVSVersionData;

static bool BUILD_FileExists(wchar_t* name) {
	uint32_t attrib = GetFileAttributesW(name);
	if (attrib == INVALID_FILE_ATTRIBUTES) return false;
	if (attrib & FILE_ATTRIBUTE_DIRECTORY) return false;
	return true;
}

#define BUILD_WStrJoin2(ARENA, A, B) BUILD_WStrJoin4(ARENA, A, B, NULL, NULL)
#define BUILD_WStrJoin3(ARENA, A, B, C) BUILD_WStrJoin4(ARENA, A, B, C, NULL)
static wchar_t* BUILD_WStrJoin4(BUILD_Arena* arena, const wchar_t* a, const wchar_t* b, const wchar_t* c, const wchar_t* d) {
	int len_a = (int)wcslen(a);
	int len_b = (int)wcslen(b);

	int len_c = 0;
	if (c) len_c = (int)wcslen(c);

	int len_d = 0;
	if (d) len_d = (int)wcslen(d);

	wchar_t* result = BUILD_ArenaPush(arena, (len_a + len_b + len_c + len_d + 1) * 2);
	memcpy(result, a, len_a * 2);
	memcpy(result + len_a, b, len_b * 2);

	if (c) memcpy(result + len_a + len_b, c, len_c * 2);
	if (d) memcpy(result + len_a + len_b + len_c, d, len_d * 2);

	result[len_a + len_b + len_c + len_d] = 0;
	return result;
}

// From "microsoft_craziness.h"
typedef void (*BUILD_FindVSVisitProc)(wchar_t* short_name, wchar_t* full_name, BUILD_FindVSVersionData* data);
static bool BUILD_FindVSVisitFiles(BUILD_Arena* arena, wchar_t* dir_name, BUILD_FindVSVersionData* data, BUILD_FindVSVisitProc proc) {
	// Visit everything in one folder (non-recursively). If it's a directory
	// that doesn't start with ".", call the visit proc on it. The visit proc
	// will see if the filename conforms to the expected versioning pattern.

	WIN32_FIND_DATAW find_data;

	wchar_t* wildcard_name = BUILD_WStrJoin2(arena, dir_name, L"\\*");
	HANDLE handle = FindFirstFileW(wildcard_name, &find_data);

	if (handle == INVALID_HANDLE_VALUE) return false;

	while (true) {
		if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (find_data.cFileName[0] != '.')) {
			wchar_t* full_name = BUILD_WStrJoin3(arena, dir_name, L"\\", find_data.cFileName);
			proc(find_data.cFileName, full_name, data);
		}

		BOOL success = FindNextFileW(handle, &find_data);
		if (!success) break;
	}

	FindClose(handle);
	return true;
}

// From "microsoft_craziness.h"
static wchar_t* BUILD_FindWindowsKitRootWithKey(BUILD_Arena* arena, HKEY key, wchar_t* version) {
	// Given a key to an already opened registry entry,
	// get the value stored under the 'version' subkey.
	// If that's not the right terminology, hey, I never do registry stuff.

	DWORD required_length;
	LSTATUS rc = RegQueryValueExW(key, version, NULL, NULL, NULL, &required_length);
	if (rc != 0)  return NULL;

	DWORD length = required_length + 2;  // The +2 is for the maybe optional zero later on. Probably we are over-allocating.
	wchar_t* value = (wchar_t*)BUILD_ArenaPush(arena, length);
	if (!value) return NULL;

	rc = RegQueryValueExW(key, version, NULL, NULL, (LPBYTE)value, &length);  // We know that version is zero-terminated...
	if (rc != 0)  return NULL;

	// The documentation says that if the string for some reason was not stored
	// with zero-termination, we need to manually terminate it. Sigh!!

	if (value[length]) {
		value[length + 1] = 0;
	}

	return value;
}

// From "microsoft_craziness.h"
static void BUILD_FindVS_Win10_Best(wchar_t* short_name, wchar_t* full_name, BUILD_FindVSVersionData* data) {
	// Find the Windows 10 subdirectory with the highest version number.

	int i0, i1, i2, i3;
	int success = swscanf_s(short_name, L"%d.%d.%d.%d", &i0, &i1, &i2, &i3);
	if (success < 4) return;

	if (i0 < data->best_version[0]) return;
	else if (i0 == data->best_version[0]) {
		if (i1 < data->best_version[1]) return;
		else if (i1 == data->best_version[1]) {
			if (i2 < data->best_version[2]) return;
			else if (i2 == data->best_version[2]) {
				if (i3 < data->best_version[3]) return;
			}
		}
	}

	data->best_name = full_name;

	if (data->best_name) {
		data->best_version[0] = i0;
		data->best_version[1] = i1;
		data->best_version[2] = i2;
		data->best_version[3] = i3;
	}
}

// From "microsoft_craziness.h"
static void BUILD_FindVS_Win8_Best(wchar_t* short_name, wchar_t* full_name, BUILD_FindVSVersionData* data) {
	// Find the Windows 8 subdirectory with the highest version number.

	int i0, i1;
	int success = swscanf_s(short_name, L"winv%d.%d", &i0, &i1);
	if (success < 2) return;

	if (i0 < data->best_version[0]) return;
	else if (i0 == data->best_version[0]) {
		if (i1 < data->best_version[1]) return;
	}

	data->best_name = full_name;

	if (data->best_name) {
		data->best_version[0] = i0;
		data->best_version[1] = i1;
	}
}

// From "microsoft_craziness.h"
static void BUILD_FindWindowsKitRoot(BUILD_Arena* arena, BUILD_FindVSResult* result) {
	// Information about the Windows 10 and Windows 8 development kits
	// is stored in the same place in the registry. We open a key
	// to that place, first checking preferntially for a Windows 10 kit,
	// then, if that's not found, a Windows 8 kit.

	HKEY main_key;

	LSTATUS rc = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots",
		0, KEY_QUERY_VALUE | KEY_WOW64_32KEY | KEY_ENUMERATE_SUB_KEYS, &main_key);
	if (rc != S_OK) return;

	// Look for a Windows 10 entry.
	wchar_t* windows10_root = BUILD_FindWindowsKitRootWithKey(arena, main_key, L"KitsRoot10");

	if (windows10_root) {
		wchar_t* windows10_lib = BUILD_WStrJoin2(arena, windows10_root, L"Lib");
		wchar_t* windows10_include = BUILD_WStrJoin2(arena, windows10_root, L"include");

		BUILD_FindVSVersionData lib_data = {0};
		BUILD_FindVSVisitFiles(arena, windows10_lib, &lib_data, BUILD_FindVS_Win10_Best);
		BUILD_FindVSVersionData include_data = {0};
		BUILD_FindVSVisitFiles(arena, windows10_include, &include_data, BUILD_FindVS_Win10_Best);

		if (lib_data.best_name) {
			result->windows_sdk_version = 10;
			result->windows_sdk_root = lib_data.best_name;
			result->windows_sdk_include_base = include_data.best_name;
			RegCloseKey(main_key);
			return;
		}
	}

	// Look for a Windows 8 entry.
	wchar_t* windows8_root = BUILD_FindWindowsKitRootWithKey(arena, main_key, L"KitsRoot81");

	if (windows8_root) {
		wchar_t* windows8_lib = BUILD_WStrJoin2(arena, windows8_root, L"Lib");

		BUILD_FindVSVersionData data = {0};
		BUILD_FindVSVisitFiles(arena, windows8_lib, &data, BUILD_FindVS_Win8_Best);

		if (data.best_name) {
			result->windows_sdk_version = 8;
			result->windows_sdk_root = data.best_name;
			RegCloseKey(main_key);
			return;
		}
	}

	// If we get here, we failed to find anything.
	RegCloseKey(main_key);
}

// From "microsoft_craziness.h"
static bool BUILD_FindVisualStudio2017ByFightingThroughMicrosoftCraziness(BUILD_Arena* arena, BUILD_FindVSResult* result) {
	CoInitialize(NULL);
	// "Subsequent valid calls return false." So ignore false.
	// if rc != S_OK  return false;

	GUID my_uid = { 0x42843719, 0xDB4C, 0x46C2, {0x8E, 0x7C, 0x64, 0xF1, 0x81, 0x6E, 0xFD, 0x5B} };
	GUID CLSID_SetupConfiguration = { 0x177F0C4A, 0x1CD3, 0x4DE7, {0xA3, 0x2C, 0x71, 0xDB, 0xBB, 0x9F, 0xA3, 0x6D} };

	ISetupConfiguration* config = NULL;

	// NOTE(Kalinovcic): This is so stupid... These functions take references, so the code is different for C and C++......
#ifdef __cplusplus
	HRESULT hr = CoCreateInstance(CLSID_SetupConfiguration, NULL, CLSCTX_INPROC_SERVER, my_uid, (void**)&config);
#else
	HRESULT hr = CoCreateInstance(&CLSID_SetupConfiguration, NULL, CLSCTX_INPROC_SERVER, &my_uid, (void**)&config);
#endif

	if (hr != 0)  return false;

	IEnumSetupInstances* instances = NULL;
	hr = CALL_STDMETHOD(config, EnumInstances, &instances);
	CALL_STDMETHOD_(config, Release);
	if (hr != 0)     return false;
	if (!instances)  return false;

	bool found_visual_studio_2017 = false;
	while (1) {
		ULONG found = 0;
		ISetupInstance* instance = NULL;
		hr = CALL_STDMETHOD(instances, Next, 1, &instance, &found);
		if (hr != S_OK) break;

		BSTR bstr_inst_path;
		hr = CALL_STDMETHOD(instance, GetInstallationPath, &bstr_inst_path);
		CALL_STDMETHOD_(instance, Release);
		if (hr != S_OK)  continue;

		wchar_t* tools_filename = BUILD_WStrJoin2(arena, bstr_inst_path, L"\\VC\\Auxiliary\\Build\\Microsoft.VCToolsVersion.default.txt");
		SysFreeString(bstr_inst_path);

		FILE* f;
		errno_t open_result = _wfopen_s(&f, tools_filename, L"rt");
		if (open_result != 0) continue;
		if (!f) continue;

		LARGE_INTEGER tools_file_size;
		HANDLE file_handle = (HANDLE)_get_osfhandle(_fileno(f));
		BOOL success = GetFileSizeEx(file_handle, &tools_file_size);
		if (!success) {
			fclose(f);
			continue;
		}

		int version_bytes = ((int)tools_file_size.QuadPart + 1) * 2;  // Warning: This multiplication by 2 presumes there is no variable-length encoding in the wchars (wacky characters in the file could betray this expectation).
		wchar_t* version = (wchar_t*)BUILD_ArenaPush(arena, version_bytes);

		wchar_t* read_result = fgetws(version, version_bytes, f);
		fclose(f);
		if (!read_result) continue;

		wchar_t* version_tail = wcschr(version, '\n');
		if (version_tail)  *version_tail = 0;  // Stomp the data, because nobody cares about it.

		wchar_t* library_path = BUILD_WStrJoin4(arena, bstr_inst_path, L"\\VC\\Tools\\MSVC\\", version, L"\\lib\\x64");
		wchar_t* library_file = BUILD_WStrJoin2(arena, library_path, L"\\vcruntime.lib");  // @Speed: Could have library_path point to this string, with a smaller count, to save on memory flailing!

		if (BUILD_FileExists(library_file)) {
			wchar_t* link_exe_path = BUILD_WStrJoin4(arena, bstr_inst_path, L"\\VC\\Tools\\MSVC\\", version, L"\\bin\\Hostx64\\x64");

			result->vs_base_path = BUILD_WStrJoin2(arena, bstr_inst_path, L"");
			result->vs_exe_path = link_exe_path;
			result->vs_library_path = library_path;
			result->vs_include_path = BUILD_WStrJoin4(arena, bstr_inst_path, L"\\VC\\Tools\\MSVC\\", version, L"\\include");
			found_visual_studio_2017 = true;
			break;
		}

		/*
		Ryan Saunderson said:
		"Clang uses the 'SetupInstance->GetInstallationVersion' / ISetupHelper->ParseVersion to find the newest version
		and then reads the tools file to define the tools path - which is definitely better than what i did."

		So... @Incomplete: Should probably pick the newest version...
		*/
	}

	CALL_STDMETHOD_(instances, Release);
	return found_visual_studio_2017;
}

// From "microsoft_craziness.h"
static void BUILD_FindVisualStudioByFightingThroughMicrosoftCraziness(BUILD_Arena* arena, BUILD_FindVSResult* result) {
	// The name of this procedure is kind of cryptic. Its purpose is
	// to fight through Microsoft craziness. The things that the fine
	// Visual Studio team want you to do, JUST TO FIND A SINGLE FOLDER
	// THAT EVERYONE NEEDS TO FIND, are ridiculous garbage.

	// For earlier versions of Visual Studio, you'd find this information in the registry,
	// similarly to the Windows Kits above. But no, now it's the future, so to ask the
	// question "Where is the Visual Studio folder?" you have to do a bunch of COM object
	// instantiation, enumeration, and querying. (For extra bonus points, try doing this in
	// a new, underdeveloped programming language where you don't have COM routines up
	// and running yet. So fun.)
	//
	// If all this COM object instantiation, enumeration, and querying doesn't give us
	// a useful result, we drop back to the registry-checking method.

	bool found_visual_studio_2017 = BUILD_FindVisualStudio2017ByFightingThroughMicrosoftCraziness(arena, result);
	if (found_visual_studio_2017)  return;

	// If we get here, we didn't find Visual Studio 2017. Try earlier versions.

	HKEY vs7_key;
	HRESULT rc = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\VisualStudio\\SxS\\VS7", 0, KEY_QUERY_VALUE | KEY_WOW64_32KEY, &vs7_key);
	if (rc != S_OK)  return;

	// Hardcoded search for 4 prior Visual Studio versions. Is there something better to do here?
	wchar_t* versions[] = { L"14.0", L"12.0", L"11.0", L"10.0" };
	const int NUM_VERSIONS = sizeof(versions) / sizeof(versions[0]);

	for (int i = 0; i < NUM_VERSIONS; i++) {
		wchar_t* v = versions[i];

		DWORD dw_type;
		DWORD cb_data;

		rc = RegQueryValueExW(vs7_key, v, NULL, &dw_type, NULL, &cb_data);
		if ((rc == ERROR_FILE_NOT_FOUND) || (dw_type != REG_SZ)) {
			continue;
		}

		wchar_t* buffer = BUILD_ArenaPush(arena, cb_data);
		if (!buffer)  return;

		rc = RegQueryValueExW(vs7_key, v, NULL, NULL, (LPBYTE)buffer, &cb_data);
		if (rc != 0)  continue;

		// @Robustness: Do the zero-termination thing suggested in the RegQueryValue docs?

		wchar_t* lib_path = BUILD_WStrJoin2(arena, buffer, L"VC\\Lib\\amd64");

		// Check to see whether a vcruntime.lib actually exists here.
		wchar_t* vcruntime_filename = BUILD_WStrJoin2(arena, lib_path, L"\\vcruntime.lib");
		bool vcruntime_exists = BUILD_FileExists(vcruntime_filename);

		if (vcruntime_exists) {
			result->vs_base_path = BUILD_WStrJoin2(arena, buffer, L""); // NOTE(Eero): this is total guess, it might be incorrect!
			result->vs_exe_path = BUILD_WStrJoin2(arena, buffer, L"VC\\bin\\amd64");
			result->vs_library_path = lib_path;
			result->vs_include_path = BUILD_WStrJoin2(arena, buffer, L"VC\\include\\amd64"); // NOTE(Eero): this is total guess, it might be incorrect!

			RegCloseKey(vs7_key);
			return;
		}
	}

	RegCloseKey(vs7_key);

	// If we get here, we failed to find anything.
}

// From "microsoft_craziness.h"
static BUILD_FindVSResult BUILD_FindVisualStudioAndWindowsSDK(BUILD_Arena* arena) {
	BUILD_FindVSResult result;

	BUILD_FindWindowsKitRoot(arena, &result);

	if (result.windows_sdk_root) {
		result.windows_sdk_um_library_path = BUILD_WStrJoin2(arena, result.windows_sdk_root, L"\\um\\x64");
		result.windows_sdk_ucrt_library_path = BUILD_WStrJoin2(arena, result.windows_sdk_root, L"\\ucrt\\x64");
	}

	BUILD_FindVisualStudioByFightingThroughMicrosoftCraziness(arena, &result);

	return result;
}

#define BUILD_LogPrint1(LOG_OR_NULL, A)       BUILD_LogPrint4(LOG_OR_NULL, A, NULL, NULL, NULL)
#define BUILD_LogPrint2(LOG_OR_NULL, A, B)    BUILD_LogPrint4(LOG_OR_NULL, A, B, NULL, NULL)
#define BUILD_LogPrint3(LOG_OR_NULL, A, B, C) BUILD_LogPrint4(LOG_OR_NULL, A, B, C, NULL)
static BUILD_LogPrint4(BUILD_Log* log_or_null, const char* A, const char* B, const char* C, const char* D) {
	if (log_or_null->print) {
		log_or_null->print(log_or_null, A);
		if (B) log_or_null->print(log_or_null, B);
		if (C) log_or_null->print(log_or_null, C);
		if (D) log_or_null->print(log_or_null, D);
	}
}

typedef BUILD_Array(wchar_t) BUILD_WStrBuilder;
typedef BUILD_Array(char) BUILD_StrBuilder;

static void BUILD_WPrint1(BUILD_WStrBuilder* builder, const wchar_t* string) {
	for (const wchar_t* ptr = string; *ptr; ptr++) {
		BUILD_ArrayPush(builder, *ptr);
	}
}

#define BUILD_WPrint2(BUILDER, A, B)    BUILD_WPrint4(BUILDER, A, B, NULL, NULL)
#define BUILD_WPrint3(BUILDER, A, B, C) BUILD_WPrint4(BUILDER, A, B, C, NULL)
static void BUILD_WPrint4(BUILD_WStrBuilder* builder, const wchar_t* a, const wchar_t* b, const wchar_t* c, const wchar_t* d) {
	BUILD_WPrint1(builder, a);
	BUILD_WPrint1(builder, b);
	if (c) BUILD_WPrint1(builder, c);
	if (d) BUILD_WPrint1(builder, d);
}

static void BUILD_Print1(BUILD_StrBuilder* builder, const char* string) {
	for (const char* ptr = string; *ptr; ptr++) {
		BUILD_ArrayPush(builder, *ptr);
	}
}

#define BUILD_Print2(BUILDER, A, B)    BUILD_Print4(BUILDER, A, B, NULL, NULL)
#define BUILD_Print3(BUILDER, A, B, C) BUILD_Print4(BUILDER, A, B, C, NULL)
static void BUILD_Print4(BUILD_StrBuilder* builder, const char* a, const char* b, const char* c, const char* d) {
	BUILD_Print1(builder, a);
	BUILD_Print1(builder, b);
	if (c) BUILD_Print1(builder, c);
	if (d) BUILD_Print1(builder, d);
}

static bool BUILD_RunProcess(const wchar_t* command_string, uint32_t* out_exit_code, BUILD_Log* log_or_null) {
	// https://learn.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output

	PROCESS_INFORMATION process_info = {0};

	// Initialize pipes
	SECURITY_ATTRIBUTES security_attrs = {
		.nLength = sizeof(SECURITY_ATTRIBUTES),
		.lpSecurityDescriptor = NULL,
		.bInheritHandle = 1,
	};

	HANDLE OUT_Rd = NULL, OUT_Wr = NULL;
	HANDLE ERR_Rd = NULL, ERR_Wr = NULL;

	bool ok = true;
	if (ok) ok = CreatePipe(&OUT_Rd, &OUT_Wr, &security_attrs, 0);
	if (ok) ok = CreatePipe(&ERR_Rd, &ERR_Wr, &security_attrs, 0);
	if (ok) ok = SetHandleInformation(OUT_Rd, HANDLE_FLAG_INHERIT, 0);

	STARTUPINFOW startup_info = {
		.cb = sizeof(STARTUPINFOW),
		.dwFlags = STARTF_USESTDHANDLES,
		.hStdOutput = OUT_Wr,
		.hStdError = ERR_Wr, // let's ignore stderr
		//.hStdOutput = OUT_Wr,
		//.hStdError = OUT_Wr,
		.hStdInput = GetStdHandle(STD_INPUT_HANDLE),
	};

	if (ok) ok = CreateProcessW(NULL, (wchar_t*)command_string, NULL, NULL, true, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &startup_info, &process_info);

	// We don't need these handles for ourselves and we must close them to say that we won't be using them, to let `ReadFile` exit
	// when the process finishes instead of locking. At least that's how I think it works.
	if (OUT_Wr) CloseHandle(OUT_Wr);
	if (ERR_Wr) CloseHandle(ERR_Wr);

	if (ok) {
		char buf[512];
		uint32_t num_read_bytes;

		for (;;) {
			if (!ReadFile(OUT_Rd, buf, sizeof(buf) - 1, &num_read_bytes, NULL)) break;

			if (log_or_null) {
				buf[num_read_bytes] = 0; // null termination
				BUILD_LogPrint1(log_or_null, buf);
			}
		}

		WaitForSingleObject(process_info.hProcess, INFINITE);

		if (out_exit_code && !GetExitCodeProcess(process_info.hProcess, out_exit_code)) ok = false;

		CloseHandle(process_info.hProcess);
		CloseHandle(process_info.hThread);
	}

	if (OUT_Rd) CloseHandle(OUT_Rd);
	if (ERR_Rd) CloseHandle(ERR_Rd);
	return ok;
}

// @cleanup: merge this with BUILD_FindVSVisitFiles
static bool BUILD_GetAllFilesInDirectory(BUILD_Arena* arena, const wchar_t* directory, char*** out_files, int* out_files_count) {
	wchar_t* match_str = BUILD_WStrJoin2(arena, directory, L"\\*");

	BUILD_Array(char*) files = { arena };

	WIN32_FIND_DATAW find_info;
	HANDLE handle = FindFirstFileW(match_str, &find_info);

	bool ok = handle != INVALID_HANDLE_VALUE;
	if (ok) {
		for (; FindNextFileW(handle, &find_info);) {
			if (find_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

			const wchar_t* file_w = BUILD_WStrJoin3(arena, directory, L"/", find_info.cFileName);
			char* file = BUILD_WideToUTF8(arena, file_w);
			BUILD_ArrayPush(&files, file);
		}
		ok = GetLastError() == ERROR_NO_MORE_FILES;
		FindClose(handle);
	}

	*out_files = files.data;
	*out_files_count = files.length;
	return ok;
}

static bool BUILD_GenerateVisualStudioProject(BUILD_Arena* arena, const BUILD_Project* project,
	const char* project_filepath, const char* project_directory, const char* relative_build_directory,
	const char* project_guid, BUILD_Log* log_or_null)
{
	BUILD_StrBuilder s = { arena };

	BUILD_Print1(&s, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
	BUILD_Print1(&s, "<Project DefaultTargets=\"Build\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n");

	BUILD_Print1(&s, "<ItemGroup Label=\"ProjectConfigurations\">\n");
	BUILD_Print1(&s, "  <ProjectConfiguration Include=\"Custom|x64\">\n");
	BUILD_Print1(&s, "    <Configuration>Custom</Configuration>\n");
	BUILD_Print1(&s, "    <Platform>x64</Platform>\n");
	BUILD_Print1(&s, "  </ProjectConfiguration>\n");
	BUILD_Print1(&s, "</ItemGroup>\n");

	BUILD_Print1(&s, "<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />\n");
	BUILD_Print1(&s, "<PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Custom|x64'\" Label=\"Configuration\">\n");
	BUILD_Print1(&s, "  <ConfigurationType>Application</ConfigurationType>\n");
	BUILD_Print1(&s, "  <UseDebugLibraries>false</UseDebugLibraries>\n");
	BUILD_Print1(&s, "  <CharacterSet>Unicode</CharacterSet>\n");
	BUILD_Print1(&s, "  <PlatformToolset>v143</PlatformToolset>\n");
	BUILD_Print1(&s, "</PropertyGroup>\n");

	BUILD_Print1(&s, "<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />\n");
	BUILD_Print1(&s, "<ImportGroup Label=\"ExtensionSettings\">\n");
	BUILD_Print1(&s, "</ImportGroup>\n");


	BUILD_Print1(&s, "<ImportGroup Label=\"PropertySheets\" Condition=\"'$(Configuration)|$(Platform)'=='Custom|x64'\">\n");
	BUILD_Print1(&s, "  <Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" />\n");
	BUILD_Print1(&s, "</ImportGroup>\n");

	BUILD_Print1(&s, "<PropertyGroup Condition=\"\'$(Configuration)|$(Platform)\'==\'Custom|x64\'\">\n");
	BUILD_Print3(&s, "  <OutDir>", relative_build_directory, "/</OutDir>\n");
	BUILD_Print3(&s, "  <IntDir>", relative_build_directory, "/");
	BUILD_Print2(&s, project->name, "/</IntDir>\n");
	BUILD_Print1(&s, "</PropertyGroup>\n");

	{
		BUILD_Print1(&s, "<ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='Custom|x64'\">\n");

		{
			BUILD_Print1(&s, "<ClCompile>\n");

			BUILD_Print1(&s, "<PrecompiledHeader>NotUsing</PrecompiledHeader>\n");
			BUILD_Print1(&s, "<WarningLevel>Level3</WarningLevel>\n");

			{
				BUILD_Print1(&s, "<PreprocessorDefinitions>");
				for (int i = 0; i < project->defines.length; i++) {
					BUILD_Print2(&s, project->defines.data[i], ";");
				}
				BUILD_Print1(&s, "</PreprocessorDefinitions>\n");
			}

			BUILD_Print1(&s, "<AdditionalIncludeDirectories>");
			for (int i = 0; i < project->include_dirs.length; i++) {
				const char* include_dir = project->include_dirs.data[i];
				BUILD_Print2(&s, include_dir, ";");
			}
			BUILD_Print1(&s, "</AdditionalIncludeDirectories>\n");

			if (project->opts.debug_info) {
				BUILD_Print1(&s, "<DebugInformationFormat>OldStyle</DebugInformationFormat>\n"); // OldStyle is the same as /Z7
			}
			if (project->opts.enable_optimizations) {
				BUILD_Print1(&s, "<Optimization>Full</Optimization>\n");
				BUILD_Print1(&s, "<FunctionLevelLinking>true</FunctionLevelLinking>\n");
				BUILD_Print1(&s, "<IntrinsicFunctions>true</IntrinsicFunctions>\n");
				BUILD_Print1(&s, "<MinimalRebuild>false</MinimalRebuild>\n");
				BUILD_Print1(&s, "<StringPooling>true</StringPooling>\n");
			}
			else {
				BUILD_Print1(&s, "<Optimization>Disabled</Optimization>\n");
			}
			BUILD_Print1(&s, "<ExceptionHandling>false</ExceptionHandling>\n");
			BUILD_Print1(&s, "<RuntimeTypeInfo>false</RuntimeTypeInfo>\n");
			BUILD_Print1(&s, "<ExternalWarningLevel>Level3</ExternalWarningLevel>\n");
			BUILD_Print1(&s, "<TreatWarningAsError>true</TreatWarningAsError>\n");
			{
				BUILD_Print1(&s, "<AdditionalOptions>");
				if (!project->opts.enable_warning_unused_variables) BUILD_Print1(&s, "/wd4101 ");
				if (!project->opts.disable_warning_unhandled_switch_cases) BUILD_Print1(&s, "/w14062 ");
				if (!project->opts.disable_warning_shadowed_locals) BUILD_Print1(&s, "/w14456 ");

				for (int i = 0; i < project->extra_compiler_args.length; i++) {
					BUILD_Print2(&s, project->extra_compiler_args.data[i], " ");
				}

				BUILD_Print1(&s, "</AdditionalOptions>\n");
			}

			const char* crt_linking_type = project->opts.c_runtime_library_debug ?
				(project->opts.c_runtime_library_dll ? "MultiThreadedDebugDLL" : "MultiThreadedDebug") :
				(project->opts.c_runtime_library_dll ? "MultiThreadedDLL" : "MultiThreaded");
			BUILD_Print3(&s, "<RuntimeLibrary>", crt_linking_type, "</RuntimeLibrary>\n");

			BUILD_Print1(&s, "</ClCompile>\n");
		}

		{
			BUILD_Print1(&s, "<Link>\n");

			BUILD_Print1(&s, project->opts.windows.disable_console ? "<SubSystem>Windows</SubSystem>\n" : "<SubSystem>Console</SubSystem>\n");

			if (project->opts.enable_optimizations) {
				BUILD_Print1(&s, "<EnableCOMDATFolding>true</EnableCOMDATFolding>\n");
				BUILD_Print1(&s, "<OptimizeReferences>true</OptimizeReferences>\n");
			}

			if (project->opts.debug_info) {
				BUILD_Print1(&s, "<GenerateDebugInformation>true</GenerateDebugInformation>\n");
			}

			BUILD_Print1(&s, "<AdditionalDependencies>");
			for (int i = 0; i < project->linker_inputs.length; i++) {
				BUILD_Print2(&s, project->linker_inputs.data[i], ";");
			}
			BUILD_Print1(&s, "</AdditionalDependencies>\n");

			{
				BUILD_Print1(&s, "<AdditionalOptions>");

				for (int i = 0; i < project->extra_linker_args.length; i++) {
					BUILD_Print2(&s, project->extra_linker_args.data[i], " ");
				}

				BUILD_Print1(&s, "/IGNORE:4099 "); // LNK4099: PDB ... was not found with ... or at ''; linking object as if no debug info

				if (project->opts.disable_aslr) BUILD_Print1(&s, "/DYNAMICBASE:NO ");

				BUILD_Print1(&s, "</AdditionalOptions>\n");
			}

			BUILD_Print1(&s, "</Link>\n");
		}
		BUILD_Print1(&s, "</ItemDefinitionGroup>\n");
	}

	for (int i = 0; i < project->natvis_files.length; i++) {
		BUILD_Print1(&s, "<ItemGroup>\n");
		BUILD_Print3(&s, "  <Natvis Include=\"", project->natvis_files.data[i], "\" />\n");
		BUILD_Print1(&s, "</ItemGroup>\n");
	}
	
	for (int i = 0; i < project->source_files.length; i++) {
		BUILD_Print1(&s, "<ItemGroup>\n");
		BUILD_Print3(&s, "  <ClCompile Include=\"", project->source_files.data[i], "\" />\n");
		BUILD_Print1(&s, "</ItemGroup>\n");
	}

	for (int i = 0; i < project->source_dirs.length; i++) {
		BUILD_StrBuilder source_dir = { arena };
		BUILD_Print3(&source_dir, project_directory, "/", project->source_dirs.data[i]);
		BUILD_ArrayPush(&source_dir, 0); // Push null-termination

		const wchar_t* source_dir_wide = BUILD_UTF8ToWide(arena, source_dir.data);

		char** files; int files_len;
		assert(BUILD_GetAllFilesInDirectory(arena, source_dir_wide, &files, &files_len));

		for (int file_idx = 0; file_idx < files_len; file_idx++) {
			const char* file = files[file_idx];
			int file_len = (int)strlen(file);

			bool is_source_file = false;
			if (file_len >= 2 && strcmp(file + file_len - 2, ".c") == 0)   is_source_file = true;
			if (file_len >= 4 && strcmp(file + file_len - 4, ".cpp") == 0) is_source_file = true;

			BUILD_Print1(&s, "<ItemGroup>\n");
			BUILD_Print3(&s, is_source_file ? "  <ClCompile Include=\"" : "  <ClInclude Include=\"", file, "\" />\n");
			BUILD_Print1(&s, "</ItemGroup>\n");
		}
	}

	BUILD_Print1(&s, "<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />\n");

	BUILD_Print1(&s, "<ImportGroup Label=\"ExtensionTargets\">\n");
	BUILD_Print1(&s, "</ImportGroup>\n");

	BUILD_Print1(&s, "</Project>\n");

	FILE* file = fopen(project_filepath, "wb");
	bool ok = file != NULL;

	if (ok) {
		fwrite(s.data, s.length, 1, file);
		fclose(file);
	}
	else {
		BUILD_LogPrint3(log_or_null, "Failed writing file: `", project_filepath, "`\n");
	}

	return ok;
}

BUILD_API void BUILD_InitProject(BUILD_Project* project, const char* name, const BUILD_ProjectOptions* options) {
	memset(project, 0, sizeof(*project));

	BUILD_InitArena(&project->arena, 4096);
	project->name = name;
	project->opts = *options;

	project->source_files.arena = &project->arena;
	project->natvis_files.arena = &project->arena;
	project->source_dirs.arena = &project->arena;
	project->defines.arena = &project->arena;
	project->include_dirs.arena = &project->arena;
	project->linker_inputs.arena = &project->arena;
	project->extra_linker_args.arena = &project->arena;
	project->extra_compiler_args.arena = &project->arena;
}

BUILD_API void BUILD_DestroyProject(BUILD_Project* project) {
	BUILD_DestroyArena(&project->arena);
}

BUILD_API void BUILD_AddSourceFile(BUILD_Project* project, const char* source_file) {
	BUILD_ArrayPush(&project->source_files, source_file);
}

BUILD_API void BUILD_AddVisualStudioNatvisFile(BUILD_Project* project, const char* natvis_file) {
	BUILD_ArrayPush(&project->natvis_files, natvis_file);
}

BUILD_API void BUILD_AddSourceDir(BUILD_Project* project, const char* source_dir) {
	BUILD_ArrayPush(&project->source_dirs, source_dir);
}

BUILD_API void BUILD_AddDefine(BUILD_Project* project, const char* define) {
	BUILD_ArrayPush(&project->defines, define);
}

BUILD_API void BUILD_AddIncludeDir(BUILD_Project* project, const char* include_dir) {
	BUILD_ArrayPush(&project->include_dirs, include_dir);
}

BUILD_API void BUILD_AddLinkerInput(BUILD_Project* project, const char* linker_input) {
	BUILD_ArrayPush(&project->linker_inputs, linker_input);
}

BUILD_API void BUILD_AddExtraLinkerArg(BUILD_Project* project, const char* arg_string) {
	BUILD_ArrayPush(&project->extra_linker_args, arg_string);
}

BUILD_API void BUILD_AddExtraCompilerArg(BUILD_Project* project, const char* arg_string) {
	BUILD_ArrayPush(&project->extra_compiler_args, arg_string);
}

static void BUILD_OSConsolePrint(BUILD_Log* log, const char* message) {
	printf("%s", message);
}

BUILD_API BUILD_Log* BUILD_GetConsole() {
	static const BUILD_Log log = { BUILD_OSConsolePrint };
	return (BUILD_Log*)&log;
}

BUILD_API bool BUILD_CompileProject(BUILD_Project* project, const char* project_directory, const char* relative_build_directory,
	BUILD_Log* log_or_null)
{
	BUILD_Arena* arena = &project->arena;
	bool ok = true;

	if (ok) {
		BUILD_FindVSResult windows_sdk = BUILD_FindVisualStudioAndWindowsSDK(arena);
		const wchar_t* msvc_directory = windows_sdk.vs_exe_path; // contains cl.exe, link.exe
		const wchar_t* windows_sdk_include_base_path = windows_sdk.windows_sdk_include_base; // contains <string.h>, etc
		const wchar_t* windows_sdk_um_library_path = windows_sdk.windows_sdk_um_library_path; // contains kernel32.lib, etc
		const wchar_t* windows_sdk_ucrt_library_path = windows_sdk.windows_sdk_ucrt_library_path; // contains libucrt.lib, etc
		const wchar_t* vs_library_path = windows_sdk.vs_library_path; // contains MSVCRT.lib etc
		const wchar_t* vs_include_path = windows_sdk.vs_include_path; // contains vcruntime.h

		if (project->opts.windows.use_msbuild) {
			assert(project->opts.target != BUILD_Target_ObjectFileOnly); // you must have a linker stage when using msbuild

			__debugbreak(); // TODO
#if 0
			const char* vs_base_path = StrFromUTF16(windows_sdk.vs_base_path, temp);

			const char* msbuild_path = StrJoin(temp, vs_base_path, L("\\MSBuild\\Current\\Bin\\amd64\\MSBuild.exe"));

			const char* vs_project_file_name = PathStem(output_file_abs);
			const char* vs_project_file_path = StrJoin(temp, directory_wide, L("/"), vs_project_file_name, L(".vcxproj"));

			const char* project_guid = OS_GenerateWindowsGUID(temp);
			ok = OS_GenerateVisualStudioProject(temp, working_dir, vs_project_file_path, PathDir(output_file_abs), project_guid, project, error_log);
			if (ok) {
				// MSBuild depends on the `Platform` environment variable...
				SetEnvironmentVariableW(StrToUTF16(L("Platform"), 1, temp, NULL), StrToUTF16(L("x64"), 1, temp, NULL));

				const char* args[] = { msbuild_path, vs_project_file_path };
				U32 exit_code = 0;
				if (!OS_RunCommand(working_dir, args, Len(args), &exit_code, error_log, error_log)) {
					ok = false; OS_LogPrint(error_log, "MSBuild.exe not found! Have you installed Visual Studio? If so, please run me from x64 Native Tools Command Prompt.\n");
				}
				if (exit_code != 0) ok = false;
			}
#endif
		}
		else {
			BUILD_StrBuilder abs_build_directory = { arena };
			BUILD_Print3(&abs_build_directory, project_directory, "/", relative_build_directory);
			BUILD_ArrayPush(&abs_build_directory, 0); // Push null-termination

			const wchar_t* abs_build_directory_wide = BUILD_UTF8ToWide(arena, abs_build_directory.data);

			BUILD_WStrBuilder msvc_args = { arena };
			BUILD_WPrint3(&msvc_args, L"\"", msvc_directory, L"\\cl.exe\""); // print cl.exe path

			BUILD_WPrint1(&msvc_args, L" /WX"); // treat warnings as errors
			BUILD_WPrint1(&msvc_args, L" /W3"); // warning level 3

			for (int i = 0; i < project->defines.length; i++) {
				const wchar_t* define = BUILD_UTF8ToWide(arena, project->defines.data[i]);
				BUILD_WPrint2(&msvc_args, L" /D", define);
			}

			if (project->opts.debug_info) BUILD_WPrint1(&msvc_args, L" /Z7");
			if (!project->opts.enable_warning_unused_variables) BUILD_WPrint1(&msvc_args, L" /wd4101");
			if (!project->opts.disable_warning_unhandled_switch_cases) BUILD_WPrint1(&msvc_args, L" /w14062");
			if (!project->opts.disable_warning_shadowed_locals) BUILD_WPrint1(&msvc_args, L" /w14456");

			for (int i = 0; i < project->extra_compiler_args.length; i++) {
				const wchar_t* arg = BUILD_UTF8ToWide(arena, project->extra_compiler_args.data[i]);
				BUILD_WPrint1(&msvc_args, L" ");
				BUILD_WPrint1(&msvc_args, arg);
			}

			if (project->opts.c_runtime_library_debug) __debugbreak(); // TODO
			if (project->opts.c_runtime_library_dll)   __debugbreak(); // TODO

			for (int i = 0; i < project->source_files.length; i++) {
				const wchar_t* source_file = BUILD_UTF8ToWide(arena, project->source_files.data[i]);
				BUILD_WPrint3(&msvc_args, L" \"", source_file, L"\"");
			}

			for (int i = 0; i < project->include_dirs.length; i++) {
				const wchar_t* include_dir = BUILD_UTF8ToWide(arena, project->include_dirs.data[i]);
				BUILD_WPrint3(&msvc_args, L" \"/I", include_dir, L"\"");
			}

			BUILD_WPrint3(&msvc_args, L" \"/Fo", abs_build_directory_wide, L"/\"");
			BUILD_WPrint3(&msvc_args, L" \"/I", windows_sdk_include_base_path, L"\\shared\"");
			BUILD_WPrint3(&msvc_args, L" \"/I", windows_sdk_include_base_path, L"\\ucrt\"");
			BUILD_WPrint3(&msvc_args, L" \"/I", windows_sdk_include_base_path, L"\\um\"");
			BUILD_WPrint3(&msvc_args, L" \"/I", vs_include_path, L"\"");

			switch (project->opts.target) {
			case BUILD_Target_Executable: break;
			case BUILD_Target_DynamicLibrary: BUILD_WPrint1(&msvc_args, L" /LD"); break;
			case BUILD_Target_ObjectFileOnly: BUILD_WPrint1(&msvc_args, L" /c"); break;
			}

			if (project->opts.target != BUILD_Target_ObjectFileOnly) {
				const wchar_t* project_name = BUILD_UTF8ToWide(arena, project->name);
				const wchar_t* file_extension = project->opts.target == BUILD_Target_Executable ? L".exe" : L".dll";

				BUILD_WPrint1(&msvc_args, L" /Fe");
				BUILD_WPrint1(&msvc_args, abs_build_directory_wide);
				BUILD_WPrint1(&msvc_args, L"\\");
				BUILD_WPrint1(&msvc_args, project_name);
				BUILD_WPrint1(&msvc_args, file_extension);

				BUILD_WPrint1(&msvc_args, L" /link");
				BUILD_WPrint1(&msvc_args, L" /NOLOGO"); // disable Microsoft linker startup banner text
				BUILD_WPrint1(&msvc_args, L" /INCREMENTAL:NO");

				if (project->opts.windows.disable_console) BUILD_WPrint1(&msvc_args, L" /SUBSYSTEM:WINDOWS");
				if (project->extra_linker_args.length > 0) __debugbreak(); // TODO

				for (int i = 0; i < project->linker_inputs.length; i++) {
					const wchar_t* linker_input = BUILD_UTF8ToWide(arena, project->linker_inputs.data[i]);
					BUILD_WPrint3(&msvc_args, L" \"", linker_input, L"\"");
				}

				BUILD_WPrint3(&msvc_args, L" \"/LIBPATH:", windows_sdk_um_library_path, L"\"");
				BUILD_WPrint3(&msvc_args, L" \"/LIBPATH:", windows_sdk_ucrt_library_path, L"\"");
				BUILD_WPrint3(&msvc_args, L" \"/LIBPATH:", vs_library_path, L"\"");
			}

			BUILD_ArrayPush(&msvc_args, 0); // Push null-termination

			uint32_t exit_code = 0;
			if (ok) ok = BUILD_RunProcess(msvc_args.data, &exit_code, log_or_null);
			if (exit_code != 0) ok = false;
		}
	}

	return ok;
}

BUILD_API char* BUILD_Concat2(BUILD_Project* project, const char* a, const char* b) {
	return BUILD_Concat4(project, a, b, NULL, NULL);
}

BUILD_API char* BUILD_Concat3(BUILD_Project* project, const char* a, const char* b, const char* c) {
	return BUILD_Concat4(project, a, b, c, NULL);
}

BUILD_API char* BUILD_Concat4(BUILD_Project* project, const char* a, const char* b, const char* c, const char* d) {
	BUILD_StrBuilder s = { &project->arena };
	if (a) BUILD_Print1(&s, a);
	if (b) BUILD_Print1(&s, b);
	if (c) BUILD_Print1(&s, c);
	if (d) BUILD_Print1(&s, d);
	BUILD_ArrayPush(&s, 0); // null-terminate;
	return s.data;
}

BUILD_API bool BUILD_CreateDirectory(const char* directory) {
	BUILD_Arena arena;
	BUILD_InitArena(&arena, 128);

	wchar_t* path_wide = BUILD_UTF8ToWide(&arena, directory);
	bool success = CreateDirectoryW(path_wide, NULL);

	BUILD_DestroyArena(&arena);
	return success || GetLastError() == ERROR_ALREADY_EXISTS;
}

BUILD_API bool BUILD_CreateVisualStudioSolution(const char* project_directory, const char* relative_build_directory,
	const char* solution_name, BUILD_Project** projects, int projects_count, BUILD_Log* log_or_null)
{
	BUILD_Arena arena;
	BUILD_InitArena(&arena, 4096);

	bool ok = true;

	BUILD_StrBuilder s = { &arena };
	BUILD_Print1(&s, "Microsoft Visual Studio Solution File, Format Version 12.00\n");
	BUILD_Print1(&s, "# Visual Studio Version 17\n");
	BUILD_Print1(&s, "VisualStudioVersion = 17.6.33712.159\n");
	BUILD_Print1(&s, "MinimumVisualStudioVersion = 10.0.40219.1\n");

	BUILD_Array(const char*) project_guids = { &arena };

	for (size_t i = 0; i < projects_count; i++) {
		BUILD_Project* project = projects[i];
		if (project == NULL) continue;

		char* project_guid;
		{ // Create new GUID
			GUID guid;
			CoCreateGuid(&guid);
			wchar_t guid_wstr[128];
			int guid_wstr_len_including_null = StringFromGUID2(&guid, guid_wstr, 128);
			project_guid = BUILD_ArenaPush(&arena, guid_wstr_len_including_null);

			wchar_t* src = guid_wstr; char* dst = project_guid;
			while (*src) {
				*dst = (char)*src;
				src++; dst++;
			}
			*dst = 0;
		}

		BUILD_ArrayPush(&project_guids, project_guid);

		BUILD_StrBuilder project_filepath = { &arena };
		BUILD_Print4(&project_filepath, project_directory, "/", project->name, ".vcxproj");
		BUILD_ArrayPush(&project_filepath, '\0'); // null-terminate

		BUILD_StrBuilder project_name = { &arena };
		BUILD_Print2(&project_name, project->name, ".vcxproj");
		BUILD_ArrayPush(&project_name, '\0'); // null-terminate

		if (ok) {
			ok = BUILD_GenerateVisualStudioProject(&arena, project,
				project_filepath.data, project_directory, relative_build_directory, project_guid, log_or_null);
		}

		BUILD_Print3(&s, "Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \"", project->name, "\"");
		BUILD_Print3(&s, ", \"", project_name.data, "\"");
		BUILD_Print3(&s, ", \"", project_guid, "\"\n");
		BUILD_Print1(&s, "EndProject\n");
	}

	BUILD_Print1(&s, "Global\n");
	BUILD_Print1(&s, "  GlobalSection(SolutionConfigurationPlatforms) = preSolution\n");
	BUILD_Print1(&s, "  EndGlobalSection\n");
	BUILD_Print1(&s, "  GlobalSection(ProjectConfigurationPlatforms) = postSolution\n");

	for (int i = 0; i < projects_count; i++) {
		const char* project_guid = project_guids.data[i];
		BUILD_Print3(&s, "    ", project_guid, ".Custom|x64.ActiveCfg = Custom|x64\n");
		BUILD_Print3(&s, "    ", project_guid, ".Custom|x64.Build.0 = Custom|x64\n");
	}

	BUILD_Print1(&s, "  EndGlobalSection\n");
	BUILD_Print1(&s, "  GlobalSection(SolutionProperties) = preSolution\n");
	BUILD_Print1(&s, "    HideSolutionNode = FALSE\n");
	BUILD_Print1(&s, "  EndGlobalSection\n");
	BUILD_Print1(&s, "  GlobalSection(ExtensibilityGlobals) = postSolution\n");
	BUILD_Print1(&s, "    SolutionGuid = {E8A6471F-96EE-4CB5-A6F7-DD09AD151C28}\n");
	BUILD_Print1(&s, "  EndGlobalSection\n");
	BUILD_Print1(&s, "EndGlobal\n");

	if (ok) {
		BUILD_StrBuilder solution_filepath = { &arena };
		BUILD_Print3(&solution_filepath, project_directory, "/", solution_name);
		BUILD_ArrayPush(&solution_filepath, '\0'); // null-terminate

		FILE* file = fopen(solution_filepath.data, "wb");
		ok = file != NULL;
		if (ok) {
			fwrite(s.data, s.length, 1, file);
			fclose(file);
		}
	}

	BUILD_DestroyArena(&arena);
	return ok;
}

#endif // BUILD_IMPLEMENTATION
#endif // BUILD_INCLUDED