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
	ID3D11Device *device;
	ID3D11DeviceContext *devicecontext;
	ID3D11VertexShader *vertexshader;
	ID3D11PixelShader *pixelshader;
	ID3D11InputLayout *inputlayout;
	ID3D11RasterizerState *rasterizerstate;
	ID3D11SamplerState *samplerstate;
	ID3D11Buffer *constantbuffer;
	ID3D11BlendState *blend_state;

	ID3D11Texture2D *atlases[2];
	ID3D11Texture2D *atlases_staging[2];
	ID3D11ShaderResourceView *atlases_srv[2];
	D3D11_MAPPED_SUBRESOURCE atlases_mapped[2];

	ID3D11Buffer *buffers[UI_MAX_BACKEND_BUFFERS];
	D3D11_MAPPED_SUBRESOURCE buffers_mapped[UI_MAX_BACKEND_BUFFERS];
} UI_DX11_State;

static UI_DX11_State UI_DX11_STATE;

static UI_TextureID UI_DX11_CreateAtlas(int atlas_id, uint32_t width, uint32_t height) {
	D3D11_TEXTURE2D_DESC staging_desc = {0};
	staging_desc.Width              = width;
	staging_desc.Height             = height;
	staging_desc.MipLevels          = 1;
	staging_desc.ArraySize          = 1;
	staging_desc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
	staging_desc.SampleDesc.Count   = 1;
	staging_desc.Usage              = D3D11_USAGE_STAGING;
	staging_desc.CPUAccessFlags     = D3D11_CPU_ACCESS_WRITE;
	
	ID3D11Texture2D *staging;
	ID3D11Device_CreateTexture2D(UI_DX11_STATE.device, &staging_desc, NULL, &staging);

	D3D11_TEXTURE2D_DESC texturedesc = {0};
	texturedesc.Width              = width;
	texturedesc.Height             = height;
	texturedesc.MipLevels          = 1;
	texturedesc.ArraySize          = 1;
	texturedesc.Format             = DXGI_FORMAT_R8G8B8A8_UNORM;
	texturedesc.SampleDesc.Count   = 1;
	texturedesc.Usage              = D3D11_USAGE_DEFAULT;
	texturedesc.BindFlags          = D3D11_BIND_SHADER_RESOURCE;
	
	ID3D11Texture2D *texture;
	ID3D11Device_CreateTexture2D(UI_DX11_STATE.device, &texturedesc, NULL, &texture);

	ID3D11ShaderResourceView* textureSRV;
	ID3D11Device_CreateShaderResourceView(UI_DX11_STATE.device, (ID3D11Resource*)texture, NULL, &textureSRV);

	UI_DX11_STATE.atlases[atlas_id] = texture;
	UI_DX11_STATE.atlases_staging[atlas_id] = staging;
	UI_DX11_STATE.atlases_srv[atlas_id] = textureSRV;

	return (UI_TextureID)textureSRV;
}

static void UI_DX11_DestroyAtlas(int atlas_id) {
	__debugbreak();
}

static void *UI_DX11_AtlasMapUntilFrameEnd(int atlas_id) {
	D3D11_MAPPED_SUBRESOURCE *mapped = &UI_DX11_STATE.atlases_mapped[atlas_id];
	if (mapped->pData == NULL) {
		ID3D11Texture2D *staging = UI_DX11_STATE.atlases_staging[atlas_id];
		ID3D11DeviceContext_Map(UI_DX11_STATE.devicecontext, (ID3D11Resource*)staging, 0, D3D11_MAP_WRITE, 0, mapped);
		assert(mapped->pData != NULL);
	}
	return mapped->pData;
}

static void *UI_DX11_BufferMapUntilFrameEnd(int buffer_id) {
	D3D11_MAPPED_SUBRESOURCE *mapped = &UI_DX11_STATE.buffers_mapped[buffer_id];
	if (mapped->pData == NULL) {
		ID3D11Buffer *buffer = UI_DX11_STATE.buffers[buffer_id];
		ID3D11DeviceContext_Map(UI_DX11_STATE.devicecontext, (ID3D11Resource*)buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, mapped);
		assert(mapped->pData != NULL);
	}

	return mapped->pData;
}

static void UI_DX11_CreateBuffer(int buffer_id, uint32_t size_in_bytes, D3D11_BIND_FLAG bind_flags) {
	D3D11_BUFFER_DESC desc = {0};
	desc.ByteWidth      = size_in_bytes;
	desc.Usage          = D3D11_USAGE_DYNAMIC;
	desc.BindFlags      = bind_flags;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	ID3D11Buffer* buffer;
	ID3D11Device_CreateBuffer(UI_DX11_STATE.device, &desc, NULL, &buffer);
	UI_DX11_STATE.buffers[buffer_id] = buffer;
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
	
	D3D11_RENDER_TARGET_BLEND_DESC blend_desc = {0};
	blend_desc.BlendEnable = 1;
	blend_desc.SrcBlend = D3D11_BLEND_SRC_ALPHA;
	blend_desc.DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blend_desc.BlendOp = D3D11_BLEND_OP_ADD;
	blend_desc.SrcBlendAlpha = D3D11_BLEND_ONE;
	blend_desc.DestBlendAlpha = D3D11_BLEND_ZERO;
	blend_desc.BlendOpAlpha = D3D11_BLEND_OP_ADD;
	blend_desc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	//blend_desc.SrcBlend                 = D3D11_BLEND_SRC_COLOR;
	//blend_desc.DestBlend                 = D3D11_BLEND_BLEND_FACTOR;
	//blend_desc.BlendOp                 = D3D11_BLEND_OP_ADD;
	//blend_desc.SrcBlendAlpha             = D3D11_BLEND_ONE;
	//blend_desc.DestBlendAlpha             = D3D11_BLEND_ZERO;
	//blend_desc.BlendOpAlpha             = D3D11_BLEND_OP_ADD;
	//blend_desc.RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;

	D3D11_BLEND_DESC blend_state_desc = {0};
	blend_state_desc.RenderTarget[0] = blend_desc;
	ID3D11Device_CreateBlendState(device, &blend_state_desc, &state.blend_state);
	
	///////////////////////////////////////////////////////////////////////////////////////////////
	
	D3D11_BUFFER_DESC constantbufferdesc = {0};
	constantbufferdesc.ByteWidth      = (sizeof(float[2]) + 0xf) & 0xfffffff0; // ensure constant buffer size is multiple of 16 bytes
	constantbufferdesc.Usage          = D3D11_USAGE_DYNAMIC; // Will be updated every frame
	constantbufferdesc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
	constantbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	ID3D11Device_CreateBuffer(device, &constantbufferdesc, NULL, &state.constantbuffer);

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

	for (int i = 0; i < 2; i++) {
		if (UI_DX11_STATE.atlases_mapped[i].pData) {
			ID3D11Texture2D *staging = UI_DX11_STATE.atlases_staging[i];
			ID3D11Texture2D *dest = UI_DX11_STATE.atlases[i];
			ID3D11DeviceContext_Unmap(dc, (ID3D11Resource*)staging, 0);
			
			ID3D11DeviceContext_CopyResource(dc, (ID3D11Resource*)dest, (ID3D11Resource*)staging);
			
			memset(&UI_DX11_STATE.atlases_mapped[i], 0, sizeof(UI_DX11_STATE.atlases_mapped[i]));
		}
	}

	for (int i = 0; i < UI_MAX_BACKEND_BUFFERS; i++) {
		if (UI_DX11_STATE.buffers_mapped[i].pData) {
			ID3D11Buffer *buffer = UI_DX11_STATE.buffers[i];
			ID3D11DeviceContext_Unmap(dc, (ID3D11Resource*)buffer, 0);
			
			memset(&UI_DX11_STATE.buffers_mapped[i], 0, sizeof(UI_DX11_STATE.buffers_mapped[i]));
		}
	}

	UINT stride = 2*sizeof(float) + 2*sizeof(float) + 4; // pos, uv, color
	UINT offset = 0;
	D3D11_VIEWPORT viewport = {0.0f, 0.0f, (float)window_size[0], (float)window_size[1], 0.0f, 1.0f};

	D3D11_MAPPED_SUBRESOURCE constantbufferMSR;

	ID3D11DeviceContext_Map(dc, (ID3D11Resource*)UI_DX11_STATE.constantbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
	float *constants = (float*)constantbufferMSR.pData;
	constants[0] = 1.f / (float)window_size[0];
	constants[1] = 1.f / (float)window_size[1];
	ID3D11DeviceContext_Unmap(dc, (ID3D11Resource*)UI_DX11_STATE.constantbuffer, 0);

	ID3D11DeviceContext_IASetPrimitiveTopology(dc, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	ID3D11DeviceContext_IASetInputLayout(dc, UI_DX11_STATE.inputlayout);

	ID3D11DeviceContext_VSSetShader(dc, UI_DX11_STATE.vertexshader, NULL, 0);
	ID3D11DeviceContext_VSSetConstantBuffers(dc, 0, 1, &UI_DX11_STATE.constantbuffer);

	ID3D11DeviceContext_RSSetViewports(dc, 1, &viewport);
	ID3D11DeviceContext_RSSetState(dc, UI_DX11_STATE.rasterizerstate);

	ID3D11DeviceContext_PSSetShader(dc, UI_DX11_STATE.pixelshader, NULL, 0);
	ID3D11DeviceContext_PSSetSamplers(dc, 0, 1, &UI_DX11_STATE.samplerstate);

	ID3D11DeviceContext_OMSetRenderTargets(dc, 1, &framebufferRTV, NULL);

	ID3D11DeviceContext_OMSetBlendState(dc, UI_DX11_STATE.blend_state, NULL, 0xffffffff);
	//ID3D11DeviceContext_OMSetBlendState(dc, NULL, NULL, 0xffffffff);

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
		
		ID3D11ShaderResourceView *textureSRV = (ID3D11ShaderResourceView*)draw_call->texture;
		ID3D11DeviceContext_PSSetShaderResources(dc, 0, 1, &textureSRV);
		ID3D11DeviceContext_DrawIndexed(dc, draw_call->index_count, draw_call->first_index, 0);
	}

}
