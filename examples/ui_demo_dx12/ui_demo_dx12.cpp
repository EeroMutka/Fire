// For debug mode, uncomment this line:
#define UI_DX12_DEBUG_MODE

#define _CRT_SECURE_NO_WARNINGS
#pragma comment (lib, "d3d12")
#pragma comment (lib, "dxgi")
#pragma comment(lib, "d3dcompiler")

#define TODO() __debugbreak()

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

#define STB_RECT_PACK_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#include "fire_ui/stb_rect_pack.h"
#include "fire_ui/stb_truetype.h"

#include "fire_ui/fire_ui.h"
#include "fire_ui/fire_ui_color_pickers.h"
#include "fire_ui/fire_ui_extras.h"
#include "fire_ui/fire_ui_backend_dx12.h"
#include "fire_ui/fire_ui_backend_fire_os.h"

#include "../shared/ui_demo_window.h"

#define BACK_BUFFER_COUNT 2


// -------------------------------------------------------------

static uint32_t g_window_size[] = {512, 512};
static OS_WINDOW g_window;
static UI_Inputs g_ui_inputs;
static UIDemoState g_demo_state;

static DS_Arena g_persist;
static UI_FontIndex g_base_font, g_icons_font;

static ID3D12Device* g_dx_device;
static ID3D12CommandQueue* g_dx_command_queue;
static IDXGISwapChain3* g_dx_swapchain;
static ID3D12DescriptorHeap* g_dx_rtv_heap;
static ID3D12DescriptorHeap* g_dx_srv_heap;
static uint32_t g_dx_rtv_descriptor_size;
static ID3D12Resource* g_dx_back_buffers[BACK_BUFFER_COUNT];
static ID3D12CommandAllocator* g_dx_command_allocator;

static ID3D12RootSignature* g_dx_root_signature;
static ID3D12PipelineState* g_dx_pipeline_state;
static ID3D12GraphicsCommandList* g_dx_command_list;
static ID3D12Resource* g_dx_vertex_buffer;
static D3D12_VERTEX_BUFFER_VIEW g_dx_vertex_buffer_view;
static ID3D12Fence* g_dx_fence;
static HANDLE g_dx_fence_event;

static uint32_t g_dx_back_buffer_index;
static uint64_t g_dx_fence_value;

// -------------------------------------------------------------

static STR_View ReadEntireFile(DS_Arena* arena, const char* file) {
    FILE* f = fopen(file, "rb");
    assert(f);

    fseek(f, 0, SEEK_END);
    long fsize = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* data = DS_ArenaPush(arena, fsize);
    fread(data, fsize, 1, f);

    fclose(f);
    STR_View result = {data, fsize};
    return result;
}

static void InitGPU() {
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
	
	g_dx_back_buffer_index = g_dx_swapchain->GetCurrentBackBufferIndex();

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

	// Create frame resources
	{
		D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = g_dx_rtv_heap->GetCPUDescriptorHandleForHeapStart();

		// Create a RTV for each frame.
		for (int i = 0; i < BACK_BUFFER_COUNT; i++)
		{
			ID3D12Resource* back_buffer;
			ok = g_dx_swapchain->GetBuffer(i, IID_PPV_ARGS(&back_buffer)) == S_OK;
			assert(ok);

			g_dx_device->CreateRenderTargetView(back_buffer, NULL, rtv_handle);
			g_dx_back_buffers[i] = back_buffer;
			rtv_handle.ptr += g_dx_rtv_descriptor_size;
		}
	}

	// Create command allocator
	ok = g_dx_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&g_dx_command_allocator)) == S_OK;
	assert(ok);
}

static void WaitForPreviousFrame() {
    // WAITING FOR THE FRAME TO COMPLETE BEFORE CONTINUING IS NOT BEST PRACTICE.
    // This is code implemented as such for simplicity. The D3D12HelloFrameBuffering
    // sample illustrates how to use fences for efficient resource usage and to
    // maximize GPU utilization.

    // Signal and increment the fence value.
    const UINT64 fence = g_dx_fence_value;
    bool ok = g_dx_command_queue->Signal(g_dx_fence, fence) == S_OK;
    g_dx_fence_value++;

    // Wait until the previous frame is finished.
    if (g_dx_fence->GetCompletedValue() < fence)
    {
        ok = g_dx_fence->SetEventOnCompletion(fence, g_dx_fence_event) == S_OK;
        assert(ok);
        WaitForSingleObject(g_dx_fence_event, INFINITE);
    }

    g_dx_back_buffer_index = g_dx_swapchain->GetCurrentBackBufferIndex();
}

static void LoadGPUAssets() {
    bool ok = true;

    // Create an empty root signature.
    {
        D3D12_ROOT_SIGNATURE_DESC root_signature_desc = {};
        root_signature_desc.NumParameters = 0;
        //root_signature_desc.pParameters;
        root_signature_desc.NumStaticSamplers = 0;
        //root_signature_desc.pStaticSamplers;
        root_signature_desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        ID3DBlob* signature = NULL;
        ok = D3D12SerializeRootSignature(&root_signature_desc, D3D_ROOT_SIGNATURE_VERSION_1, &signature, NULL) == S_OK;
        assert(ok);

        ok = g_dx_device->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&g_dx_root_signature)) == S_OK;
        assert(ok);

        signature->Release();
    }

    // Create the pipeline state, which includes compiling and loading shaders.
    {
        ID3DBlob* vertex_shader;
        ID3DBlob* pixel_shader;

#ifdef UI_DX12_DEBUG_MODE
        // Enable better shader debugging with the graphics debugging tools.
        UINT compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
        UINT compileFlags = 0;
#endif

        ok = D3DCompileFromFile(L"..\\ui_demo_dx12\\shaders.hlsl", NULL, NULL, "VSMain", "vs_5_0", compileFlags, 0, &vertex_shader, NULL) == S_OK;
        assert(ok);

        ok = D3DCompileFromFile(L"..\\ui_demo_dx12\\shaders.hlsl", NULL, NULL, "PSMain", "ps_5_0", compileFlags, 0, &pixel_shader, NULL) == S_OK;
        assert(ok);

        // Define the vertex input layout.
        D3D12_INPUT_ELEMENT_DESC input_elem_desc[] =
        {
            { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
        };

        // Describe and create the graphics pipeline state object (PSO).
        D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc = {};

        pso_desc.InputLayout = { input_elem_desc, _countof(input_elem_desc) };
        pso_desc.pRootSignature = g_dx_root_signature;
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
        pso_desc.BlendState.RenderTarget[0].BlendEnable = false;
        pso_desc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_SRC_ALPHA;
        pso_desc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        pso_desc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        pso_desc.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        pso_desc.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        pso_desc.BlendState.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        pso_desc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        pso_desc.DepthStencilState.DepthEnable = false;
        pso_desc.DepthStencilState.StencilEnable = false;
        pso_desc.SampleMask = UINT_MAX;
        pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        pso_desc.NumRenderTargets = 1;
        pso_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        pso_desc.SampleDesc.Count = 1;

        ok = g_dx_device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&g_dx_pipeline_state)) == S_OK;
        assert(ok);
    }

    // Create the command list.
    ok = g_dx_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, g_dx_command_allocator, g_dx_pipeline_state, IID_PPV_ARGS(&g_dx_command_list)) == S_OK;
    assert(ok);

    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    ok = g_dx_command_list->Close() == S_OK;
    assert(ok);

    // Create the vertex buffer.
    {
        struct Vertex {
            float position[3];
            float color[4];
        };

        // Define the geometry for a triangle.
        Vertex vertices[] =
        {
            { { 0.0f, 0.25f, 0.0f }, { 1.0f, 0.0f, 0.0f, 1.0f } },
            { { 0.25f, -0.25f, 0.0f }, { 0.0f, 1.0f, 0.0f, 1.0f } },
            { { -0.25f, -0.25f, 0.0f }, { 0.0f, 0.0f, 1.0f, 1.0f } }
        };

        D3D12_HEAP_PROPERTIES upload_props = {};
        upload_props.Type = D3D12_HEAP_TYPE_UPLOAD;
        upload_props.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        upload_props.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

        D3D12_RESOURCE_DESC upload_desc = {};
        upload_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        upload_desc.Width = sizeof(vertices);
        upload_desc.Height = 1;
        upload_desc.DepthOrArraySize = 1;
        upload_desc.MipLevels = 1;
        upload_desc.Format = DXGI_FORMAT_UNKNOWN;
        upload_desc.SampleDesc.Count = 1;
        upload_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        upload_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        // Note: using upload heaps to transfer static data like vert buffers is not 
        // recommended. Every time the GPU needs it, the upload heap will be marshalled 
        // over. Please read up on Default Heap usage. An upload heap is used here for 
        // code simplicity and because there are very few verts to actually transfer.
        ok = g_dx_device->CreateCommittedResource(&upload_props, D3D12_HEAP_FLAG_NONE, &upload_desc, D3D12_RESOURCE_STATE_GENERIC_READ, NULL, IID_PPV_ARGS(&g_dx_vertex_buffer)) == S_OK;
        assert(ok);

        // Copy the triangle data to the vertex buffer.
        UINT8* pVertexDataBegin;
        D3D12_RANGE read_range = {}; // We do not intend to read from this resource on the CPU.
        ok = g_dx_vertex_buffer->Map(0, &read_range, reinterpret_cast<void**>(&pVertexDataBegin)) == S_OK;
        assert(ok);

        memcpy(pVertexDataBegin, vertices, sizeof(vertices));
        g_dx_vertex_buffer->Unmap(0, NULL);

        // Initialize the vertex buffer view.
        g_dx_vertex_buffer_view.BufferLocation = g_dx_vertex_buffer->GetGPUVirtualAddress();
        g_dx_vertex_buffer_view.StrideInBytes = sizeof(Vertex);
        g_dx_vertex_buffer_view.SizeInBytes = sizeof(vertices);
    }

    // Create synchronization objects and wait until assets have been uploaded to the GPU.
    {
        ok = g_dx_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&g_dx_fence)) == S_OK;
        assert(ok);
        g_dx_fence_value = 1;

        // Create an event handle to use for frame synchronization.
        g_dx_fence_event = CreateEventW(NULL, FALSE, FALSE, NULL);
        assert(g_dx_fence_event != NULL);

        // Wait for the command list to execute; we are reusing the same command 
        // list in our main loop but for now, we just want to wait for setup to 
        // complete before continuing.
        WaitForPreviousFrame();
    }
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
    DS_ArenaInit(&g_persist, 4096, DS_HEAP);

    g_window = OS_WINDOW_Create(g_window_size[0], g_window_size[1], "UI demo (DX12)");
	
	InitGPU();
    LoadGPUAssets();

    UI_Backend ui_backend = {0};
    UI_DX12_Init(&ui_backend, g_dx_device);
    UI_Init(DS_HEAP, &ui_backend);

    // NOTE: the font data must remain alive across the whole program lifetime!
    STR_View roboto_mono_ttf = ReadEntireFile(&g_persist, "../../fire_ui/resources/roboto_mono.ttf");
    STR_View icons_ttf = ReadEntireFile(&g_persist, "../../fire_ui/resources/fontello/font/fontello.ttf");

    g_base_font = UI_FontInit(roboto_mono_ttf.data, -4.f);
    g_icons_font = UI_FontInit(icons_ttf.data, -2.f);
}

int main() {
    AppInit();

    while (!OS_WINDOW_ShouldClose(&g_window)) {
        UI_OS_ResetFrameInputs(&g_window, &g_ui_inputs);

        OS_WINDOW_Event event; 
        while (OS_WINDOW_PollEvent(&g_window, &event, NULL, NULL)) {
            UI_OS_RegisterInputEvent(&g_ui_inputs, &event);
        }

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

            g_dx_command_list->SetPipelineState(g_dx_pipeline_state);
            g_dx_command_list->SetGraphicsRootSignature(g_dx_root_signature);
        
            D3D12_VIEWPORT viewport = {};
            viewport.Width = (float)g_window_size[0];
            viewport.Height = (float)g_window_size[1];
            g_dx_command_list->RSSetViewports(1, &viewport);

            D3D12_RECT scissor_rect = {};
            scissor_rect.right = g_window_size[0];
            scissor_rect.bottom = g_window_size[1];
            g_dx_command_list->RSSetScissorRects(1, &scissor_rect);

            D3D12_RESOURCE_BARRIER present_to_rt = Transition(g_dx_back_buffers[g_dx_back_buffer_index], D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
            g_dx_command_list->ResourceBarrier(1, &present_to_rt);

            D3D12_CPU_DESCRIPTOR_HANDLE rtv_handle = g_dx_rtv_heap->GetCPUDescriptorHandleForHeapStart();
            rtv_handle.ptr += g_dx_back_buffer_index * g_dx_rtv_descriptor_size;
            g_dx_command_list->OMSetRenderTargets(1, &rtv_handle, false, NULL);

            // draw

            const float clear_color[] = { 0.0f, 0.2f, 0.4f, 1.0f };
            g_dx_command_list->ClearRenderTargetView(rtv_handle, clear_color, 0, NULL);

            g_dx_command_list->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
            g_dx_command_list->IASetVertexBuffers(0, 1, &g_dx_vertex_buffer_view);
            g_dx_command_list->DrawInstanced(3, 1, 0, 0);
            
            {
                UI_Vec2 window_size = {(float)g_window_size[0], (float)g_window_size[1]};
                UI_BeginFrame(&g_ui_inputs, window_size, {g_base_font, 18}, {g_icons_font, 18});

                UIDemoBuild(&g_demo_state, window_size);
                //UI_DrawCircle({5, 5}, 50.f, 12, UI_BLUE);

                UI_Outputs ui_outputs;
                UI_EndFrame(&ui_outputs);

                FLOAT clearcolor[4] = { 0.15f, 0.15f, 0.15f, 1.f };
                //g_d3d11_device_context->ClearRenderTargetView(g_d3d11_framebuffer_rtv, clearcolor);

                UI_DX12_Draw(&ui_outputs, g_dx_command_list);

                UI_OS_ApplyOutputs(&g_window, &ui_outputs);
            }

            D3D12_RESOURCE_BARRIER rt_to_present = Transition(g_dx_back_buffers[g_dx_back_buffer_index], D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
            g_dx_command_list->ResourceBarrier(1, &rt_to_present);

            ok = g_dx_command_list->Close() == S_OK;
            assert(ok);
        }

        ID3D12CommandList* command_lists[] = { g_dx_command_list };
        g_dx_command_queue->ExecuteCommandLists(DS_ArrayCount(command_lists), command_lists);

        bool ok = g_dx_swapchain->Present(1, 0) == S_OK;
        assert(ok);

        WaitForPreviousFrame();
    }
}
