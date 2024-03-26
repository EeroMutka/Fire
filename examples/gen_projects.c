#define VULKAN_SDK_PATH "C:/VulkanSDK/1.3.268.0"

#include "../fire_build.h"

#define ArrayCount(X) (sizeof(X) / sizeof(X[0]))

static void AddFireGPU(BUILD_Project* p) {
	BUILD_AddIncludeDir(p, VULKAN_SDK_PATH "/Include");
	BUILD_AddLinkerInput(p, VULKAN_SDK_PATH "/Lib/vulkan-1.lib");
	
	// If compiling with the GLSLang shader compiler, the following are required
	BUILD_AddLinkerInput(p, VULKAN_SDK_PATH "/Lib/GenericCodeGen.lib");
	BUILD_AddLinkerInput(p, VULKAN_SDK_PATH "/Lib/glslang.lib");
	BUILD_AddLinkerInput(p, VULKAN_SDK_PATH "/Lib/MachineIndependent.lib");
	BUILD_AddLinkerInput(p, VULKAN_SDK_PATH "/Lib/OGLCompiler.lib");
	BUILD_AddLinkerInput(p, VULKAN_SDK_PATH "/Lib/OSDependent.lib");
	BUILD_AddLinkerInput(p, VULKAN_SDK_PATH "/Lib/SPIRV.lib");
	BUILD_AddLinkerInput(p, VULKAN_SDK_PATH "/Lib/SPIRV-Tools.lib");
	BUILD_AddLinkerInput(p, VULKAN_SDK_PATH "/Lib/SPIRV-Tools-diff.lib");
	BUILD_AddLinkerInput(p, VULKAN_SDK_PATH "/Lib/SPIRV-Tools-link.lib");
	BUILD_AddLinkerInput(p, VULKAN_SDK_PATH "/Lib/SPIRV-Tools-lint.lib");
	BUILD_AddLinkerInput(p, VULKAN_SDK_PATH "/Lib/SPIRV-Tools-opt.lib");
	BUILD_AddLinkerInput(p, VULKAN_SDK_PATH "/Lib/SPIRV-Tools-reduce.lib");
	BUILD_AddLinkerInput(p, VULKAN_SDK_PATH "/Lib/SPIRV-Tools-shared.lib");
	BUILD_AddLinkerInput(p, VULKAN_SDK_PATH "/Lib/glslang-default-resource-limits.lib");
}

int main() {
	
	BUILD_ProjectOptions opts = {
		.debug_info = true,
		.c_runtime_library_dll = true, // glslang.lib uses /MD
	};

	//////////////////////
	
	BUILD_Project triangle;
	BUILD_InitProject(&triangle, "triangle", &opts);
	BUILD_AddIncludeDir(&triangle, "../../"); // Repository root folder
	
	AddFireGPU(&triangle);
	
	BUILD_AddSourceFile(&triangle, "../triangle/triangle.c");
	
	//////////////////////
	
	BUILD_Project blurred_triangle;
	BUILD_InitProject(&blurred_triangle, "blurred_triangle", &opts);
	BUILD_AddIncludeDir(&blurred_triangle, "../../"); // Repository root folder
	
	AddFireGPU(&blurred_triangle);
	
	BUILD_AddSourceFile(&blurred_triangle, "../blurred_triangle/blurred_triangle.c");
	
	//////////////////////
	
	BUILD_Project ui_demo_fire_gpu;
	BUILD_InitProject(&ui_demo_fire_gpu, "ui_demo_fire_gpu", &opts);
	BUILD_AddIncludeDir(&ui_demo_fire_gpu, "../../"); // Repository root folder
	
	AddFireGPU(&ui_demo_fire_gpu);
	
	BUILD_AddSourceFile(&ui_demo_fire_gpu, "../ui_demo_fire_gpu/ui_demo.c");
	
	//////////////////////
	
	BUILD_Project ui_demo_dx11;
	BUILD_InitProject(&ui_demo_dx11, "ui_demo_dx11", &opts);
	BUILD_AddIncludeDir(&ui_demo_dx11, "../../"); // Repository root folder
	
	BUILD_AddSourceFile(&ui_demo_dx11, "../ui_demo_dx11/ui_demo.c");
	
	//////////////////////
	
	BUILD_CreateDirectory(".build");
	
	BUILD_Project *projects[] = {
		&triangle,
		&blurred_triangle,
		&ui_demo_fire_gpu,
		&ui_demo_dx11,
	};
	
	if (!BUILD_CreateVisualStudioSolution(".build", ".", "examples.sln", projects, ArrayCount(projects), BUILD_GetConsole())) {
		return 1;
	}
	
	return 0;
}
