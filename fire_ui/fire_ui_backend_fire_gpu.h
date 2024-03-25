// This file is part of the Fire UI library, see "fire_ui.h"

typedef struct UI_GPU_State {
	GPU_Texture *atlases[2];
	GPU_Buffer *atlas_staging_buffers[2];
	GPU_PipelineLayout *pipeline_layout;
	GPU_Binding texture_binding;
	GPU_Binding sampler_binding;
	GPU_GraphicsPipeline *pipeline;
	GPU_Sampler *sampler; // external, not owned
	GPU_RenderPass *render_pass; // external, not owned

	// per-frame state
	bool atlas_is_mapped[2];
} UI_GPU_State;

static UI_GPU_State UI_GPU_STATE;

static UI_TextureID UI_GPU_CreateAtlas(int atlas_id, uint32_t width, uint32_t height) {
	GPU_Texture *tex = GPU_MakeTexture(GPU_Format_RGBA8UN, width, height, 1, 0, NULL);
	UI_GPU_STATE.atlas_staging_buffers[atlas_id] = GPU_MakeBuffer(width * height * 4, GPU_BufferFlag_CPU, NULL);
	UI_GPU_STATE.atlases[atlas_id] = tex;
	return (UI_TextureID)tex;
}

static void UI_GPU_DestroyAtlas(int atlas_id) {
	GPU_DestroyTexture(UI_GPU_STATE.atlases[atlas_id]);
	GPU_DestroyBuffer(UI_GPU_STATE.atlas_staging_buffers[atlas_id]);
}

static void *UI_GPU_AtlasMapUntilFrameEnd(int atlas_id) {
	UI_GPU_STATE.atlas_is_mapped[atlas_id] = true;
	return UI_GPU_STATE.atlas_staging_buffers[atlas_id]->data;
}

static void *UI_GPU_BufferMapUntilFrameEnd(UI_BufferID buffer_id) {
	GPU_Buffer *buffer = (GPU_Buffer*)buffer_id;
	return buffer->data;
}

static UI_BufferID UI_GPU_CreateBuffer(uint32_t size_in_bytes) {
	GPU_Buffer *buffer = GPU_MakeBuffer(size_in_bytes, GPU_BufferFlag_CPU|GPU_BufferFlag_GPU, NULL);
	return (UI_BufferID)buffer;
}

static void UI_GPU_DestroyBuffer(UI_BufferID buffer_id) {
	GPU_DestroyBuffer((GPU_Buffer*)buffer_id);
}

static void UI_GPU_Init(UI_Backend *backend, GPU_RenderPass *render_pass, GPU_String shader_src) {
	UI_GPU_State state = {0};
	state.render_pass = render_pass;

	state.sampler = GPU_SamplerLinearClamp();
	state.pipeline_layout = GPU_InitPipelineLayout();
	state.texture_binding = GPU_TextureBinding(state.pipeline_layout, "TEXTURE");
	state.sampler_binding = GPU_SamplerBinding(state.pipeline_layout, "SAMPLER_LINEAR_REPEAT");
	GPU_FinalizePipelineLayout(state.pipeline_layout);

	GPU_Access fs_accesses[] = {GPU_Read(state.texture_binding), GPU_Read(state.sampler_binding)};
	GPU_Format vertex_input_formats[] = {GPU_Format_RG32F, GPU_Format_RG32F, GPU_Format_RGBA8UN};

	GPU_GraphicsPipelineDesc desc = {0};
	desc.render_pass = render_pass;
	desc.layout = state.pipeline_layout;
	desc.vs.glsl = shader_src;
	desc.fs.accesses.data = fs_accesses;
	desc.fs.accesses.length = GPU_ArrayCount(fs_accesses);
	desc.fs.glsl = shader_src;
	desc.vertex_input_formats.data = vertex_input_formats;
	desc.vertex_input_formats.length = GPU_ArrayCount(vertex_input_formats);
	desc.enable_blending = true;
	desc.enable_depth_write = true; // just for testing
	//.cull_mode = gpuCullMode_DrawCW,
	state.pipeline = GPU_MakeGraphicsPipeline(&desc);
	
	UI_GPU_STATE = state;

	backend->create_vertex_buffer = UI_GPU_CreateBuffer;
	backend->create_index_buffer = UI_GPU_CreateBuffer;
	backend->destroy_buffer = UI_GPU_DestroyBuffer;

	backend->create_atlas = UI_GPU_CreateAtlas;
	backend->destroy_atlas = UI_GPU_DestroyAtlas;

	backend->buffer_map_until_frame_end = UI_GPU_BufferMapUntilFrameEnd;
	backend->atlas_map_until_frame_end = UI_GPU_AtlasMapUntilFrameEnd;
}

static void UI_GPU_Deinit(void) {
	GPU_DestroyPipelineLayout(UI_GPU_STATE.pipeline_layout);
	GPU_DestroyGraphicsPipeline(UI_GPU_STATE.pipeline);
}

static void UI_GPU_BeginFrame(void) {
	UI_GPU_STATE.atlas_is_mapped[0] = false;
	UI_GPU_STATE.atlas_is_mapped[1] = false;
}

static void UI_GPU_EndFrame(UI_Outputs *outputs, GPU_Graph *graph, GPU_DescriptorArena *descriptor_arena) {
	for (int i = 0; i < 2; i++) {
		if (UI_GPU_STATE.atlas_is_mapped[i]) {
			GPU_OpCopyBufferToTexture(graph, UI_GPU_STATE.atlas_staging_buffers[i], UI_GPU_STATE.atlases[i], 0, 1, 0);
		}
	}
	
	GPU_OpPrepareRenderPass(graph, UI_GPU_STATE.render_pass);

	DS_Vec(uint32_t) draw_params = {0};
	draw_params.arena = UI_FrameArena();
	DS_VecReserve(&draw_params, outputs->draw_calls_count);

	for (int i = 0; i < outputs->draw_calls_count; i++) {
		UI_DrawCall *draw_call = &outputs->draw_calls[i];
		GPU_DescriptorSet *desc_set = GPU_InitDescriptorSet(descriptor_arena, UI_GPU_STATE.pipeline_layout);
		GPU_SetSamplerBinding(desc_set, UI_GPU_STATE.sampler_binding, UI_GPU_STATE.sampler);
		GPU_SetTextureBinding(desc_set, UI_GPU_STATE.texture_binding, (GPU_Texture*)draw_call->texture);
		GPU_FinalizeDescriptorSet(desc_set);

		uint32_t draw_params_idx = GPU_OpPrepareDrawParams(graph, UI_GPU_STATE.pipeline, desc_set);
		DS_VecPush(&draw_params, draw_params_idx);
	}

	GPU_OpBeginRenderPass(graph);

	UI_Vec2 pixel_size = {1.f / UI_STATE.window_size.x, 1.f / UI_STATE.window_size.y};
	GPU_OpPushGraphicsConstants(graph, UI_GPU_STATE.pipeline_layout, &pixel_size, sizeof(pixel_size));

	UI_BufferID vertex_buffer = UI_BUFFER_ID_NIL;
	UI_BufferID index_buffer = UI_BUFFER_ID_NIL;

	for (int i = 0; i < outputs->draw_calls_count; i++) {
		UI_DrawCall *draw_call = &outputs->draw_calls[i];
		
		if (draw_call->vertex_buffer != vertex_buffer) {
			GPU_OpBindVertexBuffer(graph, (GPU_Buffer*)draw_call->vertex_buffer);
			vertex_buffer = draw_call->vertex_buffer;
		}

		if (draw_call->index_buffer != index_buffer) {
			GPU_OpBindIndexBuffer(graph, (GPU_Buffer*)draw_call->index_buffer);
			index_buffer = draw_call->index_buffer;
		}

		GPU_OpBindDrawParams(graph, DS_VecGet(draw_params, i));
		GPU_OpDrawIndexed(graph, draw_call->index_count, 1, draw_call->first_index, 0, 0);
	}

	GPU_OpEndRenderPass(graph);
}
