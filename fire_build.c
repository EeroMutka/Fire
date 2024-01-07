#include "fire.h"
#include "fire_os.h"
#include "fire_build.h"

// -- microsoft_craziness.h -----------------------------------------------------------------------------------------------
//
// Author:   Jonathan Blow
// Version:  1
// Date:     31 August, 2018
//
// This code is released under the MIT license, which you can find at
//
//          https://opensource.org/licenses/MIT
//
//
//
// See the comments for how to use this library just below the includes.
//
//
// NOTE(Kalinovcic): I have translated the original implementation to C,
// and made a preprocessor define that enables people to include it without
// the implementation, just as a header.
//
// I also fixed two bugs:
//   - If COM initialization for VS2017 fails, we actually properly continue
//     searching for earlier versions in the registry.
//   - For earlier versions, the code now returns the "bin\amd64" VS directory.
//     Previously it was returning the "bin" directory, which is for x86 stuff.

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <io.h>         // For _get_osfhandle
#include <string.h>

typedef struct {
	int windows_sdk_version;   // Zero if no Windows SDK found.

	wchar_t *windows_sdk_root;
	wchar_t *windows_sdk_include_base;
	wchar_t *windows_sdk_um_library_path;
	wchar_t *windows_sdk_ucrt_library_path;

	wchar_t *vs_base_path;
	wchar_t *vs_exe_path;
	wchar_t *vs_include_path;
	wchar_t *vs_library_path;
} Find_Result;

Find_Result find_visual_studio_and_windows_sdk();
void free_resources(Find_Result *result);

void free_resources(Find_Result *result) {
    free(result->windows_sdk_root);
    free(result->windows_sdk_include_base);
    free(result->windows_sdk_um_library_path);
    free(result->windows_sdk_ucrt_library_path);
    free(result->vs_base_path);
    free(result->vs_exe_path);
    free(result->vs_library_path);
    free(result->vs_include_path);
}

// COM objects for the ridiculous Microsoft craziness.

#undef  INTERFACE
#define INTERFACE ISetupInstance
DECLARE_INTERFACE_ (ISetupInstance, IUnknown)
{
    BEGIN_INTERFACE

    // IUnknown methods
    STDMETHOD (QueryInterface)   (THIS_  REFIID, void **) PURE;
    STDMETHOD_(ULONG, AddRef)    (THIS) PURE;
    STDMETHOD_(ULONG, Release)   (THIS) PURE;

    // ISetupInstance methods
    STDMETHOD(GetInstanceId)(THIS_ _Out_ BSTR* pbstrInstanceId) PURE;
    STDMETHOD(GetInstallDate)(THIS_ _Out_ LPFILETIME pInstallDate) PURE;
    STDMETHOD(GetInstallationName)(THIS_ _Out_ BSTR* pbstrInstallationName) PURE;
    STDMETHOD(GetInstallationPath)(THIS_ _Out_ BSTR* pbstrInstallationPath) PURE;
    STDMETHOD(GetInstallationVersion)(THIS_ _Out_ BSTR* pbstrInstallationVersion) PURE;
    STDMETHOD(GetDisplayName)(THIS_ _In_ LCID lcid, _Out_ BSTR* pbstrDisplayName) PURE;
    STDMETHOD(GetDescription)(THIS_ _In_ LCID lcid, _Out_ BSTR* pbstrDescription) PURE;
    STDMETHOD(ResolvePath)(THIS_ _In_opt_z_ LPCOLESTR pwszRelativePath, _Out_ BSTR* pbstrAbsolutePath) PURE;

    END_INTERFACE
};

#undef  INTERFACE
#define INTERFACE IEnumSetupInstances
DECLARE_INTERFACE_ (IEnumSetupInstances, IUnknown)
{
    BEGIN_INTERFACE

    // IUnknown methods
    STDMETHOD (QueryInterface)   (THIS_  REFIID, void **) PURE;
    STDMETHOD_(ULONG, AddRef)    (THIS) PURE;
    STDMETHOD_(ULONG, Release)   (THIS) PURE;

    // IEnumSetupInstances methods
    STDMETHOD(Next)(THIS_ _In_ ULONG celt, _Out_writes_to_(celt, *pceltFetched) ISetupInstance** rgelt, _Out_opt_ _Deref_out_range_(0, celt) ULONG* pceltFetched) PURE;
    STDMETHOD(Skip)(THIS_ _In_ ULONG celt) PURE;
    STDMETHOD(Reset)(THIS) PURE;
    STDMETHOD(Clone)(THIS_ _Deref_out_opt_ IEnumSetupInstances** ppenum) PURE;

    END_INTERFACE
};

#undef  INTERFACE
#define INTERFACE ISetupConfiguration
DECLARE_INTERFACE_ (ISetupConfiguration, IUnknown)
{
    BEGIN_INTERFACE

    // IUnknown methods
    STDMETHOD (QueryInterface)   (THIS_  REFIID, void **) PURE;
    STDMETHOD_(ULONG, AddRef)    (THIS) PURE;
    STDMETHOD_(ULONG, Release)   (THIS) PURE;

    // ISetupConfiguration methods
    STDMETHOD(EnumInstances)(THIS_ _Out_ IEnumSetupInstances** ppEnumInstances) PURE;
    STDMETHOD(GetInstanceForCurrentProcess)(THIS_ _Out_ ISetupInstance** ppInstance) PURE;
    STDMETHOD(GetInstanceForPath)(THIS_ _In_z_ LPCWSTR wzPath, _Out_ ISetupInstance** ppInstance) PURE;

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
    wchar_t *best_name;
} Version_Data;

bool os_file_exists(wchar_t *name) {
    // @Robustness: What flags do we really want to check here?

    auto attrib = GetFileAttributesW(name);
    if (attrib == INVALID_FILE_ATTRIBUTES) return false;
    if (attrib & FILE_ATTRIBUTE_DIRECTORY) return false;

    return true;
}

#define concat2(a, b) concat(a, b, NULL, NULL)
#define concat3(a, b, c) concat(a, b, c, NULL)
#define concat4(a, b, c, d) concat(a, b, c, d)
wchar_t *concat(wchar_t *a, wchar_t *b, wchar_t *c, wchar_t *d) {
    // Concatenate up to 4 wide strings together. Allocated with malloc.
    // If you don't like that, use a programming language that actually
    // helps you with using custom allocators. Or just edit the code.

    size_t len_a = wcslen(a);
    size_t len_b = wcslen(b);

	size_t len_c = 0;
    if (c) len_c = wcslen(c);

	size_t len_d = 0;
    if (d) len_d = wcslen(d);

    wchar_t *result = (wchar_t *)malloc((len_a + len_b + len_c + len_d + 1) * 2);
    memcpy(result, a, len_a*2);
    memcpy(result + len_a, b, len_b*2);

    if (c) memcpy(result + len_a + len_b, c, len_c * 2);
    if (d) memcpy(result + len_a + len_b + len_c, d, len_d * 2);

    result[len_a + len_b + len_c + len_d] = 0;

    return result;
}

typedef void (*Visit_Proc_W)(wchar_t *short_name, wchar_t *full_name, Version_Data *data);
bool visit_files_w(wchar_t *dir_name, Version_Data *data, Visit_Proc_W proc) {

    // Visit everything in one folder (non-recursively). If it's a directory
    // that doesn't start with ".", call the visit proc on it. The visit proc
    // will see if the filename conforms to the expected versioning pattern.

    WIN32_FIND_DATAW find_data;

    wchar_t *wildcard_name = concat2(dir_name, L"\\*");
    HANDLE handle = FindFirstFileW(wildcard_name, &find_data);
    free(wildcard_name);

    if (handle == INVALID_HANDLE_VALUE) return false;

    while (true) {
        if ((find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && (find_data.cFileName[0] != '.')) {
            wchar_t *full_name = concat3(dir_name, L"\\", find_data.cFileName);
            proc(find_data.cFileName, full_name, data);
            free(full_name);
        }

        BOOL success = FindNextFileW(handle, &find_data);
        if (!success) break;
    }

    FindClose(handle);

    return true;
}

wchar_t *find_windows_kit_root_with_key(HKEY key, wchar_t *version) {
    // Given a key to an already opened registry entry,
    // get the value stored under the 'version' subkey.
    // If that's not the right terminology, hey, I never do registry stuff.

    DWORD required_length;
    auto rc = RegQueryValueExW(key, version, NULL, NULL, NULL, &required_length);
    if (rc != 0)  return NULL;

    DWORD length = required_length + 2;  // The +2 is for the maybe optional zero later on. Probably we are over-allocating.
    wchar_t *value = (wchar_t *)malloc(length);
    if (!value) return NULL;

    rc = RegQueryValueExW(key, version, NULL, NULL, (LPBYTE)value, &length);  // We know that version is zero-terminated...
    if (rc != 0)  return NULL;

    // The documentation says that if the string for some reason was not stored
    // with zero-termination, we need to manually terminate it. Sigh!!

    if (value[length]) {
        value[length+1] = 0;
    }

    return value;
}

void win10_best(wchar_t *short_name, wchar_t *full_name, Version_Data *data) {
    // Find the Windows 10 subdirectory with the highest version number.

    int i0, i1, i2, i3;
    auto success = swscanf_s(short_name, L"%d.%d.%d.%d", &i0, &i1, &i2, &i3);
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

    // we have to copy_string and free here because visit_files free's the full_name string
    // after we execute this function, so Win*_Data would contain an invalid pointer.
    if (data->best_name) free(data->best_name);
    data->best_name = _wcsdup(full_name);

    if (data->best_name) {
        data->best_version[0] = i0;
        data->best_version[1] = i1;
        data->best_version[2] = i2;
        data->best_version[3] = i3;
    }
}

void win8_best(wchar_t *short_name, wchar_t *full_name, Version_Data *data) {
    // Find the Windows 8 subdirectory with the highest version number.

    int i0, i1;
    auto success = swscanf_s(short_name, L"winv%d.%d", &i0, &i1);
    if (success < 2) return;

    if (i0 < data->best_version[0]) return;
    else if (i0 == data->best_version[0]) {
        if (i1 < data->best_version[1]) return;
    }

    // we have to copy_string and free here because visit_files free's the full_name string
    // after we execute this function, so Win*_Data would contain an invalid pointer.
    if (data->best_name) free(data->best_name);
    data->best_name = _wcsdup(full_name);

    if (data->best_name) {
        data->best_version[0] = i0;
        data->best_version[1] = i1;
    }
}

void find_windows_kit_root(Find_Result *result) {
    // Information about the Windows 10 and Windows 8 development kits
    // is stored in the same place in the registry. We open a key
    // to that place, first checking preferntially for a Windows 10 kit,
    // then, if that's not found, a Windows 8 kit.

    HKEY main_key;

    LSTATUS rc = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\Windows Kits\\Installed Roots",
                               0, KEY_QUERY_VALUE | KEY_WOW64_32KEY | KEY_ENUMERATE_SUB_KEYS, &main_key);
    if (rc != S_OK) return;

    // Look for a Windows 10 entry.
    wchar_t *windows10_root = find_windows_kit_root_with_key(main_key, L"KitsRoot10");

    if (windows10_root) {
        wchar_t *windows10_lib = concat2(windows10_root, L"Lib");
        wchar_t *windows10_include = concat2(windows10_root, L"include");
        free(windows10_root);

        Version_Data lib_data = {0};
        visit_files_w(windows10_lib, &lib_data, win10_best);
        Version_Data include_data = {0};
        visit_files_w(windows10_include, &include_data, win10_best);

        free(windows10_lib);
        free(windows10_include);

        if (lib_data.best_name) {
            result->windows_sdk_version = 10;
            result->windows_sdk_root = lib_data.best_name;
            result->windows_sdk_include_base = include_data.best_name;
            RegCloseKey(main_key);
            return;
        }
    }

    // Look for a Windows 8 entry.
    wchar_t *windows8_root = find_windows_kit_root_with_key(main_key, L"KitsRoot81");

    if (windows8_root) {
        wchar_t *windows8_lib = concat2(windows8_root, L"Lib");
        free(windows8_root);

        Version_Data data = {0};
        visit_files_w(windows8_lib, &data, win8_best);
        free(windows8_lib);

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

bool find_visual_studio_2017_by_fighting_through_microsoft_craziness(Find_Result *result) {
    CoInitialize(NULL);
    // "Subsequent valid calls return false." So ignore false.
    // if rc != S_OK  return false;

    GUID my_uid                   = {0x42843719, 0xDB4C, 0x46C2, {0x8E, 0x7C, 0x64, 0xF1, 0x81, 0x6E, 0xFD, 0x5B}};
    GUID CLSID_SetupConfiguration = {0x177F0C4A, 0x1CD3, 0x4DE7, {0xA3, 0x2C, 0x71, 0xDB, 0xBB, 0x9F, 0xA3, 0x6D}};

    ISetupConfiguration *config = NULL;

    // NOTE(Kalinovcic): This is so stupid... These functions take references, so the code is different for C and C++......
#ifdef __cplusplus
    HRESULT hr = CoCreateInstance(CLSID_SetupConfiguration, NULL, CLSCTX_INPROC_SERVER, my_uid, (void **)&config);
#else
    HRESULT hr = CoCreateInstance(&CLSID_SetupConfiguration, NULL, CLSCTX_INPROC_SERVER, &my_uid, (void **)&config);
#endif

    if (hr != 0)  return false;

    IEnumSetupInstances *instances = NULL;
    hr = CALL_STDMETHOD(config, EnumInstances, &instances);
    CALL_STDMETHOD_(config, Release);
    if (hr != 0)     return false;
    if (!instances)  return false;

    bool found_visual_studio_2017 = false;
    while (1) {
        ULONG found = 0;
        ISetupInstance *instance = NULL;
        hr = CALL_STDMETHOD(instances, Next, 1, &instance, &found);
        if (hr != S_OK) break;

        BSTR bstr_inst_path;
        hr = CALL_STDMETHOD(instance, GetInstallationPath, &bstr_inst_path);
        CALL_STDMETHOD_(instance, Release);
        if (hr != S_OK)  continue;

        wchar_t *tools_filename = concat2(bstr_inst_path, L"\\VC\\Auxiliary\\Build\\Microsoft.VCToolsVersion.default.txt");
        SysFreeString(bstr_inst_path);

        FILE *f;
        errno_t open_result = _wfopen_s(&f, tools_filename, L"rt");
        free(tools_filename);
        if (open_result != 0) continue;
        if (!f) continue;

        LARGE_INTEGER tools_file_size;
        HANDLE file_handle = (HANDLE)_get_osfhandle(_fileno(f));
        BOOL success = GetFileSizeEx(file_handle, &tools_file_size);
        if (!success) {
            fclose(f);
            continue;
        }

        uint64_t version_bytes = (tools_file_size.QuadPart + 1) * 2;  // Warning: This multiplication by 2 presumes there is no variable-length encoding in the wchars (wacky characters in the file could betray this expectation).
        wchar_t *version = (wchar_t *)malloc(version_bytes);

        wchar_t *read_result = fgetws(version, (int)version_bytes, f);
        fclose(f);
        if (!read_result) continue;

        wchar_t *version_tail = wcschr(version, '\n');
        if (version_tail)  *version_tail = 0;  // Stomp the data, because nobody cares about it.

        wchar_t *library_path = concat4(bstr_inst_path, L"\\VC\\Tools\\MSVC\\", version, L"\\lib\\x64");
        wchar_t *library_file = concat2(library_path, L"\\vcruntime.lib");  // @Speed: Could have library_path point to this string, with a smaller count, to save on memory flailing!

        if (os_file_exists(library_file)) {
            wchar_t *link_exe_path = concat4(bstr_inst_path, L"\\VC\\Tools\\MSVC\\", version, L"\\bin\\Hostx64\\x64");
            free(version);
			
			result->vs_base_path = concat2(bstr_inst_path, L"");
            result->vs_exe_path     = link_exe_path;
            result->vs_library_path = library_path;
            result->vs_include_path = concat4(bstr_inst_path, L"\\VC\\Tools\\MSVC\\", version, L"\\include");
            found_visual_studio_2017 = true;
            break;
        }

        free(version);

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

void find_visual_studio_by_fighting_through_microsoft_craziness(Find_Result *result) {
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

    bool found_visual_studio_2017 = find_visual_studio_2017_by_fighting_through_microsoft_craziness(result);
    if (found_visual_studio_2017)  return;

    // If we get here, we didn't find Visual Studio 2017. Try earlier versions.

    HKEY vs7_key;
    HRESULT rc = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\Microsoft\\VisualStudio\\SxS\\VS7", 0, KEY_QUERY_VALUE | KEY_WOW64_32KEY, &vs7_key);
    if (rc != S_OK)  return;

    // Hardcoded search for 4 prior Visual Studio versions. Is there something better to do here?
    wchar_t *versions[] = { L"14.0", L"12.0", L"11.0", L"10.0" };
    const int NUM_VERSIONS = sizeof(versions) / sizeof(versions[0]);

    for (int i = 0; i < NUM_VERSIONS; i++) {
        wchar_t *v = versions[i];

        DWORD dw_type;
        DWORD cb_data;

        rc = RegQueryValueExW(vs7_key, v, NULL, &dw_type, NULL, &cb_data);
        if ((rc == ERROR_FILE_NOT_FOUND) || (dw_type != REG_SZ)) {
            continue;
        }

        wchar_t *buffer = (wchar_t *)malloc(cb_data);
        if (!buffer)  return;

        rc = RegQueryValueExW(vs7_key, v, NULL, NULL, (LPBYTE)buffer, &cb_data);
        if (rc != 0)  continue;

        // @Robustness: Do the zero-termination thing suggested in the RegQueryValue docs?

        wchar_t *lib_path = concat2(buffer, L"VC\\Lib\\amd64");

        // Check to see whether a vcruntime.lib actually exists here.
        wchar_t *vcruntime_filename = concat2(lib_path, L"\\vcruntime.lib");
        bool vcruntime_exists = os_file_exists(vcruntime_filename);
        free(vcruntime_filename);

        if (vcruntime_exists) {
			result->vs_base_path    = concat2(buffer, L""); // NOTE(Eero): this is total guess, it might be incorrect!
            result->vs_exe_path     = concat2(buffer, L"VC\\bin\\amd64");
            result->vs_library_path = lib_path;
            result->vs_include_path = concat2(buffer, L"VC\\include\\amd64"); // NOTE(Eero): this is total guess, it might be incorrect!
            
            free(buffer);
            RegCloseKey(vs7_key);
            return;
        }

        free(lib_path);
        free(buffer);
    }

    RegCloseKey(vs7_key);

    // If we get here, we failed to find anything.
}

Find_Result find_visual_studio_and_windows_sdk() {
    Find_Result result;

    find_windows_kit_root(&result);

    if (result.windows_sdk_root) {
        result.windows_sdk_um_library_path   = concat2(result.windows_sdk_root, L"\\um\\x64");
        result.windows_sdk_ucrt_library_path = concat2(result.windows_sdk_root, L"\\ucrt\\x64");
    }

    find_visual_studio_by_fighting_through_microsoft_craziness(&result);

    return result;
}

// -- end of microsoft_craziness.h ------------------------------------------------------------------------------

#define FB_FN

typedef struct FB_Project {
	FOS_Arena* arena;
	FOS_String output_file;
	FB_ProjectOptions opts;

	Array(FOS_String) source_files;
	Array(FOS_String) include_dirs;
	Array(FOS_String) defines;
	Array(FOS_String) linker_inputs;
	Array(FOS_String) extra_linker_args;
	Array(FOS_String) extra_compiler_args;
} FB_Project;

static String FOS_GenerateWindowsGUID(FOS_Arena* arena) {
	GUID guid; wchar_t str_utf16[128];
	CoCreateGuid(&guid);
	StringFromGUID2(&guid, str_utf16, 128);
	return StrFromUTF16(str_utf16, arena);
}

static bool FOS_GenerateVisualStudioProject(Arena* temp, FOS_WorkingDir working_dir, String vs_project_path, String output_dir_abs,
	String project_guid, const FB_Project* args, FOS_Log* error_log)
{
	Assert(FOS_PathIsAbsolute(vs_project_path));
	Assert(FOS_PathIsAbsolute(output_dir_abs));

	StringBuilder s = {.arena = temp};
	
	Print(&s,
		"<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
		"<Project DefaultTargets=\"Build\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n");
	
	Print(&s,
		"<ItemGroup Label=\"ProjectConfigurations\">\n"
		"	<ProjectConfiguration Include=\"Custom|x64\">\n"
		"		<Configuration>Custom</Configuration>\n"
		"		<Platform>x64</Platform>\n"
		"	</ProjectConfiguration>\n"
		"</ItemGroup>\n"
	);
	
	Print(&s, "<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />\n");
	Print(&s,
		"<PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Custom|x64'\" Label=\"Configuration\">\n"
		"	<ConfigurationType>Application</ConfigurationType>\n"
		"	<UseDebugLibraries>false</UseDebugLibraries>\n"
		"	<CharacterSet>Unicode</CharacterSet>\n"
		"	<PlatformToolset>v143</PlatformToolset>\n"
		"</PropertyGroup>\n"
	);
	
	Print(&s, "<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />\n");
	Print(&s,
		"<ImportGroup Label=\"ExtensionSettings\">\n"
		"</ImportGroup>\n"
	);
	
	//Print(&s, "<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />\n");
	
	Print(&s,
		"<ImportGroup Label=\"PropertySheets\" Condition=\"'$(Configuration)|$(Platform)'=='Custom|x64'\">\n"
		"	<Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" />\n"
		"</ImportGroup>\n"
	);
	
	/*F_Print(&s,
		"<ImportGroup Label=\"PropertySheets\" Condition=\"'$(Configuration)|$(Platform)'=='Profile|x64'\">\n"
		"	<Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" />\n"
		"</ImportGroup>\n"
	);
	
	Print(&s,
		"<ImportGroup Label=\"PropertySheets\" Condition=\"'$(Configuration)|$(Platform)'=='Release|x64'\">\n"
		"	<Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" />\n"
		"</ImportGroup>\n"
	);*/
	
	Print(&s, "<PropertyGroup Label=\"UserMacros\" />\n");
	Print(&s, 
		"<PropertyGroup Condition=\"\'$(Configuration)|$(Platform)\'==\'Custom|x64\'\">"
		"<OutDir>~s/</OutDir>"
		"<IntDir>./</IntDir>"
		"</PropertyGroup>",
		output_dir_abs
	);
	
	{
		Print(&s, "<ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='Custom|x64'\">\n");
		
		{
			Print(&s, "<ClCompile>\n");
			
			Print(&s, "<PrecompiledHeader>NotUsing</PrecompiledHeader>\n");
			Print(&s, "<WarningLevel>Level3</WarningLevel>\n");
			
			{
				Print(&s, "<PreprocessorDefinitions>");
				ArrayEach(String, args->defines, it) {
					Print(&s, "~s;", *it.elem);
				}
				Print(&s, "</PreprocessorDefinitions>\n");
			}
			
			Print(&s, "<AdditionalIncludeDirectories>");
			ArrayEach(String, args->include_dirs, it) {
				String include_dir;
				if (!FOS_PathToCanonical(temp, working_dir, *it.elem, &include_dir)) {
					FOS_LogPrint(error_log, "Invalid include directory: `~s`\n", *it.elem);
					return false;
				}
				Print(&s, "~s;", include_dir);
			}
			Print(&s, "</AdditionalIncludeDirectories>\n");
			
			if (args->opts.debug_info) {
				Print(&s, "<DebugInformationFormat>OldStyle</DebugInformationFormat>\n"); // OldStyle is the same as /Z7
			}
			if (args->opts.enable_optimizations) {
				Print(&s, "<Optimization>Full</Optimization>\n");
				Print(&s, "<FunctionLevelLinking>true</FunctionLevelLinking>\n");
				Print(&s, "<IntrinsicFunctions>true</IntrinsicFunctions>\n");
				Print(&s, "<MinimalRebuild>false</MinimalRebuild>\n");
				Print(&s, "<StringPooling>true</StringPooling>\n");
			}
			else {
				Print(&s, "<Optimization>Disabled</Optimization>\n");
			}
			Print(&s, "<ExceptionHandling>false</ExceptionHandling>\n");
			Print(&s, "<RuntimeTypeInfo>false</RuntimeTypeInfo>\n");
			Print(&s, "<ExternalWarningLevel>Level3</ExternalWarningLevel>\n");
			Print(&s, "<TreatWarningAsError>true</TreatWarningAsError>\n");
			{
				Print(&s, "<AdditionalOptions>");
				if (!args->opts.enable_warning_unused_variables) Print(&s, "/wd4101 ");
				if (!args->opts.disable_warning_unhandled_switch_cases) Print(&s, "/w14062 ");
				if (!args->opts.disable_warning_shadowed_locals) Print(&s, "/w14456 ");
				
				ArrayEach(String, args->extra_compiler_args, it) Print(&s, "~s ", *it.elem);
				
				Print(&s, "</AdditionalOptions>\n");
			}
			
			String crt_linking_type = args->opts.c_runtime_library_debug ?
				(args->opts.c_runtime_library_dll ? L("MultiThreadedDebugDLL") : L("MultiThreadedDebug")) :
				(args->opts.c_runtime_library_dll ? L("MultiThreadedDLL") : L("MultiThreaded"));
			Print(&s, "<RuntimeLibrary>~s</RuntimeLibrary>\n", crt_linking_type);
			
			Print(&s, "</ClCompile>\n");
		}
		
		{
			Print(&s, "<Link>\n");
			
			Print(&s, args->opts.windows.disable_console ? "<SubSystem>Windows</SubSystem>\n" : "<SubSystem>Console</SubSystem>\n");
			
			if (args->opts.enable_optimizations) {
				Print(&s, "<EnableCOMDATFolding>true</EnableCOMDATFolding>\n");
				Print(&s, "<OptimizeReferences>true</OptimizeReferences>\n");
			}
			
			if (args->opts.debug_info) {
				Print(&s, "<GenerateDebugInformation>true</GenerateDebugInformation>\n");
			}
			
			Print(&s, "<AdditionalDependencies>");
			ArrayEach(String, args->linker_inputs, it) {
				String linker_input;
				if (!FOS_PathToCanonical(temp, working_dir, *it.elem, &linker_input)) {
					FOS_LogPrint(error_log, "Invalid linker input: `~s`\n", *it.elem);
					return false;
				}
				Print(&s, "~s;", linker_input);
			}
			Print(&s, "</AdditionalDependencies>\n");
			
			{
				Print(&s, "<AdditionalOptions>");
				ArrayEach(String, args->extra_linker_args, it) Print(&s, "~s ", *it.elem);
				Print(&s, "/IGNORE:4099 "); // LNK4099: PDB ... was not found with ... or at ''; linking object as if no debug info
				if (!args->opts.enable_aslr) Print(&s, "/DYNAMICBASE:NO ");
				Print(&s, "</AdditionalOptions>\n");
			}
			
			Print(&s, "</Link>\n");
		}
		Print(&s, "</ItemDefinitionGroup>\n");
	}
	
	ArrayEach(String, args->source_files, it) {
		String source_file;
		if (!FOS_PathToCanonical(temp, working_dir, *it.elem, &source_file)) {
			FOS_LogPrint(error_log, "Invalid source file: `~s`\n", *it.elem);
			return false;
		}
		
		Print(&s, "<ItemGroup>\n");
		Print(&s, "	<ClCompile Include=\"~s\" />\n", source_file);
		Print(&s, "</ItemGroup>\n");
	}
	
	Print(&s, "<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />\n");
	
	Print(&s,
		"<ImportGroup Label=\"ExtensionTargets\">\n"
		"</ImportGroup>\n"
	);
	
	Print(&s, "</Project>\n");
	
	if (!FOS_WriteEntireFile(working_dir, vs_project_path, s.str)) {
		FOS_LogPrint(error_log, "Failed writing file: `~s`\n", vs_project_path);
		return false;
	}
	return true;
}

FB_FN FB_Project* FB_MakeProject(FOS_Arena* arena, FOS_String output_file, const FB_ProjectOptions* options) {
	return New(FB_Project, arena, .arena = arena, .output_file = output_file, .opts = *options);
}
FB_FN void FB_AddSourceFile(FB_Project* project, FOS_String source_file) {
	ArrayPush(project->arena, &project->source_files, source_file);
}
FB_FN void FB_AddDefine(FB_Project* project, FOS_String define) {
	ArrayPush(project->arena, &project->defines, define);
}
FB_FN void FB_AddIncludeDir(FB_Project* project, FOS_String include_dir) {
	ArrayPush(project->arena, &project->include_dirs, include_dir);
}
FB_FN void FB_AddLinkerInput(FB_Project* project, FOS_String linker_input) {
	ArrayPush(project->arena, &project->linker_inputs, linker_input);
}
FB_FN void FB_AddExtraLinkerArg(FB_Project* project, FOS_String arg_string) {
	ArrayPush(project->arena, &project->extra_linker_args, arg_string);
}
FB_FN void FB_AddExtraCompilerArg(FB_Project* project, FOS_String arg_string) {
	ArrayPush(project->arena, &project->extra_compiler_args, arg_string);
}

FB_FN bool FB_CompileCode(FB_Project* project, FOS_WorkingDir working_dir, FOS_String intermediate_dir, FOS_Log* error_log) {
	Arena* temp = g_temp_arena;
	ArenaMark T = ArenaGetMark(temp);
	// StringBuilder errors = {.arena = arena};

	bool ok = true;
	if (project->opts.windows.use_msbuild && project->output_file.len == 0) {
		ok = false; FOS_LogPrint(error_log, "`windows.use_msbuild` is set and the linker is disabled; you can't do both at the same time.\n");
	}

	String output_file_abs, intermediate_dir_abs;
	if (ok && !FOS_PathToCanonical(temp , working_dir, intermediate_dir, &intermediate_dir_abs)) {
		ok = false; FOS_LogPrint(error_log, "Invalid intermediate directory: ~s\n", intermediate_dir);
	}
	if (ok && !FOS_PathToCanonical(temp , working_dir, project->output_file, &output_file_abs)) {
		ok = false; FOS_LogPrint(error_log, "Invalid output filepath: ~s\n", project->output_file);
	}

	if (ok) {
		String arg;
		Find_Result windows_sdk = find_visual_studio_and_windows_sdk();
		String msvc_directory = StrFromUTF16(windows_sdk.vs_exe_path, temp ); // contains cl.exe, link.exe
		String windows_sdk_include_base_path = StrFromUTF16(windows_sdk.windows_sdk_include_base, temp ); // contains <string.h>, etc
		String windows_sdk_um_library_path = StrFromUTF16(windows_sdk.windows_sdk_um_library_path, temp ); // contains kernel32.lib, etc
		String windows_sdk_ucrt_library_path = StrFromUTF16(windows_sdk.windows_sdk_ucrt_library_path, temp ); // contains libucrt.lib, etc
		String vs_library_path = StrFromUTF16(windows_sdk.vs_library_path, temp); // contains MSVCRT.lib etc
		String vs_include_path = StrFromUTF16(windows_sdk.vs_include_path, temp); // contains vcruntime.h
	
		if (project->opts.windows.use_msbuild) {
			String vs_base_path = StrFromUTF16(windows_sdk.vs_base_path, temp);
			
			String msbuild_path = StrJoin(temp, vs_base_path, L("\\MSBuild\\Current\\Bin\\amd64\\MSBuild.exe"));
		
			String vs_project_file_name = PathStem(output_file_abs);
			String vs_project_file_path = StrJoin(temp, intermediate_dir_abs, L("/"), vs_project_file_name, L(".vcxproj"));
		
			String project_guid = FOS_GenerateWindowsGUID(temp);
			ok = FOS_GenerateVisualStudioProject(temp, working_dir, vs_project_file_path, PathDir(output_file_abs), project_guid, project, error_log);
			if (ok) {
				// MSBuild depends on the `Platform` environment variable...
				SetEnvironmentVariableW(StrToUTF16(L("Platform"), 1, temp, NULL), StrToUTF16(L("x64"), 1, temp, NULL));

				String args[] = {msbuild_path, vs_project_file_path};
				U32 exit_code = 0;
				if (!FOS_RunCommand(working_dir, args, Len(args), &exit_code, error_log, error_log)) {
					ok = false; FOS_LogPrint(error_log, "MSBuild.exe not found! Have you installed Visual Studio? If so, please run me from x64 Native Tools Command Prompt.\n");
				}
				if (exit_code != 0) ok = false;
			}
		}
		else {
			String cl_exe_path = StrJoin(temp, msvc_directory, L("\\cl.exe"));

			Array(String) msvc_args = {0};
			ArrayPush(temp, &msvc_args, cl_exe_path);
		
			ArrayPush(temp, &msvc_args, L("/WX")); // treat warnings as errors
			ArrayPush(temp, &msvc_args, L("/W3")); // warning level 3
		
			ArrayEach(String, project->defines, it) {
				arg = StrJoin(temp, L("/D"), *it.elem); ArrayPush(temp, &msvc_args, arg);
			}

			if (project->opts.debug_info) ArrayPush(temp, &msvc_args, L("/Z7"));
			if (!project->opts.enable_warning_unused_variables) ArrayPush(temp, &msvc_args, L("/wd4101"));
			if (!project->opts.disable_warning_unhandled_switch_cases) ArrayPush(temp, &msvc_args, L("/w14062"));
			if (!project->opts.disable_warning_shadowed_locals) ArrayPush(temp, &msvc_args, L("/w14456"));
		
			ArrayEach(String, project->extra_compiler_args, it) ArrayPush(temp, &msvc_args, *it.elem);
		
			if (project->opts.c_runtime_library_debug) TODO();
			if (project->opts.c_runtime_library_dll) TODO();
		
			ArrayEach(String, project->source_files, it) {
				ArrayPush(temp, &msvc_args, *it.elem);
			}
		
			ArrayEach(String, project->include_dirs, it) {
				arg = StrJoin(temp, L("/I"), *it.elem); ArrayPush(temp, &msvc_args, arg);
			}
		
			arg = StrJoin(temp, L("/Fo"), intermediate_dir_abs, L("/")); ArrayPush(temp, &msvc_args, arg);
			arg = StrJoin(temp, L("/I"), windows_sdk_include_base_path, L("\\shared"));               ArrayPush(temp, &msvc_args, arg);
			arg = StrJoin(temp, L("/I"), windows_sdk_include_base_path, L("\\ucrt"));                 ArrayPush(temp, &msvc_args, arg);
			arg = StrJoin(temp, L("/I"), windows_sdk_include_base_path, L("\\um"));                   ArrayPush(temp, &msvc_args, arg);
			arg = StrJoin(temp, L("/I"), vs_include_path);                                                ArrayPush(temp, &msvc_args, arg);

			if (project->output_file.len == 0) {
				ArrayPush(temp, &msvc_args, L("/c"));
			}
			else {
				ArrayPush(temp, &msvc_args, L("/link"));
				ArrayPush(temp, &msvc_args, L("/INCREMENTAL:NO"));
			
				if (project->opts.windows.disable_console) ArrayPush(temp, &msvc_args, L("/SUBSYSTEM:WINDOWS"));
				if (project->extra_linker_args.len > 0) TODO();
			
				//String output_file_dir = PathDir(output_file);
				//String output_file_stem = PathStem(output_file);
				arg = StrJoin(temp, L("/OUT:"), output_file_abs); ArrayPush(temp, &msvc_args, arg);
			
				ArrayEach(String, project->linker_inputs, it) {
					ArrayPush(temp, &msvc_args, *it.elem);
				}
			
				arg = StrJoin(temp, L("/LIBPATH:"), windows_sdk_um_library_path);        ArrayPush(temp, &msvc_args, arg);
				arg = StrJoin(temp, L("/LIBPATH:"), windows_sdk_ucrt_library_path);      ArrayPush(temp, &msvc_args, arg);
				arg = StrJoin(temp, L("/LIBPATH:"), vs_library_path);                    ArrayPush(temp, &msvc_args, arg);
			}
		
			U32 exit_code = 0;
			if (!FOS_RunCommand(working_dir, msvc_args.data, msvc_args.len, &exit_code, error_log, error_log)) {
				ok = false; FOS_LogPrint(error_log, "cl.exe not found! Have you installed Visual Studio? If so, please run me from x64 Native Tools Command Prompt.\n");
			}
			if (exit_code != 0) ok = false;
		}
	
		free_resources(&windows_sdk);
	}

	ArenaSetMark(temp, T);
	return ok;
}

FB_FN bool FB_CreateVisualStudioSolution(FOS_WorkingDir working_dir, String intermediate_dir, String sln_file,
	FB_Project** projects, size_t projects_count, FOS_Log* error_log)
{
	Arena* temp = g_temp_arena;
	ArenaMark T = ArenaGetMark(temp);
	
	StringBuilder s = {.arena = temp};
	Print(&s,
		"\n"
		"Microsoft Visual Studio Solution File, Format Version 12.00\n"
		"# Visual Studio Version 17\n"
		"VisualStudioVersion = 17.6.33712.159\n"
		"MinimumVisualStudioVersion = 10.0.40219.1\n"
	);

	String intermediate_dir_abs;
	bool ok = true;
	if (!FOS_PathToCanonical(temp, working_dir, intermediate_dir, &intermediate_dir_abs)) {
		ok = false; FOS_LogPrint(error_log, "Invalid intermediate_dir: \"~s\"\n", intermediate_dir);
	}

	Array(String) project_guids = {0};
	for (size_t i = 0; i < projects_count; i++) {
		FB_Project* it = projects[i];

		String output_file_abs;
		Assert(it->output_file.len != 0);
		if (ok && !FOS_PathToCanonical(temp, working_dir, it->output_file, &output_file_abs)) {
			ok = false; FOS_LogPrint(error_log, "Invalid output file path: \"~s\"\n", it->output_file);
		}
		
		String project_file_name = PathStem(output_file_abs);
		String project_file_path = StrJoin(temp, intermediate_dir_abs, L("/"), project_file_name, L(".vcxproj"));

		String project_guid = FOS_GenerateWindowsGUID(temp);
		ArrayPush(temp, &project_guids, project_guid);

		if (ok) ok = FOS_GenerateVisualStudioProject(temp, working_dir, project_file_path, PathDir(output_file_abs), project_guid, it, error_log);
		
		Print(&s,
			"Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \"~s\", \"~s\", \"~s\"\n"
			"EndProject\n", project_file_name, project_file_path, project_guid);
	}

	Print(&s,
		"Global\n"
			"\tGlobalSection(SolutionConfigurationPlatforms) = preSolution\n"
				"\t\tDebug|x64 = Custom|x64\n"
				"\t\tEndGlobalSection\n"
			"\tGlobalSection(ProjectConfigurationPlatforms) = postSolution\n");
	
	for (size_t i = 0; i < projects_count; i++) {
		String project_guid = ArrayGet(project_guids, i);
		Print(&s, "\t\t~s.Custom|x64.ActiveCfg = Custom|x64\n", project_guid);
		Print(&s, "\t\t~s.Custom|x64.Build.0 = Custom|x64\n", project_guid);
	}

	Print(&s,
			"\tEndGlobalSection\n"
			"\tGlobalSection(SolutionProperties) = preSolution\n"
				"\t\tHideSolutionNode = FALSE\n"
			"\tEndGlobalSection\n"
			"\tGlobalSection(ExtensibilityGlobals) = postSolution\n"
				"\t\tSolutionGuid = {E8A6471F-96EE-4CB5-A6F7-DD09AD151C28}\n"
			"\tEndGlobalSection\n"
		"EndGlobal\n");
		
	if (ok && !FOS_WriteEntireFile(working_dir, sln_file, s.str)) {
		ok = false; FOS_LogPrint(error_log, "Invalid solution file path: \"~s\"\n", sln_file);
	}

	ArenaSetMark(temp, T);
	return ok;
}