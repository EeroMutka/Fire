/* fire_build.h, created by Eero Mutka. Version date: Jan 7. 2024

fire_build is a simple library that lets you build C/C++ code or generate Visual Studio projects from code.
The idea is, you can make one build.c file, use this library in it, then compile and run it to build your code.
This should be simpler, more flexible and easier to understand for everyone compared to using some complicated
build system with its own special language.

NOTE: This library is very work-in-progress and currently only works on Windows.

LICENSE: This code is released under the MIT-0 license, which you can find at https://opensource.org/license/mit-0/,
         and embedded in its source code is a copy of `microsoft_craziness.h` (Author: Jonathan Blow), which is released under
         the MIT license, which you can find at https://opensource.org/licenses/MIT.

DEPENDENCIES:
	This library depends on the fire_os library. Before including this file, you must have included either fire_os.h, or fire_os.c.

USAGE EXAMPLE:
	#include "fire.h"
	#include "fire_os.c"
	#include "fire_build.c"
	
	int main() {
		FOS_Init();
		Arena* A = MakeArena(KIB(4));
		
		FB_ProjectOptions opts = {0};
		FB_Project* p = FB_MakeProject(A, L("hello.exe"), &opts);
		FB_AddSourceFile(p, L("hello.c"));
		
		if (!FB_CreateVisualStudioSolution(FOS_CWD, L("temp"), L("hello.sln"), &p, 1, FOS_Console())) {
			return 1;
		}
		FOS_Print("Build success!\n");
		return 0;
	}
*/

#ifndef FIRE_BUILD_INCLUDED
#define FIRE_BUILD_INCLUDED

typedef struct FB_ProjectOptions {
	bool debug_info; // in MSVC, this corresponds to the /Z7 argument.
	bool enable_optimizations;

	bool enable_warning_unused_variables;
	bool disable_warning_unhandled_switch_cases;
	bool disable_warning_shadowed_locals;

	bool enable_aslr; // Enable address-space layout randomization

	// By default, these are false, and thus will be set to /MT
	// https://learn.microsoft.com/en-us/cpp/build/reference/md-mt-ld-use-run-time-library
	bool c_runtime_library_debug;
	bool c_runtime_library_dll;

	struct {
		bool disable_console; // Corresponds to /SUBSYSTEM:WINDOWS
		bool use_msbuild; // This will generate VS project files and call the MSBuild command to build the project.
	} windows;
} FB_ProjectOptions;
typedef struct FB_Project FB_Project;

// `output_file` is the path of the resulting executable. Pass an empty string to disable the linker
FB_Project* FB_MakeProject(FOS_Arena* arena, FOS_String output_file, const FB_ProjectOptions* options);

void FB_AddSourceFile(FB_Project* project, FOS_String source_file);
void FB_AddIncludeDir(FB_Project* project, FOS_String include_dir);
void FB_AddDefine(FB_Project* project, FOS_String define);
void FB_AddLinkerInput(FB_Project* project, FOS_String linker_input);
void FB_AddExtraLinkerArg(FB_Project* project, FOS_String arg_string);
void FB_AddExtraCompilerArg(FB_Project* project, FOS_String arg_string);

// * `intermediate_dir` may be a relative filepath to `working_dir`.
// * Object files, as well as VS project files will be generated into `intermediate_dir`
// * This function is only implemented on Windows with MSVC at the moment. TODO: GCC, Clang
// * Currently this prints to stdout, however it'd be nice if we had an option to capture the output.
// * `error_log` may be NULL
bool FB_CompileCode(FB_Project* project, FOS_WorkingDir working_dir, FOS_String intermediate_dir, FOS_Log* error_log);

// * `error_log` may be NULL
bool FB_CreateVisualStudioSolution(FOS_WorkingDir working_dir, FOS_String intermediate_dir, FOS_String sln_file,
	FB_Project** projects, size_t projects_count, FOS_Log* error_log);

// bool FOS_RunLinker(FOS_WorkingDir working_dir, StringSlice inputs, FOS_String output_file, FOS_Opt(Writer*) stdout_writer);

#endif // FB_INCLUDED