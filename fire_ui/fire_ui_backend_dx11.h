// This file is part of the Fire UI library, see "fire_ui.h"

static const char UI_DX11_SHADER_SRC[] = ""
"cbuffer constants : register(b0) {\n"
"float2 pixel_size;\n"
"}\n"
"Texture2D    mytexture : register(t0);\n"
"SamplerState mysampler : register(s0);\n"
"\n"
"struct vertexdata {\n"
"	float2 position : POS;\n"
"	float2 uv       : UVPOS;\n"
"	float4 color    : COL;\n"
"};\n"
"\n"
"struct pixeldata {\n"
"	float4 position : SV_POSITION;\n"
"	float2 uv       : UVPOS;\n"
"	float4 color    : COL;\n"
"};\n"
"\n"
"pixeldata vertex_shader(vertexdata vertex) {\n"
"	pixeldata output;\n"
"	output.position = float4(2.*vertex.position*pixel_size - 1., 0.5, 1);\n"
"	output.position.y *= -1.;\n"
"	output.uv = vertex.uv;\n"
"	output.color = vertex.color;\n"
"	return output;\n"
"}\n"
"\n"
"float4 pixel_shader(pixeldata pixel) : SV_TARGET { \n"
"	return mytexture.Sample(mysampler, pixel.uv) * pixel.color;\n"
"}\n";

typedef struct UI_DX11_State {
	ID3D11Device* device;
	ID3D11DeviceContext* device_context;
	ID3D11VertexShader* vertex_shader;
	ID3D11PixelShader* pixel_shader;
	ID3D11InputLayout* input_layout;
	ID3D11RasterizerState* raster_state;
	ID3D11SamplerState* sampler_state;
	ID3D11Buffer* constant_buffer;
	ID3D11BlendState* blend_state;

	ID3D11Texture2D* atlases[2];
	ID3D11Texture2D* atlases_staging[2];
	ID3D11ShaderResourceView* atlases_srv[2];
	D3D11_MAPPED_SUBRESOURCE atlases_mapped[2];

	ID3D11Buffer* buffers[UI_MAX_BACKEND_BUFFERS];
	D3D11_MAPPED_SUBRESOURCE buffers_mapped[UI_MAX_BACKEND_BUFFERS];
} UI_DX11_State;

static UI_DX11_State UI_DX11_STATE;

static UI_TextureID UI_DX11_CreateAtlas(int atlas_id, uint32_t width, uint32_t height) {
	D3D11_TEXTURE2D_DESC staging_desc = {0};
	staging_desc.Width = width;
	staging_desc.Height = height;
	staging_desc.MipLevels = 1;
	staging_desc.ArraySize = 1;
	staging_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	staging_desc.SampleDesc.Count = 1;
	staging_desc.Usage = D3D11_USAGE_STAGING;
	staging_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	ID3D11Texture2D* staging;
	UI_DX11_STATE.device->CreateTexture2D(&staging_desc, NULL, &staging);

	D3D11_TEXTURE2D_DESC tex_desc = {0};
	tex_desc.Width = width;
	tex_desc.Height = height;
	tex_desc.MipLevels = 1;
	tex_desc.ArraySize = 1;
	tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	tex_desc.SampleDesc.Count = 1;
	tex_desc.Usage = D3D11_USAGE_DEFAULT;
	tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;

	ID3D11Texture2D* texture;
	UI_DX11_STATE.device->CreateTexture2D(&tex_desc, NULL, &texture);

	ID3D11ShaderResourceView* texture_srv;
	UI_DX11_STATE.device->CreateShaderResourceView(texture, NULL, &texture_srv);

	UI_DX11_STATE.atlases[atlas_id] = texture;
	UI_DX11_STATE.atlases_staging[atlas_id] = staging;
	UI_DX11_STATE.atlases_srv[atlas_id] = texture_srv;

	return (UI_TextureID)texture_srv;
}

static void UI_DX11_DestroyAtlas(int atlas_id) {
	UI_DX11_STATE.atlases[atlas_id]->Release();
	UI_DX11_STATE.atlases[atlas_id] = NULL;
	UI_DX11_STATE.atlases_staging[atlas_id]->Release();
	UI_DX11_STATE.atlases_staging[atlas_id] = NULL;
	UI_DX11_STATE.atlases_srv[atlas_id]->Release();
	UI_DX11_STATE.atlases_srv[atlas_id] = NULL;
}

static void* UI_DX11_AtlasMapUntilFrameEnd(int atlas_id) {
	D3D11_MAPPED_SUBRESOURCE* mapped = &UI_DX11_STATE.atlases_mapped[atlas_id];
	if (mapped->pData == NULL) {
		ID3D11Texture2D* staging = UI_DX11_STATE.atlases_staging[atlas_id];
		UI_DX11_STATE.device_context->Map(staging, 0, D3D11_MAP_WRITE, 0, mapped);
		assert(mapped->pData != NULL);
	}
	return mapped->pData;
}

static void* UI_DX11_BufferMapUntilFrameEnd(int buffer_id) {
	D3D11_MAPPED_SUBRESOURCE* mapped = &UI_DX11_STATE.buffers_mapped[buffer_id];
	if (mapped->pData == NULL) {
		ID3D11Buffer* buffer = UI_DX11_STATE.buffers[buffer_id];
		UI_DX11_STATE.device_context->Map(buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, mapped);
		assert(mapped->pData != NULL);
	}

	return mapped->pData;
}

static void UI_DX11_CreateBuffer(int buffer_id, uint32_t size_in_bytes, D3D11_BIND_FLAG bind_flags) {
	D3D11_BUFFER_DESC desc = {0};
	desc.ByteWidth = size_in_bytes;
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.BindFlags = bind_flags;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	ID3D11Buffer* buffer;
	UI_DX11_STATE.device->CreateBuffer(&desc, NULL, &buffer);
	UI_DX11_STATE.buffers[buffer_id] = buffer;
}

static void UI_DX11_CreateVertexBuffer(int buffer_id, uint32_t size_in_bytes) {
	UI_DX11_CreateBuffer(buffer_id, size_in_bytes, D3D11_BIND_VERTEX_BUFFER);
}

static void UI_DX11_CreateIndexBuffer(int buffer_id, uint32_t size_in_bytes) {
	UI_DX11_CreateBuffer(buffer_id, size_in_bytes, D3D11_BIND_INDEX_BUFFER);
}

static void UI_DX11_DestroyBuffer(int buffer_id) {
	UI_DX11_STATE.buffers[buffer_id]->Release();
	UI_DX11_STATE.buffers[buffer_id] = NULL;
}

static void UI_DX11_Init(UI_Backend* backend, ID3D11Device* device, ID3D11DeviceContext* device_context) {
	UI_DX11_State state = {0};
	state.device = device;
	state.device_context = device_context;

	ID3DBlob* vertexshaderCSO;
	D3DCompile(UI_DX11_SHADER_SRC, sizeof(UI_DX11_SHADER_SRC) - 1, "VS", NULL, NULL, "vertex_shader", "vs_5_0", 0, 0, &vertexshaderCSO, NULL);

	device->CreateVertexShader(vertexshaderCSO->GetBufferPointer(), vertexshaderCSO->GetBufferSize(), NULL, &state.vertex_shader);

	D3D11_INPUT_ELEMENT_DESC inputelementdesc[] = {
		{  "POS", 0, DXGI_FORMAT_R32G32_FLOAT,   0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"UVPOS", 0, DXGI_FORMAT_R32G32_FLOAT,   0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{  "COL", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	device->CreateInputLayout(inputelementdesc, ARRAYSIZE(inputelementdesc), vertexshaderCSO->GetBufferPointer(), vertexshaderCSO->GetBufferSize(), &state.input_layout);

	///////////////////////////////////////////////////////////////////////////////////////////////

	ID3DBlob* pixelshaderCSO;
	D3DCompile(UI_DX11_SHADER_SRC, sizeof(UI_DX11_SHADER_SRC) - 1, "PS", NULL, NULL, "pixel_shader", "ps_5_0", 0, 0, &pixelshaderCSO, NULL);

	device->CreatePixelShader(pixelshaderCSO->GetBufferPointer(), pixelshaderCSO->GetBufferSize(), NULL, &state.pixel_shader);

	///////////////////////////////////////////////////////////////////////////////////////////////

	D3D11_SAMPLER_DESC samplerdesc = {0};
	samplerdesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerdesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerdesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerdesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerdesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	device->CreateSamplerState(&samplerdesc, &state.sampler_state);

	///////////////////////////////////////////////////////////////////////////////////////////////

	D3D11_RENDER_TARGET_BLEND_DESC blend_desc = {0};
	blend_desc.BlendEnable = 1;
	blend_desc.SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blend_desc.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blend_desc.BlendOp = D3D11_BLEND_OP_ADD;
	blend_desc.SrcBlendAlpha = D3D11_BLEND_ONE;
	blend_desc.DestBlendAlpha = D3D11_BLEND_ZERO;
	blend_desc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blend_desc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	D3D11_BLEND_DESC blend_state_desc = {0};
	blend_state_desc.RenderTarget[0] = blend_desc;
	device->CreateBlendState(&blend_state_desc, &state.blend_state);

	///////////////////////////////////////////////////////////////////////////////////////////////

	D3D11_BUFFER_DESC buffer_desc = {0};
	buffer_desc.ByteWidth = (sizeof(float[2]) + 0xf) & 0xfffffff0; // ensure constant buffer size is multiple of 16 bytes
	buffer_desc.Usage = D3D11_USAGE_DYNAMIC; // Will be updated every frame
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	device->CreateBuffer(&buffer_desc, NULL, &state.constant_buffer);

	///////////////////////////////////////////////////////////////////////////////////////////////

	D3D11_RASTERIZER_DESC raster_desc = {0};
	raster_desc.FillMode = D3D11_FILL_SOLID;
	raster_desc.CullMode = D3D11_CULL_NONE;
	raster_desc.MultisampleEnable = true; // MSAA
	//raster_desc.CullMode = D3D11_CULL_BACK;
	device->CreateRasterizerState(&raster_desc, &state.raster_state);

	backend->create_vertex_buffer = UI_DX11_CreateVertexBuffer;
	backend->create_index_buffer = UI_DX11_CreateIndexBuffer;
	backend->destroy_buffer = UI_DX11_DestroyBuffer;

	backend->create_atlas = UI_DX11_CreateAtlas;
	backend->destroy_atlas = UI_DX11_DestroyAtlas;

	backend->buffer_map_until_frame_end = UI_DX11_BufferMapUntilFrameEnd;
	backend->atlas_map_until_frame_end = UI_DX11_AtlasMapUntilFrameEnd;

	UI_DX11_STATE = state;
}

static void UI_DX11_Deinit(void) {
	UI_DX11_STATE.vertex_shader->Release();
	UI_DX11_STATE.pixel_shader->Release();
	UI_DX11_STATE.input_layout->Release();
	UI_DX11_STATE.raster_state->Release();
	UI_DX11_STATE.sampler_state->Release();
	UI_DX11_STATE.constant_buffer->Release();
	UI_DX11_STATE.blend_state->Release();
	memset(&UI_DX11_STATE, 0, sizeof(UI_DX11_STATE));
}

static void UI_DX11_BeginFrame(void) {}

static void UI_DX11_EndFrame(UI_Outputs* outputs, ID3D11RenderTargetView* framebuffer_rtv) {
	ID3D11DeviceContext* dc = UI_DX11_STATE.device_context;

	for (int i = 0; i < 2; i++) {
		if (UI_DX11_STATE.atlases_mapped[i].pData) {
			ID3D11Texture2D* staging = UI_DX11_STATE.atlases_staging[i];
			ID3D11Texture2D* dest = UI_DX11_STATE.atlases[i];
			dc->Unmap(staging, 0);

			dc->CopyResource(dest, staging);

			memset(&UI_DX11_STATE.atlases_mapped[i], 0, sizeof(UI_DX11_STATE.atlases_mapped[i]));
		}
	}

	for (int i = 0; i < UI_MAX_BACKEND_BUFFERS; i++) {
		if (UI_DX11_STATE.buffers_mapped[i].pData) {
			ID3D11Buffer* buffer = UI_DX11_STATE.buffers[i];
			dc->Unmap(buffer, 0);

			memset(&UI_DX11_STATE.buffers_mapped[i], 0, sizeof(UI_DX11_STATE.buffers_mapped[i]));
		}
	}

	UINT stride = 2 * sizeof(float) + 2 * sizeof(float) + 4; // pos, uv, color
	UINT offset = 0;
	D3D11_VIEWPORT viewport = { 0.0f, 0.0f, UI_STATE.window_size.x, UI_STATE.window_size.y, 0.0f, 1.0f };

	D3D11_MAPPED_SUBRESOURCE constantbufferMSR;

	dc->Map(UI_DX11_STATE.constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
	float* constants = (float*)constantbufferMSR.pData;
	constants[0] = 1.f / UI_STATE.window_size.x;
	constants[1] = 1.f / UI_STATE.window_size.y;
	dc->Unmap(UI_DX11_STATE.constant_buffer, 0);

	dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	dc->IASetInputLayout(UI_DX11_STATE.input_layout);

	dc->VSSetShader(UI_DX11_STATE.vertex_shader, NULL, 0);
	dc->VSSetConstantBuffers(0, 1, &UI_DX11_STATE.constant_buffer);

	dc->RSSetViewports(1, &viewport);
	dc->RSSetState(UI_DX11_STATE.raster_state);

	dc->PSSetShader(UI_DX11_STATE.pixel_shader, NULL, 0);
	dc->PSSetSamplers(0, 1, &UI_DX11_STATE.sampler_state);

	dc->OMSetRenderTargets(1, &framebuffer_rtv, NULL);

	dc->OMSetBlendState(UI_DX11_STATE.blend_state, NULL, 0xffffffff);

	///////////////////////////////////////////////////////////////////////////////////////////

	int bound_vertex_buffer_id = -1;
	int bound_index_buffer_id = -1;

	for (int i = 0; i < outputs->draw_calls_count; i++) {
		UI_DrawCall* draw_call = &outputs->draw_calls[i];

		if (draw_call->vertex_buffer_id != bound_vertex_buffer_id) {
			bound_vertex_buffer_id = draw_call->vertex_buffer_id;
			dc->IASetVertexBuffers(0, 1, &UI_DX11_STATE.buffers[bound_vertex_buffer_id], &stride, &offset);
		}

		if (draw_call->index_buffer_id != bound_index_buffer_id) {
			bound_index_buffer_id = draw_call->index_buffer_id;
			dc->IASetIndexBuffer(UI_DX11_STATE.buffers[bound_index_buffer_id], DXGI_FORMAT_R32_UINT, 0);
		}

		ID3D11ShaderResourceView* texture_srv = (ID3D11ShaderResourceView*)draw_call->texture;
		assert(texture_srv != NULL);
		dc->PSSetShaderResources(0, 1, &texture_srv);
		dc->DrawIndexed(draw_call->index_count, draw_call->first_index, 0);
	}

}
