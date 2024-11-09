// fire_build.h - by Eero Mutka (https://eeromutka.github.io/)
//
// This library lets you build C/C++ code or generate Visual Studio projects directly from code.
// Only Windows and MSVC are supported targets for time being.
// 
// This code is released under the MIT license (https://opensource.org/licenses/MIT).
// 

#ifndef FIRE_BUILD_INCLUDED
#define FIRE_BUILD_INCLUDED

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifndef BUILD_API
#define FIRE_BUILD_IMPLEMENTATION
#define BUILD_API static
#endif

typedef enum BUILD_Target {
	BUILD_Target_Executable,
	BUILD_Target_DynamicLibrary,
	BUILD_Target_ObjectFile, // Same as _Executable, but the linker won't be called
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
		//  bool use_msbuild; (TODO) // This will generate VS project files and call the MSBuild command to build the project.
	} windows;
} BUILD_ProjectOptions;

typedef struct BUILD_Project BUILD_Project;

typedef struct BUILD_Log {
	void (*print)(struct BUILD_Log* self, const char* message);
} BUILD_Log;

BUILD_API void BUILD_ProjectInit(BUILD_Project* project, const char* name, const BUILD_ProjectOptions* options);
BUILD_API void BUILD_ProjectDeinit(BUILD_Project* project);

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

// Filesystem utilities that are commonly needed for build scripts
BUILD_API bool BUILD_CreateDirectory(const char* directory); // If the directory already exists or is successfully created, true is returned.
BUILD_API bool BUILD_CopyFile(const char* file, const char* new_file);

// String concatenation utilities - the returned string is heap-allocated.
BUILD_API char* BUILD_Concat2(const char* a, const char* b);
BUILD_API char* BUILD_Concat3(const char* a, const char* b, const char* c);
BUILD_API char* BUILD_Concat4(const char* a, const char* b, const char* c, const char* d);

/* ---- end of the public API --------------------------------------------- */

#define BUILD_Array(T) struct { T* data; int count; int capacity; }
typedef BUILD_Array(wchar_t) BUILD_WStrBuilder;
typedef BUILD_Array(char) BUILD_StrBuilder;

struct BUILD_Project {
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

#ifdef /* ---------------- */ FIRE_BUILD_IMPLEMENTATION /* ---------------- */

#pragma comment(lib, "Ole32.lib") // for StringFromGUID2
#pragma comment(lib, "Advapi32.lib") // for RegCloseKey

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <io.h>         // For _get_osfhandle
#include <string.h>
#include <assert.h>
#include <Windows.h>

#define BUILD_ArrayReserve(ARR, CAPACITY) \
	while ((CAPACITY) > (ARR)->capacity) { \
		int new_capacity = (ARR)->capacity == 0 ? 8 : (ARR)->capacity * 2; \
		(*(void**)&(ARR)->data) = realloc((ARR)->data, new_capacity * sizeof(*(ARR)->data)); \
		(ARR)->capacity = new_capacity; \
	}

#define BUILD_ArrayPush(ARR, ELEM) \
	{ \
		BUILD_ArrayReserve(ARR, (ARR)->count + 1); \
		(ARR)->data[(ARR)->count] = ELEM; \
		(ARR)->count++; \
	}

#define BUILD_ArrayFree(ARR) { free((ARR)->data); }

#define BUILD_LogPrint1(LOG_OR_NULL, A)       BUILD_LogPrint4(LOG_OR_NULL, A, NULL, NULL, NULL)
#define BUILD_LogPrint2(LOG_OR_NULL, A, B)    BUILD_LogPrint4(LOG_OR_NULL, A, B, NULL, NULL)
#define BUILD_LogPrint3(LOG_OR_NULL, A, B, C) BUILD_LogPrint4(LOG_OR_NULL, A, B, C, NULL)
static void BUILD_LogPrint4(BUILD_Log* log_or_null, const char* A, const char* B, const char* C, const char* D) {
	if (log_or_null) {
		log_or_null->print(log_or_null, A);
		if (B) log_or_null->print(log_or_null, B);
		if (C) log_or_null->print(log_or_null, C);
		if (D) log_or_null->print(log_or_null, D);
	}
}

static void BUILD_WPrint(BUILD_WStrBuilder* builder, const wchar_t* string) {
	for (const wchar_t* ptr = string; *ptr; ptr++) {
		BUILD_ArrayPush(builder, *ptr);
	}
}

#define BUILD_WPrintNullTermination(BUILDER) BUILD_ArrayPush(BUILDER, 0)
#define BUILD_WPrint2(BUILDER, A, B)    BUILD_WPrint4(BUILDER, A, B, NULL, NULL)
#define BUILD_WPrint3(BUILDER, A, B, C) BUILD_WPrint4(BUILDER, A, B, C, NULL)
static void BUILD_WPrint4(BUILD_WStrBuilder* builder, const wchar_t* a, const wchar_t* b, const wchar_t* c, const wchar_t* d) {
	BUILD_WPrint(builder, a);
	BUILD_WPrint(builder, b);
	if (c) BUILD_WPrint(builder, c);
	if (d) BUILD_WPrint(builder, d);
}

static void BUILD_WPrintUTF8(BUILD_WStrBuilder* builder, const char* string) {
	int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, string, -1, NULL, 0) - 1;
	BUILD_ArrayReserve(builder, builder->count + size);
	MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, string, -1, builder->data + builder->count, size + 1);
	builder->count += size;
}

static void BUILD_Print(BUILD_StrBuilder* builder, const char* string) {
	for (const char* ptr = string; *ptr; ptr++) {
		BUILD_ArrayPush(builder, *ptr);
	}
}

#define BUILD_PrintNullTermination(BUILDER) BUILD_ArrayPush(BUILDER, 0)
#define BUILD_Print2(BUILDER, A, B)    BUILD_Print4(BUILDER, A, B, NULL, NULL)
#define BUILD_Print3(BUILDER, A, B, C) BUILD_Print4(BUILDER, A, B, C, NULL)
static void BUILD_Print4(BUILD_StrBuilder* builder, const char* a, const char* b, const char* c, const char* d) {
	BUILD_Print(builder, a);
	BUILD_Print(builder, b);
	if (c) BUILD_Print(builder, c);
	if (d) BUILD_Print(builder, d);
}

static void BUILD_PrintW(BUILD_StrBuilder* builder, wchar_t* string) {
	int size = WideCharToMultiByte(CP_UTF8, 0, string, -1, NULL, 0, NULL, NULL) - 1;
	BUILD_ArrayReserve(builder, builder->count + size);
	WideCharToMultiByte(CP_UTF8, 0, string, -1, builder->data + builder->count, size + 1, NULL, NULL);
	builder->count += size;
}

BUILD_API char* BUILD_Concat2(const char* a, const char* b) {
	BUILD_StrBuilder s = {0};
	BUILD_Print(&s, a);
	BUILD_Print(&s, b);
	BUILD_PrintNullTermination(&s);
	return s.data;
}

BUILD_API char* BUILD_Concat3(const char* a, const char* b, const char* c) {
	BUILD_StrBuilder s = {0};
	BUILD_Print(&s, a);
	BUILD_Print(&s, b);
	BUILD_Print(&s, c);
	BUILD_PrintNullTermination(&s);
	return s.data;
}

BUILD_API char* BUILD_Concat4(const char* a, const char* b, const char* c, const char* d) {
	BUILD_StrBuilder s = {0};
	BUILD_Print(&s, a);
	BUILD_Print(&s, b);
	BUILD_Print(&s, c);
	BUILD_Print(&s, d);
	BUILD_PrintNullTermination(&s);
	return s.data;
}

// NOTE: CreateProcessW may write to the command_string in-place! CreateProcessW requires that.
static bool BUILD_RunProcess(wchar_t* command_string, uint32_t* out_exit_code, BUILD_Log* log_or_null) {
	// https://learn.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output

	PROCESS_INFORMATION process_info = {0};

	// Initialize pipes
	SECURITY_ATTRIBUTES security_attrs = {0};
	security_attrs.nLength = sizeof(SECURITY_ATTRIBUTES);
	security_attrs.lpSecurityDescriptor = NULL;
	security_attrs.bInheritHandle = 1;

	HANDLE OUT_Rd = NULL, OUT_Wr = NULL;
	HANDLE ERR_Rd = NULL, ERR_Wr = NULL;

	bool ok = true;
	if (ok) ok = CreatePipe(&OUT_Rd, &OUT_Wr, &security_attrs, 0);
	if (ok) ok = CreatePipe(&ERR_Rd, &ERR_Wr, &security_attrs, 0);
	if (ok) ok = SetHandleInformation(OUT_Rd, HANDLE_FLAG_INHERIT, 0);

	STARTUPINFOW startup_info = {0};
	startup_info.cb = sizeof(STARTUPINFOW);
	startup_info.dwFlags = STARTF_USESTDHANDLES;
	startup_info.hStdOutput = OUT_Wr;
	startup_info.hStdError = ERR_Wr;
	startup_info.hStdInput = GetStdHandle(STD_INPUT_HANDLE);

	if (ok) ok = CreateProcessW(NULL, command_string, NULL, NULL, true, CREATE_UNICODE_ENVIRONMENT, NULL, NULL, &startup_info, &process_info);

	// We don't need these handles for ourselves and we must close them to say that we won't be using them, to let `ReadFile` exit
	// when the process finishes instead of locking. At least that's how I think it works.
	if (OUT_Wr) CloseHandle(OUT_Wr);
	if (ERR_Wr) CloseHandle(ERR_Wr);

	if (ok) {
		char buf[512];
		uint32_t num_read_bytes;

		for (;;) {
			if (!ReadFile(OUT_Rd, buf, sizeof(buf) - 1, (DWORD*)&num_read_bytes, NULL)) break;

			if (log_or_null) {
				buf[num_read_bytes] = 0; // null termination
				BUILD_LogPrint1(log_or_null, buf);
			}
		}

		WaitForSingleObject(process_info.hProcess, INFINITE);

		if (out_exit_code && !GetExitCodeProcess(process_info.hProcess, (DWORD*)out_exit_code)) ok = false;

		CloseHandle(process_info.hProcess);
		CloseHandle(process_info.hThread);
	}

	if (OUT_Rd) CloseHandle(OUT_Rd);
	if (ERR_Rd) CloseHandle(ERR_Rd);
	return ok;
}

static bool BUILD_GenerateVisualStudioProject(const BUILD_Project* project,
	const char* project_filepath, const char* project_directory, const char* relative_build_directory,
	const wchar_t* project_guid, BUILD_Log* log_or_null)
{
	BUILD_StrBuilder s = {0};

	BUILD_Print(&s, "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n");
	BUILD_Print(&s, "<Project DefaultTargets=\"Build\" xmlns=\"http://schemas.microsoft.com/developer/msbuild/2003\">\n");

	BUILD_Print(&s, "<ItemGroup Label=\"ProjectConfigurations\">\n");
	BUILD_Print(&s, "  <ProjectConfiguration Include=\"Custom|x64\">\n");
	BUILD_Print(&s, "    <Configuration>Custom</Configuration>\n");
	BUILD_Print(&s, "    <Platform>x64</Platform>\n");
	BUILD_Print(&s, "  </ProjectConfiguration>\n");
	BUILD_Print(&s, "</ItemGroup>\n");

	BUILD_Print(&s, "<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.Default.props\" />\n");
	BUILD_Print(&s, "<PropertyGroup Condition=\"'$(Configuration)|$(Platform)'=='Custom|x64'\" Label=\"Configuration\">\n");
	BUILD_Print(&s, "  <ConfigurationType>Application</ConfigurationType>\n");
	BUILD_Print(&s, "  <UseDebugLibraries>false</UseDebugLibraries>\n");
	BUILD_Print(&s, "  <CharacterSet>Unicode</CharacterSet>\n");
	BUILD_Print(&s, "  <PlatformToolset>v143</PlatformToolset>\n");
	BUILD_Print(&s, "</PropertyGroup>\n");

	BUILD_Print(&s, "<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.props\" />\n");
	BUILD_Print(&s, "<ImportGroup Label=\"ExtensionSettings\">\n");
	BUILD_Print(&s, "</ImportGroup>\n");

	BUILD_Print(&s, "<ImportGroup Label=\"PropertySheets\" Condition=\"'$(Configuration)|$(Platform)'=='Custom|x64'\">\n");
	BUILD_Print(&s, "  <Import Project=\"$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props\" Condition=\"exists('$(UserRootDir)\\Microsoft.Cpp.$(Platform).user.props')\" Label=\"LocalAppDataPlatform\" />\n");
	BUILD_Print(&s, "</ImportGroup>\n");

	BUILD_Print(&s, "<PropertyGroup Condition=\"\'$(Configuration)|$(Platform)\'==\'Custom|x64\'\">\n");
	BUILD_Print3(&s, "  <OutDir>", relative_build_directory, "/</OutDir>\n");
	BUILD_Print3(&s, "  <IntDir>", relative_build_directory, "/");
	BUILD_Print2(&s, project->name, "/</IntDir>\n");
	BUILD_Print(&s, "</PropertyGroup>\n");

	{
		BUILD_Print(&s, "<ItemDefinitionGroup Condition=\"'$(Configuration)|$(Platform)'=='Custom|x64'\">\n");

		{
			BUILD_Print(&s, "<ClCompile>\n");

			BUILD_Print(&s, "<PrecompiledHeader>NotUsing</PrecompiledHeader>\n");
			BUILD_Print(&s, "<WarningLevel>Level3</WarningLevel>\n");

			{
				BUILD_Print(&s, "<PreprocessorDefinitions>");
				for (int i = 0; i < project->defines.count; i++) {
					BUILD_Print2(&s, project->defines.data[i], ";");
				}
				BUILD_Print(&s, "</PreprocessorDefinitions>\n");
			}

			BUILD_Print(&s, "<AdditionalIncludeDirectories>");
			for (int i = 0; i < project->include_dirs.count; i++) {
				const char* include_dir = project->include_dirs.data[i];
				BUILD_Print2(&s, include_dir, ";");
			}
			BUILD_Print(&s, "</AdditionalIncludeDirectories>\n");

			if (project->opts.debug_info) {
				BUILD_Print(&s, "<DebugInformationFormat>OldStyle</DebugInformationFormat>\n"); // OldStyle is the same as /Z7
			}
			if (project->opts.enable_optimizations) {
				BUILD_Print(&s, "<Optimization>Full</Optimization>\n");
				BUILD_Print(&s, "<FunctionLevelLinking>true</FunctionLevelLinking>\n");
				BUILD_Print(&s, "<IntrinsicFunctions>true</IntrinsicFunctions>\n");
				BUILD_Print(&s, "<MinimalRebuild>false</MinimalRebuild>\n");
				BUILD_Print(&s, "<StringPooling>true</StringPooling>\n");
			}
			else {
				BUILD_Print(&s, "<Optimization>Disabled</Optimization>\n");
			}
			BUILD_Print(&s, "<ExceptionHandling>false</ExceptionHandling>\n");
			BUILD_Print(&s, "<RuntimeTypeInfo>false</RuntimeTypeInfo>\n");
			BUILD_Print(&s, "<ExternalWarningLevel>Level3</ExternalWarningLevel>\n");
			BUILD_Print(&s, "<TreatWarningAsError>true</TreatWarningAsError>\n");
			{
				BUILD_Print(&s, "<AdditionalOptions>");
				if (!project->opts.enable_warning_unused_variables) BUILD_Print(&s, "/wd4101 ");
				if (!project->opts.disable_warning_unhandled_switch_cases) BUILD_Print(&s, "/w14062 ");
				if (!project->opts.disable_warning_shadowed_locals) BUILD_Print(&s, "/w14456 ");

				for (int i = 0; i < project->extra_compiler_args.count; i++) {
					BUILD_Print2(&s, project->extra_compiler_args.data[i], " ");
				}

				BUILD_Print(&s, "</AdditionalOptions>\n");
			}

			const char* crt_linking_type = project->opts.c_runtime_library_debug ?
				(project->opts.c_runtime_library_dll ? "MultiThreadedDebugDLL" : "MultiThreadedDebug") :
				(project->opts.c_runtime_library_dll ? "MultiThreadedDLL" : "MultiThreaded");
			BUILD_Print3(&s, "<RuntimeLibrary>", crt_linking_type, "</RuntimeLibrary>\n");

			BUILD_Print(&s, "</ClCompile>\n");
		}

		{
			BUILD_Print(&s, "<Link>\n");

			BUILD_Print(&s, project->opts.windows.disable_console ? "<SubSystem>Windows</SubSystem>\n" : "<SubSystem>Console</SubSystem>\n");

			if (project->opts.enable_optimizations) {
				BUILD_Print(&s, "<EnableCOMDATFolding>true</EnableCOMDATFolding>\n");
				BUILD_Print(&s, "<OptimizeReferences>true</OptimizeReferences>\n");
			}

			if (project->opts.debug_info) {
				BUILD_Print(&s, "<GenerateDebugInformation>true</GenerateDebugInformation>\n");
			}

			BUILD_Print(&s, "<AdditionalDependencies>");
			for (int i = 0; i < project->linker_inputs.count; i++) {
				BUILD_Print2(&s, project->linker_inputs.data[i], ";");
			}
			BUILD_Print(&s, "</AdditionalDependencies>\n");

			{
				BUILD_Print(&s, "<AdditionalOptions>");

				for (int i = 0; i < project->extra_linker_args.count; i++) {
					BUILD_Print2(&s, project->extra_linker_args.data[i], " ");
				}

				BUILD_Print(&s, "/IGNORE:4099 "); // LNK4099: PDB ... was not found with ... or at ''; linking object as if no debug info

				if (project->opts.disable_aslr) BUILD_Print(&s, "/DYNAMICBASE:NO ");

				BUILD_Print(&s, "</AdditionalOptions>\n");
			}

			BUILD_Print(&s, "</Link>\n");
		}
		BUILD_Print(&s, "</ItemDefinitionGroup>\n");
	}

	for (int i = 0; i < project->natvis_files.count; i++) {
		BUILD_Print(&s, "<ItemGroup>\n");
		BUILD_Print3(&s, "  <None Include=\"", project->natvis_files.data[i], "\" />\n");
		BUILD_Print(&s, "</ItemGroup>\n");
	}
	
	for (int i = 0; i < project->source_files.count; i++) {
		BUILD_Print(&s, "<ItemGroup>\n");
		BUILD_Print3(&s, "  <ClCompile Include=\"", project->source_files.data[i], "\" />\n");
		BUILD_Print(&s, "</ItemGroup>\n");
	}

	for (int i = 0; i < project->source_dirs.count; i++) {
		// Iterate through all files in directory

		BUILD_StrBuilder source_dir = {0};
		BUILD_Print3(&source_dir, project_directory, "/", project->source_dirs.data[i]);
		BUILD_PrintNullTermination(&source_dir);
		
		BUILD_WStrBuilder iter_files_match_str = {0};
		BUILD_WPrintUTF8(&iter_files_match_str, source_dir.data);
		BUILD_WPrint(&iter_files_match_str, L"\\*");
		BUILD_WPrintNullTermination(&iter_files_match_str);

		WIN32_FIND_DATAW iter_files_info;
		HANDLE iter_files_handle = FindFirstFileW(iter_files_match_str.data, &iter_files_info);
		BUILD_ArrayFree(&iter_files_match_str);

		bool ok = iter_files_handle != INVALID_HANDLE_VALUE;
		for (; ok && FindNextFileW(iter_files_handle, &iter_files_info);) {
			if (iter_files_info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

			int name_len = (int)wcslen(iter_files_info.cFileName);
			bool is_source_file = 
				(name_len >= 2 && !wcscmp(iter_files_info.cFileName + name_len - 2, L".c")) ||
				(name_len >= 4 && !wcscmp(iter_files_info.cFileName + name_len - 4, L".cpp"));
			
			BUILD_Print(&s, "<ItemGroup>\n");
			
			BUILD_Print(&s, is_source_file ? "  <ClCompile Include=\"" : "  <ClInclude Include=\"");
			BUILD_Print(&s, source_dir.data);
			BUILD_Print(&s, "/");
			BUILD_PrintW(&s, iter_files_info.cFileName);
			BUILD_Print(&s, "\" />\n");
			
			BUILD_Print(&s, "</ItemGroup>\n");
		}
		ok = GetLastError() == ERROR_NO_MORE_FILES;
		if (iter_files_handle != INVALID_HANDLE_VALUE) FindClose(iter_files_handle);

		BUILD_ArrayFree(&source_dir);
	}

	BUILD_Print(&s, "<Import Project=\"$(VCTargetsPath)\\Microsoft.Cpp.targets\" />\n");

	BUILD_Print(&s, "<ImportGroup Label=\"ExtensionTargets\">\n");
	BUILD_Print(&s, "</ImportGroup>\n");

	BUILD_Print(&s, "</Project>\n");

	FILE* file = fopen(project_filepath, "wb");
	bool ok = file != NULL;

	if (ok) {
		fwrite(s.data, s.count, 1, file);
		fclose(file);
	}
	else {
		BUILD_LogPrint3(log_or_null, "Failed writing file: `", project_filepath, "`\n");
	}

	BUILD_ArrayFree(&s);
	return ok;
}

BUILD_API void BUILD_ProjectInit(BUILD_Project* project, const char* name, const BUILD_ProjectOptions* options) {
	memset(project, 0, sizeof(*project));
	project->name = name;
	project->opts = *options;
}

BUILD_API void BUILD_ProjectDeinit(BUILD_Project* project) {
	BUILD_ArrayFree(&project->source_files);
	BUILD_ArrayFree(&project->natvis_files);
	BUILD_ArrayFree(&project->source_dirs);
	BUILD_ArrayFree(&project->include_dirs);
	BUILD_ArrayFree(&project->defines);
	BUILD_ArrayFree(&project->linker_inputs);
	BUILD_ArrayFree(&project->extra_linker_args);
	BUILD_ArrayFree(&project->extra_compiler_args);
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

typedef struct BUILD_VSWhereLog {
	BUILD_Log base;
	BUILD_StrBuilder output;
} BUILD_VSWhereLog;

static void BUILD_VSWhereLogFn(BUILD_Log* self, const char* message) {
	BUILD_Print(&((BUILD_VSWhereLog*)self)->output, message);
}

BUILD_API bool BUILD_CompileProject(BUILD_Project* project, const char* project_directory, const char* relative_build_directory,
	BUILD_Log* log_or_null)
{
	bool ok = project->source_files.count > 0;
	
	BUILD_VSWhereLog vswhere_log = {0};
	vswhere_log.base.print = BUILD_VSWhereLogFn;
	
	if (ok) {
		BUILD_WStrBuilder vswhere_args = {0};
		BUILD_WPrint(&vswhere_args, L"C:\\Program Files (x86)\\Microsoft Visual Studio\\Installer\\vswhere.exe -latest -property installationPath");
		BUILD_WPrintNullTermination(&vswhere_args);

		uint32_t vswhere_exit_code = 0;
		ok = BUILD_RunProcess(vswhere_args.data, &vswhere_exit_code, &vswhere_log.base) && vswhere_exit_code == 0;
		BUILD_ArrayFree(&vswhere_args);
	}

	FILE* msvc_version_file = NULL;
	if (ok) {
		BUILD_StrBuilder msvc_version_path = {0};
		vswhere_log.output.count -= 2; // cut "\r\n" from the end
		BUILD_WPrintNullTermination(&vswhere_log.output);
		
		// https://github.com/microsoft/vswhere/wiki/Find-VC
		BUILD_Print(&msvc_version_path, vswhere_log.output.data); // vswhere_log.output should now look something like "C:\Program Files\Microsoft Visual Studio\2022\Community"
		BUILD_Print(&msvc_version_path, "\\VC\\Auxiliary\\Build\\Microsoft.VCToolsVersion.default.txt");
		BUILD_WPrintNullTermination(&msvc_version_path);
			
		msvc_version_file = fopen(msvc_version_path.data, "rb");
		ok = msvc_version_file != NULL;
		BUILD_ArrayFree(&msvc_version_path);
	}

	// For the Windows 10/11 SDKs, this seems to be the one and only official install directory.
	wchar_t* win_sdk_root = L"C:\\Program Files (x86)\\Windows Kits\\10\\";
	wchar_t win_sdk_version[64];

	if (ok) {
		HKEY winsdk_version_key;
		ok = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "SOFTWARE\\WOW6432Node\\Microsoft\\Microsoft SDKs\\Windows\\v10.0", 0, KEY_QUERY_VALUE | KEY_WOW64_32KEY, &winsdk_version_key) == S_OK;
		if (ok) {
			DWORD version_buf_size = 64;
			ok = RegQueryValueExW(winsdk_version_key, L"ProductVersion", NULL, NULL, (uint8_t*)win_sdk_version, &version_buf_size) == S_OK;
			win_sdk_version[version_buf_size/2 - 1] = 0; // null-terminate, because apparently registry strings may or may not be null-terminated.
			RegCloseKey(winsdk_version_key);
		}
	}

	if (ok) {
		char msvc_version[64];
		fgets(msvc_version, 64, msvc_version_file);
		fclose(msvc_version_file);
		msvc_version[strlen(msvc_version) - 2] = 0; // cut "\r\n" from the end
		
		if (0 /*project->opts.windows.use_msbuild*/) {
			assert(project->opts.target != BUILD_Target_ObjectFile); // you must have a linker stage when using msbuild

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
			BUILD_StrBuilder abs_build_directory = {0};
			BUILD_Print(&abs_build_directory, project_directory);
			BUILD_Print(&abs_build_directory, "/");
			BUILD_Print(&abs_build_directory, relative_build_directory);
			BUILD_PrintNullTermination(&abs_build_directory);

			BUILD_WStrBuilder msvc_args = {0};

			// print cl.exe path
			BUILD_WPrint(&msvc_args, L"\"");
			BUILD_WPrintUTF8(&msvc_args, vswhere_log.output.data);
			BUILD_WPrint(&msvc_args, L"\\VC\\Tools\\MSVC\\");
			BUILD_WPrintUTF8(&msvc_args, msvc_version);
			BUILD_WPrint(&msvc_args, L"\\bin\\HostX64\\x64\\cl.exe\"");

			BUILD_WPrint(&msvc_args, L" /WX"); // treat warnings as errors
			BUILD_WPrint(&msvc_args, L" /W3"); // warning level 3

			for (int i = 0; i < project->defines.count; i++) {
				BUILD_WPrint(&msvc_args, L" /D");
				BUILD_WPrintUTF8(&msvc_args, project->defines.data[i]);
			}

			if (project->opts.debug_info) BUILD_WPrint(&msvc_args, L" /Z7");
			if (!project->opts.enable_warning_unused_variables) BUILD_WPrint(&msvc_args, L" /wd4101");
			if (!project->opts.disable_warning_unhandled_switch_cases) BUILD_WPrint(&msvc_args, L" /w14062");
			if (!project->opts.disable_warning_shadowed_locals) BUILD_WPrint(&msvc_args, L" /w14456");

			for (int i = 0; i < project->extra_compiler_args.count; i++) {
				BUILD_WPrint(&msvc_args, L" ");
				BUILD_WPrintUTF8(&msvc_args, project->extra_compiler_args.data[i]);
			}

			if (project->opts.c_runtime_library_debug) __debugbreak(); // TODO
			if (project->opts.c_runtime_library_dll)   __debugbreak(); // TODO

			for (int i = 0; i < project->source_files.count; i++) {
				BUILD_WPrint(&msvc_args, L" \"");
				BUILD_WPrintUTF8(&msvc_args, project->source_files.data[i]);
				BUILD_WPrint(&msvc_args, L"\"");
			}

			for (int i = 0; i < project->include_dirs.count; i++) {
				BUILD_WPrint(&msvc_args, L" \"/I");
				BUILD_WPrintUTF8(&msvc_args, project->include_dirs.data[i]);
				BUILD_WPrint(&msvc_args, L"\"");
			}

			BUILD_WPrint(&msvc_args, L" \"/Fo");
			BUILD_WPrintUTF8(&msvc_args, abs_build_directory.data);
			BUILD_WPrint(&msvc_args, L"/\"");
			
			// MSVC include folder
			BUILD_WPrint(&msvc_args, L" \"/I");
			BUILD_WPrintUTF8(&msvc_args, vswhere_log.output.data);
			BUILD_WPrint(&msvc_args, L"\\VC\\Tools\\MSVC\\");
			BUILD_WPrintUTF8(&msvc_args, msvc_version);
			BUILD_WPrint(&msvc_args, L"\\include\"");

			// shared/ include folder from the Windows SDK
			BUILD_WPrint(&msvc_args, L" \"/I");
			BUILD_WPrint(&msvc_args, win_sdk_root);
			BUILD_WPrint(&msvc_args, L"\\Include\\");
			BUILD_WPrint(&msvc_args, win_sdk_version);
			BUILD_WPrint(&msvc_args, L".0\\shared\"");

			// ucrt/ include folder from the Windows SDK
			BUILD_WPrint(&msvc_args, L" \"/I");
			BUILD_WPrint(&msvc_args, win_sdk_root);
			BUILD_WPrint(&msvc_args, L"\\Include\\");
			BUILD_WPrint(&msvc_args, win_sdk_version);
			BUILD_WPrint(&msvc_args, L".0\\ucrt\"");

			// um/ include folder from the Windows SDK
			BUILD_WPrint(&msvc_args, L" \"/I");
			BUILD_WPrint(&msvc_args, win_sdk_root);
			BUILD_WPrint(&msvc_args, L"\\Include\\");
			BUILD_WPrint(&msvc_args, win_sdk_version);
			BUILD_WPrint(&msvc_args, L".0\\um\"");
			
			switch (project->opts.target) {
			case BUILD_Target_Executable: break;
			case BUILD_Target_DynamicLibrary: BUILD_WPrint(&msvc_args, L" /LD"); break;
			case BUILD_Target_ObjectFile: BUILD_WPrint(&msvc_args, L" /c"); break;
			}

			if (project->opts.target != BUILD_Target_ObjectFile) {
				BUILD_WPrint(&msvc_args, L" /Fe");
				BUILD_WPrintUTF8(&msvc_args, abs_build_directory.data);
				BUILD_WPrint(&msvc_args, L"\\");
				BUILD_WPrintUTF8(&msvc_args, project->name);
				BUILD_WPrint(&msvc_args, project->opts.target == BUILD_Target_Executable ? L".exe" : L".dll");

				BUILD_WPrint(&msvc_args, L" /link");
				BUILD_WPrint(&msvc_args, L" /NOLOGO"); // disable Microsoft linker startup banner text
				BUILD_WPrint(&msvc_args, L" /INCREMENTAL:NO");

				if (project->opts.windows.disable_console) BUILD_WPrint(&msvc_args, L" /SUBSYSTEM:WINDOWS");
				if (project->extra_linker_args.count > 0) __debugbreak(); // TODO

				for (int i = 0; i < project->linker_inputs.count; i++) {
					BUILD_WPrint(&msvc_args, L" \"");
					BUILD_WPrintUTF8(&msvc_args, project->linker_inputs.data[i]);
					BUILD_WPrint(&msvc_args, L"\"");
				}

				// MSVC lib folder
				BUILD_WPrint(&msvc_args, L" \"/LIBPATH:");
				BUILD_WPrintUTF8(&msvc_args, vswhere_log.output.data);
				BUILD_WPrint(&msvc_args, L"\\VC\\Tools\\MSVC\\");
				BUILD_WPrintUTF8(&msvc_args, msvc_version);
				BUILD_WPrint(&msvc_args, L"\\Lib\\x64\"");

				// ucrt/ lib folder from the Windows SDK
				BUILD_WPrint(&msvc_args, L" \"/LIBPATH:");
				BUILD_WPrint(&msvc_args, win_sdk_root);
				BUILD_WPrint(&msvc_args, L"\\Lib\\");
				BUILD_WPrint(&msvc_args, win_sdk_version);
				BUILD_WPrint(&msvc_args, L".0\\ucrt\\x64\"");

				// um/ lib folder from the Windows SDK
				BUILD_WPrint(&msvc_args, L" \"/LIBPATH:");
				BUILD_WPrint(&msvc_args, win_sdk_root);
				BUILD_WPrint(&msvc_args, L"\\Lib\\");
				BUILD_WPrint(&msvc_args, win_sdk_version);
				BUILD_WPrint(&msvc_args, L".0\\um\\x64\"");
			}

			BUILD_WPrintNullTermination(&msvc_args);

			uint32_t exit_code = 0;
			ok = BUILD_RunProcess(msvc_args.data, &exit_code, log_or_null) && exit_code == 0;

			BUILD_ArrayFree(&abs_build_directory);
		}
		
	}

	BUILD_ArrayFree(&vswhere_log.output);

	return ok;
}

BUILD_API bool BUILD_CreateDirectory(const char* directory) {
	bool success = CreateDirectoryA(directory, NULL); // TODO: Unicode support
	return success || GetLastError() == ERROR_ALREADY_EXISTS;
}

BUILD_API bool BUILD_CopyFile(const char* file, const char* new_file) {
	return CopyFileA(file, new_file, FALSE);
}

static void BUILD_StringFromGUID(GUID* guid, wchar_t buffer[128]) {
#ifdef __cplusplus
	StringFromGUID2(*guid, buffer, 128);
#else
	StringFromGUID2(guid, buffer, 128);
#endif
}

BUILD_API bool BUILD_CreateVisualStudioSolution(const char* project_directory, const char* relative_build_directory,
	const char* solution_name, BUILD_Project** projects, int projects_count, BUILD_Log* log_or_null)
{
	bool ok = true;

	BUILD_StrBuilder s = {0};
	BUILD_Print(&s, "Microsoft Visual Studio Solution File, Format Version 12.00\n");
	BUILD_Print(&s, "# Visual Studio Version 17\n");
	BUILD_Print(&s, "VisualStudioVersion = 17.6.33712.159\n");
	BUILD_Print(&s, "MinimumVisualStudioVersion = 10.0.40219.1\n");

	BUILD_Array(GUID) project_guids = {0};

	for (size_t i = 0; i < projects_count; i++) {
		BUILD_Project* project = projects[i];
		if (project == NULL) continue;

		GUID guid;
		CoCreateGuid(&guid);
		
		wchar_t guid_str[128];
		BUILD_StringFromGUID(&guid, guid_str);

		BUILD_ArrayPush(&project_guids, guid);

		BUILD_StrBuilder project_filepath = {0};
		BUILD_Print4(&project_filepath, project_directory, "/", project->name, ".vcxproj");
		BUILD_PrintNullTermination(&project_filepath);

		BUILD_StrBuilder project_name = {0};
		BUILD_Print2(&project_name, project->name, ".vcxproj");
		BUILD_PrintNullTermination(&project_name);

		if (ok) {
			ok = BUILD_GenerateVisualStudioProject(project, project_filepath.data, project_directory, relative_build_directory, guid_str, log_or_null);
		}

		BUILD_Print3(&s, "Project(\"{8BC9CEB8-8B4A-11D0-8D11-00A0C91BC942}\") = \"", project->name, "\"");
		BUILD_Print3(&s, ", \"", project_name.data, "\"");
		
		BUILD_Print(&s, ", \"");
		BUILD_PrintW(&s, guid_str);
		BUILD_Print(&s, "\"\n");
		
		BUILD_Print(&s, "EndProject\n");
		
		BUILD_ArrayFree(&project_filepath);
		BUILD_ArrayFree(&project_name);
	}

	BUILD_Print(&s, "Global\n");
	BUILD_Print(&s, "  GlobalSection(SolutionConfigurationPlatforms) = preSolution\n");
	BUILD_Print(&s, "  EndGlobalSection\n");
	BUILD_Print(&s, "  GlobalSection(ProjectConfigurationPlatforms) = postSolution\n");

	for (int i = 0; i < projects_count; i++) {
		wchar_t guid_str[128];
		BUILD_StringFromGUID(&project_guids.data[i], guid_str);
		
		BUILD_Print(&s, "    ");
		BUILD_PrintW(&s, guid_str);
		BUILD_Print(&s, ".Custom|x64.ActiveCfg = Custom|x64\n");
		
		BUILD_Print(&s, "    ");
		BUILD_PrintW(&s, guid_str);
		BUILD_Print(&s, ".Custom|x64.Build.0 = Custom|x64\n");
	}

	BUILD_Print(&s, "  EndGlobalSection\n");
	BUILD_Print(&s, "  GlobalSection(SolutionProperties) = preSolution\n");
	BUILD_Print(&s, "    HideSolutionNode = FALSE\n");
	BUILD_Print(&s, "  EndGlobalSection\n");
	BUILD_Print(&s, "  GlobalSection(ExtensibilityGlobals) = postSolution\n");
	BUILD_Print(&s, "    SolutionGuid = {E8A6471F-96EE-4CB5-A6F7-DD09AD151C28}\n");
	BUILD_Print(&s, "  EndGlobalSection\n");
	BUILD_Print(&s, "EndGlobal\n");

	if (ok) {
		BUILD_StrBuilder solution_filepath = {0};
		BUILD_Print3(&solution_filepath, project_directory, "/", solution_name);
		BUILD_PrintNullTermination(&solution_filepath);

		FILE* file = fopen(solution_filepath.data, "wb");
		ok = file != NULL;
		if (ok) {
			fwrite(s.data, s.count, 1, file);
			fclose(file);
		}
		
		BUILD_ArrayFree(&solution_filepath);
	}

	BUILD_ArrayFree(&project_guids);
	BUILD_ArrayFree(&s);
	return ok;
}

#endif // FIRE_BUILD_IMPLEMENTATION
#endif // FIRE_BUILD_INCLUDED