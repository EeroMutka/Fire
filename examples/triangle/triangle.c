#include "fire_ds.h"
#include "fire_os.h"

#include "fire_gpu/fire_gpu.h"
#include <vulkan/vulkan.h>
#include <glslang/Include/glslang_c_interface.h>
#include "fire_gpu/fire_gpu_vulkan.h"

#include <assert.h>
#include <stdio.h>

typedef struct {
	float x, y;
	float r, g, b;
} Vertex;

//// Globals ///////////////////////////////////////////////

static GPU_Buffer *g_vertex_buffer;
static GPU_Buffer *g_index_buffer;

static GPU_RenderPass *g_render_pass;
static GPU_GraphicsPipeline *g_pipeline;
static GPU_DescriptorSet *g_desc_set;

static GPU_Graph *g_graphs[2];
static int g_current_graph_idx = 0;

////////////////////////////////////////////////////////////

static void Render() {
	g_current_graph_idx = (g_current_graph_idx + 1) % 2;
	GPU_Graph *graph = g_graphs[g_current_graph_idx];
	GPU_GraphWait(graph);

	GPU_Texture *backbuffer = GPU_GetBackbuffer(graph);
	if (backbuffer) {
		GPU_OpClearColorF(graph, backbuffer, GPU_MIP_LEVEL_ALL, 0.1f, 0.2f, 0.5f, 1.f);

		GPU_OpPrepareRenderPass(graph, g_render_pass);
		uint32_t draw_type = GPU_OpPrepareDrawParams(graph, g_pipeline, g_desc_set);
		GPU_OpBeginRenderPass(graph);

		GPU_OpBindDrawParams(graph, draw_type);
		GPU_OpBindVertexBuffer(graph, g_vertex_buffer);
		GPU_OpBindIndexBuffer(graph, g_index_buffer);
		GPU_OpDraw(graph, 3, 1, 0, 0);

		GPU_OpEndRenderPass(graph);

		GPU_GraphSubmit(graph);
	}
}

static void OnResizeWindow(uint32_t width, uint32_t height, void *user_ptr) {
	Render();
}

typedef struct {
	int x;
} Foo;

int main() {
	OS_Init();

	OS_Arena os_temp_arena;
	OS_ArenaInit(&os_temp_arena, 4096);

	OS_Window window = OS_WindowCreate(1200, 900, OS_STR("Triangle"));

	GPU_Init(window.handle);

	GPU_RenderPassDesc render_pass_desc = {
		.color_targets_count = 1,
		.color_targets = GPU_SWAPCHAIN_COLOR_TARGET,
	};
	g_render_pass = GPU_MakeRenderPass(&render_pass_desc);
	
	OS_String shader_src;
	OS_String shader_path = OS_STR("../triangle/triangle_shader.glsl");
	assert(OS_ReadEntireFile(&os_temp_arena, shader_path, &shader_src));

	GPU_PipelineLayout *pipeline_layout = GPU_InitPipelineLayout(NULL);
	GPU_FinalizePipelineLayout(pipeline_layout);

	g_desc_set = GPU_InitDescriptorSet(NULL, pipeline_layout);
	GPU_FinalizeDescriptorSet(g_desc_set);

	GPU_Format vertex_input_formats[] = {GPU_Format_RG32F, GPU_Format_RGB32F};
	GPU_GraphicsPipelineDesc pipeline_desc = {
		.layout = pipeline_layout,
		.render_pass = g_render_pass,
		.vs = {
			.glsl = {shader_src.data, shader_src.size},
			.glsl_debug_filepath = {shader_path.data, shader_path.size},
		},
		.fs = {
			.glsl = {shader_src.data, shader_src.size},
			.glsl_debug_filepath = {shader_path.data, shader_path.size},
		},
		.vertex_input_formats = {vertex_input_formats, GPU_ArrayCount(vertex_input_formats)},
	};
	g_pipeline = GPU_MakeGraphicsPipeline(&pipeline_desc);

	Vertex vertices[] = {
		{-0.5f, -0.5f,  1.f, 0.f, 0.f},
		{0.5f, -0.5f,   0.f, 1.f, 0.f},
		{0.f, 0.5f,     0.f, 0.f, 1.f}};
	uint32_t indices[] = {0, 1, 2};

	g_vertex_buffer = GPU_MakeBuffer(sizeof(vertices), GPU_BufferFlag_GPU, vertices);
	g_index_buffer = GPU_MakeBuffer(sizeof(indices), GPU_BufferFlag_GPU, indices);

	GPU_MakeSwapchainGraphs(2, &g_graphs[0]);

	// Main loop
	while (!OS_WindowShouldClose(&window)) {
		for (OS_Event event; OS_WindowPollEvent(&window, &event, OnResizeWindow, NULL);) {}

		Render();
	}

	GPU_WaitUntilIdle();

	// Cleanup resources

	GPU_DestroyGraph(g_graphs[0]);
	GPU_DestroyGraph(g_graphs[1]);
	GPU_DestroyBuffer(g_vertex_buffer);
	GPU_DestroyBuffer(g_index_buffer);
	GPU_DestroyGraphicsPipeline(g_pipeline);
	GPU_DestroyDescriptorSet(g_desc_set);
	GPU_DestroyPipelineLayout(pipeline_layout);
	GPU_DestroyRenderPass(g_render_pass);
	GPU_Deinit();

	OS_ArenaDeinit(&os_temp_arena);
	OS_Deinit();

	return 0;
}