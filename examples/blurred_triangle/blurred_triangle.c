#include "fire_ds.h"
#include "fire_os.h"

#include "fire_gpu/fire_gpu.h"

#include <vulkan/vulkan.h>
#include <glslang/Include/glslang_c_interface.h>
#include "fire_gpu/fire_gpu_vulkan.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <assert.h>

//// Globals ///////////////////////////////////////////////

static float g_time = 0.f;

static GPU_Texture *g_offscreen_buffers[2];

static GPU_Buffer *g_vertex_buffer;
static GPU_Buffer *g_index_buffer;

static GPU_RenderPass *g_triangle_pass;
static GPU_GraphicsPipeline *g_triangle_pipeline;
static GPU_DescriptorSet *g_triangle_desc_set;

static GPU_RenderPass *g_blur_passes[2];
static GPU_PipelineLayout *g_blur_pipeline_layout;
static GPU_GraphicsPipeline *g_blur_pipelines[2];
static GPU_DescriptorSet *g_blur_desc_sets[2];

static GPU_Graph *g_graphs[2];
static int g_current_graph_idx = 0;
static uint32_t g_window_size[2] = {1200, 900};

////////////////////////////////////////////////////////////

typedef struct {
	float x, y;
} Vertex;

static void Render() {
	g_current_graph_idx = (g_current_graph_idx + 1) % 2;
	GPU_Graph *graph = g_graphs[g_current_graph_idx];
	GPU_GraphWait(graph);

	GPU_Texture *backbuffer = GPU_GetBackbuffer(graph);
	if (backbuffer) {
		GPU_OpClearColorF(graph, g_offscreen_buffers[0], 0, 0.1f, 0.2f, 0.5f, 1.f);

		GPU_OpPrepareRenderPass(graph, g_triangle_pass);
		uint32_t triangle_draw_params = GPU_OpPrepareDrawParams(graph, g_triangle_pipeline, g_triangle_desc_set);
		GPU_OpBeginRenderPass(graph);

		GPU_OpBindDrawParams(graph, triangle_draw_params);
		GPU_OpBindVertexBuffer(graph, g_vertex_buffer);
		GPU_OpBindIndexBuffer(graph, g_index_buffer);
		GPU_OpDrawIndexed(graph, 3, 1, 0, 0, 0);

		GPU_OpEndRenderPass(graph);

		// First do horizontal blur, then vertical blur
		for (int i = 0; i < 2; i++) {
			struct {
				float blur_unit[2];
				float time;
			} push_constants = {0};
			push_constants.blur_unit[i] = 1.f / (float)g_window_size[i];
			push_constants.time = g_time;

			GPU_OpPrepareRenderPass(graph, g_blur_passes[i]);
			uint32_t blur_draw_params = GPU_OpPrepareDrawParams(graph, g_blur_pipelines[i], g_blur_desc_sets[i]);
			GPU_OpBeginRenderPass(graph);

			GPU_OpBindDrawParams(graph, blur_draw_params);
			GPU_OpPushGraphicsConstants(graph, g_blur_pipeline_layout, &push_constants, sizeof(push_constants));
			GPU_OpDraw(graph, 3, 1, 0, 0);

			GPU_OpEndRenderPass(graph);
		}

		GPU_GraphSubmit(graph);
	}
}

static void OnResizeWindow(uint32_t width, uint32_t height, void *user_ptr) {
	g_time += 1.f / 60.f;
	g_window_size[0] = width;
	g_window_size[1] = height;
	Render();
}

int main() {
	OS_Init();

	OS_Arena os_temp_arena;
	OS_ArenaInit(&os_temp_arena, 4096);

	OS_Window window = OS_WindowCreate(g_window_size[0], g_window_size[1], OS_STR("Blurred Triangle"));
	GPU_Init(window.handle);
	
	g_offscreen_buffers[0] = GPU_MakeTexture(GPU_Format_RGBA8UN, g_window_size[0], g_window_size[1], 1, GPU_TextureFlag_RenderTarget, NULL);
	g_offscreen_buffers[1] = GPU_MakeTexture(GPU_Format_RGBA8UN, g_window_size[0], g_window_size[1], 1, GPU_TextureFlag_RenderTarget, NULL);
	
	int tex_size_x, tex_size_y, tex_num_components;
	char *tex_data = stbi_load("../blurred_triangle/goat.png", &tex_size_x, &tex_size_y, &tex_num_components, 4);
	GPU_Texture *tex = GPU_MakeTexture(GPU_Format_RGBA8UN, tex_size_x, tex_size_y, 1, 0, tex_data);
	stbi_image_free(tex_data);
	
	OS_String main_shader_filepath = OS_STR("../blurred_triangle/textured_triangle.glsl");
	OS_String blur_shader_filepath = OS_STR("../blurred_triangle/blur_shader.glsl");

	OS_String main_shader_src, blur_shader_src;
	assert(OS_ReadEntireFile(&os_temp_arena, main_shader_filepath, &main_shader_src));
	assert(OS_ReadEntireFile(&os_temp_arena, blur_shader_filepath, &blur_shader_src));

	GPU_RenderPassDesc directional_blur_pass_desc = {
		.color_targets_count = 1,
		.color_targets = GPU_SWAPCHAIN_COLOR_TARGET,
	};
	GPU_RenderPass *directional_blur_pass = GPU_MakeRenderPass(&directional_blur_pass_desc);

	GPU_Sampler *my_sampler = GPU_SamplerLinearWrap();

	GPU_TextureView triangle_pass_color_targets[] = {{g_offscreen_buffers[0], 0}};
	GPU_RenderPassDesc triangle_pass_desc = {
		.color_targets = triangle_pass_color_targets,
		.color_targets_count = 1,
	};
	g_triangle_pass = GPU_MakeRenderPass(&triangle_pass_desc);
		
	GPU_PipelineLayout *triangle_pipeline_layout = GPU_InitPipelineLayout();
	GPU_Binding BINDING_MY_TEXTURE = GPU_TextureBinding(triangle_pipeline_layout, "MY_TEXTURE");
	GPU_Binding BINDING_MY_SAMPLER = GPU_SamplerBinding(triangle_pipeline_layout, "MY_SAMPLER");
	GPU_FinalizePipelineLayout(triangle_pipeline_layout);

	GPU_Format triangle_vertex_input_formats[] = {GPU_Format_RG32F};
	GPU_Access triangle_fs_accesses[] = {GPU_Read(BINDING_MY_TEXTURE), GPU_Read(BINDING_MY_SAMPLER)};
	g_triangle_pipeline = GPU_MakeGraphicsPipeline(&(GPU_GraphicsPipelineDesc){
		.layout = triangle_pipeline_layout,
		.render_pass = g_triangle_pass,
		.vs = {
			.glsl = {main_shader_src.data, main_shader_src.size},
			.glsl_debug_filepath = {main_shader_filepath.data, main_shader_filepath.size},
		},
		.fs = {
			.accesses = {triangle_fs_accesses, 2},
			.glsl = {main_shader_src.data, main_shader_src.size},
			.glsl_debug_filepath = {main_shader_filepath.data, main_shader_filepath.size},
		},
		.vertex_input_formats = {triangle_vertex_input_formats, 1},
	});

	g_triangle_desc_set = GPU_InitDescriptorSet(NULL, triangle_pipeline_layout);
	GPU_SetTextureBinding(g_triangle_desc_set, BINDING_MY_TEXTURE, tex);
	GPU_SetSamplerBinding(g_triangle_desc_set, BINDING_MY_SAMPLER, my_sampler);
	GPU_FinalizeDescriptorSet(g_triangle_desc_set);
	
	g_blur_pipeline_layout = GPU_InitPipelineLayout();
	GPU_Binding BINDING_BLUR_INPUT = GPU_TextureBinding(g_blur_pipeline_layout, "BLUR_INPUT");
	GPU_Binding BINDING_BLUR_SAMPLER = GPU_SamplerBinding(g_blur_pipeline_layout, "BLUR_SAMPLER");
	GPU_FinalizePipelineLayout(g_blur_pipeline_layout);

	for (int i = 0; i < 2; i++) {
		GPU_TextureView color_target_1 = {g_offscreen_buffers[1], 0};
		GPU_RenderPassDesc pass_desc = {
			.color_targets = i == 0 ? &color_target_1 : GPU_SWAPCHAIN_COLOR_TARGET,
			.color_targets_count = 1,
		};
		g_blur_passes[i] = GPU_MakeRenderPass(&pass_desc);

		GPU_Access blur_fs_accesses[] = {GPU_Read(BINDING_BLUR_INPUT), GPU_Read(BINDING_BLUR_SAMPLER)};
		GPU_Format blur_vertex_input_formats[] = {GPU_Format_RG32F, GPU_Format_RGB32F};
		
		GPU_GraphicsPipelineDesc pipeline_desc = {
			.layout = g_blur_pipeline_layout,
			.render_pass = g_blur_passes[i],
			.vs = {
				.glsl = {blur_shader_src.data, blur_shader_src.size},
				.glsl_debug_filepath = {blur_shader_filepath.data, blur_shader_filepath.size}, // optional, nice for Renderdoc
			},
			.fs = {
				.accesses = {blur_fs_accesses, 2},
				.glsl = {blur_shader_src.data, blur_shader_src.size},
				.glsl_debug_filepath = {blur_shader_filepath.data, blur_shader_filepath.size},
			},
			.vertex_input_formats = {blur_vertex_input_formats, 2},
		};
		g_blur_pipelines[i] = GPU_MakeGraphicsPipeline(&pipeline_desc);

		g_blur_desc_sets[i] = GPU_InitDescriptorSet(NULL, g_blur_pipeline_layout);
		GPU_SetTextureBinding(g_blur_desc_sets[i], BINDING_BLUR_INPUT, g_offscreen_buffers[i]);
		GPU_SetSamplerBinding(g_blur_desc_sets[i], BINDING_BLUR_SAMPLER, my_sampler);
		GPU_FinalizeDescriptorSet(g_blur_desc_sets[i]);
	}

	Vertex vertices[] = {{-0.5f, -0.5f}, {0.5f, -0.5f}, {0.f, 0.5f}};
	uint32_t indices[] = {0, 1, 2};

	g_vertex_buffer = GPU_MakeBuffer(sizeof(vertices), GPU_BufferFlag_GPU, vertices);
	g_index_buffer = GPU_MakeBuffer(sizeof(indices), GPU_BufferFlag_GPU, indices);

	GPU_MakeSwapchainGraphs(2, &g_graphs[0]);

	// Main loop
	while (!OS_WindowShouldClose(&window)) {
		for (OS_Event event; OS_WindowPollEvent(&window, &event, OnResizeWindow, NULL);) {}

		g_time += 1.f / 60.f;

		Render();
	}
	
	GPU_WaitUntilIdle();

	// Cleanup resources
	
	GPU_DestroyGraph(g_graphs[0]);
	GPU_DestroyGraph(g_graphs[1]);
	GPU_DestroyBuffer(g_vertex_buffer);
	GPU_DestroyBuffer(g_index_buffer);
	
	for (int i = 0; i < 2; i++) {
		GPU_DestroyDescriptorSet(g_blur_desc_sets[i]);
		GPU_DestroyGraphicsPipeline(g_blur_pipelines[i]);
		GPU_DestroyRenderPass(g_blur_passes[i]);
	}
	
	GPU_DestroyPipelineLayout(g_blur_pipeline_layout);
	GPU_DestroyDescriptorSet(g_triangle_desc_set);
	GPU_DestroyGraphicsPipeline(g_triangle_pipeline);
	GPU_DestroyPipelineLayout(triangle_pipeline_layout);
	GPU_DestroyRenderPass(g_triangle_pass);
	GPU_DestroyRenderPass(directional_blur_pass);
	GPU_DestroyTexture(tex);
	GPU_DestroyTexture(g_offscreen_buffers[0]);
	GPU_DestroyTexture(g_offscreen_buffers[1]);
	GPU_Deinit();

	OS_ArenaDeinit(&os_temp_arena);
	OS_Deinit();

	return 0;
}