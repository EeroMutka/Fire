
typedef struct UI_DX12_Buffer {
	ID3D12Resource* handle;
	uint32_t size;
	void* mapped_ptr;
} UI_DX12_Buffer;

typedef struct UI_DX12_State {
	ID3D12Device* device;
	D3D12_CPU_DESCRIPTOR_HANDLE atlas_cpu_descriptor;
	D3D12_GPU_DESCRIPTOR_HANDLE atlas_gpu_descriptor;

	ID3D12RootSignature* root_signature;
	ID3D12PipelineState* pipeline_state;

	UI_DX12_Buffer vertex_buffer;
	UI_DX12_Buffer index_buffer;

	ID3D12Resource* atlas;
	ID3D12Resource* atlas_staging_buffer;
	uint32_t atlas_width;
	uint32_t atlas_height;
	void* atlas_mapped_ptr;
} UI_DX12_State;

// -- GLOBALS ----------

UI_API UI_DX12_State UI_DX12_STATE;

// ---------------------

static UI_Texture* UI_DX12_CreateAtlas(uint32_t width, uint32_t height) {
	UI_ASSERT(UI_DX12_STATE.atlas == NULL); // TODO

	// Create the GPU-local texture
	ID3D12Resource* texture;
	{
		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
		desc.Alignment = 0;
		desc.Width = width;
		desc.Height = height;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;
		
		D3D12_HEAP_PROPERTIES props = {};
		props.Type = D3D12_HEAP_TYPE_DEFAULT;
		props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

		bool ok = UI_DX12_STATE.device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, NULL, IID_PPV_ARGS(&texture)) == S_OK;
		UI_ASSERT(ok);
		UI_DX12_STATE.atlas = texture;
		UI_DX12_STATE.atlas_width = width;
		UI_DX12_STATE.atlas_height = height;
	}

	// Create the staging buffer
	{
		D3D12_RESOURCE_DESC desc = {};
		desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
		desc.Alignment = 0;
		desc.Width = height * ((width * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1));
		desc.Height = 1;
		desc.DepthOrArraySize = 1;
		desc.MipLevels = 1;
		desc.Format = DXGI_FORMAT_UNKNOWN;
		desc.SampleDesc.Count = 1;
		desc.SampleDesc.Quality = 0;
		desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
		desc.Flags = D3D12_RESOURCE_FLAG_NONE;

		D3D12_HEAP_PROPERTIES props = {};
		props.Type = D3D12_HEAP_TYPE_UPLOAD;
		props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

		bool ok = UI_DX12_STATE.device->CreateCommittedResource(&props, D3D12_HEAP_FLAG_NONE, &desc,
			D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&UI_DX12_STATE.atlas_staging_buffer)) == S_OK;
		UI_ASSERT(ok);
	}

	// Create texture view
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC desc = {};
		desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		desc.Texture2D.MipLevels = 1;
		UI_DX12_STATE.device->CreateShaderResourceView(texture, &desc, UI_DX12_STATE.atlas_cpu_descriptor);
	}

	return (UI_Texture*)UI_DX12_STATE.atlas_gpu_descriptor.ptr;
}

static void* UI_DX12_MapAtlas() {
	if (UI_DX12_STATE.atlas_mapped_ptr == NULL) {
		D3D12_RANGE read_range = {}; // We do not intend to read from this resource on the CPU.
		bool ok = UI_DX12_STATE.atlas_staging_buffer->Map(0, &read_range, &UI_DX12_STATE.atlas_mapped_ptr) == S_OK;
		UI_ASSERT(ok);
	}
	return UI_DX12_STATE.atlas_mapped_ptr;
}

// Resizes and maps a buffer
static void UI_DX12_ResizeAndMapBuffer(UI_DX12_Buffer* buffer, uint32_t size) {
	if (size == 0) {
		buffer->handle->Release();
	}
	else {
		// resize if necessary
		if (size > buffer->size) {
			D3D12_HEAP_PROPERTIES heap_props = {};
			heap_props.Type = D3D12_HEAP_TYPE_UPLOAD;
			heap_props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
			heap_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

			D3D12_RESOURCE_DESC desc = {};
			desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
			desc.Width = size;
			desc.Height = 1;
			desc.DepthOrArraySize = 1;
			desc.MipLevels = 1;
			desc.Format = DXGI_FORMAT_UNKNOWN;
			desc.SampleDesc.Count = 1;
			desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
			desc.Flags = D3D12_RESOURCE_FLAG_NONE;

			ID3D12Resource* new_buffer;
			bool ok = UI_DX12_STATE.device->CreateCommittedResource(&heap_props, D3D12_HEAP_FLAG_NONE, &desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL,
				IID_PPV_ARGS(&new_buffer)) == S_OK;
			UI_ASSERT(ok);
		
			void* new_buffer_mapped_ptr;
			ok = new_buffer->Map(0, NULL, &new_buffer_mapped_ptr) == S_OK;
			UI_ASSERT(ok);
		
			if (buffer->mapped_ptr) {
				UI_ASSERT(buffer->size > 0);
				// copy existing data over to the new buffer
				memcpy(new_buffer_mapped_ptr, buffer->mapped_ptr, buffer->size);
			}
			
			if (buffer->handle) {
				buffer->handle->Release();
			}

			buffer->handle = new_buffer;
			buffer->size = size;
			buffer->mapped_ptr = new_buffer_mapped_ptr;
		}
		
		// map if necessary
		if (buffer->mapped_ptr == NULL) {
			bool ok = buffer->handle->Map(0, NULL, &buffer->mapped_ptr) == S_OK;
			UI_ASSERT(ok);
		}
	}
}

static UI_DrawVertex* UI_DX12_ResizeAndMapVertexBuffer(int num_vertices) {
	UI_DX12_ResizeAndMapBuffer(&UI_DX12_STATE.vertex_buffer, num_vertices * sizeof(UI_DrawVertex));
	return (UI_DrawVertex*)UI_DX12_STATE.vertex_buffer.mapped_ptr;
}

static uint32_t* UI_DX12_ResizeAndMapIndexBuffer(int num_indices) {
	UI_DX12_ResizeAndMapBuffer(&UI_DX12_STATE.index_buffer, num_indices * sizeof(uint32_t));
	return (uint32_t*)UI_DX12_STATE.index_buffer.mapped_ptr;
}

// must be called *after* UI_Init
static void UI_DX12_Init(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE atlas_cpu_descriptor, D3D12_GPU_DESCRIPTOR_HANDLE atlas_gpu_descriptor) {
	memset(&UI_DX12_STATE, 0, sizeof(UI_DX12_STATE));
	UI_DX12_STATE.device = device;
	UI_DX12_STATE.atlas_cpu_descriptor = atlas_cpu_descriptor;
	UI_DX12_STATE.atlas_gpu_descriptor = atlas_gpu_descriptor;

	UI_STATE.backend.ResizeAndMapVertexBuffer = UI_DX12_ResizeAndMapVertexBuffer;
	UI_STATE.backend.ResizeAndMapIndexBuffer = UI_DX12_ResizeAndMapIndexBuffer;

	// Create the root signature
	{
		D3D12_DESCRIPTOR_RANGE descriptor_range = {};
		descriptor_range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		descriptor_range.NumDescriptors = 1;
		descriptor_range.BaseShaderRegister = 0;
		descriptor_range.RegisterSpace = 0;
		descriptor_range.OffsetInDescriptorsFromTableStart = 0;

		D3D12_ROOT_PARAMETER root_params[2] = {};

		root_params[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
		root_params[0].Constants.ShaderRegister = 0;
		root_params[0].Constants.RegisterSpace = 0;
		root_params[0].Constants.Num32BitValues = 16;
		root_params[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

		root_params[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		root_params[1].DescriptorTable.NumDescriptorRanges = 1;
		root_params[1].DescriptorTable.pDescriptorRanges = &descriptor_range;
		root_params[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_STATIC_SAMPLER_DESC sampler = {};
		sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
		sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
		sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_ALWAYS;
		sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
		sampler.ShaderRegister = 0;
		sampler.RegisterSpace = 0;
		sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

		D3D12_ROOT_SIGNATURE_DESC desc = {};
		desc.pParameters = root_params;
		desc.NumParameters = DS_ArrayCount(root_params);
		desc.pStaticSamplers = &sampler;
		desc.NumStaticSamplers = 1;
		desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

		ID3DBlob* signature = NULL;
		bool ok = D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, NULL) == S_OK;
		UI_ASSERT(ok);

		ok = device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&UI_DX12_STATE.root_signature)) == S_OK;
		UI_ASSERT(ok);

		signature->Release();
	}

	// Create the pipeline
	{
		ID3DBlob* vertex_shader;
		ID3DBlob* pixel_shader;

#ifdef UI_DX12_DEBUG_MODE
		UINT compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
		UINT compile_flags = 0;
#endif

		static const char shader_src[] = "\
		cbuffer Vertex32BitConstants : register(b0)\
		{\
			float4x4 projection_matrix;\
		};\
		\
		struct PSInput\
		{\
			float4 position : SV_POSITION;\
			float2 uv       : TEXCOORD0;\
			float4 color    : TEXCOORD1;\
		};\
		\
		PSInput VSMain(float2 position : POSITION, float2 uv : UV, float4 color : COLOR)\
		{\
			PSInput result;\
			result.position = mul(projection_matrix, float4(position, 0.f, 1.f));\
			result.uv = uv;\
			result.color = color;\
			return result;\
		}\
		\
		SamplerState sampler0 : register(s0);\
        Texture2D texture0 : register(t0);\
		\
		float4 PSMain(PSInput input) : SV_TARGET\
		{\
			return input.color * texture0.Sample(sampler0, input.uv);\
		}";

		bool ok = D3DCompile(shader_src, sizeof(shader_src), NULL, NULL, NULL, "VSMain", "vs_5_0", compile_flags, 0, &vertex_shader, NULL) == S_OK;
		UI_ASSERT(ok);

		ok = D3DCompile(shader_src, sizeof(shader_src), NULL, NULL, NULL, "PSMain", "ps_5_0", compile_flags, 0, &pixel_shader, NULL) == S_OK;
		UI_ASSERT(ok);

		D3D12_INPUT_ELEMENT_DESC input_elem_desc[] = {
			{ "POSITION", 0,   DXGI_FORMAT_R32G32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{       "UV", 0,   DXGI_FORMAT_R32G32_FLOAT, 0,  8, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
			{    "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		};

		// Describe and create the graphics pipeline state object (PSO).
		D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};

		pso_desc.InputLayout = { input_elem_desc, _countof(input_elem_desc) };
		pso_desc.pRootSignature = UI_DX12_STATE.root_signature;
		pso_desc.VS = { vertex_shader->GetBufferPointer(), vertex_shader->GetBufferSize() };
		pso_desc.PS = { pixel_shader->GetBufferPointer(), pixel_shader->GetBufferSize() };

		pso_desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		pso_desc.RasterizerState.CullMode = D3D12_CULL_MODE_NONE;
		pso_desc.RasterizerState.FrontCounterClockwise = false;
		pso_desc.RasterizerState.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
		pso_desc.RasterizerState.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
		pso_desc.RasterizerState.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
		pso_desc.RasterizerState.DepthClipEnable = true;
		pso_desc.RasterizerState.MultisampleEnable = false;
		pso_desc.RasterizerState.AntialiasedLineEnable = false;
		pso_desc.RasterizerState.ForcedSampleCount = 0;
		pso_desc.RasterizerState.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

		pso_desc.BlendState.AlphaToCoverageEnable = false;
		pso_desc.BlendState.RenderTarget[0].BlendEnable = true;
		pso_desc.BlendState.RenderTarget[0].SrcBlend              = D3D12_BLEND_SRC_ALPHA;
		pso_desc.BlendState.RenderTarget[0].DestBlend             = D3D12_BLEND_INV_SRC_ALPHA;
		pso_desc.BlendState.RenderTarget[0].BlendOp               = D3D12_BLEND_OP_ADD;
		pso_desc.BlendState.RenderTarget[0].SrcBlendAlpha         = D3D12_BLEND_ONE;
		pso_desc.BlendState.RenderTarget[0].DestBlendAlpha        = D3D12_BLEND_INV_SRC_ALPHA;
		pso_desc.BlendState.RenderTarget[0].BlendOpAlpha          = D3D12_BLEND_OP_ADD;
		pso_desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		pso_desc.DepthStencilState.DepthEnable = false;
		pso_desc.DepthStencilState.StencilEnable = false;
		pso_desc.SampleMask = UINT_MAX;
		pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
		pso_desc.NumRenderTargets = 1;
		pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		pso_desc.SampleDesc.Count = 1;

		ok = device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&UI_DX12_STATE.pipeline_state)) == S_OK;
		UI_ASSERT(ok);

		vertex_shader->Release();
		pixel_shader->Release();
	}
}

static void UI_DX12_Deinit(void) {
	UI_DX12_ResizeAndMapVertexBuffer(0);
	UI_DX12_ResizeAndMapIndexBuffer(0);
	
	if (UI_DX12_STATE.atlas) {
		UI_DX12_STATE.atlas_staging_buffer->Release();
		UI_DX12_STATE.atlas->Release();
	}

	UI_DX12_STATE.pipeline_state->Release();
	UI_DX12_STATE.root_signature->Release();
}

// The following graphics state is expected to be already set, and will not be modified:
// - Primitive topology (must be D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST)
// - Viewport
// - Scissor
// - Render target
// - SRV descriptor heap
// The following graphics state will be overwritten:
// - Pipeline state
// - Graphics root signature
// - Graphics root descriptor table
// - Vertex buffer
// - Index buffer
static void UI_DX12_Draw(UI_Outputs* outputs, UI_Vec2 window_size, ID3D12GraphicsCommandList* command_list) {
	UI_DX12_State* s = &UI_DX12_STATE;

	if (s->atlas_mapped_ptr) {
		s->atlas_staging_buffer->Unmap(0, NULL);

		uint32_t width = s->atlas_width;
		uint32_t height = s->atlas_height;
		uint32_t row_pitch = (width * 4 + D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1) & ~(D3D12_TEXTURE_DATA_PITCH_ALIGNMENT - 1);
			
		D3D12_RESOURCE_BARRIER pre_barrier = {};
		pre_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		pre_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		pre_barrier.Transition.pResource   = s->atlas;
		pre_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		pre_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		pre_barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_COPY_DEST;
		command_list->ResourceBarrier(1, &pre_barrier);

		D3D12_TEXTURE_COPY_LOCATION src_loc = {};
		src_loc.pResource = s->atlas_staging_buffer;
		src_loc.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
		src_loc.PlacedFootprint.Footprint.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		src_loc.PlacedFootprint.Footprint.Width = width;
		src_loc.PlacedFootprint.Footprint.Height = height;
		src_loc.PlacedFootprint.Footprint.Depth = 1;
		src_loc.PlacedFootprint.Footprint.RowPitch = row_pitch;

		D3D12_TEXTURE_COPY_LOCATION dst_loc = {};
		dst_loc.pResource = s->atlas;
		dst_loc.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
		dst_loc.SubresourceIndex = 0;
		
		command_list->CopyTextureRegion(&dst_loc, 0, 0, 0, &src_loc, NULL);

		D3D12_RESOURCE_BARRIER post_barrier = {};
		post_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		post_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		post_barrier.Transition.pResource   = s->atlas;
		post_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		post_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
		post_barrier.Transition.StateAfter  = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
		command_list->ResourceBarrier(1, &post_barrier);

		s->atlas_mapped_ptr = NULL;
	}
	
	UI_DX12_Buffer* buffers[] = { &s->vertex_buffer, &s->index_buffer };
	for (int i = 0; i < 2; i++) {
		if (buffers[i]->mapped_ptr) {
			buffers[i]->handle->Unmap(0, NULL);
			buffers[i]->mapped_ptr = NULL;
		}
	}

	float L = 0.f;
	float R = window_size.x;
	float T = 0.f;
	float B = window_size.y;
	float vertex_32bit_constants[4][4] = {
		{ 2.0f/(R-L),  0.0f,        0.0f, 0.0f },
		{ 0.0f,        2.0f/(T-B),  0.0f, 0.0f },
		{ 0.0f,        0.0f,        0.5f, 0.0f },
		{ (R+L)/(L-R), (T+B)/(B-T), 0.5f, 1.0f },
	};

	command_list->SetPipelineState(s->pipeline_state);
	command_list->SetGraphicsRootSignature(s->root_signature);
	command_list->SetGraphicsRoot32BitConstants(0, 16, vertex_32bit_constants, 0);

	D3D12_VERTEX_BUFFER_VIEW vb_view;
	vb_view.BufferLocation = s->vertex_buffer.handle->GetGPUVirtualAddress();
	vb_view.StrideInBytes = 2 * sizeof(float) + 2 * sizeof(float) + 4; // pos, uv, color
	vb_view.SizeInBytes = s->vertex_buffer.size;
	command_list->IASetVertexBuffers(0, 1, &vb_view);

	D3D12_INDEX_BUFFER_VIEW ib_view;
	ib_view.BufferLocation = s->index_buffer.handle->GetGPUVirtualAddress();
	ib_view.SizeInBytes = s->index_buffer.size;
	ib_view.Format = DXGI_FORMAT_R32_UINT;
	command_list->IASetIndexBuffer(&ib_view);

	for (int i = 0; i < outputs->draw_commands_count; i++) {
		UI_DrawCommand* draw = &outputs->draw_commands[i];
		
		D3D12_RECT scissor_rect;
		scissor_rect.left = (int)draw->scissor_rect.min.x;
		scissor_rect.right = (int)draw->scissor_rect.max.x;
		scissor_rect.top = (int)draw->scissor_rect.min.y;
		scissor_rect.bottom = (int)draw->scissor_rect.max.y;
		command_list->RSSetScissorRects(1, &scissor_rect);

		D3D12_GPU_DESCRIPTOR_HANDLE handle = {};
		handle.ptr = draw->texture ? (UINT64)draw->texture : UI_DX12_STATE.atlas_gpu_descriptor.ptr;
		command_list->SetGraphicsRootDescriptorTable(1, handle);
		command_list->DrawIndexedInstanced(draw->index_count, 1, draw->first_index, 0, 0);
	}
}
