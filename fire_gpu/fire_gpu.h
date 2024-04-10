// Fire GPU - graphics API abstraction layer
//
// Author: Eero Mutka
// Version: 0
// Date: 25 March, 2024
//
// This code is released under the MIT license (https://opensource.org/licenses/MIT).
//
// Headers that must have been included before this file:
// - "fire_ds.h"
//

#ifndef GPU_INCLUDED
#define GPU_INCLUDED

#include <stdbool.h>
#include <stdint.h>

typedef struct GPU_RenderPass GPU_RenderPass;
typedef struct GPU_GraphicsPipeline GPU_GraphicsPipeline;
typedef struct GPU_ComputePipeline GPU_ComputePipeline;
typedef struct GPU_PipelineLayout GPU_PipelineLayout;
typedef struct GPU_DescriptorSet GPU_DescriptorSet;
typedef struct GPU_Sampler GPU_Sampler;
typedef struct GPU_DescriptorArena GPU_DescriptorArena;

// A graph is a collection of tasks that are executed in parallel.
// A task is a list of operations that are executed in order.
typedef struct GPU_Graph GPU_Graph;

typedef struct GPU_String {
	const char* data;
	int length;
} GPU_String;

#ifdef __cplusplus
#define GPU_LangAgnosticLiteral(T) T   // in C++, struct and union literals are of the form MyStructType{...}
#else
#define GPU_LangAgnosticLiteral(T) (T) // in C, struct and union literals are of the form (MyStructType){...}
#endif

#define GPU_STR(x) GPU_LangAgnosticLiteral(GPU_String){x, sizeof(x)-1}

// TODO: custom arena support?
#define GPU_ARENA DS_Arena
#define GPU_ARENA_ALLOCATE(ARENA, SIZE) (char*)DS_ArenaPush(ARENA, SIZE)

#define GPU_ArrayCount(ARRAY) sizeof(ARRAY) / sizeof(ARRAY[0])

/*typedef struct GPU_Options {
	// Allocation strategy idea:
	// We have a chain of memory blocks that grow exponentially in size, and each holding its own generic offset-allocator.
	// When allocating a resource, first we check if it's bigger than the largest block in the chain, and if so, allocate it directly through vulkan.
	// Otherwise, start from the smallest block, and stop until allocation succeeds. If it fails, we make a new block at the end.

	// One alternative allocation strategy would be arenas; e.g. have a block size of 4MB and hand these out to arenas. Resource freeing would be done
	// automatically when freeing the arena. Pro: simplicity, Con: having lots of small allocations with their own lifetimes would need to be managed by the user.

	uint32_t first_memory_block_size;
	uint32_t first_memory_block_max_allocations;
	uint32_t memory_block_grow_factor;
} GPU_Options;

const static GPU_Options GPU_Options_defaults = {
	MIB(64), // first_memory_block_size
	1024,      // first_memory_block_max_allocations
	200,       // memory_block_grow_factor
};*/

typedef struct GPU_FormatInfo {
	uint32_t block_extent;  // normally 1, but for example BC1_RGB has this 4
	uint32_t block_size;

	// feature support
	bool sampled;
	bool vertex_input;
	bool color_target;
	bool depth_target;
	bool stencil_target;

	bool is_int; // integer format?
	const char* glsl; // glsl image format qualifier
} GPU_FormatInfo;

// For now, the list of formats is very conservative.
// TODO: API to ask for what is supported, and document the support better here.
// Different formats support different rendering features. See GPU_GetFormatInfo() to check for feature support per format.
// Or alternatively, https://docs.vulkan.org/spec/latest/chapters/formats.html#features-required-format-support
typedef enum GPU_Format {
	GPU_Format_Invalid,
	// Unsigned normalized (UN) formats representing a value between 0 and 1.
	GPU_Format_R8UN,
	GPU_Format_RG8UN,
	GPU_Format_RGBA8UN,
	GPU_Format_BGRA8UN,
	// TODO: RGBA16_Unorm?

	// Floats formats (F), no range limit.
	GPU_Format_R16F,
	GPU_Format_RG16F,
	GPU_Format_RGB16F,
	GPU_Format_RGBA16F,
	GPU_Format_R32F,
	GPU_Format_RG32F,
	GPU_Format_RGB32F,
	GPU_Format_RGBA32F,

	// Unsigned integer formats (I), no range limit.
	GPU_Format_R8I,
	GPU_Format_R16I,
	GPU_Format_RG16I,
	GPU_Format_RGBA16I,
	GPU_Format_R32I,
	GPU_Format_RG32I,
	GPU_Format_RGB32I,
	GPU_Format_RGBA32I,
	GPU_Format_R64I,

	// Depth (D) / stencil (S) formats. These are the only formats that work as depth/stencil attachments.
	// The _Or_ versions use the format before the _Or_, unless if it's unsupported in which case the second format is used.
	// X means padding bits.
	GPU_Format_D16UN,
	GPU_Format_D32F_Or_X8D24UN,
	GPU_Format_D32FS8I_Or_D24UNS8I,
	GPU_Format_D24UNS8I_Or_D32FS8I,

	// Compressed formats
	GPU_Format_BC1_RGB_UN,
	GPU_Format_BC1_RGBA_UN,
	GPU_Format_BC3_RGBA_UN,
	GPU_Format_BC5_UN,
} GPU_Format;

#define GPU_FORMAT_INFO(BLOCK_EXTENT, BLOCK_SIZE, SAMPLED, VERTEX_INPUT, COLOR_TARGET, DEPTH_TARGET, STENCIL_TARGET, IS_INT, GLSL) \
    GPU_LangAgnosticLiteral(GPU_FormatInfo){BLOCK_EXTENT, BLOCK_SIZE, SAMPLED, VERTEX_INPUT, COLOR_TARGET, DEPTH_TARGET, STENCIL_TARGET, IS_INT, GLSL}

static GPU_FormatInfo GPU_GetFormatInfo(GPU_Format format) {
	// ................................................................................ GLSL string|
	// ................................................................. Integer Format|
	// .............................................................. Stencil Target|  ,
	// ............................................................. Depth Target|  ,  ,
	// .......................................................... Color Target|  ,  ,  ,
	// ....................................................... Vertex Input|  ,  ,  ,  ,
	// ......................................................... Sampled|  ,  ,  ,  ,  ,
	// ................................................... Block Size|  ,  ,  ,  ,  ,  ,
	switch (format) { // ................................. Extent|   ,  ,  ,  ,  ,  ,  ,
	case GPU_Format_R8UN:                return GPU_FORMAT_INFO(1,  1, 1, 1, 1, 0, 0, 0, "r8");
	case GPU_Format_RG8UN:               return GPU_FORMAT_INFO(1,  2, 1, 1, 1, 0, 0, 0, "rg8");
	case GPU_Format_RGBA8UN:             return GPU_FORMAT_INFO(1,  4, 1, 1, 1, 0, 0, 0, "rgba8");
	case GPU_Format_BGRA8UN:             return GPU_FORMAT_INFO(1,  4, 1, 1, 1, 0, 0, 0, NULL);
	case GPU_Format_R16F:                return GPU_FORMAT_INFO(1,  2, 1, 1, 1, 0, 0, 0, "r16f");
	case GPU_Format_RG16F:               return GPU_FORMAT_INFO(1,  4, 1, 1, 1, 0, 0, 0, "rg16f");
	case GPU_Format_RGB16F:              return GPU_FORMAT_INFO(1,  6, 0, 1, 0, 0, 0, 0, NULL);
	case GPU_Format_RGBA16F:             return GPU_FORMAT_INFO(1,  8, 1, 1, 1, 0, 0, 0, "rgba16f");
	case GPU_Format_R32F:                return GPU_FORMAT_INFO(1,  4, 1, 1, 1, 0, 0, 0, "r32f");
	case GPU_Format_RG32F:               return GPU_FORMAT_INFO(1,  8, 1, 1, 1, 0, 0, 0, "rg32f");
	case GPU_Format_RGB32F:              return GPU_FORMAT_INFO(1, 12, 0, 1, 0, 0, 0, 0, NULL);
	case GPU_Format_RGBA32F:             return GPU_FORMAT_INFO(1, 16, 1, 1, 1, 0, 0, 0, "rgba32f");
	case GPU_Format_R8I:                 return GPU_FORMAT_INFO(1,  1, 1, 1, 1, 0, 0, 1, "r8ui");
	case GPU_Format_R16I:                return GPU_FORMAT_INFO(1,  2, 1, 1, 1, 0, 0, 1, "r16ui");
	case GPU_Format_RG16I:               return GPU_FORMAT_INFO(1,  4, 1, 1, 1, 0, 0, 1, "rg16ui");
	case GPU_Format_RGBA16I:             return GPU_FORMAT_INFO(1,  8, 1, 1, 1, 0, 0, 1, "rgba16ui");
	case GPU_Format_R32I:                return GPU_FORMAT_INFO(1,  4, 1, 1, 1, 0, 0, 1, "r32ui");
	case GPU_Format_RG32I:               return GPU_FORMAT_INFO(1,  8, 1, 1, 1, 0, 0, 1, "rg32ui");
	case GPU_Format_RGB32I:              return GPU_FORMAT_INFO(1, 12, 1, 1, 0, 0, 0, 1, NULL);
	case GPU_Format_RGBA32I:             return GPU_FORMAT_INFO(1, 16, 1, 1, 1, 0, 0, 1, "rgba32ui");
	case GPU_Format_R64I:                return GPU_FORMAT_INFO(1,  8, 0, 0, 0, 0, 0, 1, "r64ui");
	case GPU_Format_D16UN:               return GPU_FORMAT_INFO(1,  2, 1, 0, 0, 1, 0, 0, NULL);

		// TODO: implement automatic switching to the other format if no support for the first one.
	case GPU_Format_D32F_Or_X8D24UN:     return GPU_FORMAT_INFO(1,  4, 1, 0, 0, 1, 0, 0, NULL);
	case GPU_Format_D32FS8I_Or_D24UNS8I: return GPU_FORMAT_INFO(1,  5, 0, 0, 0, 1, 0, 0, NULL);
	case GPU_Format_D24UNS8I_Or_D32FS8I: return GPU_FORMAT_INFO(1,  4, 0, 0, 0, 1, 0, 0, NULL);

	case GPU_Format_BC1_RGB_UN:          return GPU_FORMAT_INFO(4,  8, 1, 0, 0, 0, 0, 0, NULL);
	case GPU_Format_BC1_RGBA_UN:         return GPU_FORMAT_INFO(4,  8, 1, 0, 0, 0, 0, 0, NULL);
	case GPU_Format_BC3_RGBA_UN:         return GPU_FORMAT_INFO(4, 16, 1, 0, 0, 0, 0, 0, NULL);
	case GPU_Format_BC5_UN:              return GPU_FORMAT_INFO(4, 16, 1, 0, 0, 0, 0, 0, NULL);
	case GPU_Format_Invalid: break;
	}
	return GPU_FORMAT_INFO(0, 0, 0, 0, 0, 0, 0, 0, NULL);
}

#define GPU_SWAPCHAIN_FORMAT GPU_Format_BGRA8UN

typedef enum GPU_ShaderStage {
	GPU_ShaderStage_Vertex,
	GPU_ShaderStage_Fragment,
	GPU_ShaderStage_Compute,
} GPU_ShaderStage;

//typedef Slice(GPU_Format) GPU_FormatSlice;

typedef enum GPU_CullMode {
	GPU_CullMode_TwoSided,
	GPU_CullMode_DrawCW, // Draw only clockwise triangles
	GPU_CullMode_DrawCCW, // Draw only counter-clockwise triangles
} GPU_CullMode;

typedef enum GPU_LayoutHint {
	GPU_LayoutHint_RenderTarget,
	GPU_LayoutHint_ShaderRead,
	GPU_LayoutHint_TransferSrc,
	GPU_LayoutHint_TransferDest,
	GPU_LayoutHint_Present,
} GPU_LayoutHint;

//typedef enum GPU_GraphFlags {
//	// GPU_GraphFlag_HasDescriptorArena = 1 << 0,
//} GPU_GraphFlags;

typedef int GPU_BufferFlags;
typedef enum GPU_BufferFlag {
	GPU_BufferFlag_CPU = 1 << 0,
	GPU_BufferFlag_GPU = 1 << 1,
	GPU_BufferFlag_StorageBuffer = 1 << 2,
} GPU_BufferFlag;

typedef int GPU_TextureFlags;
typedef enum GPU_TextureFlag {
	GPU_TextureFlag_StorageImage = 1 << 0,
	GPU_TextureFlag_RenderTarget = 1 << 1,
	GPU_TextureFlag_HasMipmaps = 1 << 2,
	GPU_TextureFlag_Cubemap = 1 << 3,
	GPU_TextureFlag_MSAA2x = 1 << 4,
	GPU_TextureFlag_MSAA4x = 1 << 5,
	GPU_TextureFlag_MSAA8x = 1 << 6,
	GPU_TextureFlag_PerMipBinding = 1 << 7,
	GPU_TextureFlag_SwapchainTarget = 1 << 8, // For internal use only
} GPU_TextureFlag;

typedef struct GPU_Texture {
	uint32_t width, height, depth;
	uint32_t layer_count;
	uint32_t mip_level_count;
	GPU_Format format;
	GPU_TextureFlags flags;
} GPU_Texture;

typedef struct GPU_Buffer {
	GPU_BufferFlags flags;
	uint32_t size;
	void* data; // only valid when GPU_BufferFlag_CPUAccess is set
} GPU_Buffer;

// typedef struct GPU_Define {
// 	GPU_String name;
// 	GPU_String value;
// } GPU_Define;

typedef struct GPU_TextureView {
	GPU_Texture* texture;
	uint32_t mip_level;
} GPU_TextureView;

#define GPU_SWAPCHAIN_COLOR_TARGET ((GPU_TextureView*)0)

typedef struct GPU_RenderPassDesc {
	// To enable multisampling in your renderpass, set `msaa_color_resolve_targets` to an array of resolve targets per each color target.
	// In multisampled rendering, all of the color targets and the depth-stencil target must have one of the MSAA texture flags (2/4/8x) set.

	uint32_t color_targets_count;
	GPU_TextureView* color_targets; // may be GPU_SWAPCHAIN_COLOR_TARGET, in which case `color_targets_count` must be 1
	GPU_TextureView* msaa_color_resolve_targets; // NULL means MSAA is disabled

	GPU_Texture* depth_stencil_target; // NULL means no depth stencil attachment
} GPU_RenderPassDesc;

typedef int GPU_AccessFlags;
typedef enum GPU_AccessFlag {
	GPU_AccessFlag_Read = 1 << 0,
	GPU_AccessFlag_Write = 1 << 1,
} GPU_AccessFlag;

typedef struct GPU_Access {
	GPU_AccessFlags flags;
	uint32_t binding;
} GPU_Access;

typedef bool (*GPU_ShaderIncluderFn)(GPU_ARENA* arena, GPU_String filepath, GPU_String* out_source, void* ctx);

typedef struct GPU_ShaderDesc {
	struct { GPU_Access* data; uint32_t length; } accesses;

	GPU_String glsl_debug_filepath;
	GPU_ShaderIncluderFn glsl_includer; // if NULL, then #include is forbidden
	void* glsl_includer_ctx;

	// If `spirv` is non-empty, it will be used. Otherwise `glsl` will be used.
	GPU_String spirv;
	GPU_String glsl;
} GPU_ShaderDesc;

typedef struct GPU_GraphicsPipelineDesc {
	GPU_PipelineLayout* layout;
	GPU_RenderPass* render_pass; // The pipeline must be used with this render pass

	GPU_ShaderDesc vs;
	GPU_ShaderDesc fs; // If zero-initialized, no fragment shader will be used.

	struct { GPU_Format* data; uint32_t length; } vertex_input_formats;

	// NOTE: the swapchain doesn't contain a depth attachment, so any depth-buffered rendering should be done on custom render-targets.
	bool enable_depth_test;
	bool enable_depth_write;

	bool enable_blending; // TODO: expand the blending options
	bool blending_mode_additive; // TODO: expand the blending options

	bool enable_conservative_rasterization; // @cleanup

	GPU_CullMode cull_mode;
} GPU_GraphicsPipelineDesc;

// OS-specific window handle. On windows, it's represented as HWND.
// If you're using the f library, then this maps directly to `OS_Window`.
typedef void* GPU_WindowHandle;

typedef struct GPU_Color {
	float r;
	float g;
	float b;
	float a; // TODO: investigate clearing with alpha. Currently only 0 and 1 are valid values.
} GPU_Color;

typedef struct GPU_Offset3D {
	int32_t x, y, z;
} GPU_Offset3D;

typedef enum GPU_Filter {
	GPU_Filter_Linear = 0,
	GPU_Filter_Nearest,
} GPU_Filter;

typedef enum GPU_AddressMode {
	GPU_AddressMode_Wrap = 0,
	GPU_AddressMode_Clamp,
	GPU_AddressMode_Mirror,
} GPU_AddressMode;

typedef enum GPU_CompareOp {
	GPU_CompareOp_Never = 0,
	GPU_CompareOp_Less,
	GPU_CompareOp_Equal,
	GPU_CompareOp_LessOrEqual,
	GPU_CompareOp_Greater,
	GPU_CompareOp_NotEqual,
	GPU_CompareOp_GreaterOrEqual,
	GPU_CompareOp_Always,
} GPU_CompareOp;

typedef struct GPU_SamplerDesc {
	GPU_Filter min_filter;
	GPU_Filter mag_filter;
	GPU_Filter mipmap_mode;
	GPU_AddressMode address_modes[3];
	float mip_lod_bias;
	float min_lod;
	float max_lod; // To have no maximum limit, set this to a large value, e.g. 1000
	GPU_CompareOp compare_op;
} GPU_SamplerDesc;

typedef struct GPU_OpBlitInfo {
	GPU_Filter filter;
	GPU_Texture* src_texture;
	GPU_Texture* dst_texture;
	uint32_t src_layer;
	uint32_t dst_layer;
	uint32_t src_mip_level;
	uint32_t dst_mip_level;
	GPU_Offset3D src_area[2];
	GPU_Offset3D dst_area[2];
} GPU_OpBlitInfo;

typedef struct GPU_GLSLError {
	GPU_ShaderStage shader_stage;
	uint32_t line;
	GPU_String error_message;
} GPU_GLSLError;

typedef struct GPU_GLSLErrorArray {
	GPU_GLSLError* data;
	uint32_t length;
} GPU_GLSLErrorArray;

typedef uint32_t GPU_Binding;

// -- API ------------------------------------------------

#ifndef GPU_API
#define GPU_API static
#endif

// NOTE: currently, glslang_initialize_process
// If you're not using GPU_CUSTOM_ARENA, `arena_push_fn` must be NULL.
GPU_API void GPU_Init(GPU_WindowHandle window);
GPU_API void GPU_Deinit();

static GPU_FormatInfo GPU_GetFormatInfo(GPU_Format format);

// Common sampler shortcuts; these are views into common resources, so you must not call GPU_DestroySampler on them.
GPU_API GPU_Sampler* GPU_SamplerLinearWrap();
GPU_API GPU_Sampler* GPU_SamplerLinearClamp();
GPU_API GPU_Sampler* GPU_SamplerLinearMirror();
GPU_API GPU_Sampler* GPU_SamplerNearestClamp();
GPU_API GPU_Sampler* GPU_SamplerNearestWrap();
GPU_API GPU_Sampler* GPU_SamplerNearestMirror();

GPU_API GPU_Sampler* GPU_MakeSampler(const GPU_SamplerDesc* desc);
GPU_API void GPU_DestroySampler(GPU_Sampler* sampler);

GPU_API GPU_PipelineLayout* GPU_InitPipelineLayout();
GPU_API GPU_Binding GPU_TextureBinding(GPU_PipelineLayout* layout, const char* name);
GPU_API GPU_Binding GPU_SamplerBinding(GPU_PipelineLayout* layout, const char* name);
GPU_API GPU_Binding GPU_BufferBinding(GPU_PipelineLayout* layout, const char* name); // Can be either a storage buffer, or a uniform buffer
GPU_API GPU_Binding GPU_StorageImageBinding(GPU_PipelineLayout* layout, const char* name, GPU_Format image_format); // TODO: remove `image_format` requirement from here; we should be able to bind different kinds of storage images to the same storage image descriptor. I think we just need to make the user specify the format in the shader, rather than here. OR, when passing GPU_Read() / GPU_Write() into the pipeline if for some reason we need the metadata.
GPU_API void GPU_FinalizePipelineLayout(GPU_PipelineLayout* layout);
GPU_API void GPU_DestroyPipelineLayout(GPU_PipelineLayout* layout);

GPU_API GPU_DescriptorArena* GPU_MakeDescriptorArena();
GPU_API void GPU_ResetDescriptorArena(GPU_DescriptorArena* descriptor_arena);

// `descriptor_arena` may be NULL
GPU_API void GPU_DestroyDescriptorArena(GPU_DescriptorArena* descriptor_arena);


// ------------------------------------------------------------------------------

// TODO: For any resource creation/destruction thing, we should have GPU_InitDescriptorSets as the base API, and also a wrapper function that only does it for a single object.
// hmm... If we want to allocate many descriptor sets from the same lifetime, what to do?
// On GPU_Lifetime, we could add a flag to say "GPU_LifetimeFlag_MakeDescriptorPool"
// * `descriptor_arena` may be NULL
GPU_API GPU_DescriptorSet* GPU_InitDescriptorSet(GPU_DescriptorArena* descriptor_arena, GPU_PipelineLayout* pipeline_layout);

GPU_API void GPU_SetTextureBinding(GPU_DescriptorSet* set, GPU_Binding binding, GPU_Texture* value);

// - SetTextureBinding binds all of the mip levels of a texture into a descriptor. This is normally fine, but it won't work in all cases.
// For example, if you want to have one mip level of a texture bound as a render target, while reading from another, this introduces a conflict
// with SetTextureBinding, as you'd be implying that sampling from ANY mip level is OK. GPU_SetTextureMipBinding lets you specify
// one mip level to bind, rather than all of them.
// - The texture must have been created with GPU_TextureFlag_PerMipBinding set
GPU_API void GPU_SetTextureMipBinding(GPU_DescriptorSet* set, GPU_Binding binding, GPU_Texture* value, uint32_t mip_level);

GPU_API void GPU_SetSamplerBinding(GPU_DescriptorSet* set, GPU_Binding binding, GPU_Sampler* value);

GPU_API void GPU_SetBufferBinding(GPU_DescriptorSet* set, GPU_Binding binding, GPU_Buffer* value);

GPU_API void GPU_SetStorageImageBinding(GPU_DescriptorSet* set, GPU_Binding binding, GPU_Texture* value, uint32_t mip_level);

GPU_API void GPU_FinalizeDescriptorSet(GPU_DescriptorSet* set);

// this must be called on descriptor sets which were initialized with descriptor_arena == NULL
// * `set` may be NULL
GPU_API void GPU_DestroyDescriptorSet(GPU_DescriptorSet* set);

// ------------------------------------------------------------------------------

// If `data` is non-NULL, the texture data will be uploaded immediately, and mipmaps will be generated if `HasMipmaps` is provided. The function will wait
// for the GPU to finish before returning. If the texture is a cubemap, the provided data must contain all 6 cubemap faces tightly packed together.
// NOTE: You probably shouldn't use the `data` parameter at all, but rather use GPU_OpCopyBufferToTexture on your own graph to reduce unnecessary idling.
GPU_API GPU_Texture* GPU_MakeTexture(GPU_Format format, uint32_t width, uint32_t height, uint32_t depth, GPU_TextureFlags flags, const void* data);

// * `texture` may be NULL
GPU_API void GPU_DestroyTexture(GPU_Texture* texture);

// * `data` may be NULL
GPU_API GPU_Buffer* GPU_MakeBuffer(uint32_t size, GPU_BufferFlags flags, const void* data);

// * `buffer` may be NULL
GPU_API void GPU_DestroyBuffer(GPU_Buffer* buffer);

// If NULL is passed to `out_errors`, the function will assert that the shader compilation succeeds.
// Returns an empty string if failed.
// * `out_errors` may be NULL
GPU_API GPU_String GPU_SPIRVFromGLSL(GPU_ARENA* arena, GPU_ShaderStage stage, GPU_PipelineLayout* pipeline_layout, const GPU_ShaderDesc* desc, GPU_GLSLErrorArray* out_errors);
GPU_API GPU_String GPU_JoinGLSLErrorString(GPU_ARENA* arena, GPU_GLSLErrorArray errors);

// Renderpass and Pipeline are separated, so that you can have a single renderpass with MSAA resolve at the end, and have multiple draw commands with different pipelines inside.
GPU_API GPU_RenderPass* GPU_MakeRenderPass(const GPU_RenderPassDesc* desc);
GPU_API void GPU_DestroyRenderPass(GPU_RenderPass* render_pass);

static inline GPU_Access GPU_Read(uint32_t binding) { GPU_Access x = { GPU_AccessFlag_Read, binding }; return x; }
static inline GPU_Access GPU_Write(uint32_t binding) { GPU_Access x = { GPU_AccessFlag_Write, binding }; return x; }
static inline GPU_Access GPU_ReadWrite(uint32_t binding) { GPU_Access x = { GPU_AccessFlag_Read | GPU_AccessFlag_Write, binding }; return x; }

GPU_API GPU_GraphicsPipeline* GPU_MakeGraphicsPipeline(const GPU_GraphicsPipelineDesc* desc);

// * `pipeline` may be NULL
GPU_API void GPU_DestroyGraphicsPipeline(GPU_GraphicsPipeline* pipeline);

GPU_API GPU_ComputePipeline* GPU_MakeComputePipeline(GPU_PipelineLayout* layout, const GPU_ShaderDesc* cs);

// * `pipeline` may be NULL
GPU_API void GPU_DestroyComputePipeline(GPU_ComputePipeline* pipeline);

// -- Render Graph ----------------------------------------------

GPU_API GPU_Graph* GPU_MakeGraph(void);
GPU_API void GPU_GraphSubmit(GPU_Graph* graph); // Submit a graph to the GPU, but do not wait for it to complete
GPU_API void GPU_GraphWait(GPU_Graph* graph); // Wait will also implicitly reset the graph and its descriptor arena if it has one.
GPU_API void GPU_DestroyGraph(GPU_Graph* graph); // You may only destroy a graph when the GPU is not working on it.

// NOTE: You may only have one set of swapchain graphs alive at once.
// Swapchain graphs are created with the GPU_GraphFlag_HasDescriptorArena flag.
GPU_API void GPU_MakeSwapchainGraphs(uint32_t count, GPU_Graph** out_graphs);
// void GPU_DestroySwapchainGraphs();

// * Calling this is only valid for graphs created with GPU_MakeSwapchainGraph.
// * NULL is returned if the window is minimized.
GPU_API GPU_Texture* GPU_GetBackbuffer(GPU_Graph* graph);

// * NULL is returned if the graph was not created with GPU_GraphFlag_HasDescriptorArena
// GPU_DescriptorArena *GPU_GetDescriptorArena(GPU_Graph *graph);

GPU_API void GPU_WaitUntilIdle();

GPU_API void GPU_OpBindVertexBuffer(GPU_Graph* graph, GPU_Buffer* buffer);
GPU_API void GPU_OpBindIndexBuffer(GPU_Graph* graph, GPU_Buffer* buffer);

GPU_API void GPU_OpBindComputePipeline(GPU_Graph* graph, GPU_ComputePipeline* pipeline);

// void GPU_OpBindGraphicsDescriptorSet(GPU_Graph *graph, GPU_DescriptorSet *set);
GPU_API void GPU_OpBindComputeDescriptorSet(GPU_Graph* graph, GPU_DescriptorSet* set);

GPU_API void GPU_OpPrepareRenderPass(GPU_Graph* graph, GPU_RenderPass* render_pass);
GPU_API uint32_t GPU_OpPrepareDrawParams(GPU_Graph* graph, GPU_GraphicsPipeline* pipeline, GPU_DescriptorSet* descriptor_set);

GPU_API void GPU_OpBeginRenderPass(GPU_Graph* graph); // Begin the prepared renderpass from GPU_OpPrepareRenderPass
GPU_API void GPU_OpEndRenderPass(GPU_Graph* graph);

GPU_API void GPU_OpBindDrawParams(GPU_Graph* graph, uint32_t draw_params);

GPU_API void GPU_OpDraw(GPU_Graph* graph, uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance);
GPU_API void GPU_OpDrawIndexed(GPU_Graph* graph, uint32_t index_count, uint32_t instance_count, uint32_t first_index, uint32_t vertex_offset, uint32_t first_instance);

GPU_API void GPU_OpDispatch(GPU_Graph* graph, uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z);

// The data is copied internally when this is called.
GPU_API void GPU_OpPushGraphicsConstants(GPU_Graph* graph, GPU_PipelineLayout* pipeline_layout, void* data, uint32_t size);
GPU_API void GPU_OpPushComputeConstants(GPU_Graph* graph, GPU_PipelineLayout* pipeline_layout, void* data, uint32_t size);

// Transfer operations

GPU_API void GPU_OpCopyBufferToBuffer(GPU_Graph* graph, GPU_Buffer* src, GPU_Buffer* dst, uint32_t dst_offset, uint32_t src_offset, uint32_t size);
GPU_API void GPU_OpCopyBufferToTexture(GPU_Graph* graph, GPU_Buffer* src, GPU_Texture* dst, uint32_t dst_first_layer, uint32_t dst_layer_count, uint32_t dst_mip_level);
GPU_API void GPU_OpCopyTextureToBuffer(GPU_Graph* graph, GPU_Texture* src, GPU_Buffer* dst);

GPU_API void GPU_OpBlit(GPU_Graph* graph, const GPU_OpBlitInfo* info);
GPU_API void GPU_OpGenerateMipmaps(GPU_Graph* graph, GPU_Texture* texture);

#define GPU_MIP_LEVEL_ALL 0xFFFFFFFF

// * You must use ClearColorI for integer formats, ClearDepthStencil for depth/stencil formats, and ClearColorF for all other formats.
// * `mip_level` may be GPU_MIP_LEVEL_ALL
GPU_API void GPU_OpClearColorF(GPU_Graph* graph, GPU_Texture* dst, uint32_t mip_level, float r, float g, float b, float a);
GPU_API void GPU_OpClearColorI(GPU_Graph* graph, GPU_Texture* dst, uint32_t mip_level, uint32_t r, uint32_t g, uint32_t b, uint32_t a);
GPU_API void GPU_OpClearDepthStencil(GPU_Graph* graph, GPU_Texture* dst, uint32_t mip_level);

#endif // GPU_INCLUDED