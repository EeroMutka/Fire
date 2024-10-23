
typedef struct UI_DX11_Buffer {
	ID3D11Buffer* handle;
	D3D11_MAPPED_SUBRESOURCE mapped;
} UI_DX11_Buffer;

typedef struct UI_DX11_State {
	ID3D11Device* device;
	ID3D11DeviceContext* dc;
	ID3D11VertexShader* vertex_shader;
	ID3D11PixelShader* pixel_shader;
	ID3D11InputLayout* input_layout;
	ID3D11RasterizerState* raster_state;
	ID3D11SamplerState* sampler_state;
	ID3D11Buffer* constant_buffer;
	ID3D11BlendState* blend_state;

	ID3D11Texture2D* atlas;
	ID3D11ShaderResourceView* atlas_srv;
	ID3D11Texture2D* atlas_staging;
	D3D11_MAPPED_SUBRESOURCE atlas_mapped;

	UI_DX11_Buffer vertex_buffer;
	UI_DX11_Buffer index_buffer;
} UI_DX11_State;

static UI_DX11_State UI_DX11_STATE;

static UI_TextureID UI_DX11_ResizeAtlas(uint32_t width, uint32_t height) {
	if (width == 0 && height == 0) {
		UI_DX11_STATE.atlas->Release();
		UI_DX11_STATE.atlas = NULL;
		UI_DX11_STATE.atlas_staging->Release();
		UI_DX11_STATE.atlas_staging = NULL;
		UI_DX11_STATE.atlas_srv->Release();
		UI_DX11_STATE.atlas_srv = NULL;
		return UI_TEXTURE_ID_NIL;
	}
	else {
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

		UI_DX11_STATE.atlas = texture;
		UI_DX11_STATE.atlas_staging = staging;
		UI_DX11_STATE.atlas_srv = texture_srv;
		return (UI_TextureID)texture_srv;
	}
}

static void* UI_DX11_MapAtlas() {
	D3D11_MAPPED_SUBRESOURCE* mapped = &UI_DX11_STATE.atlas_mapped;
	if (mapped->pData == NULL) {
		ID3D11Texture2D* staging = UI_DX11_STATE.atlas_staging;
		bool ok = UI_DX11_STATE.dc->Map(staging, 0, D3D11_MAP_WRITE, 0, mapped) == S_OK;
		UI_ASSERT(ok);
	}
	return mapped->pData;
}

static void* UI_DX11_MapVertexBuffer() {
	D3D11_MAPPED_SUBRESOURCE* mapped = &UI_DX11_STATE.vertex_buffer.mapped;
	if (mapped->pData == NULL) {
		bool ok = UI_DX11_STATE.dc->Map(UI_DX11_STATE.vertex_buffer.handle, 0, D3D11_MAP_WRITE_DISCARD, 0, mapped) == S_OK;
		UI_ASSERT(ok);
	}
	return mapped->pData;
}

static void* UI_DX11_MapIndexBuffer() {
	D3D11_MAPPED_SUBRESOURCE* mapped = &UI_DX11_STATE.index_buffer.mapped;
	if (mapped->pData == NULL) {
		bool ok = UI_DX11_STATE.dc->Map(UI_DX11_STATE.index_buffer.handle, 0, D3D11_MAP_WRITE_DISCARD, 0, mapped) == S_OK;
		UI_ASSERT(ok);
	}
	return mapped->pData;
}

static void UI_DX11_ResizeBuffer(UI_DX11_Buffer* buffer, uint32_t size, D3D11_BIND_FLAG bind_flags) {
	if (size == 0) {
		buffer->handle->Release();
	}
	else {
		D3D11_BUFFER_DESC desc = {0};
		desc.ByteWidth = size;
		desc.Usage = D3D11_USAGE_DYNAMIC;
		desc.BindFlags = bind_flags;
		desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

		bool ok = UI_DX11_STATE.device->CreateBuffer(&desc, NULL, &buffer->handle) == S_OK;
		UI_ASSERT(ok);
	}
}

static void UI_DX11_ResizeVertexBuffer(uint32_t size) {
	UI_DX11_ResizeBuffer(&UI_DX11_STATE.vertex_buffer, size, D3D11_BIND_VERTEX_BUFFER);
}

static void UI_DX11_ResizeIndexBuffer(uint32_t size) {
	UI_DX11_ResizeBuffer(&UI_DX11_STATE.index_buffer, size, D3D11_BIND_INDEX_BUFFER);
}

static void UI_DX11_Init(UI_Backend* backend, ID3D11Device* device, ID3D11DeviceContext* dc) {
	UI_DX11_State state = {0};
	state.device = device;
	state.dc = dc;

	static const char shader_src[] = "\
	cbuffer constants : register(b0) {\
		float2 pixel_size;\
	}\
	Texture2D    mytexture : register(t0);\
	SamplerState mysampler : register(s0);\
	\
	struct vertexdata {\
		float2 position : POS;\
		float2 uv       : UVPOS;\
		float4 color    : COL;\
	};\
	\
	struct pixeldata {\
		float4 position : SV_POSITION;\
		float2 uv       : UVPOS;\
		float4 color    : COL;\
	};\
	\
	pixeldata vertex_shader(vertexdata vertex) {\
		pixeldata output;\
		output.position = float4(2.*vertex.position*pixel_size - 1., 0.5, 1);\
		output.position.y *= -1.;\
		output.uv = vertex.uv;\
		output.color = vertex.color;\
		return output;\
	}\
	\
	float4 pixel_shader(pixeldata pixel) : SV_TARGET {\
		return mytexture.Sample(mysampler, pixel.uv) * pixel.color;\
	}";

	ID3DBlob* vertexshaderCSO;
	bool ok = D3DCompile(shader_src, sizeof(shader_src) - 1, "VS", NULL, NULL, "vertex_shader", "vs_5_0", 0, 0, &vertexshaderCSO, NULL) == S_OK;
	UI_ASSERT(ok);

	ok = device->CreateVertexShader(vertexshaderCSO->GetBufferPointer(), vertexshaderCSO->GetBufferSize(), NULL, &state.vertex_shader) == S_OK;
	UI_ASSERT(ok);

	D3D11_INPUT_ELEMENT_DESC inputelementdesc[] = {
		{  "POS", 0, DXGI_FORMAT_R32G32_FLOAT,   0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{"UVPOS", 0, DXGI_FORMAT_R32G32_FLOAT,   0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
		{  "COL", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0},
	};

	ok = device->CreateInputLayout(inputelementdesc, ARRAYSIZE(inputelementdesc), vertexshaderCSO->GetBufferPointer(), vertexshaderCSO->GetBufferSize(), &state.input_layout) == S_OK;
	UI_ASSERT(ok);

	///////////////////////////////////////////////////////////////////////////////////////////////

	ID3DBlob* pixelshaderCSO;
	ok = D3DCompile(shader_src, sizeof(shader_src) - 1, "PS", NULL, NULL, "pixel_shader", "ps_5_0", 0, 0, &pixelshaderCSO, NULL) == S_OK;
	UI_ASSERT(ok);

	ok = device->CreatePixelShader(pixelshaderCSO->GetBufferPointer(), pixelshaderCSO->GetBufferSize(), NULL, &state.pixel_shader) == S_OK;
	UI_ASSERT(ok);

	///////////////////////////////////////////////////////////////////////////////////////////////

	D3D11_SAMPLER_DESC samplerdesc = {0};
	samplerdesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerdesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerdesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerdesc.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerdesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	ok = device->CreateSamplerState(&samplerdesc, &state.sampler_state) == S_OK;
	UI_ASSERT(ok);

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
	ok = device->CreateBlendState(&blend_state_desc, &state.blend_state) == S_OK;
	UI_ASSERT(ok);

	///////////////////////////////////////////////////////////////////////////////////////////////

	D3D11_BUFFER_DESC buffer_desc = {0};
	buffer_desc.ByteWidth = (sizeof(float[2]) + 0xf) & 0xfffffff0; // ensure constant buffer size is multiple of 16 bytes
	buffer_desc.Usage = D3D11_USAGE_DYNAMIC; // Will be updated every frame
	buffer_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	buffer_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	ok = device->CreateBuffer(&buffer_desc, NULL, &state.constant_buffer) == S_OK;
	UI_ASSERT(ok);

	///////////////////////////////////////////////////////////////////////////////////////////////

	D3D11_RASTERIZER_DESC raster_desc = {0};
	raster_desc.FillMode = D3D11_FILL_SOLID;
	raster_desc.CullMode = D3D11_CULL_NONE;
	raster_desc.MultisampleEnable = true; // MSAA
	//raster_desc.CullMode = D3D11_CULL_BACK;
	ok = device->CreateRasterizerState(&raster_desc, &state.raster_state) == S_OK;
	UI_ASSERT(ok);

	backend->resize_atlas = UI_DX11_ResizeAtlas;
	backend->resize_vertex_buffer = UI_DX11_ResizeVertexBuffer;
	backend->resize_index_buffer = UI_DX11_ResizeIndexBuffer;

	backend->map_vertex_buffer = UI_DX11_MapVertexBuffer;
	backend->map_index_buffer = UI_DX11_MapIndexBuffer;
	backend->map_atlas = UI_DX11_MapAtlas;

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

static void UI_DX11_Draw(UI_Outputs* outputs, ID3D11RenderTargetView* framebuffer_rtv) {
	UI_DX11_State* s = &UI_DX11_STATE;

	if (s->atlas_mapped.pData) {
		ID3D11Texture2D* staging = s->atlas_staging;
		ID3D11Texture2D* dest = s->atlas;
		s->dc->Unmap(staging, 0);

		s->dc->CopyResource(dest, staging);

		memset(&s->atlas_mapped, 0, sizeof(s->atlas_mapped));
	}

	UI_DX11_Buffer* buffers[] = { &s->vertex_buffer, &s->index_buffer };
	for (int i = 0; i < 2; i++) {
		if (buffers[i]->mapped.pData != NULL) {
			s->dc->Unmap(buffers[i]->handle, 0);
			buffers[i]->mapped.pData = NULL;
		}
	}
	
	UINT stride = 2 * sizeof(float) + 2 * sizeof(float) + 4; // pos, uv, color
	UINT offset = 0;
	D3D11_VIEWPORT viewport = { 0.0f, 0.0f, UI_STATE.window_size.x, UI_STATE.window_size.y, 0.0f, 1.0f };

	D3D11_MAPPED_SUBRESOURCE constantbufferMSR;

	s->dc->Map(s->constant_buffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR);
	float* constants = (float*)constantbufferMSR.pData;
	constants[0] = 1.f / UI_STATE.window_size.x;
	constants[1] = 1.f / UI_STATE.window_size.y;
	s->dc->Unmap(s->constant_buffer, 0);

	s->dc->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	s->dc->IASetInputLayout(s->input_layout);

	s->dc->VSSetShader(s->vertex_shader, NULL, 0);
	s->dc->VSSetConstantBuffers(0, 1, &s->constant_buffer);

	s->dc->RSSetViewports(1, &viewport);
	s->dc->RSSetState(s->raster_state);

	s->dc->PSSetShader(s->pixel_shader, NULL, 0);
	s->dc->PSSetSamplers(0, 1, &s->sampler_state);

	s->dc->OMSetRenderTargets(1, &framebuffer_rtv, NULL);

	s->dc->OMSetBlendState(s->blend_state, NULL, 0xffffffff);

	///////////////////////////////////////////////////////////////////////////////////////////

	s->dc->IASetVertexBuffers(0, 1, &s->vertex_buffer.handle, &stride, &offset);
	s->dc->IASetIndexBuffer(s->index_buffer.handle, DXGI_FORMAT_R32_UINT, 0);

	for (int i = 0; i < outputs->draw_calls_count; i++) {
		UI_DrawCall* draw_call = &outputs->draw_calls[i];

		ID3D11ShaderResourceView* texture_srv = (ID3D11ShaderResourceView*)draw_call->texture;
		UI_ASSERT(texture_srv != NULL);
		s->dc->PSSetShaderResources(0, 1, &texture_srv);
		s->dc->DrawIndexed(draw_call->index_count, draw_call->first_index, 0);
	}

}
