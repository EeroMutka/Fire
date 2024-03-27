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
static IDXGISwapChain* g_swapchain;
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
	
	FLOAT clearcolor[4] = { 0.15f, 0.15f, 0.15f, 1.f };
	ID3D11DeviceContext_ClearRenderTargetView(UI_DX11_STATE.devicecontext, g_framebufferRTV, clearcolor);

	UI_EndFrame(&g_ui_outputs);
	UI_DX11_EndFrame(&g_ui_outputs, g_framebufferRTV, g_window_size);
	
	IDXGISwapChain1_Present(g_swapchain, 1, 0);
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

static void OnResizeWindow(uint32_t width, uint32_t height, void *user_ptr) {
	g_window_size[0] = width;
	g_window_size[1] = height;

	// Recreate swapchain
	ID3D11Resource_Release((ID3D11Resource*)g_framebufferRTV);

	IDXGISwapChain1_ResizeBuffers(g_swapchain, 0, 0, 0, DXGI_FORMAT_UNKNOWN, 0);
	
	ID3D11Texture2D* framebuffer;
	IDXGISwapChain1_GetBuffer(g_swapchain, 0, &IID_ID3D11Texture2D, (void**)&framebuffer); // grab framebuffer from swapchain

	D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {0};
	framebufferRTVdesc.Format        = DXGI_FORMAT_B8G8R8A8_UNORM;
	framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	ID3D11Device_CreateRenderTargetView(UI_DX11_STATE.device, (ID3D11Resource*)framebuffer, &framebufferRTVdesc, &g_framebufferRTV);
	
	ID3D11Resource_Release((ID3D11Resource*)framebuffer); // We don't need this handle anymore

	UpdateAndRender();
}

int main() {
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
	ID3D11DeviceContext* dc;
	D3D11CreateDeviceAndSwapChain(NULL, D3D_DRIVER_TYPE_HARDWARE, NULL, D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG, featurelevels, ARRAYSIZE(featurelevels), D3D11_SDK_VERSION, &swapchaindesc, &g_swapchain, &device, NULL, &dc);

	IDXGISwapChain1_GetDesc(g_swapchain, &swapchaindesc); // Update swapchaindesc with actual window size

	///////////////////////////////////////////////////////////////////////////////////////////////

	ID3D11Texture2D* framebuffer;
	IDXGISwapChain1_GetBuffer(g_swapchain, 0, &IID_ID3D11Texture2D, (void**)&framebuffer); // grab framebuffer from swapchain

	D3D11_RENDER_TARGET_VIEW_DESC framebufferRTVdesc = {0};
	framebufferRTVdesc.Format        = DXGI_FORMAT_B8G8R8A8_UNORM;
	framebufferRTVdesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
	ID3D11Device_CreateRenderTargetView(device, (ID3D11Resource*)framebuffer, &framebufferRTVdesc, &g_framebufferRTV);
	
	ID3D11Resource_Release((ID3D11Resource*)framebuffer); // We don't need this handle anymore

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
		if (!OS_WindowPoll(&os_temp_arena, &window_inputs, &window, OnResizeWindow, NULL)) break;
		
		g_ui_inputs = (UI_Inputs){0};
		g_ui_inputs.base_font = &base_font;
		g_ui_inputs.icons_font = &icons_font;
		UI_OS_TranslateInputs(&g_ui_inputs , &window_inputs, &os_temp_arena);
		
		UpdateAndRender();
	}

	// TODO: cleanup resources
}
