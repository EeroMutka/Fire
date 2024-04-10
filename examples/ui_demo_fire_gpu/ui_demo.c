#include "fire_ds.h"

#define STR_USE_FIRE_DS_ARENA
#include "fire_string.h"

#define OS_USE_FIRE_DS_ARENA
#define OS_STRING_OVERRIDE
#define OS_String STR
#include "fire_os.h"

#include "fire_gpu/fire_gpu.h"

#include <vulkan/vulkan.h>
#include <glslang/Include/glslang_c_interface.h>
#include "fire_gpu/fire_gpu_vulkan.h"

#define STB_RECT_PACK_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#include "fire_ui/stb_rect_pack.h"
#include "fire_ui/stb_truetype.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "fire_ui/fire_ui.h"
#include "fire_ui/fire_ui_backend_fire_gpu.h"
#include "fire_ui/fire_ui_backend_fire_os.h"

#include "../shared/ui_demo.h"

//// Globals ///////////////////////////////////////////////

static UI_Vec2 g_window_size = {1200, 900};

static GPU_Graph *g_graphs[2];
static GPU_DescriptorArena *g_descriptor_arenas[2];
static int g_current_graph_idx = 0;

static UI_Inputs g_ui_inputs;

static UIDemoState g_demo_state;

////////////////////////////////////////////////////////////

static void UpdateAndRender() {
	UI_GPU_BeginFrame();
	UI_BeginFrame(&g_ui_inputs, g_window_size);

	UIDemoBuild(&g_demo_state, g_window_size);

	UI_Outputs ui_outputs;
	UI_EndFrame(&ui_outputs);

	g_current_graph_idx = (g_current_graph_idx + 1) % 2;
	GPU_Graph *graph = g_graphs[g_current_graph_idx];
	GPU_GraphWait(graph);
	GPU_ResetDescriptorArena(g_descriptor_arenas[g_current_graph_idx]);

	GPU_Texture *backbuffer = GPU_GetBackbuffer(graph);
	if (backbuffer) {
		GPU_OpClearColorF(graph, backbuffer, GPU_MIP_LEVEL_ALL, 0.15f, 0.15f, 0.15f, 1.f);

		UI_GPU_EndFrame(&ui_outputs, graph, g_descriptor_arenas[g_current_graph_idx]);
		UI_OS_ApplyOutputs(&ui_outputs);

		GPU_GraphSubmit(graph);
	}
}

static void OnResizeWindow(uint32_t width, uint32_t height, void *user_ptr) {
	g_window_size.x = (float)width;
	g_window_size.y = (float)height;
	UpdateAndRender();
}

int main() {
	OS_Init();

	DS_Arena persist, temp;
	DS_ArenaInit(&persist, 4096, DS_HEAP);
	DS_ArenaInit(&temp, 4096, DS_HEAP);

	UIDemoInit(&g_demo_state, &persist);

	OS_Inputs window_inputs = {0};
	OS_Window window = OS_WindowCreate((uint32_t)g_window_size.x, (uint32_t)g_window_size.y, OS_STR("UI demo (Fire GPU)"));

	GPU_Init(window.handle);
	
	GPU_RenderPass *main_renderpass = GPU_MakeRenderPass(&(GPU_RenderPassDesc){
		.color_targets_count = 1,
		.color_targets = GPU_SWAPCHAIN_COLOR_TARGET,
	});

	GPU_MakeSwapchainGraphs(2, &g_graphs[0]);
	g_descriptor_arenas[0] = GPU_MakeDescriptorArena();
	g_descriptor_arenas[1] = GPU_MakeDescriptorArena();

	UI_Backend ui_backend = {0};
	UI_GPU_Init(&ui_backend, main_renderpass);
	UI_Init(&persist, &ui_backend);

	// NOTE: the font data must remain alive across the whole program lifetime!
	STR roboto_mono_ttf, icons_ttf;
	assert(OS_ReadEntireFile(&persist, STR_("../../fire_ui/resources/roboto_mono.ttf"), &roboto_mono_ttf));
	assert(OS_ReadEntireFile(&persist, STR_("../../fire_ui/resources/fontello/font/fontello.ttf"), &icons_ttf));

	UI_Font base_font, icons_font;
	UI_FontInit(&base_font, roboto_mono_ttf.data, -4.f);
	UI_FontInit(&icons_font, icons_ttf.data, -2.f);

	//// Main loop /////////////////////////////////////
	
	for (;;) {
		DS_ArenaReset(&temp);

		if (!OS_WindowPoll(&temp, &window_inputs, &window, OnResizeWindow, NULL)) break;

		g_ui_inputs = (UI_Inputs){0};
		g_ui_inputs.base_font = &base_font;
		g_ui_inputs.icons_font = &icons_font;
		UI_OS_TranslateInputs(&g_ui_inputs, &window_inputs, &temp);
		
		UpdateAndRender();
	}

	GPU_WaitUntilIdle();
	
	//// Cleanup ///////////////////////////////////////
	
	UI_FontDeinit(&base_font);
	UI_FontDeinit(&icons_font);
	
	UI_Deinit();
	UI_GPU_Deinit();

	GPU_DestroyDescriptorArena(g_descriptor_arenas[0]);
	GPU_DestroyDescriptorArena(g_descriptor_arenas[1]);
	GPU_DestroyGraph(g_graphs[0]);
	GPU_DestroyGraph(g_graphs[1]);
	GPU_DestroyRenderPass(main_renderpass);
	GPU_Deinit();

	DS_ArenaDeinit(&persist);
	DS_ArenaDeinit(&temp);

	OS_Deinit();

	return 0;
}