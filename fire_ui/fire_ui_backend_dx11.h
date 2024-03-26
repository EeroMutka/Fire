// This file is part of the Fire UI library, see "fire_ui.h"

static const char UI_DX11_SHADER_SRC[] = ""
	"struct vertexdata {\n"
	"	float2 position : POS;\n"
	"	float2 uv       : UVPOS;\n"
	"	float4 color    : COL;\n"
	"};\n"
	"\n"
	"struct pixeldata {\n"
	"	float4 position : SV_POSITION;\n"
	"};\n"
	"\n"
	"pixeldata vertex_shader(vertexdata vertex) {\n"
	"	pixeldata output;\n"
	"	output.position = float4(vertex.position, 0, 1);\n"
	"	return output;\n"
	"}\n"
	"\n"
	"float4 pixel_shader(pixeldata input) : SV_TARGET { \n"
	"	return float4(1, 0, 0, 1);\n"
	"}\n";

typedef struct UI_DX11_State {
	ID3D11Device *device;
	ID3D11DeviceContext *devicecontext;
	ID3D11VertexShader *vertexshader;
	ID3D11PixelShader *pixelshader;
	ID3D11InputLayout *inputlayout;
	ID3D11RasterizerState* rasterizerstate;
	ID3D11SamplerState* samplerstate;
	
	//bool atlas_is_mapped[2];
	ID3D11Texture2D *atlases[2];
	D3D11_MAPPED_SUBRESOURCE atlases_mapped[2];

	ID3D11Buffer *buffers[UI_MAX_BACKEND_BUFFERS];
	D3D11_MAPPED_SUBRESOURCE buffers_mapped[UI_MAX_BACKEND_BUFFERS];

	// vertexbuffer;
	//ID3D11Buffer* indexbuffer;

	// per-frame state
} UI_DX11_State;

static UI_DX11_State UI_DX11_STATE;

static UI_TextureID UI_DX11_CreateAtlas(int atlas_id, uint32_t width, uint32_t height) {
	
	D3D11_TEXTURE2D_DESC texturedesc = {0};
	texturedesc.Width              = width;
	texturedesc.Height             = height;
	texturedesc.MipLevels          = 1;
	texturedesc.ArraySize          = 1;
	texturedesc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
	texturedesc.SampleDesc.Count   = 1;
	texturedesc.Usage              = D3D11_USAGE_DYNAMIC; // TODO: use a staging texture instead of dynamic
	texturedesc.CPUAccessFlags     = D3D11_CPU_ACCESS_WRITE;
	texturedesc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;
	
	ID3D11Texture2D* texture;
	ID3D11Device_CreateTexture2D(UI_DX11_STATE.device, &texturedesc, NULL, &texture);
	UI_DX11_STATE.atlases[atlas_id] = texture;

	D3D11_MAPPED_SUBRESOURCE mapped;
	HRESULT result = ID3D11DeviceContext_Map(UI_DX11_STATE.devicecontext, (ID3D11Resource*)texture, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	UI_DX11_STATE.atlases_mapped[atlas_id] = mapped;

	// ID3D11ShaderResourceView* textureSRV;
	// device->CreateShaderResourceView(texture, nullptr, &textureSRV);

	//GPU_Texture *tex = GPU_MakeTexture(GPU_Format_RGBA8UN, width, height, 1, 0, NULL);
	//UI_DX11_STATE.atlas_staging_buffers[atlas_id] = GPU_MakeBuffer(width * height * 4, GPU_BufferFlag_CPU, NULL);
	//UI_DX11_STATE.atlases[atlas_id] = tex;
	//return (UI_TextureID)tex;
	//__debugbreak();
	return (UI_TextureID)texture;
}

static void UI_DX11_DestroyAtlas(int atlas_id) {
	__debugbreak();
}

static void *UI_DX11_AtlasMapUntilFrameEnd(int atlas_id) {
	return UI_DX11_STATE.atlases_mapped[atlas_id].pData;
}

static void *UI_DX11_BufferMapUntilFrameEnd(int buffer_id) {
	return UI_DX11_STATE.buffers_mapped[buffer_id].pData;
}

static void UI_DX11_CreateBuffer(int buffer_id, uint32_t size_in_bytes, D3D11_BIND_FLAG bind_flags) {
	D3D11_BUFFER_DESC desc = {0};
	desc.ByteWidth      = size_in_bytes;
	desc.Usage          = D3D11_USAGE_DYNAMIC; // TODO: use a staging buffer instead of dynamic
	desc.BindFlags      = bind_flags;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	ID3D11Buffer* buffer;
	ID3D11Device_CreateBuffer(UI_DX11_STATE.device, &desc, NULL, &buffer);
	UI_DX11_STATE.buffers[buffer_id] = buffer;
	
	D3D11_MAPPED_SUBRESOURCE mapped;
	ID3D11DeviceContext_Map(UI_DX11_STATE.devicecontext, (ID3D11Resource*)buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
	assert(mapped.pData != NULL);
	UI_DX11_STATE.buffers_mapped[buffer_id] = mapped;
}

static void UI_DX11_CreateVertexBuffer(int buffer_id, uint32_t size_in_bytes) {
	UI_DX11_CreateBuffer(buffer_id, size_in_bytes, D3D11_BIND_VERTEX_BUFFER);
}

static void UI_DX11_CreateIndexBuffer(int buffer_id, uint32_t size_in_bytes) {
	UI_DX11_CreateBuffer(buffer_id, size_in_bytes, D3D11_BIND_INDEX_BUFFER);
}

static void UI_DX11_DestroyBuffer(int buffer_id) {
	__debugbreak();
}

static void UI_DX11_Init(UI_Backend *backend, ID3D11Device *device, ID3D11DeviceContext *devicecontext, GPU_String shader_src) {
	UI_DX11_State state = {0};
	state.device = device;
	state.devicecontext = devicecontext;
	
	ID3DBlob* vertexshaderCSO;
	D3DCompile(UI_DX11_SHADER_SRC, sizeof(UI_DX11_SHADER_SRC), "VS", NULL, NULL, "vertex_shader", "vs_5_0", 0, 0, &vertexshaderCSO, NULL);

	ID3D11Device_CreateVertexShader(device, ID3D10Blob_GetBufferPointer(vertexshaderCSO), ID3D10Blob_GetBufferSize(vertexshaderCSO), NULL, &state.vertexshader);

	D3D11_INPUT_ELEMENT_DESC inputelementdesc[] = {
		{  "POS", 0, DXGI_FORMAT_R32G32_FLOAT,   0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"UVPOS", 0, DXGI_FORMAT_R32G32_FLOAT,   0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{  "COL", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	ID3D11Device_CreateInputLayout(device, inputelementdesc, ARRAYSIZE(inputelementdesc), ID3D10Blob_GetBufferPointer(vertexshaderCSO), ID3D10Blob_GetBufferSize(vertexshaderCSO), &state.inputlayout);

	///////////////////////////////////////////////////////////////////////////////////////////////

	ID3DBlob* pixelshaderCSO;
	D3DCompile(UI_DX11_SHADER_SRC, sizeof(UI_DX11_SHADER_SRC), "PS", NULL, NULL, "pixel_shader", "ps_5_0", 0, 0, &pixelshaderCSO, NULL);

	ID3D11Device_CreatePixelShader(device, ID3D10Blob_GetBufferPointer(pixelshaderCSO), ID3D10Blob_GetBufferSize(pixelshaderCSO), NULL, &state.pixelshader);

	///////////////////////////////////////////////////////////////////////////////////////////////
	
	D3D11_SAMPLER_DESC samplerdesc = {0};
	samplerdesc.Filter         = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerdesc.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerdesc.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerdesc.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerdesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	ID3D11Device_CreateSamplerState(device, &samplerdesc, &state.samplerstate);

	///////////////////////////////////////////////////////////////////////////////////////////////
	
	D3D11_RASTERIZER_DESC rasterizerdesc = {0};
	rasterizerdesc.FillMode = D3D11_FILL_SOLID;
	rasterizerdesc.CullMode = D3D11_CULL_BACK;
	ID3D11Device_CreateRasterizerState(device, &rasterizerdesc, &state.rasterizerstate);

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
	__debugbreak();
}

static void UI_DX11_BeginFrame(void) {
	//UI_DX11_STATE.atlas_is_mapped[0] = false;
	//UI_DX11_STATE.atlas_is_mapped[1] = false;
}

static void UI_DX11_EndFrame(UI_Outputs *outputs, ID3D11RenderTargetView* framebufferRTV, uint32_t window_size[2]) {
	ID3D11DeviceContext *dc = UI_DX11_STATE.devicecontext;

	//for (int i = 0; i < 2; i++) {
	//	if (UI_DX11_STATE.atlas_is_mapped[i]) {
	//		// unmap?
	//		__debugbreak();
	//
	//		// GPU_OpCopyBufferToTexture(graph, UI_GPU_STATE.atlas_staging_buffers[i], UI_GPU_STATE.atlases[i], 0, 1, 0);
	//	}
	//}

	UINT stride = 2*sizeof(float) + 2*sizeof(float) + 4; // pos, uv, color
	UINT offset = 0;
	D3D11_VIEWPORT viewport = {0.0f, 0.0f, (float)window_size[0], (float)window_size[1], 0.0f, 1.0f};

	ID3D11DeviceContext_IASetPrimitiveTopology(dc, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	ID3D11DeviceContext_IASetInputLayout(dc, UI_DX11_STATE.inputlayout);

	ID3D11DeviceContext_VSSetShader(dc, UI_DX11_STATE.vertexshader, NULL, 0);
	//ID3D11DeviceContext_VSSetConstantBuffers(dc, 0, 1, &constantbuffer);

	ID3D11DeviceContext_RSSetViewports(dc, 1, &viewport);
	ID3D11DeviceContext_RSSetState(dc, UI_DX11_STATE.rasterizerstate);

	ID3D11DeviceContext_PSSetShader(dc, UI_DX11_STATE.pixelshader, NULL, 0);
	// ID3D11DeviceContext_PSSetShaderResources(devicecontext, 0, 1, &textureSRV);
	ID3D11DeviceContext_PSSetSamplers(dc, 0, 1, &UI_DX11_STATE.samplerstate);

	ID3D11DeviceContext_OMSetRenderTargets(dc, 1, &framebufferRTV, NULL);
	ID3D11DeviceContext_OMSetBlendState(dc, NULL, NULL, 0xffffffff); // Use default blend mode (i.e. no blending)

	///////////////////////////////////////////////////////////////////////////////////////////

	int bound_vertex_buffer_id = -1;
	int bound_index_buffer_id = -1;

	for (int i = 0; i < outputs->draw_calls_count; i++) {
		UI_DrawCall *draw_call = &outputs->draw_calls[i];

		if (draw_call->vertex_buffer_id != bound_vertex_buffer_id) {
			bound_vertex_buffer_id = draw_call->vertex_buffer_id;
			ID3D11DeviceContext_IASetVertexBuffers(dc, 0, 1, &UI_DX11_STATE.buffers[bound_vertex_buffer_id], &stride, &offset);
		}

		if (draw_call->index_buffer_id != bound_index_buffer_id) {
			bound_index_buffer_id = draw_call->index_buffer_id;
			ID3D11DeviceContext_IASetIndexBuffer(dc, UI_DX11_STATE.buffers[bound_index_buffer_id], DXGI_FORMAT_R32_UINT, 0);
		}
		
		ID3D11DeviceContext_DrawIndexed(dc, draw_call->index_count, draw_call->first_index, 0);
	}

}
