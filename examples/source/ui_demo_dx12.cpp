#ifdef UI_DEMO_DX12

// For debug mode, uncomment this line:
//#define UI_DX12_DEBUG_MODE

#define _CRT_SECURE_NO_WARNINGS
#pragma comment (lib, "d3d12")
#pragma comment (lib, "dxgi")
#pragma comment(lib, "d3dcompiler")

#include <d3d12.h>
#include <dxgi1_4.h>
#include <d3dcompiler.h>
#ifdef UI_DX12_DEBUG_MODE
#include <dxgidebug.h>
#endif

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#include "fire_ds.h"

#define STR_USE_FIRE_DS_ARENA
#include "fire_string.h"

#define FIRE_OS_WINDOW_IMPLEMENTATION
#include "fire_os_window.h"

#define FIRE_OS_CLIPBOARD_IMPLEMENTATION
#include "fire_os_clipboard.h"

#define UI_API static
#define UI_EXTERN extern
#include "fire_ui/fire_ui.h"
#include "fire_ui/fire_ui_backend_stb_truetype.h"
#include "fire_ui/fire_ui_backend_dx12.h"
#include "fire_ui/fire_ui_backend_fire_os.h"

// Build everything in this translation unit
#include "fire_ui/fire_ui.c"
#include "fire_ui/fire_ui_color_pickers.c"
#include "fire_ui/fire_ui_extras.c"

#include "ui_demo_window.h"

#define BACK_BUFFER_COUNT 2

// -------------------------------------------------------------

static uint32_t g_window_size[] = {512, 512};
static OS_Window g_window;
static UI_Inputs g_ui_inputs;
static UIDemoState g_demo_state;

static DS_BasicMemConfig g_mem;
static DS_Arena g_persist_arena;
static UI_Font g_base_font;
static UI_Font g_icons_font;

static ID3D12Device* g_dx_device;
static ID3D12CommandQueue* g_dx_command_queue;
static IDXGISwapChain3* g_dx_swapchain;
static ID3D12DescriptorHeap* g_dx_rtv_heap;
static ID3D12DescriptorHeap* g_dx_srv_heap;
static uint32_t g_dx_rtv_descriptor_size;
static ID3D12Resource* g_dx_back_buffers[BACK_BUFFER_COUNT];
static ID3D12CommandAllocator* g_dx_command_allocator;

static ID3D12GraphicsCommandList* g_dx_command_list;
static ID3D12Fence* g_dx_fence;
static HANDLE g_dx_fence_event;

static uint32_t g_dx_frame_index; // cycles through the frames
//static uint64_t g_dx_render_finished_fence_value[2];
static uint64_t g_dx_render_finished_fence_value;

// -------------------------------------------------------------

static STR_View ReadEntireFile(DS_Arena* arena, const char* file) {
    FILE* f = fopen(file, "rb");
    assert(f);

    fseek(f, 0, SEEK_END);
    size_t fsize = (size_t)ftell(f);
    fseek(f, 0, SEEK_SET);

    char* data = DS_ArenaPush(arena, fsize);
    fread(data, fsize, 1, f);

    fclose(f);
    STR_View result = {data, fsize};
    return result;
}

static void WaitForGPU() {
    // Wait for the GPU to finish rendering this frame.
    // This means that the CPU will have a bunch of idle time, and the GPU will have a bunch of idle time.
    // For computationally heavy applications, this is not good. But it's the simplest to implement.
    // There are some questions like how the dynamic texture atlas should be built if there are two
    // frames-in-flight - should there be two atlases, one per frame? Or should the CPU wait on frames where it modifies it?
    // I guess when only adding things to the atlas, no frame sync is even needed. This needs some thought.

    g_dx_render_finished_fence_value++;

    uint64_t render_finished_fence_value = g_dx_render_finished_fence_value;
    g_dx_command_queue->Signal(g_dx_fence, render_finished_fence_value);

    if (g_dx_fence->GetCompletedValue() < g_dx_render_finished_fence_value)
    {
        bool ok = g_dx_fence->SetEventOnCompletion(g_dx_render_finished_fence_value, g_dx_fence_event) == S_OK;
        assert(ok);
        WaitForSingleObjectEx(g_dx_fence_event, INFINITE, FALSE);
    }

    g_dx_frame_index = g_dx_swapchain->GetCurrentBackBufferIndex();
}

static void MoveToNextFrame() {
    // About frame pacing:
    // https://raphlinus.github.io/ui/graphics/gpu/2021/10/22/swapchain-frame-pacing.html
    
#if 1
    WaitForGPU();
#else
    // Once the GPU is done presenting this newly built frame, it will signal the fence to the "render finished" fence value
    uint64_t render_finished_fence_value = g_dx_render_finished_fence_value[g_dx_frame_index];
    g_dx_command_queue->Signal(g_dx_fence, render_finished_fence_value);
    
    // Increment the frame index (wraps around to 0) to the next frame index.
    g_dx_frame_index = g_dx_swapchain->GetCurrentBackBufferIndex();

    // if the backbuffer that the next frame wants to use is not ready to be rendered yet, wait until it is ready.
    if (g_dx_fence->GetCompletedValue() < g_dx_render_finished_fence_value[g_dx_frame_index])
    {
        bool ok = g_dx_fence->SetEventOnCompletion(g_dx_render_finished_fence_value[g_dx_frame_index], g_dx_fence_event) == S_OK;
        assert(ok);
        WaitForSingleObjectEx(g_dx_fence_event, INFINITE, FALSE);
    }
    g_dx_render_finished_fence_value[g_dx_frame_index] = render_finished_fence_value + 1;
#endif
}

static void FreeRenderTargets() {
    for (int i = 0; i < BACK_BUFFER_COUNT; i++) {
        g_dx_back_buffers[i]->Release();
        g_dx_back_buffers[i] = NULL;
    }
}

static void CreateRenderTargets() {
    D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = g_dx_rtv_heap->GetCPUDescriptorHandleForHeapStart();

    // Create a RTV for each frame.
    for (int i = 0; i < BACK_BUFFER_COUNT; i++) {
        ID3D12Resource* back_buffer;
        bool ok = g_dx_swapchain->GetBuffer(i, IID_PPV_ARGS(&back_buffer)) == S_OK;
        assert(ok);

        g_dx_device->CreateRenderTargetView(back_buffer, NULL, rtv_handle);
        g_dx_back_buffers[i] = back_buffer;
        rtv_handle.ptr += g_dx_rtv_descriptor_size;
    }
}

static void InitDX12() {
#ifdef UI_DX12_DEBUG_MODE
    ID3D12Debug* debug_interface = NULL;
    if (D3D12GetDebugInterface(IID_PPV_ARGS(&debug_interface)) == S_OK) {
        debug_interface->EnableDebugLayer();
    }
#endif

    bool ok = D3D12CreateDevice(NULL, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&g_dx_device)) == S_OK;
    assert(ok);

#ifdef UI_DX12_DEBUG_MODE
    if (debug_interface) {
        ID3D12InfoQueue* info_queue = NULL;
        g_dx_device->QueryInterface(IID_PPV_ARGS(&info_queue));
        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, true);
        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, true);
        info_queue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, true);
        info_queue->Release();
        debug_interface->Release();
    }
#endif

	// Create queue
	{
		D3D12_COMMAND_QUEUE_DESC queue_desc = {};
		queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
		ok = g_dx_device->CreateCommandQueue(&queue_desc, IID_PPV_ARGS(&g_dx_command_queue)) == S_OK;
		assert(ok);
	}


	// Create descriptor heaps
	{
		// Describe and create a render target view (RTV) descriptor heap
		D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc = {};
		rtv_heap_desc.NumDescriptors = BACK_BUFFER_COUNT;
		rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
		ok = g_dx_device->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(&g_dx_rtv_heap)) == S_OK;
		assert(ok);

        D3D12_DESCRIPTOR_HEAP_DESC srv_heap_desc = {};
        srv_heap_desc.NumDescriptors = 1;
        srv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        srv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        ok = g_dx_device->CreateDescriptorHeap(&srv_heap_desc, IID_PPV_ARGS(&g_dx_srv_heap)) == S_OK;
        assert(ok);

		g_dx_rtv_descriptor_size = g_dx_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	}

	// Create command allocator
	ok = g_dx_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_dx_command_allocator)) == S_OK;
	assert(ok);

    // Create the command list.
    ok = g_dx_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_dx_command_allocator, NULL, IID_PPV_ARGS(&g_dx_command_list)) == S_OK;
    assert(ok);

    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    ok = g_dx_command_list->Close() == S_OK;
    assert(ok);

    // Create synchronization objects
    {
        ok = g_dx_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_dx_fence)) == S_OK;
        assert(ok);
        //g_dx_render_finished_fence_value[0] = 1;
        //g_dx_render_finished_fence_value[1] = 1;

        // Create an event handle to use for frame synchronization.
        g_dx_fence_event = CreateEventW(NULL, FALSE, FALSE, NULL);
        assert(g_dx_fence_event != NULL);
    }
    
    // Create swapchain
    {
        DXGI_SWAP_CHAIN_DESC1 swapchain_desc = {};
        swapchain_desc.BufferCount = BACK_BUFFER_COUNT;
        swapchain_desc.Width = g_window_size[0];
        swapchain_desc.Height = g_window_size[1];
        swapchain_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        swapchain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        swapchain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        swapchain_desc.SampleDesc.Count = 1;

        IDXGIFactory4* dxgi_factory = NULL;
        ok = CreateDXGIFactory1(IID_PPV_ARGS(&dxgi_factory)) == S_OK;
        assert(ok);

        IDXGISwapChain1* swapchain1 = NULL;
        ok = dxgi_factory->CreateSwapChainForHwnd(g_dx_command_queue, (HWND)g_window.handle, &swapchain_desc, NULL, NULL, &swapchain1) == S_OK;
        assert(ok);

        ok = swapchain1->QueryInterface(IID_PPV_ARGS(&g_dx_swapchain)) == S_OK;
        assert(ok);

        swapchain1->Release();
        dxgi_factory->Release();
    }

    CreateRenderTargets();

	g_dx_frame_index = g_dx_swapchain->GetCurrentBackBufferIndex();
}

static D3D12_RESOURCE_BARRIER Transition(ID3D12Resource* resource, D3D12_RESOURCE_STATES old_state, D3D12_RESOURCE_STATES new_state) {
    D3D12_RESOURCE_BARRIER barrier = { D3D12_RESOURCE_BARRIER_TYPE_TRANSITION };
    barrier.Transition.pResource = resource;
    barrier.Transition.StateBefore = old_state;
    barrier.Transition.StateAfter = new_state;
    barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    return barrier;
}

static void AppInit() {
    DS_InitBasicMemConfig(&g_mem);
    DS_ArenaInit(&g_persist_arena, 4096, g_mem.heap);

    UIDemoInit(&g_demo_state, &g_persist_arena);

    g_window = OS_CreateWindow(g_window_size[0], g_window_size[1], "UI demo (DX12)");
	
	InitDX12();
    
    D3D12_CPU_DESCRIPTOR_HANDLE atlas_cpu_descriptor = g_dx_srv_heap->GetCPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE atlas_gpu_descriptor = g_dx_srv_heap->GetGPUDescriptorHandleForHeapStart();

    UI_Init(g_mem.heap);
    UI_DX12_Init(g_dx_device, atlas_cpu_descriptor, atlas_gpu_descriptor);
    UI_STBTT_Init(UI_DX12_CreateAtlas, UI_DX12_MapAtlas);

    // NOTE: the font data must remain alive across the whole program lifetime!
    STR_View roboto_mono_ttf = ReadEntireFile(&g_persist_arena, "../../fire_ui/resources/roboto_mono.ttf");
    STR_View icons_ttf = ReadEntireFile(&g_persist_arena, "../../fire_ui/resources/fontello/font/fontello.ttf");

    g_base_font = { UI_STBTT_FontInit(roboto_mono_ttf.data, -4.f), 18 };
    g_icons_font = { UI_STBTT_FontInit(icons_ttf.data, -2.f), 18 };
}

static void AppDeinit() {
    UI_STBTT_FontDeinit(g_base_font.id);
    UI_STBTT_FontDeinit(g_icons_font.id);

    UI_STBTT_Deinit();
    UI_DX12_Deinit();
    UI_Deinit();

    // Release DX12 resources
    CloseHandle(g_dx_fence_event);
    g_dx_fence->Release();
    g_dx_command_list->Release();
    g_dx_command_allocator->Release();
    FreeRenderTargets();
    g_dx_srv_heap->Release();
    g_dx_rtv_heap->Release();
    g_dx_swapchain->Release();
    g_dx_command_queue->Release();
    g_dx_device->Release();

    DS_ArenaDeinit(&g_persist_arena);
    DS_DeinitBasicMemConfig(&g_mem);
}

static void UpdateAndRender() {

    UI_Vec2 window_size = {(float)g_window_size[0], (float)g_window_size[1]};
    UI_BeginFrame(&g_ui_inputs, g_base_font, g_icons_font);

    UIDemoBuild(&g_demo_state, window_size);

    UI_Outputs ui_outputs;
    UI_EndFrame(&ui_outputs);

    UI_OS_ApplyOutputs(&g_window, &ui_outputs);

    // Populate the command buffer
    {
        // Command list allocators can only be reset when the associated
        // command lists have finished execution on the GPU; apps should use
        // fences to determine GPU execution progress.
        bool ok = g_dx_command_allocator->Reset() == S_OK;
        assert(ok);

        // However, when ExecuteCommandList() is called on a particular command 
        // list, that command list can then be reset at any time and must be before 
        // re-recording.
        ok = g_dx_command_list->Reset(g_dx_command_allocator, NULL) == S_OK;
        assert(ok);

        D3D12_VIEWPORT viewport = {};
        viewport.Width = (float)g_window_size[0];
        viewport.Height = (float)g_window_size[1];
        g_dx_command_list->RSSetViewports(1, &viewport);

        D3D12_RECT scissor_rect = {};
        scissor_rect.right = g_window_size[0];
        scissor_rect.bottom = g_window_size[1];
        g_dx_command_list->RSSetScissorRects(1, &scissor_rect);

        D3D12_RESOURCE_BARRIER present_to_rt = Transition(g_dx_back_buffers[g_dx_frame_index], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        g_dx_command_list->ResourceBarrier(1, &present_to_rt);

        D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = g_dx_rtv_heap->GetCPUDescriptorHandleForHeapStart();
        rtv_handle.ptr += g_dx_frame_index * g_dx_rtv_descriptor_size;
        g_dx_command_list->OMSetRenderTargets(1, &rtv_handle, false, NULL);

        const float clear_color[] = { 0.15f, 0.15f, 0.15f, 1.0f };
        g_dx_command_list->ClearRenderTargetView(rtv_handle, clear_color, 0, NULL);

        g_dx_command_list->SetDescriptorHeaps(1, &g_dx_srv_heap);
        g_dx_command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        UI_DX12_Draw(&ui_outputs, {(float)g_window_size[0], (float)g_window_size[1]}, g_dx_command_list);

        D3D12_RESOURCE_BARRIER rt_to_present = Transition(g_dx_back_buffers[g_dx_frame_index], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        g_dx_command_list->ResourceBarrier(1, &rt_to_present);

        ok = g_dx_command_list->Close() == S_OK;
        assert(ok);
    }

    ID3D12CommandList* command_lists[] = { g_dx_command_list };
    g_dx_command_queue->ExecuteCommandLists(DS_ArrayCount(command_lists), command_lists);

    bool ok = g_dx_swapchain->Present(1, 0) == S_OK;
    assert(ok);

    MoveToNextFrame();
}

static void OnResizeWindow(uint32_t width, uint32_t height, void* user_ptr) {
    g_window_size[0] = width;
    g_window_size[1] = height;

    FreeRenderTargets();
    bool ok = g_dx_swapchain->ResizeBuffers(0, width, height, DXGI_FORMAT_UNKNOWN, 0) == S_OK;
    assert(ok);
    CreateRenderTargets();
    g_dx_frame_index = g_dx_swapchain->GetCurrentBackBufferIndex();

    UpdateAndRender();
}

int main() {
    AppInit();

    while (!OS_WindowShouldClose(&g_window)) {
        UI_OS_ResetFrameInputs(&g_window, &g_ui_inputs);

        OS_Event event; 
        while (OS_PollEvent(&g_window, &event, OnResizeWindow, NULL)) {
            UI_OS_RegisterInputEvent(&g_ui_inputs, &event);
        }

        UpdateAndRender();
    }

    AppDeinit();
}

#endif // UI_DEMO_DX12