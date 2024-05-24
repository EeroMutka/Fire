#pragma comment (lib, "gdi32")
#pragma comment (lib, "user32")
#pragma comment (lib, "dxguid")
#pragma comment (lib, "dxgi")
#pragma comment (lib, "d3d11")
#pragma comment (lib, "d3dcompiler")

#define COBJMACROS
#include <windows.h>
#include <d3d11.h>
#include <dxgi1_3.h>
#include <d3dcompiler.h>

#include "fire_ds.h"

#define STR_USE_FIRE_DS_ARENA
#include "fire_string.h"

#define OS_USE_FIRE_DS_ARENA
#define OS_STRING_OVERRIDE
#define OS_String STR
#include "fire_os.h"

#include "fire_gpu/fire_gpu.h"

#define STB_RECT_PACK_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#include "fire_ui/stb_rect_pack.h"
#include "fire_ui/stb_truetype.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "fire_ui/fire_ui.h"
#include "fire_ui/fire_ui_color_pickers.h"
#include "fire_ui/fire_ui_backend_dx11.h"
#include "fire_ui/fire_ui_backend_fire_os.h"

#include "../shared/ui_demo.h"

//// Globals ///////////////////////////////////////////////

static UI_Vec2 g_window_size = {1200, 900};

static IDXGISwapChain* g_swapchain;
static ID3D11RenderTargetView* g_framebuffer_rtv;

static UI_Inputs g_ui_inputs;

static UIDemoState g_demo_state;

////////////////////////////////////////////////////////////

static void UpdateAndRender() {
	UI_DX11_BeginFrame();
	UI_BeginFrame(&g_ui_inputs, g_window_size);

	UIDemoBuild(&g_demo_state, g_window_size);

	UI_Outputs ui_outputs;
	UI_EndFrame(&ui_outputs);
	
	FLOAT clearcolor[4] = { 0.15f, 0.15f, 0.15f, 1.f };
	UI_DX11_STATE.device_context->ClearRenderTargetView(g_framebuffer_rtv, clearcolor);

	UI_DX11_EndFrame(&ui_outputs, g_framebuffer_rtv);
	UI_OS_ApplyOutputs(&ui_outputs);
	
	g_swapchain->Present(1, 0);
}

static void OnResizeWindow(uint32_t width, uint32_t height, void *user_ptr) {
	g_window_size.x = (float)width;
	g_window_size.y = (float)height;

	// Recreate swapchain

	g_framebuffer_rtv->Release();

	g_swapchain->ResizeBuffers(0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
	
	ID3D11Texture2D* framebuffer;
	g_swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), (void**)&framebuffer); // grab framebuffer from swapchain

	D3D11_RENDER_TARGET_VIEW_DESC framebuffer_rtv_desc = {0};
	framebuffer_rtv_desc.Format        = DXGI_FORMAT_B8G8R8A8_UNORM;
	framebuffer_rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	UI_DX11_STATE.device->CreateRenderTargetView(framebuffer, &framebuffer_rtv_desc, &g_framebuffer_rtv);
	
	framebuffer->Release(); // We don't need this handle anymore

	UpdateAndRender();
}

int main() {
	OS_Init();

	DS_Arena persist, temp;
	DS_ArenaInit(&persist, 4096, DS_HEAP);
	DS_ArenaInit(&temp, 4096, DS_HEAP);

	UIDemoInit(&g_demo_state, &persist);

	OS_Inputs window_inputs = {0};
	OS_Window window = OS_WindowCreate((uint32_t)g_window_size.x, (uint32_t)g_window_size.y, OS_STR("UI demo (DX11)"));

	D3D_FEATURE_LEVEL dx_feature_levels[] = { D3D_FEATURE_LEVEL_11_0 };
	
	DXGI_SWAP_CHAIN_DESC swapchain_desc = {0};
	swapchain_desc.BufferDesc.Width  = 0; // use window width
	swapchain_desc.BufferDesc.Height = 0; // use window height
	swapchain_desc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapchain_desc.SampleDesc.Count  = 1;
	swapchain_desc.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchain_desc.BufferCount       = 2;
	swapchain_desc.OutputWindow      = (HWND)window.handle;
	swapchain_desc.Windowed          = TRUE;
	swapchain_desc.SwapEffect        = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	
	ID3D11Device* device;
	ID3D11DeviceContext* device_context;
	D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG, dx_feature_levels, ARRAYSIZE(dx_feature_levels), D3D11_SDK_VERSION, &swapchain_desc, &g_swapchain, &device, NULL, &device_context);

	g_swapchain->GetDesc(&swapchain_desc); // Update swapchain_desc with actual window size
	
	///////////////////////////////////////////////////////////////////////////////////////////////

	ID3D11Texture2D* framebuffer;
	g_swapchain->GetBuffer(0, _uuidof(ID3D11Texture2D), (void**)&framebuffer); // grab framebuffer from swapchain

	D3D11_RENDER_TARGET_VIEW_DESC framebuffer_rtv_desc = {0};
	framebuffer_rtv_desc.Format        = DXGI_FORMAT_B8G8R8A8_UNORM;
	framebuffer_rtv_desc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	device->CreateRenderTargetView(framebuffer, &framebuffer_rtv_desc, &g_framebuffer_rtv);
	
	framebuffer->Release(); // We don't need this handle anymore

	///////////////////////////////////////////////////////////////////////////////////////////////
	
	UI_Backend ui_backend = {0};
	UI_DX11_Init(&ui_backend, device, device_context);
	UI_Init(&persist, &ui_backend);

	// NOTE: the font data must remain alive across the whole program lifetime!
	STR roboto_mono_ttf, icons_ttf;
	assert(OS_ReadEntireFile(&persist, STR_("../../fire_ui/resources/roboto_mono.ttf"), &roboto_mono_ttf));
	assert(OS_ReadEntireFile(&persist, STR_("../../fire_ui/resources/fontello/font/fontello.ttf"), &icons_ttf));

	UI_Font base_font, icons_font;
	UI_FontInit(&base_font, roboto_mono_ttf.data, -4.f);
	UI_FontInit(&icons_font, icons_ttf.data, -2.f);

	for (;;) {
		DS_ArenaReset(&temp);
		if (!OS_WindowPoll(&temp, &window_inputs, &window, OnResizeWindow, NULL)) break;
		
		g_ui_inputs = UI_Inputs{};
		g_ui_inputs.base_font = &base_font;
		g_ui_inputs.icons_font = &icons_font;
		UI_OS_TranslateInputs(&g_ui_inputs, &window_inputs, &temp);
		
		UpdateAndRender();
	}

	UI_FontDeinit(&base_font);
	UI_FontDeinit(&icons_font);

	UI_Deinit();
	UI_DX11_Deinit();

	g_framebuffer_rtv->Release();
	g_swapchain->Release();
	device->Release();
	device_context->Release();

	DS_ArenaDeinit(&persist);
	DS_ArenaDeinit(&temp);

	OS_Deinit();
}
