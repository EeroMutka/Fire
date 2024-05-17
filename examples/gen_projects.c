#include "../fire_build.h"

static void AddFireGPU(BUILD_Project* p, const char *vk_sdk) {
	BUILD_AddIncludeDir(p, BUILD_Concat2(p, vk_sdk, "/Include"));
	BUILD_AddLinkerInput(p, BUILD_Concat2(p, vk_sdk, "/Lib/vulkan-1.lib"));
	
	// If compiling with the GLSLang shader compiler, the following are required
	BUILD_AddLinkerInput(p, BUILD_Concat2(p, vk_sdk, "/Lib/GenericCodeGen.lib"));
	BUILD_AddLinkerInput(p, BUILD_Concat2(p, vk_sdk, "/Lib/glslang.lib"));
	BUILD_AddLinkerInput(p, BUILD_Concat2(p, vk_sdk, "/Lib/MachineIndependent.lib"));
	BUILD_AddLinkerInput(p, BUILD_Concat2(p, vk_sdk, "/Lib/OGLCompiler.lib"));
	BUILD_AddLinkerInput(p, BUILD_Concat2(p, vk_sdk, "/Lib/OSDependent.lib"));
	BUILD_AddLinkerInput(p, BUILD_Concat2(p, vk_sdk, "/Lib/SPIRV.lib"));
	BUILD_AddLinkerInput(p, BUILD_Concat2(p, vk_sdk, "/Lib/SPIRV-Tools.lib"));
	BUILD_AddLinkerInput(p, BUILD_Concat2(p, vk_sdk, "/Lib/SPIRV-Tools-diff.lib"));
	BUILD_AddLinkerInput(p, BUILD_Concat2(p, vk_sdk, "/Lib/SPIRV-Tools-link.lib"));
	BUILD_AddLinkerInput(p, BUILD_Concat2(p, vk_sdk, "/Lib/SPIRV-Tools-lint.lib"));
	BUILD_AddLinkerInput(p, BUILD_Concat2(p, vk_sdk, "/Lib/SPIRV-Tools-opt.lib"));
	BUILD_AddLinkerInput(p, BUILD_Concat2(p, vk_sdk, "/Lib/SPIRV-Tools-reduce.lib"));
	BUILD_AddLinkerInput(p, BUILD_Concat2(p, vk_sdk, "/Lib/SPIRV-Tools-shared.lib"));
	BUILD_AddLinkerInput(p, BUILD_Concat2(p, vk_sdk, "/Lib/glslang-default-resource-limits.lib"));
}

int main() {
	
	BUILD_ProjectOptions opts = {
		.debug_info = true,
		.c_runtime_library_dll = true, // glslang.lib uses /MD
	};
	
	const char* vk_sdk = getenv("VULKAN_SDK");
	if (vk_sdk == NULL) {
		printf("WARNING: Vulkan SDK not found (\"VULKAN_SDK\" environment variable is missing).\n"
			"    Example projects which use Fire GPU won't be generated.");
	}
	
	BUILD_Project* projects[128];
	int projects_count = 0;
	
	//////////////////////
	
	BUILD_Project triangle;
	if (vk_sdk) {
		BUILD_InitProject(&triangle, "triangle", &opts);
		BUILD_AddIncludeDir(&triangle, "../../"); // Repository root folder
		
		AddFireGPU(&triangle, vk_sdk);
		
		BUILD_AddSourceFile(&triangle, "../triangle/triangle.c");
		projects[projects_count++] = &triangle;
	}
	
	//////////////////////
	
	BUILD_Project blurred_triangle;
	if (vk_sdk) {
		BUILD_InitProject(&blurred_triangle, "blurred_triangle", &opts);
		BUILD_AddIncludeDir(&blurred_triangle, "../../"); // Repository root folder
		
		AddFireGPU(&blurred_triangle, vk_sdk);
		
		BUILD_AddSourceFile(&blurred_triangle, "../blurred_triangle/blurred_triangle.c");
		projects[projects_count++] = &blurred_triangle;
	}
	
	//////////////////////
	
	BUILD_Project ui_demo_fire_gpu;
	if (vk_sdk) {
		BUILD_InitProject(&ui_demo_fire_gpu, "ui_demo_fire_gpu", &opts);
		BUILD_AddIncludeDir(&ui_demo_fire_gpu, "../../"); // Repository root folder
		
		AddFireGPU(&ui_demo_fire_gpu, vk_sdk);
		
		BUILD_AddSourceFile(&ui_demo_fire_gpu, "../ui_demo_fire_gpu/ui_demo.c");
		projects[projects_count++] = &ui_demo_fire_gpu;
	}
	
	//////////////////////
	
	BUILD_Project ui_demo_dx11;
	BUILD_InitProject(&ui_demo_dx11, "ui_demo_dx11", &opts);
	BUILD_AddIncludeDir(&ui_demo_dx11, "../../"); // Repository root folder
	
	BUILD_AddSourceFile(&ui_demo_dx11, "../ui_demo_dx11/ui_demo.cpp");
	
	projects[projects_count++] = &ui_demo_dx11;
	
	//////////////////////
	
	BUILD_CreateDirectory(".build");
	
	if (!BUILD_CreateVisualStudioSolution(".build", ".", "examples.sln", projects, projects_count, BUILD_GetConsole())) {
		return 1;
	}
	
	return 0;
}
