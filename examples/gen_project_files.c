#include "../fire_build.h"

int main() {
	BUILD_ProjectOptions opts = {
		.debug_info = true,
		.c_runtime_library_dll = true,
	};
	
	//////////////////////
	
	BUILD_Project ui_demo_dx11;
	BUILD_InitProject(&ui_demo_dx11, "ui_demo_dx11", &opts);
	BUILD_AddIncludeDir(&ui_demo_dx11, "../../"); // Repository root folder
	
	BUILD_AddSourceFile(&ui_demo_dx11, "../ui_demo_dx11/ui_demo.cpp");
	
	
	//////////////////////

	BUILD_Project* projects[1] = {&ui_demo_dx11};
	BUILD_CreateDirectory("build");
	
	if (!BUILD_CreateVisualStudioSolution("build", ".", "examples.sln", projects, 1, BUILD_GetConsole())) {
		return 1;
	}
	
	return 0;
}
