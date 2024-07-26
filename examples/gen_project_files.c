#include "../fire_build.h"

int main() {
	BUILD_ProjectOptions opts = {
		.debug_info = true,
		.c_runtime_library_dll = true, // glslang.lib uses /MD
	};
	
	BUILD_Project* projects[16];
	int projects_count = 0;
	
	//////////////////////
	
	BUILD_Project ui_demo_dx11;
	BUILD_InitProject(&ui_demo_dx11, "ui_demo_dx11", &opts);
	BUILD_AddIncludeDir(&ui_demo_dx11, "../../"); // Repository root folder
	
	BUILD_AddSourceFile(&ui_demo_dx11, "../ui_demo_dx11/ui_demo.cpp");
	
	projects[projects_count++] = &ui_demo_dx11;
	
	//////////////////////
	
	BUILD_CreateDirectory("build");
	
	if (!BUILD_CreateVisualStudioSolution("build", ".", "examples.sln", projects, projects_count, BUILD_GetConsole())) {
		return 1;
	}
	
	return 0;
}
