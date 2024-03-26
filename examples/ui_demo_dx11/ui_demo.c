#pragma comment (lib, "gdi32")
#pragma comment (lib, "user32")
#pragma comment (lib, "dxguid")
#pragma comment (lib, "dxgi")
#pragma comment (lib, "d3d11")
#pragma comment (lib, "d3dcompiler")

#define COBJMACROS
//#define WIN32_LEAN_AND_MEAN

#include "fire_ds.h"

#define STR_USE_FIRE_DS_ARENA
#include "fire_string.h"

#define OS_STRING_OVERRIDE
typedef STR OS_String;
#include "fire_os.h"

#include "fire_gpu/fire_gpu.h"

#define STB_RECT_PACK_IMPLEMENTATION
#define STB_TRUETYPE_IMPLEMENTATION
#include "fire_ui/stb_rect_pack.h"
#include "fire_ui/stb_truetype.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

// #pragma comment(lib, "user32")
// #pragma comment(lib, "d3d11")
// #pragma comment(lib, "d3dcompiler")

//#include <windows.h>
//#include <d3d11.h>
//#include <d3dcompiler.h>

#include <windows.h>
#include <d3d11.h>
#include <dxgi1_3.h>
#include <d3dcompiler.h>
//#include <dxgidebug.h>

#include "fire_ui/fire_ui.h"
#include "fire_ui/fire_ui_backend_dx11.h"
#include "fire_ui/fire_ui_backend_fire_os.h"

#define ArrayCount(x) (sizeof(x) / sizeof(x[0]))

typedef struct {
	UI_Key key;
	STR name;
	UI_Text text;
	bool show;
} TreeSpecie;

//// Globals ///////////////////////////////////////////////

// static GPU_Graph *g_graphs[2];
// static GPU_DescriptorArena *g_descriptor_arenas[2];
// static int g_current_graph_idx = 0;
static ID3D11RenderTargetView* g_framebufferRTV;

static UI_Inputs g_ui_inputs;
static UI_Outputs g_ui_outputs;

static UI_Text g_dummy_text;
static UI_Text g_dummy_text_2;

static DS_Vec(TreeSpecie) g_trees;

static uint32_t g_window_size[2] = {1200, 900};

////////////////////////////////////////////////////////////

static UI_Box *TopBarButton(UI_Key key, UI_Size w, UI_Size h, STR string) {
	return UI_AddBoxWithText(key, w, h, UI_BoxFlag_Clickable|UI_BoxFlag_Selectable, string);
}

static void UpdateAndRender() {
	UI_DX11_BeginFrame();
	UI_BeginFrame(&g_ui_inputs, UI_VEC2{(float)g_window_size[0], (float)g_window_size[1]});

	UI_Box *top_bar = NULL;
	UI_Box *file_button = NULL;
	static bool file_dropdown_is_open = false;

	//// Top bar ///////////////////////////////////////////////

	UI_Box *top_bar_root = UI_AddBox(UI_KEY(), UI_SizePx((float)g_window_size[0]), UI_SizeFit(), 0);
	UI_PushBox(top_bar_root);

	{
		top_bar = UI_AddBox(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_LayoutInX|UI_BoxFlag_DrawTransparentBackground);
		UI_PushBox(top_bar);

		file_button = TopBarButton(UI_KEY(), UI_SizeFit(), UI_SizeFit(), STR_("File"));
		if (UI_Pressed(file_button->key, UI_Input_MouseLeft)) {
			file_dropdown_is_open = true;
		}

		if (UI_Clicked(TopBarButton(UI_KEY(), UI_SizeFit(), UI_SizeFit(), STR_("Help"))->key)) {
			printf("No help for you today, sorry.\n");
		}

		UI_PopBox(top_bar);
	}

	UI_PopBox(top_bar_root);
	UI_BoxComputeRects(top_bar_root, UI_VEC2{0, 0});
	UI_DrawBox(top_bar_root);

	//// Main area /////////////////////////////////////////////

	UI_Vec2 main_area_size = {(float)g_window_size[0], (float)g_window_size[1] - top_bar_root->computed_size.y};
	UI_Box *main_area = UI_AddBox(UI_KEY(), UI_SizePx(main_area_size.x), UI_SizePx(main_area_size.y), UI_BoxFlag_DrawBorder|UI_BoxFlag_ChildPadding);
	UI_PushBox(main_area);

	UI_Box *click_me = UI_Button(UI_KEY(), UI_SizeFit(), UI_SizeFit(), STR_("Click me!"));
	if (UI_Clicked(click_me->key)) {
		printf("Button says thanks you!\n");
	}

	UI_AddBox(UI_KEY(), UI_SizePx(0.f), UI_SizePx(5.f), 0); // padding

	UI_Box *another_button = UI_Button(UI_KEY(), UI_SizeFit(), UI_SizeFit(), STR_("Another button"));
	if (UI_Clicked(another_button->key)) {
		printf("Another button was clicked. He's not as thankful.\n");
	}

	UI_AddBox(UI_KEY(), UI_SizePx(0.f), UI_SizePx(5.f), 0); // padding

	{
		UI_Box *row = UI_AddBox(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_LayoutInX|UI_BoxFlag_DrawBorder|UI_BoxFlag_ChildPadding);
		UI_PushBox(row);
		UI_Button(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit(), STR_("Greedy button (does nothing)"));
		UI_Button(UI_KEY(), UI_SizeFit(), UI_SizeFit(), STR_("Humble button (does nothing)"));
		UI_PopBox(row);
	}

	UI_AddBox(UI_KEY(), UI_SizePx(0.f), UI_SizePx(5.f), 0); // padding

	{
		UI_Box *row = UI_AddBox(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_LayoutInX);
		UI_PushBox(row);
		UI_AddBoxWithText(UI_KEY(), UI_SizeFit(), UI_SizeFit(), 0, STR_("Enter text: "));

		static bool editing_dummy_text;
		static UI_Selection dummy_text_selection;

		UI_EditTextRequest edit_request;
		UI_EditText(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit(), g_dummy_text, &editing_dummy_text, &dummy_text_selection, &edit_request);
		UI_ApplyEditTextRequest(&g_dummy_text, &edit_request);

		UI_PopBox(row);
	}

	UI_AddBox(UI_KEY(), UI_SizePx(0.f), UI_SizePx(5.f), 0); // padding

	{
		UI_Box *row = UI_AddBox(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_LayoutInX);
		UI_PushBox(row);
		static float my_float = 320.5f;
		UI_AddBoxWithText(UI_KEY(), UI_SizeFit(), UI_SizeFit(), 0, STR_("Edit float: "));
		UI_EditFloat(UI_KEY(), UI_SizeFit(), UI_SizeFit(), &my_float);
		UI_PopBox(row);
	}

	UI_AddBox(UI_KEY(), UI_SizePx(0.f), UI_SizePx(5.f), 0); // padding

	{
		UI_Box *row = UI_AddBox(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_LayoutInX);
		UI_PushBox(row);
		static int64_t my_int = 8281;
		UI_AddBoxWithText(UI_KEY(), UI_SizeFit(), UI_SizeFit(), 0, STR_("Edit int: "));
		UI_EditInt(UI_KEY(), UI_SizeFit(), UI_SizeFit(), &my_int);
		UI_PopBox(row);
	}

	UI_AddBox(UI_KEY(), UI_SizePx(0.f), UI_SizePx(5.f), 0); // padding

	{
		UI_Box *row = UI_AddBox(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_LayoutInX);
		UI_PushBox(row);
		UI_AddBoxWithText(UI_KEY(), UI_SizeFit(), UI_SizeFit(), 0, STR_("A bunch of checkboxes:"));

		static bool checkboxes[5];
		for (int i=0; i<5; i++) UI_Checkbox(UI_KEY1(i), &checkboxes[i]);

		UI_PopBox(row);
	}

	UI_AddBox(UI_KEY(), UI_SizePx(0.f), UI_SizePx(5.f), 0); // padding

	{ // Arrangers 
		UI_AddBoxWithText(UI_KEY(), UI_SizeFit(), UI_SizeFit(), 0, STR_("And here we have some useful tree facts."));

		UI_Box *arrangers = UI_ArrangersPush(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit());

		for (int i = 0; i < g_trees.length; i++)  {
			TreeSpecie *tree = &g_trees.data[i];

			UI_Box *tree_box = UI_AddBox(UI_KEY1(tree->key),
				UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_LayoutInX|UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawOpaqueBackground);
			UI_PushBox(tree_box);

			UI_Arranger(UI_KEY1(tree->key), UI_SizeFit(), UI_SizeFit());

			UI_Checkbox(UI_KEY1(tree->key), &tree->show);
			UI_AddBoxWithText(UI_KEY1(tree->key), UI_SizeFit(), UI_SizeFit(), 0, tree->name);

			if (tree->show) {
				UI_Box *box = UI_AddBox(UI_KEY1(tree->key),
					UI_SizeFit(), UI_SizeFit(), UI_BoxFlag_DrawBorder|UI_BoxFlag_DrawOpaqueBackground);
				UI_PushBox(box);

				UI_AddBoxWithText(UI_KEY1(tree->key), UI_SizeFit(), UI_SizeFit(), 0, STR_("Tree fact:"));

				static UI_Key editing_text = UI_INVALID_KEY;
				static UI_Selection edit_text_selection;

				UI_Key key = UI_KEY1(tree->key);
				bool was_editing_this = editing_text == key;

				UI_EditTextRequest edit_request;
				bool editing_this = was_editing_this;
				UI_EditText(key, UI_SizeFit(), UI_SizeFit(), tree->text, &editing_this, &edit_text_selection, &edit_request);
				UI_ApplyEditTextRequest(&tree->text, &edit_request);

				if (editing_this) editing_text = key;
				else if (was_editing_this) editing_text = UI_INVALID_KEY;

				UI_PopBox(box);
			}

			UI_PopBox(tree_box);
		}

		UI_ArrangersRequest edit_request;
		UI_ArrangersPop(arrangers, &edit_request);

		if (edit_request.move_from != edit_request.move_to) {
			TreeSpecie moved = DS_VecGet(g_trees, edit_request.move_from);
			DS_VecRemove(&g_trees, edit_request.move_from);
			DS_VecInsert(&g_trees, edit_request.move_to, moved);
		}
	}

	UI_AddBox(UI_KEY(), UI_SizePx(0.f), UI_SizePx(5.f), 0); // padding

	UI_Box *scroll_area = UI_PushScrollArea(UI_KEY(), UI_SizeFlex(1.f), UI_SizePx(200.f), UI_BoxFlag_DrawBorder, 0, 0);
	UI_AddBoxWithText(UI_KEY(), UI_SizeFit(), UI_SizeFit(), 0, STR_("This marks the beginning of a scrollable area."));

	STR lorem_ipsum_lines[] = {
		STR_("Lorem ipsum dolor sit amet, consectetur adipiscing elit."),
		STR_("In sagittis in enim a aliquam."),
		STR_("Curabitur congue metus volutpat mi accumsan, ac dapibus augue euismod."),
		STR_("Praesent nec est mollis quam feugiat tincidunt."),
		STR_("Mauris ut ipsum tristique, commodo enim eu, consectetur odio."),
		STR_("Mauris eget consectetur risus."),
		STR_("Curabitur aliquam orci laoreet tortor varius feugiat."),
		STR_("Phasellus dapibus laoreet imperdiet."),
		STR_("Pellentesque at molestie lectus."),
	};
	for (int i = 0; i < ArrayCount(lorem_ipsum_lines); i++) {
		UI_Box *button = UI_Button(UI_KEY1(i), UI_SizeFit(), UI_SizeFit(), lorem_ipsum_lines[i]);
		if (UI_Clicked(button->key)) {
			printf("Clicked button index %d\n", i);
		}
	}

	UI_PopScrollArea(scroll_area);

	UI_AddBox(UI_KEY(), UI_SizePx(0.f), UI_SizePx(5.f), 0); // padding

	UI_AddBoxWithText(UI_KEY(), UI_SizeFit(), UI_SizeFit(), 0, STR_("Some more text."));

	UI_AddBox(UI_KEY(), UI_SizePx(0.f), UI_SizeFlex(1.f), 0); // padding

	UI_AddBoxWithText(UI_KEY(), UI_SizeFit(), UI_SizeFit(), 0, STR_("Above this text is some flexy padding. Try resizing the window!"));

	UI_PopBox(main_area);
	UI_BoxComputeRects(main_area, UI_VEC2{0.f, top_bar_root->computed_size.y});
	UI_DrawBox(main_area);

	//// Dropdown menus ////////////////////////////////////////

	if (file_dropdown_is_open) {
		static bool nested_dropdown_is_open = false;

		UI_Key file_dropdown_key = UI_KEY();
		file_dropdown_is_open = UI_DropdownShouldKeepOpen(file_dropdown_key);

		UI_BoxFlags dropdown_window_flags = UI_BoxFlag_DrawOpaqueBackground|UI_BoxFlag_DrawTransparentBackground|UI_BoxFlag_DrawBorder;

		UI_Box *dropdown = UI_AddBox(file_dropdown_key, UI_SizeFit(), UI_SizeFit(), dropdown_window_flags);
		UI_PushBox(dropdown);

		if (UI_Clicked(UI_Button(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit(), STR_("Say Hello!"))->key)) {
			printf("Hello!\n");
		}

		static bool curious_checkbox_active;
		static int curious_checkbox_state = 3;

		bool curious_checkbox_active_before = curious_checkbox_active;
		{
			UI_Box *row = UI_AddBox(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit(), UI_BoxFlag_LayoutInX);
			UI_PushBox(row);
			UI_AddBoxWithText(UI_KEY(), UI_SizeFit(), UI_SizeFit(), 0, STR_("Curious checkbox"));

			UI_Checkbox(UI_KEY(), &curious_checkbox_active);

			UI_PopBox(row);
		}

		if (curious_checkbox_active) {
			STR states[] = {STR_(":|"), STR_(":)"), STR_(":|"), STR_(":(")};

			UI_AddBoxWithText(UI_KEY(), UI_SizeFit(), UI_SizeFit(), 0, states[curious_checkbox_state]);
			if (!curious_checkbox_active_before) {
				curious_checkbox_state = (curious_checkbox_state + 1) % ArrayCount(states);
			}
		}

		UI_Box *nested_dropdown_button = UI_Button(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit(), STR_("Open nested dropdown:"));
		if (UI_Pressed(nested_dropdown_button->key, UI_Input_MouseLeft)) {
			nested_dropdown_is_open = true;
		}

		if (UI_Clicked(UI_Button(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFlex(1.f), STR_("Exit Program"))->key)) {
			exit(0);
		}

		UI_PopBox(dropdown);

		UI_BoxComputeRects(dropdown, UI_VEC2{file_button->computed_position.x, file_button->computed_rect_clipped.max.y});
		UI_DrawBox(dropdown);

		UI_Key nested_dropdown_key = UI_KEY();
		if (nested_dropdown_is_open) {
			nested_dropdown_is_open = UI_DropdownShouldKeepOpen(nested_dropdown_key);

			UI_Box *nested_dropdown = UI_AddBox(nested_dropdown_key, UI_SizeFit(), UI_SizeFit(), dropdown_window_flags);
			UI_PushBox(nested_dropdown);
			UI_Button(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit(), STR_("Do nothing (1)"));
			UI_Button(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit(), STR_("Do nothing (2)"));
			UI_Button(UI_KEY(), UI_SizeFlex(1.f), UI_SizeFit(), STR_("Do nothing (3)"));
			UI_PopBox(nested_dropdown);

			UI_Vec2 pos = {
				nested_dropdown_button->computed_rect_clipped.max.x,
				nested_dropdown_button->computed_rect_clipped.min.y,
			};
			UI_BoxComputeRects(nested_dropdown, pos);
			UI_DrawBox(nested_dropdown);
		}
	}

	//// Render frame /////////////////////////////

	UI_EndFrame(&g_ui_outputs);
	UI_DX11_EndFrame(&g_ui_outputs, g_framebufferRTV, g_window_size);

	//g_current_graph_idx = (g_current_graph_idx + 1) % 2;
	//GPU_Graph *graph = g_graphs[g_current_graph_idx];
	//GPU_GraphWait(graph);
	//GPU_ResetDescriptorArena(g_descriptor_arenas[g_current_graph_idx]);

	//GPU_Texture *backbuffer = GPU_GetBackbuffer(graph);
	//if (backbuffer) {
	//	GPU_OpClearColorF(graph, backbuffer, GPU_MIP_LEVEL_ALL, 0.15f, 0.15f, 0.15f, 1.f);
	//
	//	UI_DX11_EndFrame(&g_ui_outputs, graph]);
	//	UI_OS_ApplyOutputs(&g_ui_outputs);
	//
	//	GPU_GraphSubmit(graph);
	//}
}

static void AddTreeSpecie(UI_Key key, STR name, STR information) {
	TreeSpecie tree = {0};
	tree.name = name;
	tree.show = false;
	tree.key = key;
	UI_TextInit(&tree.text, information);
	DS_VecPush(&g_trees, tree);
}

static void DestroyTreeSpecie(TreeSpecie *tree) {
	UI_TextFree(&tree->text);
}

int main() {
	//ID3D11RenderTargetView* rendertargetview;
	OS_Init();

	DS_VecInit(&g_trees);
	AddTreeSpecie(UI_KEY(), STR_("Pine"), STR_("Pine trees can live up to 1000 years."));
	AddTreeSpecie(UI_KEY(), STR_("Oak"), STR_("Oak is commonly used in construction and furniture."));
	AddTreeSpecie(UI_KEY(), STR_("Maple"), STR_("Maple trees are typically 10 to 45 meters tall."));
	AddTreeSpecie(UI_KEY(), STR_("Birch"), STR_("Birch is most commonly found in the Northern hemisphere."));
	AddTreeSpecie(UI_KEY(), STR_("Cedar"), STR_("Cedar wood is a natural repellent to moths."));

	UI_TextInit(&g_dummy_text, STR_(""));
	UI_TextInit(&g_dummy_text_2, STR_(""));

	DS_Arena persistent_arena;
	DS_InitArena(&persistent_arena, 4096);

	OS_Arena os_persistent_arena;
	OS_InitArena(&os_persistent_arena, 4096);
	
	OS_Arena os_temp_arena;
	OS_InitArena(&os_temp_arena, 4096);

	OS_Inputs window_inputs = {0};
	OS_Window window = OS_WindowCreate(1200, 900, OS_STR("UI demo (DX11)"));

	STR ui_shader_filepath = STR_("../../fire_ui/fire_ui_backend_fire_gpu_shader.glsl");
	STR ui_shader_src;
	UI_CHECK(OS_ReadEntireFile(&os_persistent_arena, ui_shader_filepath, &ui_shader_src));

	//WNDCLASSA wndclass = {0, DefWindowProcA, 0, 0, 0, 0, 0, 0, 0, "MyWindowClass"};
	//RegisterClassA(&wndclass);

	//HWND window = CreateWindowExA(0, "MyWindowClass", "My window", WS_POPUP | WS_MAXIMIZE | WS_VISIBLE, 0, 0, 0, 0, NULL, NULL, NULL, NULL);
	
	D3D_FEATURE_LEVEL featurelevels[] = { D3D_FEATURE_LEVEL_11_0 };
	
	DXGI_SWAP_CHAIN_DESC swapchaindesc = {0};
	swapchaindesc.BufferDesc.Width  = 0; // use window width
	swapchaindesc.BufferDesc.Height = 0; // use window height
	swapchaindesc.BufferDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapchaindesc.SampleDesc.Count  = 1;
	swapchaindesc.BufferUsage       = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapchaindesc.BufferCount       = 2;
	swapchaindesc.OutputWindow      = (HWND)window.handle;
	swapchaindesc.Windowed          = TRUE;
	swapchaindesc.SwapEffect        = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	
	ID3D11Device* device;
	IDXGISwapChain* swapchain;
	ID3D11DeviceContext* dc;
	D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG, featurelevels, ARRAYSIZE(featurelevels), D3D11_SDK_VERSION, &swapchaindesc, &swapchain, &device, NULL, &dc);

	IDXGISwapChain1_GetDesc(swapchain, &swapchaindesc); // Update swapchaindesc with actual window size

	///////////////////////////////////////////////////////////////////////////////////////////////
	
	ID3D11Texture2D* framebuffer;
	IDXGISwapChain1_GetBuffer(swapchain, 0, &IID_ID3D11Texture2D, (void**)&framebuffer); // grab framebuffer from swapchain

	D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {0};
	//framebufferRTVdesc.Format        = DXGI_FORMAT_B8G8R8A8_UNORM_SRGB;
	framebufferRTVdesc.Format        = DXGI_FORMAT_B8G8R8A8_UNORM;
	framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;

	ID3D11Device_CreateRenderTargetView(device, (ID3D11Resource*)framebuffer, &framebufferRTVdesc, &g_framebufferRTV);

	///////////////////////////////////////////////////////////////////////////////////////////////

	ID3DBlob* vertexshaderCSO;
	ID3D11VertexShader* vertexshader;
	D3DCompileFromFile(L"../ui_demo_dx11/gpu.hlsl", NULL, NULL, "vertex_shader", "vs_5_0", 0, 0, &vertexshaderCSO, NULL);
	
	ID3D11Device_CreateVertexShader(device, ID3D10Blob_GetBufferPointer(vertexshaderCSO), ID3D10Blob_GetBufferSize(vertexshaderCSO), NULL, &vertexshader);

	D3D11_INPUT_ELEMENT_DESC inputelementdesc[] = // maps to vertexdata struct in gpu.hlsl via semantic names ("POS", "NOR", "TEX", "COL")
	{
		{ "POS", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,                            0, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // float3 position
		{ "COL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D11_APPEND_ALIGNED_ELEMENT, D3D11_INPUT_PER_VERTEX_DATA, 0 }, // float3 color
	};

	ID3D11InputLayout* inputlayout;

	ID3D11Device_CreateInputLayout(device, inputelementdesc, ARRAYSIZE(inputelementdesc), ID3D10Blob_GetBufferPointer(vertexshaderCSO), ID3D10Blob_GetBufferSize(vertexshaderCSO), &inputlayout);

	///////////////////////////////////////////////////////////////////////////////////////////////

	ID3DBlob* pixelshaderCSO;
	D3DCompileFromFile(L"../ui_demo_dx11/gpu.hlsl", NULL, NULL, "pixel_shader", "ps_5_0", 0, 0, &pixelshaderCSO, NULL);
	
	ID3D11PixelShader* pixelshader;
	ID3D11Device_CreatePixelShader(device, ID3D10Blob_GetBufferPointer(pixelshaderCSO), ID3D10Blob_GetBufferSize(pixelshaderCSO), NULL, &pixelshader);

	///////////////////////////////////////////////////////////////////////////////////////////////

	D3D11_RASTERIZER_DESC rasterizerdesc = {0};
	rasterizerdesc.FillMode = D3D11_FILL_SOLID;
	rasterizerdesc.CullMode = D3D11_CULL_BACK;

	ID3D11RasterizerState* rasterizerstate;
	ID3D11Device_CreateRasterizerState(device, &rasterizerdesc, &rasterizerstate);
	
	///////////////////////////////////////////////////////////////////////////////////////////////

	D3D11_SAMPLER_DESC samplerdesc = {0};
	samplerdesc.Filter         = D3D11_FILTER_MIN_MAG_MIP_POINT;
	samplerdesc.AddressU       = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerdesc.AddressV       = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerdesc.AddressW       = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerdesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
	ID3D11SamplerState* samplerstate;
	ID3D11Device_CreateSamplerState(device, &samplerdesc, &samplerstate);
	
	///////////////////////////////////////////////////////////////////////////////////////////////

	typedef struct { float test; } Constants;

	D3D11_BUFFER_DESC constantbufferdesc = {0};
	constantbufferdesc.ByteWidth      = (sizeof(Constants) + 0xf) & 0xfffffff0; // ensure constant buffer size is multiple of 16 bytes
	constantbufferdesc.Usage          = D3D11_USAGE_DYNAMIC; // will be updated every frame
	constantbufferdesc.BindFlags      = D3D11_BIND_CONSTANT_BUFFER;
	constantbufferdesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

	ID3D11Buffer* constantbuffer;
	ID3D11Device_CreateBuffer(device, &constantbufferdesc, NULL, &constantbuffer);

	///////////////////////////////////////////////////////////////////////////////////////////////
	
	float vertexdata[] = {
		0.f, 0.f, 0.f,   1.f, 0.f, 0.f,
		0.f, 1.f, 0.f,   1.f, 0.f, 0.f,
		1.f, 0.f, 0.f,   1.f, 0.f, 0.f,
	};
	int indexdata[] = {0, 1, 2};

	D3D11_BUFFER_DESC vertexbufferdesc = {0};
	vertexbufferdesc.ByteWidth = sizeof(vertexdata);
	vertexbufferdesc.Usage     = D3D11_USAGE_IMMUTABLE;
	vertexbufferdesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

	D3D11_SUBRESOURCE_DATA vertexbufferSRD = { vertexdata };

	ID3D11Buffer* vertexbuffer;
	ID3D11Device_CreateBuffer(device, &vertexbufferdesc, &vertexbufferSRD, &vertexbuffer);
	
	///////////////////////////////////////////////////////////////////////////////////////////////
	
	D3D11_BUFFER_DESC indexbufferdesc = {0};
	indexbufferdesc.ByteWidth = sizeof(indexdata);
	indexbufferdesc.Usage     = D3D11_USAGE_IMMUTABLE;
	indexbufferdesc.BindFlags = D3D11_BIND_INDEX_BUFFER;

	D3D11_SUBRESOURCE_DATA indexbufferSRD = { indexdata };

	ID3D11Buffer* indexbuffer;
	ID3D11Device_CreateBuffer(device, &indexbufferdesc, &indexbufferSRD, &indexbuffer);

	///////////////////////////////////////////////////////////////////////////////////////////////

	UINT stride = 6 * sizeof(float);
	UINT offset = 0;

	D3D11_VIEWPORT viewport = {0.0f, 0.0f, (float)swapchaindesc.BufferDesc.Width, (float)swapchaindesc.BufferDesc.Height, 0.0f, 1.0f};

	///////////////////////////////////////////////////////////////////////////////////////////////

	UI_Backend ui_backend = {0};
	UI_DX11_Init(&ui_backend, device, dc, (GPU_String){ui_shader_src.data, ui_shader_src.size});
	UI_Init(&persistent_arena, &ui_backend);

	// NOTE: the font data must remain alive across the whole program lifetime!
	STR roboto_mono_ttf, icons_ttf;
	assert(OS_ReadEntireFile(&os_persistent_arena, STR_("../../fire_ui/resources/roboto_mono.ttf"), &roboto_mono_ttf));
	assert(OS_ReadEntireFile(&os_persistent_arena, STR_("../../fire_ui/resources/fontello/font/fontello.ttf"), &icons_ttf));

	UI_Font base_font, icons_font;
	UI_LoadFontFromData(&base_font, roboto_mono_ttf.data, -4.f);
	UI_LoadFontFromData(&icons_font, icons_ttf.data, -2.f);

	for (;;) {
		OS_ArenaReset(&os_temp_arena);
		if (!OS_WindowPoll(&os_temp_arena, &window_inputs, &window, NULL, NULL)) break;
		
		g_ui_inputs = (UI_Inputs){0};
		g_ui_inputs.base_font = &base_font;
		g_ui_inputs.icons_font = &icons_font;
		UI_OS_TranslateInputs(&g_ui_inputs , &window_inputs, &os_temp_arena);

		FLOAT clearcolor[4] = { 0.15f, 0.15f, 0.15f, 1.f };
		ID3D11DeviceContext_ClearRenderTargetView(dc, g_framebufferRTV, clearcolor);
		
		UpdateAndRender();

		///////////////////////////////////////////////////////////////////////////////////////////
#if 0
		D3D11_MAPPED_SUBRESOURCE constantbufferMSR;

		ID3D11DeviceContext_Map(dc, (ID3D11Resource*)constantbuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &constantbufferMSR); // update constant buffer every frame
		Constants* constants = (Constants*)constantbufferMSR.pData;
		constants->test = 0.f;

		ID3D11DeviceContext_Unmap(dc, (ID3D11Resource*)constantbuffer, 0);

		ID3D11DeviceContext_ClearRenderTargetView(dc, g_framebufferRTV, clearcolor);
		// ID3D11DeviceContext_ClearDepthStencilView(devicecontext, depthbufferDSV, D3D11_CLEAR_DEPTH, 1.0f, 0);

		ID3D11DeviceContext_IASetPrimitiveTopology(dc, D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		ID3D11DeviceContext_IASetInputLayout(dc, inputlayout);
		ID3D11DeviceContext_IASetVertexBuffers(dc, 0, 1, &vertexbuffer, &stride, &offset);
		ID3D11DeviceContext_IASetIndexBuffer(dc, indexbuffer, DXGI_FORMAT_R32_UINT, 0);

		ID3D11DeviceContext_VSSetShader(dc, vertexshader, NULL, 0);
		ID3D11DeviceContext_VSSetConstantBuffers(dc, 0, 1, &constantbuffer);

		ID3D11DeviceContext_RSSetViewports(dc, 1, &viewport);
		ID3D11DeviceContext_RSSetState(dc, rasterizerstate);

		ID3D11DeviceContext_PSSetShader(dc, pixelshader, NULL, 0);
		// ID3D11DeviceContext_PSSetShaderResources(devicecontext, 0, 1, &textureSRV);
		ID3D11DeviceContext_PSSetSamplers(dc, 0, 1, &samplerstate);

		ID3D11DeviceContext_OMSetRenderTargets(dc, 1, &g_framebufferRTV, NULL);
		ID3D11DeviceContext_OMSetBlendState(dc, NULL, NULL, 0xffffffff); // use default blend mode (i.e. no blending)

		///////////////////////////////////////////////////////////////////////////////////////////

		ID3D11DeviceContext_DrawIndexed(dc, ARRAYSIZE(indexdata), 0, 0);

		///////////////////////////////////////////////////////////////////////////////////////////
#endif

		IDXGISwapChain1_Present(swapchain, 1, 0);
	}
}

#if 0

static void OnResizeWindow(uint32_t width, uint32_t height, void *user_ptr) {
	g_window_width = width;
	g_window_height = height;
	UpdateAndRender();
}

int main() {

	OS_Inputs window_inputs = {0};
	OS_Window window = OS_WindowCreate(g_window_width, g_window_height, OS_STR("Fire UI demo"));

	GPU_Init(window.handle);
	
	GPU_RenderPass *main_renderpass = GPU_MakeRenderPass(&(GPU_RenderPassDesc){
		.color_targets_count = 1,
		.color_targets = GPU_SWAPCHAIN_COLOR_TARGET,
	});

	GPU_MakeSwapchainGraphs(2, &g_graphs[0]);
	g_descriptor_arenas[0] = GPU_MakeDescriptorArena();
	g_descriptor_arenas[1] = GPU_MakeDescriptorArena();

	//// Main loop /////////////////////////////////////
	
	for (;;) {
		OS_ArenaReset(&os_temp_arena);

		if (!OS_WindowPoll(&os_temp_arena, &window_inputs, &window, OnResizeWindow, NULL)) break;

		g_ui_inputs = (UI_Inputs){0};
		g_ui_inputs.base_font = &base_font;
		g_ui_inputs.icons_font = &icons_font;
		UI_OS_TranslateInputs(&g_ui_inputs , &window_inputs, &os_temp_arena);
		
		UpdateAndRender();
	}

	GPU_WaitUntilIdle();
	
	//// Cleanup resources /////////////////////////////

	UI_TextFree(&g_dummy_text);
	UI_TextFree(&g_dummy_text_2);
	UI_DestroyFont(&base_font);
	UI_DestroyFont(&icons_font);
	UI_Deinit();
	UI_DX11_Deinit();

	GPU_DestroyDescriptorArena(g_descriptor_arenas[0]);
	GPU_DestroyDescriptorArena(g_descriptor_arenas[1]);
	GPU_DestroyGraph(g_graphs[0]);
	GPU_DestroyGraph(g_graphs[1]);
	GPU_DestroyRenderPass(main_renderpass);
	GPU_Deinit();

	OS_DestroyArena(&os_persistent_arena);
	OS_DestroyArena(&os_temp_arena);
	DS_DestroyArena(&persistent_arena);
	
	for (int i = 0; i < g_trees.length; i++) DestroyTreeSpecie(&g_trees.data[i]);
	DS_VecDestroy(&g_trees);

	OS_Deinit();

	return 0;
}
#endif