// This file defines how the VS project files for the examples are generated.
// Currently, there's not much benefit of this compared to just configuring
// the VS project files by hand, but in the future I will add more compiler
// and target platforms to fire_build.h, which will make easy cross-platform
// build configuration possible.
// 
// Also, this file also acts as an example program for fire_build.h!
//

#include "../fire_build.h"

#define ArrCount(x) (sizeof(x) / sizeof(x[0]))

int main()
{
	BUILD_ProjectOptions opts = {
		.debug_info = true,
		.c_runtime_library_dll = true,
	};
	
	// -- D3D11 Demo --
	
	BUILD_Project ui_demo_dx11;
	BUILD_ProjectInit(&ui_demo_dx11, "ui_demo_dx11", &opts);
	BUILD_AddIncludeDir(&ui_demo_dx11, "../../"); // Repository root folder
	
	BUILD_AddSourceFile(&ui_demo_dx11, "../ui_demo_dx11/ui_demo_dx11.cpp");	
	
	// -- D3D12 Demo --
	
	BUILD_Project ui_demo_dx12;
	BUILD_ProjectInit(&ui_demo_dx12, "ui_demo_dx12", &opts);
	BUILD_AddIncludeDir(&ui_demo_dx12, "../../"); // Repository root folder
	
	BUILD_AddSourceFile(&ui_demo_dx12, "../ui_demo_dx12/ui_demo_dx12.cpp");
	
	// -- Generate --
	
	BUILD_Project* projects[] = {&ui_demo_dx11, &ui_demo_dx12};
	BUILD_CreateDirectory("build");
	
	if (!BUILD_CreateVisualStudioSolution("build", ".", "examples.sln", projects, ArrCount(projects), BUILD_GetConsole())) {
		return 1;
	}
	
	return 0;
}
