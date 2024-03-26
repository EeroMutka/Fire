// Fire GPU, Vulkan Backend
// This file is part of the Fire GPU library, see "fire_gpu.h"
// 
// Headers that must have been included before this file:
// - "fire_ds.h"
// - "fire_gpu.h"
// - "vulkan.h" (from the Vulkan SDK)
// - "glslang/Include/glslang_c_interface.h" (from the Vulkan SDK)
//

#ifndef GPU_CHECK_OVERRIDE
#include <assert.h>
// GPU_Check is used to check for correct API usage
#define GPU_Check(x) assert(x)
#endif

#define GPU_TODO() GPU_Check(false)

const glslang_resource_t *glslang_default_resource(void);

#if 1 // Windows
typedef VkFlags VkWin32SurfaceCreateFlagsKHR;
typedef struct HWND__ *HWND;
typedef struct VkWin32SurfaceCreateInfoKHR {
	VkStructureType				 sType;
	const void*					 pNext;
	VkWin32SurfaceCreateFlagsKHR	flags;
	void*					   hinstance;
	HWND							 hwnd;
} VkWin32SurfaceCreateInfoKHR;

__declspec(dllimport) int64_t __stdcall GetWindowLongPtrW(HWND hWnd, int nIndex);

VKAPI_ATTR VkResult VKAPI_CALL vkCreateWin32SurfaceKHR(VkInstance instance, const VkWin32SurfaceCreateInfoKHR *pCreateInfo, const VkAllocationCallbacks *pAllocator, VkSurfaceKHR *pSurface);
#endif

// ----------------------------------------------------------------------------

#ifndef GPU_ENABLE_VALIDATION
#define GPU_ENABLE_VALIDATION true
#endif

#ifndef GPU_REVERSE_DEPTH
#define GPU_REVERSE_DEPTH false
#endif

#define GPU_SWAPCHAIN_IMG_COUNT 3
// #define GPU_INDEX_ALLOCATOR_ALIVE_SLOT 0xFFFFFFFF
// #define GPU_INDEX_ALLOCATOR_FREELIST_END (0xFFFFFFFF - 1)

typedef enum GPU_ResourceKind {
	GPU_ResourceKind_Texture,
	GPU_ResourceKind_Sampler,
	GPU_ResourceKind_Buffer,
	GPU_ResourceKind_StorageImage,
} GPU_ResourceKind;

typedef int GPU_ResourceAccessFlags;
typedef enum GPU_ResourceAccessFlag {
	GPU_ResourceAccessFlag_ColorTargetRead = 1 << 0,
	GPU_ResourceAccessFlag_ColorTargetWrite = 1 << 1,
	GPU_ResourceAccessFlag_DepthStencilTargetWrite = 1 << 2,
	GPU_ResourceAccessFlag_TextureRead = 1 << 3,
	GPU_ResourceAccessFlag_StorageImageRead = 1 << 4,
	GPU_ResourceAccessFlag_StorageImageWrite = 1 << 5,
	GPU_ResourceAccessFlag_TransferRead = 1 << 6,
	GPU_ResourceAccessFlag_TransferWrite = 1 << 7,
	GPU_ResourceAccessFlag_BufferRead = 1 << 8,
	GPU_ResourceAccessFlag_BufferWrite = 1 << 9,

	// GPU_ResourceAccessFlag_Compute = 1 << 10, // Specifies if the access happens inside compute shader
} GPU_ResourceAccessFlag;

typedef struct GPU_ResourceAccess GPU_ResourceAccess;
struct GPU_ResourceAccess {
	void *resource;
	GPU_ResourceKind resource_kind;
	GPU_ResourceAccessFlags access_flags;

	uint32_t first_layer, layer_count;
	uint32_t first_mip_level, mip_level_count;
};

typedef struct GPU_SubresourceState {
	VkImageLayout layout;
	VkPipelineStageFlags stage;
	VkAccessFlags access_flags;
} GPU_SubresourceState;

// typedef struct { GPU_TextureImpl *texture; uint32_t layer; uint32_t level; } TextureView;
typedef struct GPU_TextureState {
	GPU_SubresourceState *sub_states; // [level + layer*level_count]
} GPU_TextureState; // GPU_TextureImpl::temp will be set to this per each texture

typedef struct GPU_BufferState {
	VkPipelineStageFlags stage;
	VkAccessFlags access_flags;
} GPU_BufferState; // GPU_BufferImpl::temp will be set to this per each buffer

typedef struct GPU_BindingInfo {
	GPU_ResourceKind kind;
	GPU_Format image_format;
	const char *name;
} GPU_BindingInfo;

typedef struct GPU_PipelineLayout {
	DS_Vec(GPU_BindingInfo) bindings;
	//uint32_t bindings_count;
	VkDescriptorSetLayout descriptor_set_layout;
	VkPipelineLayout vk_handle;
} GPU_PipelineLayout;

/*typedef struct GPU_DescriptorPool GPU_DescriptorPool;
struct GPU_DescriptorPool {
	VkDescriptorPool vk_handle;
	GPU_DescriptorPool *next;
};*/

typedef struct GPU_DescriptorArena {
	DS_Arena arena;
	VkDescriptorPool pool;
	// GPU_DescriptorPool first_pool;
	// GPU_DescriptorPool *last_pool;
} GPU_DescriptorArena;

typedef struct GPU_BindingValue {
	void *ptr;
	uint32_t mip_level; // may be GPU_MIP_LEVEL_ALL
} GPU_BindingValue;

typedef struct GPU_DescriptorSet {
	GPU_DescriptorArena *descriptor_arena; // may be NULL
	VkDescriptorPool global_descriptor_pool; // may be NULL
	GPU_PipelineLayout *pipeline_layout;
	DS_Vec(GPU_BindingValue) bindings; // Has same order as the descriptors in the descriptor set layout
	VkDescriptorSet vk_handle;
} GPU_DescriptorSet;

typedef struct GPU_RenderPass {
	VkRenderPass vk_handle;

	// 0 means that the renderpass does not render to swapchain
	int64_t depends_on_swapchain_gen_id;
	
	// If one of the attachments is the swapchain image, then all of the elements in `framebuffers` are used.
	// Otherwise, only the first one is used.
	VkFramebuffer framebuffers[GPU_SWAPCHAIN_IMG_COUNT];

	VkSampleCountFlagBits msaa_samples;
		
	uint32_t color_targets_count;
	GPU_TextureView color_targets[8];
	GPU_TextureView resolve_targets[8];
	GPU_Texture *depth_stencil_target;
} GPU_RenderPass;

typedef struct GPU_GraphicsPipeline {
	//GPU_Entity base; // Must be the first member, as we outwards-cast in GPU_ReleaseGraphicsPipeline()
	GPU_PipelineLayout *layout;
	DS_Vec(GPU_Access) accesses; // TODO: turn into BucketList
	GPU_RenderPass *render_pass;
	VkPipeline vk_handle;
} GPU_GraphicsPipeline;

typedef struct GPU_ComputePipeline {
	//GPU_Entity base; // Must be the first member, as we outwards-cast in GPU_ReleaseComputePipeline()
	GPU_PipelineLayout *layout;
	DS_Vec(GPU_Access) accesses; // TODO: turn into BucketList
	VkPipeline vk_handle;
} GPU_ComputePipeline;

#if 0
/*
 Index allocator is an utility to allocate and free slots within a fixed range.
*/
typedef struct GPU_IndexAllocator {
	// The value of an element in `buffer` is the next free index, or INDEX_ALLOCATOR_FREELIST_END if the last free index,
	// or INDEX_ALLOCATOR_ALIVE_SLOT if currently alive
	DS_Vec(uint32_t) buffer;
	uint32_t first_free_index;
	uint32_t ceiling;
} GPU_IndexAllocator;
#endif


typedef struct GPU_TextureImpl {
	GPU_Texture base;
	VkImage vk_handle;
	VkDeviceMemory allocation;
	VkImageView img_view;
	VkImageView atomics_img_view;
	VkImageView *mip_level_img_views; // May be NULL. Each image view is a view into a single mip level, starting from 0
		
	VkImageLayout idle_layout_;
		
	GPU_TextureState *temp; // state in graph
		
	//Opt(struct GPU_TextureImpl*) next;
} GPU_TextureImpl;
// typedef struct { GPU_Entity base; GPU_TextureImpl texture; } GPU_TextureImplRes;

typedef struct GPU_BufferImpl {
	GPU_Buffer base;
	VkDeviceMemory allocation;
	VkBuffer vk_handle;
		
	GPU_BufferState *temp; // state in graph
	//Opt(struct GPU_BufferImpl*) next;
} GPU_BufferImpl;
// typedef struct { GPU_Entity base; GPU_BufferImpl buffer; } GPU_BufferImplRes;

typedef union GPU_Entity {
	GPU_TextureImpl texture;
	GPU_BufferImpl buffer;
	GPU_DescriptorSet descriptor_set;
	//GPU_DescriptorPool descriptor_pool;
	GPU_DescriptorArena descriptor_arena;
	GPU_GraphicsPipeline graphics_pipeline;
	GPU_ComputePipeline compute_pipeline;
	GPU_PipelineLayout pipeline_layout;
	GPU_RenderPass render_pass; // maybe we could put RenderPasses into a separate bucket list, since the struct is bigger than the other ones
} GPU_Entity;

typedef struct GPU_Swapchain {
	bool needs_rebuild;
	int64_t gen_id;
	VkSwapchainKHR vk_handle;
	uint32_t width, height;
	GPU_TextureImpl textures[GPU_SWAPCHAIN_IMG_COUNT];
} GPU_Swapchain;

typedef struct GPU_Context {
	GPU_WindowHandle window;
		
	DS_Arena persistent_arena;
	DS_Arena temp_arena;

	VkInstance instance;
	VkPhysicalDevice physical_device;
	VkDebugReportCallbackEXT debug_callback;
	VkDevice device;
	VkPhysicalDeviceMemoryProperties mem_properties;
		
	uint32_t queue_family;
	VkQueue queue;

	VkSurfaceKHR surface;
	GPU_Swapchain swapchain;

	VkDescriptorPool global_descriptor_pool; // may be NULL
	// GPU_DescriptorPool *first_global_descriptor_pool; // may be NULL
	// GPU_DescriptorPool *last_global_descriptor_pool; // may be NULL

	DS_SlotAllocator(GPU_Entity, 32) entities;

	// Is the scope thing too much? I mean, it is kind of unnecessary. Yeah, maybe let's just not do that.
	//GPU_Scope *first_free_scope;
	//GPU_BucketList(GPU_Scope, 32) scopes;

	// the GPU can be working on both of these frames at the same time.
	// GPU_FrameInFlight old_frame;
	// GPU_FrameInFlight new_frame;

	VkCommandPool cmd_pool;
	// VkDescriptorPool descriptor_pool;

	GPU_Texture *nil_storage_image;
	GPU_Buffer *nil_storage_buffer;
	GPU_Sampler *nil_sampler;
		
	GPU_Sampler *sampler_linear_wrap;
	GPU_Sampler *sampler_linear_clamp;
	GPU_Sampler *sampler_linear_mirror;
	GPU_Sampler *sampler_nearest_clamp;
	GPU_Sampler *sampler_nearest_wrap;
	GPU_Sampler *sampler_nearest_mirror;
} GPU_Context;

#define GPU_NOT_A_SWAPCHAIN_GRAPH 0xFFFFFFFF
#define GPU_SWAPCHAIN_GRAPH_HASNT_CALLED_WAIT (0xFFFFFFFF-1)

typedef struct GPU_DrawParams {
	GPU_GraphicsPipeline *pipeline;
	GPU_DescriptorSet *desc_set;
} GPU_DrawParams;

typedef struct GPU_Graph {
	DS_Arena arena;
	DS_ArenaMark arena_begin_mark;
	VkCommandBuffer cmd_buffer;
	bool has_began_cmd_buffer;
		
	VkFence gpu_finished_working_fence;

	// GPU_DescriptorArena *descriptor_arena; // may be NULL
		
	struct {
		GPU_RenderPass *render_pass;			   // may be NULL
		GPU_ComputePipeline *compute_pipeline;	 // may be NULL
		GPU_DescriptorSet *compute_descriptor_set; // may be NULL
		
		GPU_GraphicsPipeline *draw_pipeline;	   // may be NULL
		GPU_DescriptorSet *draw_descriptor_set;	// may be NULL

		GPU_RenderPass *preparing_render_pass;	 // may be NULL
		DS_Vec(GPU_DrawParams) prepared_draw_params;

		// Accessed resources
		DS_Vec(GPU_TextureImpl*) textures;
		DS_Vec(GPU_BufferImpl*) buffers;
	} builder_state;

	// only used when the graph is made with GPU_MakeSwapchainGraph
	struct {
		uint32_t img_index; // valid image index or GPU_NOT_A_SWAPCHAIN_GRAPH or GPU_SWAPCHAIN_GRAPH_HASNT_CALLED_WAIT
		GPU_Texture *backbuffer; // NULL if window is minimized
		VkSemaphore img_available_semaphore;		  // signaled when the image becomes available
		VkSemaphore img_finished_rendering_semaphore; // signaled when the image has finished rendering
	} frame;
} GPU_Graph;


// -- global state ------------------------------------------------------------

GPU_Context FGPU; // :GLSLangInitNote

// ----------------------------------------------------------------------------

typedef DS_Vec(char) GPU_StrBuilder;

#define GPU_PrintL(BUILDER, STRING_LITERAL) DS_VecPushN(BUILDER, STRING_LITERAL, sizeof(STRING_LITERAL)-1)

static void GPU_PrintS(GPU_StrBuilder *builder, GPU_String str) {
	DS_VecPushN(builder, str.data, (int)str.length);
}

static void GPU_PrintC(GPU_StrBuilder *builder, const char *str) {
	DS_VecPushN(builder, str, (int)strlen(str));
}

static GPU_String GPU_ParseUntil(GPU_String *remaining, char until_character) {
	GPU_String line = {remaining->data, 0};

	for (; remaining->length > 0; ) {
		char c = remaining->data[0];
		remaining->data += 1;
		remaining->length -= 1;
		
		if (c == until_character) break;
		line.length += 1;
	}

	return line;
}

static bool GPU_ParseInt(GPU_String s, int64_t *out_value) {
	int64_t value = 0;
	int i = 0;
	for (; i < s.length; i++) {
		int c = s.data[i];
		int digit;
		if (c >= '0' && c <= '9') digit = c - '0';
		else break;
		value = value * 10 + digit;
	}
	*out_value = value;
	return i == s.length;
}

static void GPU_PrintI(GPU_StrBuilder *builder, int64_t value) {
	char buffer[100];
	int offset = 0;
	int64_t remaining = value;

	bool is_negative = remaining < 0;
	if (is_negative) remaining = -remaining;

	for (;;) {
		int64_t temp = remaining;
		remaining /= 10;

		int64_t digit = temp - remaining * 10;
		buffer[offset++] = "0123456789"[digit];

		if (remaining == 0) break;
	}
	if (is_negative) buffer[offset++] = '-';

	int i = 0, j = offset - 1;
	while (i < j) { // Reverse the string
		char temp = buffer[j];
		buffer[j] = buffer[i];
		buffer[i] = temp;
		i++;
		j--;
	}

	GPU_String str = {buffer, offset};
	GPU_PrintS(builder, str);
}

static void GPU_CheckVK(VkResult err) {
	if (err != 0) {
		//String err_str = int_to_str(GPU.temp_allocator, AS_BYTES(err), false);
		//print(LIT("[vulkan] Error: VkResult = "), err_str, LIT("\n"));
		GPU_Check(false);
	}
}

VKAPI_ATTR static VkBool32 GPU_DebugReport(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objectType,
	uint64_t object, size_t location, int32_t messageCode, const char *pLayerPrefix, const char *pMessage, void *pUserData)
{
	//(void)flags; (void)object; (void)location; (void)messageCode; (void)pUserData; (void)pLayerPrefix; // Unused arguments
	if ((flags & VK_DEBUG_REPORT_ERROR_BIT_EXT) || (flags & VK_DEBUG_REPORT_WARNING_BIT_EXT)) {
		// GPU_DebugLog("FGPU: ~c\n", pMessage);
		
		GPU_Check(false);

		//Crash(); //print(LIT("[vulkan] "), STRING_FROM_CSTR_(pMessage), LIT("\n\n"));
		//fprintf(stderr, "[vulkan] Debug report from ObjectType: %i\nMessage: %s\n\n", objectType, pMessage);
	}

	return VK_FALSE;
}

#if 0
static GPU_IndexAllocator GPU_MakeIndexAllocator(DS_Arena *arena, uint32_t capacity) {
	GPU_IndexAllocator result = {
		.first_free_index = GPU_INDEX_ALLOCATOR_FREELIST_END,
		.ceiling		  = 0,
	};
	ArrayResizeUndef(arena, &result.buffer, capacity);
	return result;
}

static uint32_t GPU_IndexAllocatorAdd(GPU_IndexAllocator *index_allocator) {
	Assert(index_allocator->first_free_index != GPU_INDEX_ALLOCATOR_ALIVE_SLOT);
	uint32_t index;

	if (index_allocator->first_free_index == GPU_INDEX_ALLOCATOR_FREELIST_END) {
		// allocate from the end
		Assert(index_allocator->ceiling < index_allocator->buffer.length);
		index = index_allocator->ceiling++;
	}
	else {
		// take the first free index
		index = index_allocator->first_free_index;
		index_allocator->first_free_index = ArrayGet(index_allocator->buffer, index);
	}

	ArraySet(index_allocator->buffer, index, GPU_INDEX_ALLOCATOR_ALIVE_SLOT);
	return index;
}

static void GPU_IndexAllocatorRemove(GPU_IndexAllocator *index_allocator, uint32_t index) {
	// insert the index to the free list
	ArraySet(index_allocator->buffer, index, index_allocator->first_free_index);
	index_allocator->first_free_index = index;
}
#endif

inline GPU_Entity *GPU_NewEntity() {
	GPU_Entity *entity = (GPU_Entity*)DS_TakeSlot(&FGPU.entities);
	memset(entity, 0, sizeof(*entity));
	return entity;
}

inline void GPU_FreeEntity(GPU_Entity *entity) {
	DS_FreeSlot(&FGPU.entities, entity);
}

static void GPU_DestroySemaphore(VkSemaphore semaphore) {
	DS_ProfEnter();
	vkDestroySemaphore(FGPU.device, semaphore, NULL);
	DS_ProfExit();
}

static void GPU_DestroySwapchain(GPU_Swapchain *swapchain) {
	DS_ProfEnter();
	for (int i = 0; i < GPU_SWAPCHAIN_IMG_COUNT; i++) {
		vkDestroyImageView(FGPU.device, swapchain->textures[i].img_view, NULL);
	}
	vkDestroySwapchainKHR(FGPU.device, swapchain->vk_handle, NULL);
	DS_ProfExit();
}

static VkImageMemoryBarrier GPU_ImageBarrier(VkImage img, VkImageAspectFlags aspect,
	uint32_t baseArrayLayer, uint32_t layerCount, uint32_t baseMipLevel, uint32_t levelCount,
	VkImageLayout src_layout, VkImageLayout dst_layout,
	VkAccessFlags src_mask, VkAccessFlags dst_mask)
{
	VkImageMemoryBarrier barrier = { VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER };
	barrier.srcAccessMask = src_mask;
	barrier.dstAccessMask = dst_mask;
	barrier.oldLayout = src_layout;
	barrier.newLayout = dst_layout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = img;
	barrier.subresourceRange.aspectMask = aspect;
	barrier.subresourceRange.baseMipLevel = baseMipLevel;
	barrier.subresourceRange.levelCount   = levelCount;
	barrier.subresourceRange.baseArrayLayer = baseArrayLayer;
	barrier.subresourceRange.layerCount	 = layerCount;
	return barrier;
};

static VkFormat GPU_GetVkFormat(GPU_Format format) {
	switch (format) {
	case GPU_Format_R8UN:			 return VK_FORMAT_R8_UNORM;
	case GPU_Format_RG8UN:			return VK_FORMAT_R8G8_UNORM;
	case GPU_Format_RGBA8UN:		  return VK_FORMAT_R8G8B8A8_UNORM;
	case GPU_Format_BGRA8UN:		  return VK_FORMAT_B8G8R8A8_UNORM;
	case GPU_Format_R16F:			 return VK_FORMAT_R16_SFLOAT;
	case GPU_Format_RG16F:			return VK_FORMAT_R16G16_SFLOAT;
	case GPU_Format_RGB16F:		   return VK_FORMAT_R16G16B16_SFLOAT;
	case GPU_Format_RGBA16F:		  return VK_FORMAT_R16G16B16A16_SFLOAT;
	case GPU_Format_R32F:			 return VK_FORMAT_R32_SFLOAT;
	case GPU_Format_RG32F:			return VK_FORMAT_R32G32_SFLOAT;
	case GPU_Format_RGB32F:		   return VK_FORMAT_R32G32B32_SFLOAT;
	case GPU_Format_RGBA32F:		  return VK_FORMAT_R32G32B32A32_SFLOAT;
	case GPU_Format_R8I:			  return VK_FORMAT_R8_UINT;
	case GPU_Format_R16I:			 return VK_FORMAT_R16_UINT;
	case GPU_Format_RG16I:			return VK_FORMAT_R16G16_UINT;
	case GPU_Format_RGBA16I:		  return VK_FORMAT_R16G16B16A16_UINT;
	case GPU_Format_R32I:			 return VK_FORMAT_R32_UINT;
	case GPU_Format_RG32I:			return VK_FORMAT_R32G32_UINT;
	case GPU_Format_RGB32I:		   return VK_FORMAT_R32G32B32_UINT;
	case GPU_Format_RGBA32I:		  return VK_FORMAT_R32G32B32A32_UINT;
	case GPU_Format_R64I:			 return VK_FORMAT_R64_UINT;
	case GPU_Format_D16UN:			return VK_FORMAT_D16_UNORM;
	case GPU_Format_D32F_Or_X8D24UN:  return VK_FORMAT_D32_SFLOAT; // @robustness
	case GPU_Format_D32FS8I_Or_D24UNS8I: return VK_FORMAT_D32_SFLOAT_S8_UINT; // @robustness
	case GPU_Format_D24UNS8I_Or_D32FS8I: return VK_FORMAT_D24_UNORM_S8_UINT; // @robustness
	case GPU_Format_BC1_RGB_UN:	   return VK_FORMAT_BC1_RGB_UNORM_BLOCK;
	case GPU_Format_BC1_RGBA_UN:	  return VK_FORMAT_BC1_RGBA_UNORM_BLOCK;
	case GPU_Format_BC3_RGBA_UN:	  return VK_FORMAT_BC3_UNORM_BLOCK;
	case GPU_Format_BC5_UN:		   return VK_FORMAT_BC5_UNORM_BLOCK;
	case GPU_Format_Invalid:	 break;
	}
	return VK_FORMAT_UNDEFINED;
}

static void GPU_MaybeRecreateSwapchain(void) {
	DS_ProfEnter();

	VkSurfaceCapabilitiesKHR caps;
	VkResult surface_caps_result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(FGPU.physical_device, FGPU.surface, &caps);
	
	// After closing a window, FGPU_GraphWait() may still be called and surface_caps_result may then return VK_ERROR_SURFACE_LOST_KHR.
	GPU_Check(surface_caps_result == VK_SUCCESS || surface_caps_result == VK_ERROR_SURFACE_LOST_KHR);
	
	uint32_t width = caps.currentExtent.width, height = caps.currentExtent.height;
	
	if (surface_caps_result == VK_SUCCESS &&
		width != 0 && height != 0 &&
		(width != FGPU.swapchain.width || height != FGPU.swapchain.height))
	{
		// printf("RECREATE SWAPCHAIN!\n");

		GPU_CheckVK(vkDeviceWaitIdle(FGPU.device));

		GPU_Swapchain old_swapchain = FGPU.swapchain;
		
		memset(&FGPU.swapchain, 0, sizeof(FGPU.swapchain));
		FGPU.swapchain.gen_id = old_swapchain.gen_id + 1;
		FGPU.swapchain.width = width;
		FGPU.swapchain.height = height;
		
		VkSwapchainCreateInfoKHR info = { VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR };
		info.surface = FGPU.surface;
		info.minImageCount = GPU_SWAPCHAIN_IMG_COUNT;
		info.imageFormat = GPU_GetVkFormat(GPU_SWAPCHAIN_FORMAT);
		info.imageColorSpace = VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;
		info.imageExtent.width = FGPU.swapchain.width;
		info.imageExtent.height = FGPU.swapchain.height;
		info.imageArrayLayers = 1;
		info.imageUsage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
		info.queueFamilyIndexCount = 1;
		info.pQueueFamilyIndices = &FGPU.queue_family;
		info.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
		info.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
		info.presentMode = VK_PRESENT_MODE_FIFO_KHR; //VK_PRESENT_MODE_MAILBOX_KHR; // enable vsync
		info.oldSwapchain = old_swapchain.vk_handle;
		GPU_CheckVK(vkCreateSwapchainKHR(FGPU.device, &info, NULL, &FGPU.swapchain.vk_handle));

		uint32_t images_count = GPU_SWAPCHAIN_IMG_COUNT;
		VkImage swapchain_images[GPU_SWAPCHAIN_IMG_COUNT];
		GPU_CheckVK(vkGetSwapchainImagesKHR(FGPU.device, FGPU.swapchain.vk_handle, &images_count, &swapchain_images[0]));

		// Emulate the behaviour of GPU_MakeTexture() for the swapchain textures

		for (int i = 0; i < GPU_SWAPCHAIN_IMG_COUNT; i++) {
			GPU_TextureImpl *texture_impl = &FGPU.swapchain.textures[i];
			memset(texture_impl, 0, sizeof(*texture_impl));
			texture_impl->idle_layout_ = VK_IMAGE_LAYOUT_UNDEFINED;
			texture_impl->vk_handle = swapchain_images[i];
			texture_impl->base.depth = 1;
			texture_impl->base.mip_level_count = 1;
			texture_impl->base.layer_count = 1;
			texture_impl->base.width = width;
			texture_impl->base.height = height;
			texture_impl->base.format = GPU_SWAPCHAIN_FORMAT;
			texture_impl->base.flags = GPU_TextureFlag_RenderTarget | GPU_TextureFlag_SwapchainTarget;
			
			VkImageViewCreateInfo img_view_info = { VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO };
			img_view_info.image = swapchain_images[i];
			img_view_info.viewType = VK_IMAGE_VIEW_TYPE_2D;
			img_view_info.format = info.imageFormat;
			img_view_info.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
			img_view_info.subresourceRange.baseMipLevel = 0;
			img_view_info.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS;
			img_view_info.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
			GPU_CheckVK(vkCreateImageView(FGPU.device, &img_view_info, NULL, &texture_impl->img_view));
		}

		if (old_swapchain.vk_handle != 0) {
			GPU_DestroySwapchain(&old_swapchain);
		}
	}
	DS_ProfExit();
}

GPU_API GPU_Sampler *GPU_SamplerLinearWrap() { return FGPU.sampler_linear_wrap; }
GPU_API GPU_Sampler *GPU_SamplerLinearClamp() { return FGPU.sampler_linear_clamp; }
GPU_API GPU_Sampler *GPU_SamplerLinearMirror() { return FGPU.sampler_linear_mirror; }
GPU_API GPU_Sampler *GPU_SamplerNearestClamp() { return FGPU.sampler_nearest_clamp; }
GPU_API GPU_Sampler *GPU_SamplerNearestWrap() { return FGPU.sampler_nearest_wrap; }
GPU_API GPU_Sampler *GPU_SamplerNearestMirror() { return FGPU.sampler_nearest_mirror; }

GPU_API GPU_Sampler *GPU_MakeSampler(const GPU_SamplerDesc *desc) {
	DS_ProfEnter();
	VkSampler sampler;
	VkSamplerAddressMode vk_address_modes[3] = {VK_SAMPLER_ADDRESS_MODE_REPEAT, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE, VK_SAMPLER_ADDRESS_MODE_MIRRORED_REPEAT};
		
	VkSamplerCreateInfo info = {VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO};
	info.minFilter = desc->min_filter == GPU_Filter_Linear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
	info.magFilter = desc->mag_filter == GPU_Filter_Linear ? VK_FILTER_LINEAR : VK_FILTER_NEAREST;
	info.addressModeU = vk_address_modes[desc->address_modes[0]];
	info.addressModeV = vk_address_modes[desc->address_modes[1]];
	info.addressModeW = vk_address_modes[desc->address_modes[2]];
	info.anisotropyEnable = false;
	info.maxAnisotropy = 1.f;
	info.compareEnable = desc->compare_op != GPU_CompareOp_Never;
	info.compareOp = (VkCompareOp)desc->compare_op;
	info.mipmapMode = desc->mipmap_mode == GPU_Filter_Linear ? VK_SAMPLER_MIPMAP_MODE_LINEAR : VK_SAMPLER_MIPMAP_MODE_NEAREST;
	info.minLod = desc->min_lod;
	info.maxLod = desc->max_lod;
	GPU_CheckVK(vkCreateSampler(FGPU.device, &info, NULL, &sampler));
	DS_ProfExit();
	return (GPU_Sampler*)sampler;
};

GPU_API void GPU_DestroySampler(GPU_Sampler *sampler) {
	if (sampler) {
		vkDestroySampler(FGPU.device, (VkSampler)sampler, NULL);
	}
}

GPU_API GPU_PipelineLayout *GPU_InitPipelineLayout() {
	GPU_PipelineLayout *layout = &GPU_NewEntity()->pipeline_layout;
	DS_VecInit(&layout->bindings);
	//BucketListInitUsingDS_SlotAllocator(&layout->bindings, &FGPU.entities);
	return layout;
}

static GPU_Binding GPU_AddBinding(GPU_PipelineLayout *layout, const char *name, GPU_ResourceKind kind, GPU_Format image_format) {
	GPU_Binding binding = (uint32_t)layout->bindings.length;
		
	GPU_BindingInfo binding_info = {kind, image_format, name};
	DS_VecPush(&layout->bindings, binding_info);
	//BucketListIndex end = BucketListGetEnd(&layout->bindings);
	//BucketListPush(&layout->bindings, &binding_info);
	//layout->bindings_count++;
	//GPU_Binding binding = {end.bucket, end.slot_index};

	return binding;
}

GPU_API GPU_Binding GPU_TextureBinding(GPU_PipelineLayout *layout, const char *name) { return GPU_AddBinding(layout, name, GPU_ResourceKind_Texture, GPU_Format_Invalid); }

GPU_API GPU_Binding GPU_SamplerBinding(GPU_PipelineLayout *layout, const char *name) { return GPU_AddBinding(layout, name, GPU_ResourceKind_Sampler, GPU_Format_Invalid); }

GPU_API GPU_Binding GPU_BufferBinding(GPU_PipelineLayout *layout, const char *name)  { return GPU_AddBinding(layout, name, GPU_ResourceKind_Buffer, GPU_Format_Invalid); }

GPU_API GPU_Binding GPU_StorageImageBinding(GPU_PipelineLayout *layout, const char *name, GPU_Format image_format) { return GPU_AddBinding(layout, name, GPU_ResourceKind_StorageImage, image_format); }

GPU_API void GPU_DestroyPipelineLayout(GPU_PipelineLayout *layout) {
	if (layout) {
		DS_VecDestroy(&layout->bindings);
		vkDestroyPipelineLayout(FGPU.device, layout->vk_handle, NULL);
		vkDestroyDescriptorSetLayout(FGPU.device, layout->descriptor_set_layout, NULL);
		GPU_FreeEntity((GPU_Entity*)layout);
	}
}

GPU_API void GPU_FinalizePipelineLayout(GPU_PipelineLayout *layout) {
	DS_ProfEnter();
		
	GPU_Check(layout->descriptor_set_layout == 0);
	DS_ArenaMark T = DS_ArenaGetMark(&FGPU.temp_arena);

	DS_Vec(VkDescriptorSetLayoutBinding) vk_bindings = {0};
	vk_bindings.arena = &FGPU.temp_arena;
	DS_VecResizeUndef(&vk_bindings, layout->bindings.length);
		
	uint32_t i = 0;
	DS_ForVecEach(GPU_BindingInfo, &layout->bindings, it) {
		VkDescriptorSetLayoutBinding vk_binding = {0};
		vk_binding.binding = i;
		vk_binding.descriptorCount = 1;
		vk_binding.stageFlags = VK_SHADER_STAGE_ALL;
		
		switch (it.elem->kind) {
		case GPU_ResourceKind_Texture: { vk_binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE; } break;
		case GPU_ResourceKind_Sampler: { vk_binding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER; } break;
		case GPU_ResourceKind_Buffer:  { vk_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; } break;
		case GPU_ResourceKind_StorageImage: { vk_binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE; } break;
		}

		DS_VecSet(vk_bindings, i, vk_binding);
		i++;
	}
		
	VkDescriptorSetLayoutCreateInfo layout_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
	layout_info.bindingCount = (uint32_t)vk_bindings.length;
	layout_info.pBindings = vk_bindings.data;
	GPU_CheckVK(vkCreateDescriptorSetLayout(FGPU.device, &layout_info, NULL, &layout->descriptor_set_layout));

	// Create pipeline layout
	// hmm... should we do this per descriptor set layout then?
	VkPushConstantRange push_constant_range = {0};
	push_constant_range.stageFlags = VK_SHADER_STAGE_ALL;
	push_constant_range.size = 128;

	VkPipelineLayoutCreateInfo pipeline_layout_info = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipeline_layout_info.setLayoutCount = 1;
	pipeline_layout_info.pSetLayouts = &layout->descriptor_set_layout;
	pipeline_layout_info.pushConstantRangeCount = 1;
	pipeline_layout_info.pPushConstantRanges = &push_constant_range;
	GPU_CheckVK(vkCreatePipelineLayout(FGPU.device, &pipeline_layout_info, NULL, &layout->vk_handle));
		
	DS_ArenaSetMark(&FGPU.temp_arena, T);
	DS_ProfExit();
}

GPU_API GPU_DescriptorSet *GPU_InitDescriptorSet(GPU_DescriptorArena *descriptor_arena, GPU_PipelineLayout *pipeline_layout) {
	DS_ProfEnter();
	GPU_DescriptorSet *set;
	if (descriptor_arena) {
		set = DS_New(GPU_DescriptorSet, &descriptor_arena->arena);
		DS_VecInitA(&set->bindings, &descriptor_arena->arena);
	}
	else {
		set = &GPU_NewEntity()->descriptor_set;
		DS_VecInit(&set->bindings);
	}
	set->descriptor_arena = descriptor_arena;
	set->pipeline_layout = pipeline_layout;
	DS_ProfExit();
	return set;
}

GPU_API void GPU_DestroyDescriptorSet(GPU_DescriptorSet *set) {
	if (set) {
		GPU_Check(set->descriptor_arena == NULL);
		GPU_Check(set->global_descriptor_pool != NULL);
		DS_VecDestroy(&set->bindings);
		GPU_CheckVK(vkFreeDescriptorSets(FGPU.device, set->global_descriptor_pool, 1, &set->vk_handle));
	}
}

// `mip_level` may be GPU_MIP_LEVEL_ALL
static void GPU_SetBinding(GPU_DescriptorSet *set, uint32_t binding, void *value, GPU_ResourceKind kind, uint32_t mip_level) {
	DS_ProfEnter();
	if (value == NULL) {
		GPU_TODO(); // We should figure out what to do with passing NULL descriptor to cubemap textures, as vulkan gives a validation error if the nil image has the wrong image view type
		
		//switch (kind) {
		//case GPU_ResourceKind_Buffer:  { value = FGPU.nil_storage_buffer; } break;
		//case GPU_ResourceKind_Sampler: { value = FGPU.nil_sampler; } break;
		//case GPU_ResourceKind_Texture: { value = FGPU.nil_storage_image; } break;
		//case GPU_ResourceKind_StorageImage: { value = FGPU.nil_storage_image; } break;
		//}
	}

	GPU_BindingInfo binding_info = DS_VecGet(set->pipeline_layout->bindings, binding);
	GPU_Check(binding_info.kind == kind);
	if (kind == GPU_ResourceKind_StorageImage) {
		GPU_TextureImpl *texture = (GPU_TextureImpl*)value;
		GPU_Check(texture->base.flags & GPU_TextureFlag_StorageImage);
		// GPU_Check(texture->base.format == binding_info.image_format); // Passed format must match the expected format
	}

	if (binding >= (uint32_t)set->bindings.length) {
		GPU_BindingValue empty = {0};
		DS_VecResize(&set->bindings, empty, binding + 1);
	}

	GPU_BindingValue binding_value = {value, mip_level};
	DS_VecSet(set->bindings, binding, binding_value);
	DS_ProfExit();
}

GPU_API void GPU_SetTextureBinding(GPU_DescriptorSet *set, uint32_t binding, GPU_Texture *value) { GPU_SetBinding(set, binding, value, GPU_ResourceKind_Texture, GPU_MIP_LEVEL_ALL); }
GPU_API void GPU_SetSamplerBinding(GPU_DescriptorSet *set, uint32_t binding, GPU_Sampler *value) { GPU_SetBinding(set, binding, value, GPU_ResourceKind_Sampler, 0); }
GPU_API void GPU_SetBufferBinding(GPU_DescriptorSet *set, uint32_t binding, GPU_Buffer *value)   { GPU_SetBinding(set, binding, value, GPU_ResourceKind_Buffer, 0); }
GPU_API void GPU_SetStorageImageBinding(GPU_DescriptorSet *set, uint32_t binding, GPU_Texture *value, uint32_t mip_level) {
	GPU_SetBinding(set, binding, value, GPU_ResourceKind_StorageImage, mip_level);
}

void GPU_SetTextureMipBinding(GPU_DescriptorSet *set, uint32_t binding, GPU_Texture *value, uint32_t mip_level) {
	GPU_Check(value->flags & GPU_TextureFlag_PerMipBinding);
	GPU_SetBinding(set, binding, value, GPU_ResourceKind_Texture, mip_level);
}

static GPU_Format GPU_GetIntegerFormat(uint32_t size) {
	switch (size) {
	case 1: return GPU_Format_R8I;
	case 2: return GPU_Format_R16I;
	case 4: return GPU_Format_R32I;
	case 8: return GPU_Format_R64I;
	default: return GPU_Format_Invalid;
	}
}

static void GPU_FinalizeDescriptorSetEx(GPU_DescriptorSet *set, bool check_for_completeness) {
	DS_ProfEnter();
	GPU_Check(set->vk_handle == 0); // Did you already call finalize?

	DS_ArenaMark T = DS_ArenaGetMark(&FGPU.temp_arena);

	VkDescriptorPool descriptor_pool = NULL;
		
	if (set->descriptor_arena == NULL) {
		if (FGPU.global_descriptor_pool == NULL) {
			VkDescriptorPoolSize pool_sizes[4];
			pool_sizes[0].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			pool_sizes[0].descriptorCount = 256; // does this mean the maximum number of descriptors per one descriptor set, or maximum number of descriptors in total for all descriptor sets in this pool? It seems to be the latter.
			pool_sizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLER;
			pool_sizes[1].descriptorCount = 256;
			pool_sizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			pool_sizes[2].descriptorCount = 256;
			pool_sizes[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
			pool_sizes[3].descriptorCount = 256;

			VkDescriptorPoolCreateInfo pool_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
			pool_info.poolSizeCount = GPU_ArrayCount(pool_sizes);
			pool_info.pPoolSizes = pool_sizes;
			pool_info.maxSets = 512;
			pool_info.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
			
			GPU_CheckVK(vkCreateDescriptorPool(FGPU.device, &pool_info, NULL, &descriptor_pool));
			FGPU.global_descriptor_pool = descriptor_pool;
		}
		set->global_descriptor_pool = FGPU.global_descriptor_pool;
		descriptor_pool = FGPU.global_descriptor_pool;
	}
	else {
		descriptor_pool = set->descriptor_arena->pool;
	}
	// TODO: we should keep track of the limits and allocate a new pool if we go over them. Now, we will just error and crash.

	VkDescriptorSetAllocateInfo descriptor_set_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO };
	descriptor_set_info.descriptorPool = descriptor_pool;
	descriptor_set_info.descriptorSetCount = 1;
	descriptor_set_info.pSetLayouts = &set->pipeline_layout->descriptor_set_layout;
	if (vkAllocateDescriptorSets(FGPU.device, &descriptor_set_info, &set->vk_handle) != VK_SUCCESS) {
		GPU_TODO(); // Allocate a new descriptor pool
	}
		
	if (check_for_completeness) GPU_Check(set->bindings.length == set->pipeline_layout->bindings.length); // Did you remember to call GPU_Set[*]Binding on all of the binding slots?
		
	DS_Vec(VkWriteDescriptorSet) writes = {0};
	writes.arena = &FGPU.temp_arena;
	DS_VecReserve(&writes, set->bindings.length);
		
	for (uint32_t i = 0; i < (uint32_t)set->bindings.length; i++) {
		GPU_BindingInfo binding_info = DS_VecGet(set->pipeline_layout->bindings, i);
		GPU_BindingValue binding_value = DS_VecGet(set->bindings, i);
		if (check_for_completeness) GPU_Check(binding_value.ptr != NULL); // Did you remember to call GPU_Set[*]Binding on this binding?

		VkWriteDescriptorSet write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
		write.dstSet = set->vk_handle;
		write.dstBinding = i;
		write.dstArrayElement = 0;
		write.descriptorCount = 1;

		switch (binding_info.kind) {
		case GPU_ResourceKind_Texture: {
			GPU_TextureImpl *texture = (GPU_TextureImpl*)binding_value.ptr;
			write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
			
			VkImageView image_view = binding_value.mip_level == GPU_MIP_LEVEL_ALL ?
				texture->img_view : texture->mip_level_img_views[binding_value.mip_level];

			VkDescriptorImageInfo *info = DS_New(VkDescriptorImageInfo, &FGPU.temp_arena);
			info->imageView = image_view;
			info->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			write.pImageInfo = info;
		} break;

		case GPU_ResourceKind_Sampler: {
			GPU_Sampler *sampler = (GPU_Sampler*)binding_value.ptr;
			
			VkDescriptorImageInfo *info = DS_New(VkDescriptorImageInfo, &FGPU.temp_arena);
			info->sampler = (VkSampler)sampler;
			info->imageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			
			write.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
			write.pImageInfo = info;
		} break;

		case GPU_ResourceKind_Buffer: {
			GPU_Buffer *buffer = (GPU_Buffer*)binding_value.ptr;
			GPU_Check((buffer->flags & GPU_BufferFlag_StorageBuffer) && (buffer->flags & GPU_BufferFlag_GPU));
			
			VkDescriptorBufferInfo *info = DS_New(VkDescriptorBufferInfo, &FGPU.temp_arena);
			info->buffer = ((GPU_BufferImpl*)buffer)->vk_handle;
			info->range = buffer->size;

			write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
			write.pBufferInfo = info;
		} break;

		case GPU_ResourceKind_StorageImage: {
			GPU_TextureImpl *texture = (GPU_TextureImpl*)binding_value.ptr;
			GPU_Check(texture->base.flags & GPU_TextureFlag_StorageImage);
			
			VkImageView img_view = texture->mip_level_img_views[binding_value.mip_level];
			
			if (binding_info.image_format != texture->base.format) {
				// use atomics image view
				GPU_Format integer_format = GPU_GetIntegerFormat(GPU_GetFormatInfo(texture->base.format).block_size);
				GPU_Check(binding_info.image_format == integer_format);
				GPU_Check(texture->atomics_img_view);
				if (binding_value.mip_level != 0) GPU_TODO();

				img_view = texture->atomics_img_view;
			}

			VkDescriptorImageInfo *info = DS_New(VkDescriptorImageInfo, &FGPU.temp_arena);
			info->imageView = img_view;
			info->imageLayout = VK_IMAGE_LAYOUT_GENERAL;
			
			write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
			write.pImageInfo = info;
		} break;
		}
		
		DS_VecPush(&writes, write);
	}
		
	if (writes.length > 0) {
		vkUpdateDescriptorSets(FGPU.device, (uint32_t)writes.length, writes.data, 0, NULL);
	}

	DS_ArenaSetMark(&FGPU.temp_arena, T);
	DS_ProfExit();
}

GPU_API void GPU_FinalizeDescriptorSet(GPU_DescriptorSet *set) {
	GPU_FinalizeDescriptorSetEx(set, true);
}

static GPU_Sampler *MakeCommonSampler(GPU_AddressMode address_mode, GPU_Filter filter) {
	GPU_SamplerDesc desc = {0};
	for (int i = 0; i < 3; i++) desc.address_modes[i] = address_mode;
	desc.min_filter = filter;
	desc.mag_filter = filter;
	desc.mipmap_mode = filter;
	desc.max_lod = 1000.f;
	return GPU_MakeSampler(&desc);
}

GPU_API void GPU_Init(GPU_WindowHandle window) {
	DS_ProfEnter();

	glslang_initialize_process();

	memset(&FGPU, 0, sizeof(FGPU));
	FGPU.window = window;
	DS_InitArena(&FGPU.persistent_arena, DS_KIB(4));
	DS_InitArena(&FGPU.temp_arena, DS_KIB(1));
		
	DS_SlotAllocatorInitA(&FGPU.entities, &FGPU.persistent_arena);

	DS_ArenaMark T = DS_ArenaGetMark(&FGPU.temp_arena);

	{ // Create Vulkan Instance
		const char *extensions[] = {
			"VK_KHR_surface",
			"VK_EXT_debug_report",
			"VK_KHR_win32_surface",
		};

		// VK_EXT_conservative_rasterization
		VkApplicationInfo app_info = {VK_STRUCTURE_TYPE_APPLICATION_INFO};
		app_info.pApplicationName = "MyApplication";
		app_info.apiVersion = VK_MAKE_VERSION(1, 3, 0); // TODO: make this work for older versions

		#if GPU_ENABLE_VALIDATION
		const char *validation_layers[] = {"VK_LAYER_KHRONOS_validation"};
		#endif
		
		VkInstanceCreateInfo create_info = {VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO};
		create_info.enabledExtensionCount = GPU_ArrayCount(extensions);
		create_info.ppEnabledExtensionNames = extensions;
		create_info.pApplicationInfo = &app_info;
		#if GPU_ENABLE_VALIDATION
		create_info.enabledLayerCount = GPU_ArrayCount(validation_layers);
		create_info.ppEnabledLayerNames = validation_layers;
		#endif
		
		GPU_CheckVK(vkCreateInstance(&create_info, NULL, &FGPU.instance));

#if GPU_ENABLE_VALIDATION
		// Get the function pointer (required for any extensions)
		PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(FGPU.instance, "vkCreateDebugReportCallbackEXT");
		GPU_Check(vkCreateDebugReportCallbackEXT != NULL);
		// Setup the debug report callback
		VkDebugReportCallbackCreateInfoEXT debug_report_ci = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT };
		debug_report_ci.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT | VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
		debug_report_ci.pfnCallback = GPU_DebugReport;
		debug_report_ci.pUserData = NULL;
		GPU_CheckVK(vkCreateDebugReportCallbackEXT(FGPU.instance, &debug_report_ci, NULL, &FGPU.debug_callback));
#endif
	}

	{ // Select GPU and create physical device
		uint32_t gpu_count;
		GPU_CheckVK(vkEnumeratePhysicalDevices(FGPU.instance, &gpu_count, NULL));
		GPU_Check(gpu_count > 0);

		VkPhysicalDevice *gpus = (VkPhysicalDevice*)DS_ArenaPush(&FGPU.temp_arena, gpu_count * sizeof(VkPhysicalDevice));
		GPU_CheckVK(vkEnumeratePhysicalDevices(FGPU.instance, &gpu_count, gpus));

		// If a number > 1 of GPUs got reported, find discrete GPU if present, or use first one available.
		int use_gpu = 0;
		VkPhysicalDeviceProperties properties;
		for (int i = 0; i < (int)gpu_count; i++) {
			vkGetPhysicalDeviceProperties(gpus[i], &properties);
			
			if (properties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
				use_gpu = i;
				break;
			}
		}

		FGPU.physical_device = gpus[use_gpu];
		
		//VkPhysicalDeviceLimits *limits = &properties.limits;
		vkGetPhysicalDeviceMemoryProperties(FGPU.physical_device, &FGPU.mem_properties);
	}

	{ // Select graphics queue family
		uint32_t count;
		vkGetPhysicalDeviceQueueFamilyProperties(FGPU.physical_device, &count, NULL);
		VkQueueFamilyProperties *queues = (VkQueueFamilyProperties*)DS_ArenaPush(&FGPU.temp_arena, count * sizeof(VkQueueFamilyProperties));
		vkGetPhysicalDeviceQueueFamilyProperties(FGPU.physical_device, &count, queues);
		
		int queue_family = -1;
		for (int i = 0; i < (int)count; i++) {
			if ((queues[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queues[i].queueFlags & VK_QUEUE_COMPUTE_BIT)) {
				queue_family = i; // must support both graphics and compute
				break;
			}
		}
		GPU_Check(queue_family != -1);
		FGPU.queue_family = (uint32_t)queue_family;
	}

	{ // Create device (with 1 queue)
		const char *device_extensions[] = {
			VK_KHR_SWAPCHAIN_EXTENSION_NAME,
			VK_EXT_CONSERVATIVE_RASTERIZATION_EXTENSION_NAME,
			VK_KHR_SHADER_ATOMIC_INT64_EXTENSION_NAME, // 64 bit atomics are widely supported (core in vulkan 1.2)
			VK_EXT_SHADER_IMAGE_ATOMIC_INT64_EXTENSION_NAME,
			VK_KHR_MAINTENANCE2_EXTENSION_NAME,
			VK_KHR_SHADER_FLOAT16_INT8_EXTENSION_NAME,
			// "VK_KHR_dynamic_rendering",
			// "VK_EXT_descriptor_indexing",
			// "VK_KHR_shader_non_semantic_info", // for shader printf
		};
		const float queue_priority[] = { 1.0f };
		
		VkDeviceQueueCreateInfo queue_info[1] = {{0}};
		queue_info[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queue_info[0].queueFamilyIndex = FGPU.queue_family;
		queue_info[0].queueCount = 1;
		queue_info[0].pQueuePriorities = queue_priority;

		VkPhysicalDeviceFloat16Int8FeaturesKHR features_4 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_FLOAT16_INT8_FEATURES};
		features_4.shaderFloat16 = true;

		VkPhysicalDeviceShaderImageAtomicInt64FeaturesEXT features_3 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_IMAGE_ATOMIC_INT64_FEATURES_EXT};
		features_3.shaderImageInt64Atomics = true;

		VkPhysicalDeviceShaderAtomicInt64FeaturesKHR features_2 = {VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_SHADER_ATOMIC_INT64_FEATURES_KHR};
		features_2.shaderBufferInt64Atomics = true; // do we need this?
		features_2.shaderSharedInt64Atomics = true; // do we need this?

		VkPhysicalDeviceFeatures features = {0};
		features.shaderInt64 = true; // required for 64 bit atomics I think

		// features.geometryShader = true;
		// features.fillModeNonSolid = true; // wireframe rendering
		features.vertexPipelineStoresAndAtomics = true;
		features.fragmentStoresAndAtomics = true;

		//VkPhysicalDeviceVulkan12Features vk_1_2_features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_2_FEATURES };
		//vk_1_2_features.descriptorIndexing = true;
		//vk_1_2_features.shaderSampledImageArrayNonUniformIndexing = true;
		//vk_1_2_features.shaderStorageTexelBufferArrayDynamicIndexing = true;
		//vk_1_2_features.shaderStorageBufferArrayNonUniformIndexing = true;
		//vk_1_2_features.shaderInputAttachmentArrayDynamicIndexing = true;
		//vk_1_2_features.shaderInputAttachmentArrayNonUniformIndexing = true;
		//vk_1_2_features.shaderStorageImageArrayNonUniformIndexing = true;
		//vk_1_2_features.runtimeDescriptorArray = true;
		//vk_1_2_features.descriptorBindingPartiallyBound = true;

		//VkPhysicalDeviceVulkan13Features vk_1_3_features = { VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_VULKAN_1_3_FEATURES };
		//vk_1_3_features.dynamicRendering = true;

		VkDeviceCreateInfo device_info = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
		device_info.queueCreateInfoCount = GPU_ArrayCount(queue_info);
		device_info.pQueueCreateInfos = queue_info;
		device_info.enabledExtensionCount = GPU_ArrayCount(device_extensions);
		device_info.ppEnabledExtensionNames = device_extensions;
		device_info.pEnabledFeatures = &features;
		device_info.pNext = &features_2;
		features_2.pNext = &features_3;
		features_3.pNext = &features_4;
		//vk_1_2_features.pNext = &vk_1_3_features;

		GPU_CheckVK(vkCreateDevice(FGPU.physical_device, &device_info, NULL, &FGPU.device));

		vkGetDeviceQueue(FGPU.device, FGPU.queue_family, 0, &FGPU.queue);
	}

	{ // Create command pool
		VkCommandPoolCreateInfo pool_info = { VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO };
		pool_info.flags = VK_COMMAND_POOL_CREATE_TRANSIENT_BIT | VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
		pool_info.queueFamilyIndex = FGPU.queue_family;
		GPU_CheckVK(vkCreateCommandPool(FGPU.device, &pool_info, NULL, &FGPU.cmd_pool));
	}

	{ // Create surface
		VkWin32SurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR };
		createInfo.pNext = NULL;
		createInfo.flags = 0;
		createInfo.hinstance = (void*)GetWindowLongPtrW((HWND)window, -6 /*GWL_HINSTANCE*/);
		createInfo.hwnd = (HWND)window;
		GPU_CheckVK(vkCreateWin32SurfaceKHR(FGPU.instance, &createInfo, NULL, &FGPU.surface));
	}

	//GPU_InitFrameInFlight(&FGPU.old_frame);
	//GPU_InitFrameInFlight(&FGPU.new_frame);

	GPU_MaybeRecreateSwapchain();

	{ // common resources
		FGPU.sampler_linear_clamp = MakeCommonSampler(GPU_AddressMode_Clamp, GPU_Filter_Linear);
		FGPU.sampler_linear_wrap = MakeCommonSampler(GPU_AddressMode_Wrap, GPU_Filter_Linear);
		FGPU.sampler_linear_mirror = MakeCommonSampler(GPU_AddressMode_Mirror, GPU_Filter_Linear);
		FGPU.sampler_nearest_clamp = MakeCommonSampler(GPU_AddressMode_Clamp, GPU_Filter_Nearest);
		FGPU.sampler_nearest_wrap = MakeCommonSampler(GPU_AddressMode_Wrap, GPU_Filter_Nearest);
		FGPU.sampler_nearest_mirror = MakeCommonSampler(GPU_AddressMode_Mirror, GPU_Filter_Nearest);
		FGPU.nil_storage_buffer = GPU_MakeBuffer(1, GPU_BufferFlag_GPU | GPU_BufferFlag_StorageBuffer, NULL);
		FGPU.nil_storage_image = GPU_MakeTexture(GPU_Format_R8UN, 1, 1, 1, GPU_TextureFlag_StorageImage, NULL);
		FGPU.nil_sampler = FGPU.sampler_linear_clamp;
	}

	DS_ArenaSetMark(&FGPU.temp_arena, T);
	DS_ProfExit();
}

GPU_API void GPU_Deinit() {
	DS_ProfEnter();
		
	{ // common resources
		GPU_DestroySampler(FGPU.sampler_linear_clamp);
		GPU_DestroySampler(FGPU.sampler_linear_wrap);
		GPU_DestroySampler(FGPU.sampler_linear_mirror);
		GPU_DestroySampler(FGPU.sampler_nearest_clamp);
		GPU_DestroySampler(FGPU.sampler_nearest_wrap);
		GPU_DestroySampler(FGPU.sampler_nearest_mirror);
		GPU_DestroyBuffer(FGPU.nil_storage_buffer);
		GPU_DestroyTexture(FGPU.nil_storage_image);
	}

	GPU_CheckVK(vkDeviceWaitIdle(FGPU.device));

	vkDestroyCommandPool(FGPU.device, FGPU.cmd_pool, NULL);

	//vkDestroyPipelineLayout(FGPU.device, FGPU.pipeline_layout, NULL);
	vkDestroyDescriptorPool(FGPU.device, FGPU.global_descriptor_pool, NULL);
	//vkDestroyDescriptorSetLayout(FGPU.device, FGPU.descriptor_set_layout, NULL);

	GPU_DestroySwapchain(&FGPU.swapchain);
	vkDestroySurfaceKHR(FGPU.instance, FGPU.surface, NULL);

	// Remove the debug report callback
	PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(FGPU.instance, "vkDestroyDebugReportCallbackEXT");
	vkDestroyDebugReportCallbackEXT(FGPU.instance, FGPU.debug_callback, NULL);

	// TODO: make sure there aren't any unfreed GPU resources.

	vkDestroyDevice(FGPU.device, NULL);
	vkDestroyInstance(FGPU.instance, NULL);
		
	glslang_finalize_process(); // Right now, we use global state for GPU (can't have multiple instances at the same time), so this is fine. But if we change the fact, we need to deal with this being per process, not per thread.  :GLSLangInitNote

	DS_DestroyArena(&FGPU.persistent_arena);
	DS_DestroyArena(&FGPU.temp_arena);

	DS_ProfExit();
}

static bool FindVKMemoryTypeIndex(VkMemoryRequirements requirements, VkMemoryPropertyFlags required_properties, uint32_t *out_index) {
	DS_ProfEnter();
	bool found = false;
	for (uint32_t i = 0; i < FGPU.mem_properties.memoryTypeCount; i++) {
		bool is_required_memory_type = requirements.memoryTypeBits & (1 << i);
		bool has_required_properties = (required_properties & FGPU.mem_properties.memoryTypes[i].propertyFlags) == required_properties;
		if (is_required_memory_type && has_required_properties) {
			*out_index = i;
			found = true;
			break;
		}
	}
	DS_ProfExit();
	return found;
}

GPU_API void GPU_DestroyBuffer(GPU_Buffer *buffer) {
	if (buffer) {
		DS_ProfEnter();
		GPU_BufferImpl *buffer_impl = (GPU_BufferImpl*)buffer;
		vkFreeMemory(FGPU.device, buffer_impl->allocation, NULL);
		vkDestroyBuffer(FGPU.device, buffer_impl->vk_handle, NULL);
		GPU_FreeEntity((GPU_Entity*)buffer);
		DS_ProfExit();
	}
}

GPU_API GPU_Buffer *GPU_MakeBuffer(uint32_t size, GPU_BufferFlags flags, const void *data) {
	DS_ProfEnter();
	GPU_Check((flags & GPU_BufferFlag_CPU) || (flags & GPU_BufferFlag_GPU)); // The buffer must be accessible from either the CPU or the GPU, or both.
	GPU_Check(size > 0);
		
	GPU_BufferImpl *buffer_impl = &GPU_NewEntity()->buffer;
		
	VkBufferCreateInfo info = { VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO };
	info.size = size;
	info.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	if (flags & GPU_BufferFlag_GPU) info.usage |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	if (flags & GPU_BufferFlag_StorageBuffer) info.usage |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;

	buffer_impl->base.flags = flags;
	buffer_impl->base.size = size;

	GPU_CheckVK(vkCreateBuffer(FGPU.device, &info, NULL, &buffer_impl->vk_handle));

	VkMemoryRequirements mem_requirements;
	vkGetBufferMemoryRequirements(FGPU.device, buffer_impl->vk_handle, &mem_requirements);
		
	VkMemoryPropertyFlags required_properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
	if ((flags & GPU_BufferFlag_GPU) && !(flags & GPU_BufferFlag_CPU)) {
		required_properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
	}
		
	uint32_t memory_type_idx;
	GPU_Check(FindVKMemoryTypeIndex(mem_requirements, required_properties, &memory_type_idx));

	VkMemoryAllocateInfo alloc_info = {0};
	alloc_info.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	alloc_info.allocationSize = mem_requirements.size;
	alloc_info.memoryTypeIndex = memory_type_idx;
	GPU_CheckVK(vkAllocateMemory(FGPU.device, &alloc_info, NULL, &buffer_impl->allocation));

	GPU_CheckVK(vkBindBufferMemory(FGPU.device, buffer_impl->vk_handle, buffer_impl->allocation, 0 /* specify memory offset */));

	if (flags & GPU_BufferFlag_CPU) {
		GPU_CheckVK(vkMapMemory(FGPU.device, buffer_impl->allocation, 0, mem_requirements.size, 0, &buffer_impl->base.data));
	}

	if (data) {
		if (flags & GPU_BufferFlag_CPU) {
			memcpy(buffer_impl->base.data, data, size);
		}
		else {
			GPU_Buffer *staging_buf = GPU_MakeBuffer(size, GPU_BufferFlag_CPU, data);
			GPU_Graph *graph = GPU_MakeGraph();

			GPU_OpCopyBufferToBuffer(graph, staging_buf, &buffer_impl->base, 0, 0, size);
			
			GPU_GraphSubmit(graph);
			GPU_GraphWait(graph);
			GPU_DestroyGraph(graph);
			GPU_DestroyBuffer(staging_buf);
		}
	}
	DS_ProfExit();
	return &buffer_impl->base;
}

static VkImageAspectFlags GPU_GetImageAspectFlags(GPU_Format format) {
	DS_ProfEnter();
	VkImageAspectFlags result = 0;
	GPU_FormatInfo format_info = GPU_GetFormatInfo(format);
	if (format_info.depth_target)   result |= VK_IMAGE_ASPECT_DEPTH_BIT;
	if (format_info.stencil_target) result |= VK_IMAGE_ASPECT_STENCIL_BIT;
	if (result == 0) result = VK_IMAGE_ASPECT_COLOR_BIT;
	DS_ProfExit();
	return result;
}

typedef struct { uint32_t count; VkSampleCountFlagBits vk_count; } GPU_MSAASampleCount;

static GPU_MSAASampleCount GPU_GetTextureMSAASampleCount(GPU_TextureFlags flags) {
	GPU_MSAASampleCount result = {1, VK_SAMPLE_COUNT_1_BIT};
	if (flags & GPU_TextureFlag_MSAA8x) { result.count = 8; result.vk_count = VK_SAMPLE_COUNT_8_BIT; }
	if (flags & GPU_TextureFlag_MSAA4x) { result.count = 4; result.vk_count = VK_SAMPLE_COUNT_4_BIT; }
	if (flags & GPU_TextureFlag_MSAA2x) { result.count = 2; result.vk_count = VK_SAMPLE_COUNT_2_BIT; }
	return result;
}

static VkImageView GPU_MakeImageView(VkFormat vk_format, VkImageUsageFlags usage, GPU_FormatInfo format_info, GPU_TextureImpl *texture_impl, uint32_t baseMipLevel, uint32_t levelCount) {
	DS_ProfEnter();
	VkImageView result;
	if (format_info.stencil_target) {
		// TODO: for depth textures with a stencil part, make two image views so that you can access the stencil part in a shader. For now, you'll just be accessing
		// the depth part.
	}

	GPU_Check(!(texture_impl->base.flags & GPU_TextureFlag_Cubemap) || texture_impl->base.depth == 1);
	VkImageViewType view_type = texture_impl->base.flags & GPU_TextureFlag_Cubemap ? VK_IMAGE_VIEW_TYPE_CUBE :
		texture_impl->base.depth > 1 ? VK_IMAGE_VIEW_TYPE_3D : VK_IMAGE_VIEW_TYPE_2D;
		
	VkImageViewUsageCreateInfo usage_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_USAGE_CREATE_INFO};
	usage_info.usage = usage;

	VkImageViewCreateInfo view_info = {VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO};
	view_info.pNext = &usage_info;
	view_info.image = texture_impl->vk_handle;
	view_info.viewType = view_type;
	view_info.format = vk_format;
	view_info.subresourceRange.aspectMask = format_info.depth_target ? VK_IMAGE_ASPECT_DEPTH_BIT/* | VK_IMAGE_ASPECT_STENCIL_BIT*/ : VK_IMAGE_ASPECT_COLOR_BIT;
	view_info.subresourceRange.baseMipLevel = baseMipLevel;
	view_info.subresourceRange.levelCount = levelCount;
	view_info.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS;
	GPU_CheckVK(vkCreateImageView(FGPU.device, &view_info, NULL, &result));
	DS_ProfExit();
	return result;
}

GPU_API void GPU_DestroyTexture(GPU_Texture *texture) {
	if (texture) {
		GPU_TextureImpl *texture_impl = (GPU_TextureImpl*)texture;
		if (texture_impl->mip_level_img_views) {
			for (uint32_t i = 0; i < texture_impl->base.mip_level_count; i++) vkDestroyImageView(FGPU.device, texture_impl->mip_level_img_views[i], NULL);
			DS_MemFree(texture_impl->mip_level_img_views);
		}
		vkDestroyImageView(FGPU.device, texture_impl->img_view, NULL);
		vkDestroyImageView(FGPU.device, texture_impl->atomics_img_view, NULL);
		vkFreeMemory(FGPU.device, texture_impl->allocation, NULL);
		vkDestroyImage(FGPU.device, texture_impl->vk_handle, NULL);
		GPU_FreeEntity((GPU_Entity*)texture);
	}
}

GPU_API GPU_Texture *GPU_MakeTexture(GPU_Format format, uint32_t width, uint32_t height, uint32_t depth, GPU_TextureFlags flags, const void *data) {
	DS_ProfEnter();
	GPU_Check(width > 0 && height > 0 && depth > 0);
		
	GPU_TextureImpl *texture_impl = &GPU_NewEntity()->texture;
	texture_impl->idle_layout_ = VK_IMAGE_LAYOUT_UNDEFINED;

	uint32_t mip_level_count = 1;
	if (flags & GPU_TextureFlag_HasMipmaps) {
		uint32_t smallest_dimension = width < height ? width : height;
		while (smallest_dimension > 1) {
			smallest_dimension /= 2;
			mip_level_count += 1;
		}
	}
		
	GPU_FormatInfo format_info = GPU_GetFormatInfo(format);

	VkImageCreateInfo info = { VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO };
	info.imageType = depth > 1 ? VK_IMAGE_TYPE_3D : VK_IMAGE_TYPE_2D;
	info.extent.width = width;
	info.extent.height = height;
	info.extent.depth = depth;
	info.mipLevels = mip_level_count;
	info.arrayLayers = flags & GPU_TextureFlag_Cubemap ? 6 : 1;
	info.format = GPU_GetVkFormat(format);
	info.tiling = VK_IMAGE_TILING_OPTIMAL;
	info.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	info.usage = VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
	info.flags = flags & GPU_TextureFlag_Cubemap ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0;

	if (format_info.sampled)					info.usage |= VK_IMAGE_USAGE_SAMPLED_BIT;
	if (flags & GPU_TextureFlag_StorageImage)  info.usage |= VK_IMAGE_USAGE_STORAGE_BIT;

	if (flags & GPU_TextureFlag_RenderTarget) {
		info.usage |= format_info.depth_target ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
	}

	info.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	info.samples = GPU_GetTextureMSAASampleCount(flags).vk_count;
		
	GPU_Format make_atomics_img_view_with_format = GPU_Format_Invalid;
	if (flags & GPU_TextureFlag_StorageImage) {
		// Make an integer-format image view for this texture. Maybe we should do this on demand instead of here, since most storage images probably don't need this.
		// It's mainly for supporting image atomics on otherwise floating point textures.
		make_atomics_img_view_with_format = GPU_GetIntegerFormat(format_info.block_size);
		if (make_atomics_img_view_with_format != GPU_Format_Invalid) {
			info.flags |= VK_IMAGE_CREATE_MUTABLE_FORMAT_BIT | VK_IMAGE_CREATE_EXTENDED_USAGE_BIT;
		}
	}

	texture_impl->base.format = format;
	texture_impl->base.flags = flags;
	texture_impl->base.width = width;
	texture_impl->base.height = height;
	texture_impl->base.depth = depth;
	texture_impl->base.layer_count = info.arrayLayers;
	texture_impl->base.mip_level_count = info.mipLevels;
		
	GPU_CheckVK(vkCreateImage(FGPU.device, &info, NULL, &texture_impl->vk_handle));

	VkMemoryRequirements mem_requirements;
	vkGetImageMemoryRequirements(FGPU.device, texture_impl->vk_handle, &mem_requirements);

	uint32_t memory_type_idx;
	GPU_Check(FindVKMemoryTypeIndex(mem_requirements, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &memory_type_idx));

	VkMemoryAllocateInfo alloc_info = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
	alloc_info.allocationSize = mem_requirements.size;
	alloc_info.memoryTypeIndex = memory_type_idx;
	GPU_CheckVK(vkAllocateMemory(FGPU.device, &alloc_info, NULL, &texture_impl->allocation));

	GPU_CheckVK(vkBindImageMemory(FGPU.device, texture_impl->vk_handle, texture_impl->allocation, 0 /* specify memory offset */));

	// Create default image view
	texture_impl->img_view = GPU_MakeImageView(info.format, info.usage, format_info, texture_impl, 0, VK_REMAINING_MIP_LEVELS);

	if (make_atomics_img_view_with_format != GPU_Format_Invalid) {
		//CB(_c, 2);
		// We need to promise vulkan that we won't be using this image view to smample from the image.
		VkImageUsageFlags atomics_usage = info.usage & ~(VK_IMAGE_USAGE_SAMPLED_BIT);
		VkFormat atomics_format = GPU_GetVkFormat(make_atomics_img_view_with_format);
		texture_impl->atomics_img_view = GPU_MakeImageView(atomics_format, atomics_usage, format_info, texture_impl, 0, VK_REMAINING_MIP_LEVELS);
	}

	// Let's make an image view per mip level if the texture is a storage image or rendertarget. As a rendertarget, you should be able to render to a specific mipmap,
	// and as an image you need to specify which mip level to bind into the image descriptor.

	if ((flags & GPU_TextureFlag_StorageImage) || (flags & GPU_TextureFlag_RenderTarget) || (flags & GPU_TextureFlag_PerMipBinding)) {
		texture_impl->mip_level_img_views = (VkImageView*)DS_MemAlloc(sizeof(VkImageView) * texture_impl->base.mip_level_count);
		for (uint32_t i = 0; i < texture_impl->base.mip_level_count; i++) {
			texture_impl->mip_level_img_views[i] = GPU_MakeImageView(info.format, info.usage, format_info, texture_impl, i, 1);
		}
	}

	if (data) {
		uint32_t num_blocks = width * height;
		if (format_info.block_extent != 1) {
			num_blocks /= format_info.block_extent * format_info.block_extent;
			if (num_blocks < 1) num_blocks = 1;
		}

		uint32_t size_in_bytes = num_blocks * format_info.block_size * info.arrayLayers;
		GPU_Buffer *staging_buffer = GPU_MakeBuffer(size_in_bytes, GPU_BufferFlag_CPU, data);
		GPU_Graph *graph = GPU_MakeGraph();

		GPU_OpCopyBufferToTexture(graph, staging_buffer, &texture_impl->base, 0, texture_impl->base.layer_count, 0);

		if (mip_level_count > 1) {
			GPU_OpGenerateMipmaps(graph, &texture_impl->base);
		}
		
		//CB(__c, 1);
		GPU_GraphSubmit(graph);
		GPU_GraphWait(graph);
		GPU_DestroyGraph(graph);

		GPU_DestroyBuffer(staging_buffer);
	}
	DS_ProfExit();
	return &texture_impl->base;
}

GPU_API void GPU_OpGenerateMipmaps(GPU_Graph *graph, GPU_Texture *texture) {
	uint32_t src_mip_width = texture->width;
	uint32_t src_mip_height = texture->height;
		
	for (uint32_t dst_i = 1; dst_i < texture->mip_level_count; dst_i++) {
		for (uint32_t layer = 0; layer < texture->layer_count; layer++) {
			GPU_Offset3D src_area_max = {(int)src_mip_width, (int)src_mip_height, 1};
			GPU_Offset3D dst_area_max = {(int)src_mip_width / 2, (int)src_mip_height / 2, 1};
			
			GPU_OpBlitInfo blit = {0};
			blit.filter = GPU_Filter_Linear;
			blit.src_texture = texture;
			blit.src_mip_level = dst_i - 1;
			blit.src_layer = layer;
			blit.src_area[1] = src_area_max;
			blit.dst_texture = texture;
			blit.dst_mip_level = dst_i;
			blit.dst_layer = layer;
			blit.dst_area[1] = dst_area_max;
			GPU_OpBlit(graph, &blit);
		}

		src_mip_width /= 2;
		src_mip_height /= 2;
	}
}

static void GPU_RenderPassRebuildSwapchainFramebuffers(GPU_RenderPass *render_pass) {
	for (int i = 0; i < GPU_SWAPCHAIN_IMG_COUNT; i++) {
		vkDestroyFramebuffer(FGPU.device, render_pass->framebuffers[i], NULL); // NULL frame-buffers are ignored by vulkan
	}

	VkFramebufferCreateInfo framebuffer_info = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
	framebuffer_info.renderPass = render_pass->vk_handle;
	framebuffer_info.attachmentCount = 1;
	framebuffer_info.width = FGPU.swapchain.width;
	framebuffer_info.height = FGPU.swapchain.height;
	framebuffer_info.layers = 1;

	for (uint32_t i = 0; i < GPU_SWAPCHAIN_IMG_COUNT; i++) {
		framebuffer_info.pAttachments = &FGPU.swapchain.textures[i].img_view;
		GPU_CheckVK(vkCreateFramebuffer(FGPU.device, &framebuffer_info, NULL, &render_pass->framebuffers[i]));
	}
}

GPU_API GPU_RenderPass *GPU_MakeRenderPass(const GPU_RenderPassDesc *desc) {
	DS_ProfEnter();
	GPU_RenderPass *render_pass = &GPU_NewEntity()->render_pass;
		
	// TODO: make sure that the sample count is supported by the device!
	GPU_MSAASampleCount msaa_samples = {1, VK_SAMPLE_COUNT_1_BIT};
	if (desc->color_targets_count > 0 && desc->color_targets != GPU_SWAPCHAIN_COLOR_TARGET) {
		msaa_samples = GPU_GetTextureMSAASampleCount(desc->color_targets[0].texture->flags);
	}
	GPU_Check(desc->msaa_color_resolve_targets == NULL || msaa_samples.count > 1);
	
	render_pass->msaa_samples = msaa_samples.vk_count;
	render_pass->color_targets_count = desc->color_targets_count;

	VkAttachmentDescription attachments[8+8+1];
	VkImageView attachment_img_views[8+8+1];
	uint32_t attachments_count = 0;

	VkAttachmentReference color_attachment_refs[8];
	VkAttachmentReference resolve_attachment_refs[8];
	VkAttachmentReference depth_stencil_attachment_ref = {0};

	/*if (desc->color_targets == GPU_SWAPCHAIN_COLOR_TARGET) {
		GPU_Check(desc->color_targets_count == 1 && desc->depth_stencil_target == NULL);
		VkAttachmentReference ref = {0};
		ref.attachment = 0;
		ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		
		VkAttachmentDescription attachment = {0};
		attachment.format = GPU_GetVkFormat(GPU_SWAPCHAIN_FORMAT);
		attachment.samples = msaa_samples.vk_count;
		attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachments[attachments_count] = attachment;
		color_attachment_refs[attachments_count] = ref;
		attachments_count++;
	}*/

	for (uint32_t i = 0; i < desc->color_targets_count; i++) {
		VkAttachmentReference ref = {0};
		ref.attachment = attachments_count;
		ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		VkAttachmentDescription attachment = {0};
		attachment.samples = msaa_samples.vk_count;
		attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD;
		attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachment.initialLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
		attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // To know what to put here, we need to know what should happen after this render pass to this image. Let's default to no implicit transition, but we should have this as a hint.
		
		if (desc->color_targets == GPU_SWAPCHAIN_COLOR_TARGET) {
			attachment.format = GPU_GetVkFormat(GPU_SWAPCHAIN_FORMAT);
		}
		else {
			GPU_TextureView color_target = desc->color_targets[i];
			attachment.format = GPU_GetVkFormat(color_target.texture->format);
			
			// GPU_Check(color_target.texture->flags & GPU_TextureFlag_RenderTarget);
			// GPU_Check(GPU_GetTextureMSAASampleCount(color_target.texture->flags).count == msaa_samples.count); // All attachments must have the same sample count

			GPU_TextureImpl *color_target_tex = (GPU_TextureImpl*)color_target.texture;
			render_pass->color_targets[i] = color_target;
			attachment_img_views[attachments_count] = color_target_tex->mip_level_img_views[color_target.mip_level];
		}
		
		color_attachment_refs[attachments_count] = ref;
		attachments[attachments_count] = attachment;
		attachments_count++;

		if (msaa_samples.count > 1) {
			GPU_TextureView resolve_target = desc->msaa_color_resolve_targets[i];
			GPU_TextureImpl	*resolve_target_tex = (GPU_TextureImpl*)resolve_target.texture;
			render_pass->resolve_targets[i] = resolve_target;

			VkAttachmentReference resolve_ref = {0};
			resolve_ref.attachment = attachments_count;
			resolve_ref.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

			VkAttachmentDescription resolve_attachment = {0};
			resolve_attachment.format = GPU_GetVkFormat(resolve_target.texture->format);
			resolve_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
			resolve_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			resolve_attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
			resolve_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
			resolve_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
			resolve_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
			resolve_attachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL; // To know what to put here, we need to know what should happen after this render pass to this image. Let's default to no implicit transition, but we should have this as a hint.
			resolve_attachment_refs[i] = resolve_ref;
			attachments[attachments_count] = resolve_attachment;
			attachment_img_views[attachments_count] = resolve_target_tex->mip_level_img_views[resolve_target.mip_level];
			attachments_count++;
		}
	}

	if (desc->depth_stencil_target) {
		render_pass->depth_stencil_target = desc->depth_stencil_target;

		depth_stencil_attachment_ref.attachment = attachments_count;
		depth_stencil_attachment_ref.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
		
		VkAttachmentDescription attachment = {0};
		attachment.format = GPU_GetVkFormat(desc->depth_stencil_target->format),
		attachment.samples = msaa_samples.vk_count,
		attachment.loadOp = VK_ATTACHMENT_LOAD_OP_LOAD,
		attachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		attachment.initialLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
		attachments[attachments_count] = attachment;
		attachment_img_views[attachments_count] = ((GPU_TextureImpl*)desc->depth_stencil_target)->img_view;
		attachments_count++;

		GPU_Check(GPU_GetTextureMSAASampleCount(desc->depth_stencil_target->flags).count == msaa_samples.count); // All attachments must have the same sample count
	}

	VkSubpassDescription subpass_info = {0};
	subpass_info.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass_info.colorAttachmentCount = desc->color_targets_count;
	subpass_info.pColorAttachments = color_attachment_refs;
	subpass_info.pResolveAttachments = msaa_samples.count > 1 ? resolve_attachment_refs : NULL;
	subpass_info.pDepthStencilAttachment = desc->depth_stencil_target ? &depth_stencil_attachment_ref : NULL;

	VkRenderPassCreateInfo render_pass_info = {VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO};
	render_pass_info.attachmentCount = attachments_count;
	render_pass_info.pAttachments = attachments;
	render_pass_info.subpassCount = 1;
	render_pass_info.pSubpasses = &subpass_info;
		
	GPU_CheckVK(vkCreateRenderPass(FGPU.device, &render_pass_info, NULL, &render_pass->vk_handle));

	if (desc->color_targets == GPU_SWAPCHAIN_COLOR_TARGET) {
		GPU_RenderPassRebuildSwapchainFramebuffers(render_pass);
		render_pass->depends_on_swapchain_gen_id = FGPU.swapchain.gen_id;
		GPU_Check(render_pass->depends_on_swapchain_gen_id > 0);
	}
	else {
		VkFramebufferCreateInfo framebuffer_info = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
		framebuffer_info.renderPass = render_pass->vk_handle;
		framebuffer_info.pAttachments = attachment_img_views;
		framebuffer_info.attachmentCount = attachments_count;
		framebuffer_info.width = desc->color_targets[0].texture->width;
		framebuffer_info.height = desc->color_targets[0].texture->height;
		framebuffer_info.layers = 1;
		GPU_CheckVK(vkCreateFramebuffer(FGPU.device, &framebuffer_info, NULL, &render_pass->framebuffers[0]));
	}
		
	DS_ProfExit();
	return render_pass;
}

GPU_API void GPU_DestroyRenderPass(GPU_RenderPass *render_pass) {
	if (render_pass) {
		for (int i = 0; i < GPU_SWAPCHAIN_IMG_COUNT; i++) {
			vkDestroyFramebuffer(FGPU.device, render_pass->framebuffers[i], NULL); // NULL frame-buffers are ignored by vulkan
		}
		vkDestroyRenderPass(FGPU.device, render_pass->vk_handle, NULL);
		GPU_FreeEntity((GPU_Entity*)render_pass);
	}
}

static void GPU_VkCreateShaderModule(GPU_String spirv, VkShaderModule *shader) {
	DS_ProfEnter();
	VkShaderModuleCreateInfo create_info = { VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO };
	create_info.codeSize = spirv.length;
	create_info.pCode = (uint32_t*)spirv.data;
	GPU_CheckVK(vkCreateShaderModule(FGPU.device, &create_info, NULL, shader));
	DS_ProfExit();
}

static void GPU_ReleaseGraphicsPipeline(GPU_Entity *base) {
	GPU_TODO();
}

static void GPU_ReleaseComputePipeline(GPU_Entity *base) {
	GPU_TODO();
}

static GPU_GraphicsPipeline *GPU_MakePipelineEx(const GPU_GraphicsPipelineDesc *desc, bool swapchain) {
	DS_ProfEnter();
	DS_ArenaMark T = DS_ArenaGetMark(&FGPU.temp_arena);

	GPU_GraphicsPipeline *pipeline = &GPU_NewEntity()->graphics_pipeline;
	pipeline->render_pass = desc->render_pass;
	pipeline->layout = desc->layout;
	DS_VecInit(&pipeline->accesses);
	
	VkGraphicsPipelineCreateInfo info = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
		
	// SHADER STAGES
	DS_Vec(VkPipelineShaderStageCreateInfo) stages = {0};
	stages.arena = &FGPU.temp_arena;

	GPU_ShaderDesc vs_desc = desc->vs, fs_desc = desc->fs;
	VkShaderModule vs = 0, fs = 0;

	if (vs_desc.spirv.length == 0) {
		GPU_Check(vs_desc.glsl.length > 0);
		vs_desc.spirv = GPU_SPIRVFromGLSL(&FGPU.temp_arena, GPU_ShaderStage_Vertex, desc->layout, &vs_desc, NULL);
		// OS_WriteEntireFile(OS_CWD, STR_("C:/temp/test.spv"), vs_desc.spirv); //test
	}
	if (fs_desc.spirv.length == 0) {
		GPU_Check(fs_desc.glsl.length > 0);
		fs_desc.spirv = GPU_SPIRVFromGLSL(&FGPU.temp_arena, GPU_ShaderStage_Fragment, desc->layout, &fs_desc, NULL);
	}
		
	{
		GPU_Check(vs_desc.spirv.length > 0);
		GPU_VkCreateShaderModule(vs_desc.spirv, &vs);
		VkPipelineShaderStageCreateInfo vs_info = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		vs_info.stage = VK_SHADER_STAGE_VERTEX_BIT;
		vs_info.module = vs;
		vs_info.pName = "main";
		DS_VecPush(&stages, vs_info);
		
		DS_VecPushN(&pipeline->accesses, vs_desc.accesses.data, vs_desc.accesses.length);
	}

	if (fs_desc.spirv.length > 0) { // It's valid to have a pipeline without pixel shader, e.g. depth only
		GPU_VkCreateShaderModule(fs_desc.spirv, &fs);
		VkPipelineShaderStageCreateInfo fs_info = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		fs_info.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
		fs_info.module = fs;
		fs_info.pName = "main";
		DS_VecPush(&stages, fs_info);

		DS_VecPushN(&pipeline->accesses, fs_desc.accesses.data, fs_desc.accesses.length);
	}

	info.stageCount = (uint32_t)stages.length;
	info.pStages = stages.data;

	// VERTEX INPUT STATE

	VkPipelineVertexInputStateCreateInfo vertex_input_state_info = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	info.pVertexInputState = &vertex_input_state_info;

	VkVertexInputAttributeDescription input_attributes[16];
	GPU_Check(desc->vertex_input_formats.length <= 16);
		
	VkVertexInputBindingDescription input_streams[16];
	uint32_t input_streams_count = 0;

	if (desc->vertex_input_formats.length > 0) {
		uint32_t offset = 0;
		for (uint32_t i = 0; i < desc->vertex_input_formats.length; i++) {
			GPU_Format format = desc->vertex_input_formats.data[i];

			VkVertexInputAttributeDescription attribute = {0};
			attribute.location = (uint32_t)i;
			attribute.binding = 0;
			attribute.format = GPU_GetVkFormat(format);
			attribute.offset = offset;
			input_attributes[i] = attribute;

			GPU_FormatInfo format_info = GPU_GetFormatInfo(format);
			GPU_Check(format_info.vertex_input);
			offset += format_info.block_size;
		}

		input_streams_count = 1;
		VkVertexInputBindingDescription stream = {0, offset};
		input_streams[0] = stream;
	}

	vertex_input_state_info.vertexBindingDescriptionCount = input_streams_count;
	vertex_input_state_info.pVertexBindingDescriptions = input_streams;

	vertex_input_state_info.vertexAttributeDescriptionCount = (uint32_t)desc->vertex_input_formats.length;
	vertex_input_state_info.pVertexAttributeDescriptions = input_attributes;

	// INPUT ASSEMBLY STATE

	VkPipelineInputAssemblyStateCreateInfo input_assembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	input_assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	info.pInputAssemblyState = &input_assembly;

	// VIEWPORT STATE

	//VkRect2D scissor = { {0}, { 4096 * 4096, 4096 * 4096 } };
	VkPipelineViewportStateCreateInfo viewport_state = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
		
	// We're using dynamic state
	viewport_state.viewportCount = 1;
	viewport_state.scissorCount = 1;
	info.pViewportState = &viewport_state;

	// RASTERIZATION STATE

	VkPipelineRasterizationStateCreateInfo rasterization_state = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterization_state.lineWidth = 1.f;
		
	VkPipelineRasterizationConservativeStateCreateInfoEXT conservative_state = {VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_CONSERVATIVE_STATE_CREATE_INFO_EXT};
	conservative_state.conservativeRasterizationMode = VK_CONSERVATIVE_RASTERIZATION_MODE_OVERESTIMATE_EXT;
	if (desc->enable_conservative_rasterization) {
		rasterization_state.pNext = &conservative_state;
	}
		
	if (desc->cull_mode == GPU_CullMode_DrawCW) {
		rasterization_state.cullMode = VK_CULL_MODE_FRONT_BIT;
	} else if (desc->cull_mode == GPU_CullMode_DrawCCW) {
		rasterization_state.cullMode = VK_CULL_MODE_BACK_BIT;
	}
	info.pRasterizationState = &rasterization_state;

	// MULTISAMPLE STATE

	VkPipelineMultisampleStateCreateInfo multisample_state = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisample_state.rasterizationSamples = desc->render_pass->msaa_samples;
	info.pMultisampleState = &multisample_state;

	// DEPTH STENCIL STATE

	VkPipelineDepthStencilStateCreateInfo depth_stencil_state = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	depth_stencil_state.depthTestEnable = desc->enable_depth_write || desc->enable_depth_test;
	depth_stencil_state.depthWriteEnable = desc->enable_depth_write;
	depth_stencil_state.depthCompareOp = desc->enable_depth_test ? (GPU_REVERSE_DEPTH ? VK_COMPARE_OP_GREATER : VK_COMPARE_OP_LESS) : VK_COMPARE_OP_ALWAYS;
	info.pDepthStencilState = &depth_stencil_state;

	// BLEND STATES

	VkPipelineColorBlendAttachmentState color_attachment_states[8];
	//Assert(desc->color_targets.length <= GPU_ArrayCount(color_attachment_states));

	for (uint32_t i = 0; i < desc->render_pass->color_targets_count; i++) {
		VkPipelineColorBlendAttachmentState *state = &color_attachment_states[i];
		state->blendEnable = desc->enable_blending;
		
		if (desc->blending_mode_additive) {
			state->srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
			state->dstColorBlendFactor = VK_BLEND_FACTOR_ONE;
		}
		else {
			state->srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
			state->dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
		}

		state->colorBlendOp = VK_BLEND_OP_ADD;
		state->srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
		state->dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
		state->alphaBlendOp = VK_BLEND_OP_ADD;
		state->colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	}

	VkPipelineColorBlendStateCreateInfo color_blend_state = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	color_blend_state.attachmentCount = desc->render_pass->color_targets_count;
	color_blend_state.pAttachments = color_attachment_states;
	info.pColorBlendState = &color_blend_state;

	// DYNAMIC STATES

	VkDynamicState dynamic_states[] = { VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR };
	VkPipelineDynamicStateCreateInfo dynamic_state_info = { VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO };
	dynamic_state_info.dynamicStateCount = GPU_ArrayCount(dynamic_states);
	dynamic_state_info.pDynamicStates = dynamic_states;
	info.pDynamicState = &dynamic_state_info;

	info.renderPass = desc->render_pass->vk_handle;
	info.subpass = 0;

	info.layout = desc->layout->vk_handle;

	GPU_CheckVK(vkCreateGraphicsPipelines(FGPU.device, 0, 1, &info, NULL, &pipeline->vk_handle));

	if (vs) vkDestroyShaderModule(FGPU.device, vs, NULL);
	if (fs) vkDestroyShaderModule(FGPU.device, fs, NULL);

	DS_ArenaSetMark(&FGPU.temp_arena, T);
	DS_ProfExit();
	return pipeline;
}

GPU_API GPU_GraphicsPipeline *GPU_MakeGraphicsPipeline(const GPU_GraphicsPipelineDesc *desc) {
	return GPU_MakePipelineEx(desc, false);
}

GPU_API void GPU_DestroyComputePipeline(GPU_ComputePipeline *pipeline) {
	if (pipeline) {
		vkDestroyPipeline(FGPU.device, pipeline->vk_handle, NULL);
		DS_VecDestroy(&pipeline->accesses);
		GPU_FreeEntity((GPU_Entity*)pipeline);
	}
}

GPU_API void GPU_DestroyGraphicsPipeline(GPU_GraphicsPipeline *pipeline) {
	if (pipeline) {
		vkDestroyPipeline(FGPU.device, pipeline->vk_handle, NULL);
		DS_VecDestroy(&pipeline->accesses);
		GPU_FreeEntity((GPU_Entity*)pipeline);
	}
}

GPU_API GPU_ComputePipeline *GPU_MakeComputePipeline(GPU_PipelineLayout *layout, const GPU_ShaderDesc *cs) {
	DS_ProfEnter();
	DS_ArenaMark T = DS_ArenaGetMark(&FGPU.temp_arena);
		
	GPU_ComputePipeline *pipeline = &GPU_NewEntity()->compute_pipeline;
	pipeline->layout = layout;
		
	VkShaderModule compute_shader;
	GPU_ShaderDesc cs_desc = *cs;
	if (cs_desc.spirv.length == 0) {
		GPU_Check(cs_desc.glsl.length > 0);
		cs_desc.spirv = GPU_SPIRVFromGLSL(&FGPU.temp_arena, GPU_ShaderStage_Compute, layout, &cs_desc, NULL);
	}
		
	GPU_Check(cs_desc.spirv.length > 0);
	GPU_VkCreateShaderModule(cs_desc.spirv, &compute_shader);
	DS_VecPushN(&pipeline->accesses, cs_desc.accesses.data, cs_desc.accesses.length);
		
	VkPipelineShaderStageCreateInfo stage = {VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO};
	stage.stage = VK_SHADER_STAGE_COMPUTE_BIT;
	stage.module = compute_shader;
	stage.pName = "main";
		
	VkComputePipelineCreateInfo info = {VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO};
	info.layout = layout->vk_handle;
	info.stage = stage;
	GPU_CheckVK(vkCreateComputePipelines(FGPU.device, 0, 1, &info, NULL, &pipeline->vk_handle));
		
	vkDestroyShaderModule(FGPU.device, compute_shader, NULL);
	DS_ArenaSetMark(&FGPU.temp_arena, T);
	DS_ProfExit();
	return pipeline;
}

typedef struct { DS_Arena *temp; GPU_ShaderIncluderFn includer; void *includer_ctx; } GPU_GLSLIncludeHandlerCtx;

static glsl_include_result_t *GPU_IncludeHandlerGLSL(void *ctx, const char *header_name_cstr, const char *includer_name_cstr, size_t include_depth) {
	GPU_GLSLIncludeHandlerCtx *ctx_ = (GPU_GLSLIncludeHandlerCtx*)ctx;
	if (ctx_->includer == NULL) return NULL;
		
	GPU_String source;
	GPU_String header_name = {header_name_cstr, (int)strlen(header_name_cstr)};
	bool ok = ctx_->includer(ctx_->temp, header_name, &source, ctx_->includer_ctx);
		
	glsl_include_result_t result = {0};
	result.header_name = ok ? header_name_cstr : NULL;
	result.header_data = ok ? source.data : NULL;
	result.header_length = ok ? source.length : 0;
	return DS_Clone(glsl_include_result_t, ctx_->temp, result);
}

GPU_API GPU_String GPU_JoinGLSLErrorString(GPU_ARENA *arena, GPU_GLSLErrorArray errors) {
	GPU_StrBuilder s = {0};
	s.arena = arena;
	for (uint32_t i = 0; i < errors.length; i++) {
		GPU_PrintI(&s, errors.data[i].line);
		GPU_PrintL(&s, ": ");
		GPU_PrintS(&s, errors.data[i].error_message);
		GPU_PrintL(&s, "\n");
	}
	GPU_String result = {s.data, s.length};
	return result;
}

GPU_API GPU_String GPU_SPIRVFromGLSL(GPU_ARENA *arena, GPU_ShaderStage stage, GPU_PipelineLayout *pipeline_layout, const GPU_ShaderDesc *desc, GPU_GLSLErrorArray *out_errors) {
	DS_ProfEnter();
	DS_ArenaMark T = DS_ArenaGetMark(&FGPU.temp_arena);

	glslang_stage_t glsl_stage;
	switch (stage) {
	case GPU_ShaderStage_Vertex:   glsl_stage = GLSLANG_STAGE_VERTEX; break;
	case GPU_ShaderStage_Fragment: glsl_stage = GLSLANG_STAGE_FRAGMENT; break;
	case GPU_ShaderStage_Compute:  glsl_stage = GLSLANG_STAGE_COMPUTE; break;
	}
		
	GPU_StrBuilder glsl = {0};
	glsl.arena = &FGPU.temp_arena;
		
	GPU_PrintL(&glsl, "#version 450\n");
	GPU_PrintL(&glsl, "#define GPU_BINDING(NAME) GPU_BINDING_##NAME\n");

	switch (stage) {
	case GPU_ShaderStage_Vertex:   { GPU_PrintL(&glsl, "#define GPU_STAGE_VERTEX\n"); } break;
	case GPU_ShaderStage_Fragment: { GPU_PrintL(&glsl, "#define GPU_STAGE_FRAGMENT\n"); } break;
	case GPU_ShaderStage_Compute:  { GPU_PrintL(&glsl, "#define GPU_STAGE_COMPUTE\n"); } break;
	}
		
	for (uint32_t i = 0; i < desc->accesses.length; i++) {
		GPU_Access *access = &desc->accesses.data[i];
		uint32_t binding_index = access->binding;
		GPU_BindingInfo binding_info = DS_VecGet(pipeline_layout->bindings, binding_index);
		const char *binding_name = binding_info.name;
		
		// We could just make the uniforms directly accessible by name, but then it would be slightly less clear to the user where the variable is coming from when used
		// With the GPU_BINDING() macro, the reader can CTRL+F their way into the GPU_BINDING(), and then it will be obvious. It also makes it possible to define buffers
		// of different types in the shader that all bind to the same binding.

		const char *access_tag = access->flags & GPU_AccessFlag_Read ? (access->flags & GPU_AccessFlag_Write ? "" : "readonly") : "writeonly";
		
		switch (binding_info.kind) {
		case GPU_ResourceKind_Texture: {
			GPU_PrintL(&glsl, "#define GPU_BINDING_");
			GPU_PrintC(&glsl, binding_name);
			GPU_PrintL(&glsl, " layout(set=0, binding=");
			GPU_PrintI(&glsl, binding_index);
			GPU_PrintL(&glsl, ") uniform\n");
		} break;
		case GPU_ResourceKind_Sampler: {
			GPU_PrintL(&glsl, "#define GPU_BINDING_");
			GPU_PrintC(&glsl, binding_name);
			GPU_PrintL(&glsl, " layout(set=0, binding=");
			GPU_PrintI(&glsl, binding_index);
			GPU_PrintL(&glsl, ") uniform\n");
		} break;
		case GPU_ResourceKind_Buffer: {
			GPU_PrintL(&glsl, "#define GPU_BINDING_");
			GPU_PrintC(&glsl, binding_name);
			GPU_PrintL(&glsl, " layout(set=0, binding=");
			GPU_PrintI(&glsl, binding_index);
			GPU_PrintL(&glsl, ") ");
			GPU_PrintC(&glsl, access_tag);
			GPU_PrintL(&glsl, " buffer _");
			GPU_PrintC(&glsl, binding_name);
			GPU_PrintL(&glsl, "\n");
		} break;
		case GPU_ResourceKind_StorageImage: {
			GPU_FormatInfo format_info = GPU_GetFormatInfo(binding_info.image_format);
			GPU_Check(format_info.glsl != NULL);
			GPU_PrintL(&glsl, "#define GPU_BINDING_");
			GPU_PrintC(&glsl, binding_name);
			GPU_PrintL(&glsl, " layout(set=0, binding=");
			GPU_PrintI(&glsl, binding_index);
			GPU_PrintL(&glsl, ", ");
			GPU_PrintC(&glsl, format_info.glsl);
			GPU_PrintL(&glsl, ") ");
			GPU_PrintC(&glsl, access_tag);
			GPU_PrintL(&glsl, " uniform\n");
		} break;
		}
	}
		
	// Add user shader code
	DS_VecPushN(&glsl, desc->glsl.data, desc->glsl.length);
	DS_VecPush(&glsl, 0); // null termination

	// GPU_DebugLog("=========== FGPU / Generated shader ===========\n");
	// GPU_DebugLog("~s\n", generated_glsl.str);
	// GPU_DebugLog("===============================================\n");

	GPU_GLSLIncludeHandlerCtx includer_handler = {0};
	includer_handler.temp = &FGPU.temp_arena;
	includer_handler.includer = desc->glsl_includer;
	includer_handler.includer_ctx = desc->glsl_includer_ctx;

	glslang_input_t input = {0};
	input.language = GLSLANG_SOURCE_GLSL;
	input.stage = glsl_stage;
	input.client = GLSLANG_CLIENT_VULKAN;
	input.client_version = GLSLANG_TARGET_VULKAN_1_0;
	input.target_language = GLSLANG_TARGET_SPV;
	input.target_language_version = GLSLANG_TARGET_SPV_1_0;
	input.code = glsl.data;
	input.default_version = 450;
	input.default_profile = GLSLANG_NO_PROFILE;
	input.force_default_version_and_profile = false;
	input.forward_compatible = false;
	input.messages = GLSLANG_MSG_DEFAULT_BIT;
	input.resource = glslang_default_resource();
	input.callbacks.include_local = GPU_IncludeHandlerGLSL;
	input.callbacks_ctx = &includer_handler;

	glslang_shader_t *shader = glslang_shader_create(&input);

	const char *log_cstr = NULL;

	bool ok = glslang_shader_preprocess(shader, &input) != 0;
		
	if (ok) ok = glslang_shader_parse(shader, &input) != 0;
	if (!ok) log_cstr = glslang_shader_get_info_log(shader);
		
	glslang_program_t *program = NULL;
	GPU_String result = {0};

	if (ok) {
		program = glslang_program_create();
		glslang_program_add_shader(program, shader);

		ok = glslang_program_link(program, GLSLANG_MSG_SPV_RULES_BIT | GLSLANG_MSG_VULKAN_RULES_BIT);
		if (!ok) log_cstr = glslang_program_get_info_log(program);
	}

	if (ok) {
		// Add debug info to the shader
		
		char *glsl_debug_filepath_cstr = GPU_ARENA_ALLOCATE(&FGPU.temp_arena, desc->glsl_debug_filepath.length + 1);
		memcpy(glsl_debug_filepath_cstr, desc->glsl_debug_filepath.data, desc->glsl_debug_filepath.length);
		glsl_debug_filepath_cstr[desc->glsl_debug_filepath.length] = 0; // null termination

		glslang_program_set_source_file(program, glsl_stage, glsl_debug_filepath_cstr);
		glslang_program_add_source_text(program, glsl_stage, glsl.data, glsl.length);
		
		glslang_spv_options_t spv_options = {0};
		spv_options.generate_debug_info = true;
		spv_options.emit_nonsemantic_shader_debug_info = true;
		spv_options.emit_nonsemantic_shader_debug_source = true;

		glslang_program_SPIRV_generate_with_options(program, glsl_stage, &spv_options);

		int size_in_dwords = (int)glslang_program_SPIRV_get_size(program);
		char *result_data = DS_ArenaPush(arena, size_in_dwords * sizeof(uint32_t));
		glslang_program_SPIRV_get(program, (uint32_t*)result_data);
		
		result.data = result_data;
		result.length = size_in_dwords * sizeof(uint32_t);
	}
	else {
		GPU_String log = {log_cstr, (int)strlen(log_cstr)};
		
		int gen_glsl_line_count = 0, desc_glsl_line_count = 0;

		GPU_String gen_glsl_remaining = {glsl.data, glsl.length};
		for (; gen_glsl_remaining.length > 0;) {
			GPU_ParseUntil(&gen_glsl_remaining, '\n');
			gen_glsl_line_count++;
		}

		GPU_String desc_glsl_remaining = desc->glsl;
		for (; desc_glsl_remaining.length > 0;) {
			GPU_ParseUntil(&desc_glsl_remaining, '\n');
			desc_glsl_line_count++;
		}
		
		int fgpu_generated_extra_lines_count = gen_glsl_line_count - desc_glsl_line_count;

		DS_Vec(GPU_GLSLError) errors = {0};
		errors.arena = arena;

		GPU_String log_remaining = log;
		for (; log_remaining.length > 0;) {
			GPU_String line = GPU_ParseUntil(&log_remaining, '\n');

			const GPU_String start = GPU_STR("ERROR: 0:");
			if (line.length >= start.length && memcmp(line.data, start.data, start.length) == 0) {
				GPU_String line_remaining = {line.data + start.length, line.length - start.length};
				
				GPU_String line_number_str = GPU_ParseUntil(&line_remaining, ':');
				
				int64_t line_idx;
				GPU_Check(GPU_ParseInt(line_number_str, &line_idx));

				// Skip : and spaces
				for (;;) {
					line_remaining.data += 1;
					line_remaining.length += 1;
					if (line_remaining.length == 0 || line_remaining.data[0] != ' ') break;
				}

				GPU_GLSLError error = {0};
				error.shader_stage = stage;
				error.line = (uint32_t)((int)line_idx - fgpu_generated_extra_lines_count);
				error.error_message = line_remaining;
				DS_VecPush(&errors, error);
			}
		}
#if 0
		StrRangeArray lines = StrSplit(&FGPU.temp_arena, log, '\n');

		// if (out_errors == NULL) GPU_DebugLog("FGPU: Errors in a shader (~s) that was guaranteed to be valid by the user:\n", desc->glsl_debug_filepath);

		// Since we modified the GLSL code, the error lines will be wrong. Let's map the line numbers back to the user code.
		uint32_t generated_glsl_line_count = (uint32_t)StrSplit(&FGPU.temp_arena, glsl.str, '\n').length;
		uint32_t desc_glsl_line_count = (uint32_t)StrSplit(&FGPU.temp_arena, desc->glsl, '\n').length;
		uint32_t generated_extra_lines_count = generated_glsl_line_count - desc_glsl_line_count;

		DS_Vec(GPU_GLSLError) errors = {.arena = arena};

		DS_VecEach(StrRange, &lines, it) {
			String line_str = StrSlice(log, it.elem->min, it.elem->max);
			if (StrCutStart(&line_str, STR_("ERROR: 0:"))) {
				StrRange second_colon_range;
				GPU_Check(StrFind(line_str, STR_(":"), &second_colon_range));

				int64_t line_idx;
				String line_number_str = StrSliceBefore(line_str, second_colon_range.min);
				GPU_Check(StrToI64Ex(line_number_str, 10, &line_idx));
				
				line_str = StrSliceAfter(line_str, second_colon_range.max);
				while (StrCutStart(&line_str, STR_(" "))) {}

				GPU_GLSLError error = {.shader_stage = stage, .line = (uint32_t)(line_idx - generated_extra_lines_count), .error_message = line_str};
				// if (out_errors == NULL) GPU_DebugLog(" line ~i64: ~s\n", error.line, error.error_message);
				// else DS_VecPush(&errors, error);
				DS_VecPush(&errors, error);
			}
		}
#endif

		if (out_errors == NULL) GPU_Check(false);
		out_errors->data = errors.data;
		out_errors->length = (uint32_t)errors.length;
	}

	glslang_shader_delete(shader);
	if (program) glslang_program_delete(program);
	if (arena != &FGPU.temp_arena) DS_ArenaSetMark(&FGPU.temp_arena, T);
	DS_ProfExit();
	return result;
}

static void GPU_GetVkStageAccessLayout(const GPU_ResourceAccess *access, VkPipelineStageFlags *out_stage_flags, VkAccessFlags *out_access_flags, VkImageLayout *out_img_layout) {
	DS_ProfEnter();
	if ((access->access_flags & GPU_ResourceAccessFlag_ColorTargetRead) || (access->access_flags & GPU_ResourceAccessFlag_ColorTargetWrite)) {
		*out_stage_flags |= VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		*out_img_layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

		if (access->access_flags & GPU_ResourceAccessFlag_ColorTargetRead) {
			*out_access_flags |= VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;
		}
		if (access->access_flags & GPU_ResourceAccessFlag_ColorTargetWrite) {
			*out_access_flags |= VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		}
	}
	else if (access->access_flags & GPU_ResourceAccessFlag_DepthStencilTargetWrite) {
		*out_stage_flags |= VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		*out_img_layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; // hmm... what's the difference between this and VK_IMAGE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL?
		*out_access_flags |= VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
	}
	else if (access->access_flags & GPU_ResourceAccessFlag_TextureRead) {
		*out_stage_flags |= VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT; // TODO: add more knowledge about stage
		*out_access_flags |= VK_ACCESS_SHADER_READ_BIT;
		*out_img_layout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	}
	else if ((access->access_flags & GPU_ResourceAccessFlag_StorageImageRead) || (access->access_flags & GPU_ResourceAccessFlag_StorageImageWrite)) {
		*out_stage_flags = VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_VERTEX_SHADER_BIT; // TODO: add more knowledge about stage
		
		if (access->access_flags & GPU_ResourceAccessFlag_StorageImageRead) {
			*out_access_flags |= VK_ACCESS_SHADER_READ_BIT;
		}
		if (access->access_flags & GPU_ResourceAccessFlag_StorageImageWrite) {
			*out_access_flags |= VK_ACCESS_SHADER_WRITE_BIT;
		}
		*out_img_layout = VK_IMAGE_LAYOUT_GENERAL;
	}
	else if (access->access_flags & GPU_ResourceAccessFlag_TransferRead) {
		GPU_Check(!(access->access_flags & GPU_ResourceAccessFlag_TransferWrite));
		*out_stage_flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;
		*out_access_flags |= VK_ACCESS_TRANSFER_READ_BIT;
		*out_img_layout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
	}
	else if (access->access_flags & GPU_ResourceAccessFlag_TransferWrite) {
		GPU_Check(!(access->access_flags & GPU_ResourceAccessFlag_TransferRead));
		*out_stage_flags |= VK_PIPELINE_STAGE_TRANSFER_BIT;
		*out_access_flags |= VK_ACCESS_TRANSFER_WRITE_BIT;
		*out_img_layout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	}
	else if ((access->access_flags & GPU_ResourceAccessFlag_BufferRead) || (access->access_flags & GPU_ResourceAccessFlag_BufferWrite)) {
		*out_stage_flags |= VK_PIPELINE_STAGE_VERTEX_SHADER_BIT | VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT; // TODO: add more knowledge about stage
		
		if (access->access_flags & GPU_ResourceAccessFlag_BufferRead) {
			*out_access_flags |= VK_ACCESS_SHADER_READ_BIT;
		}
		if (access->access_flags & GPU_ResourceAccessFlag_BufferWrite) {
			*out_access_flags |= VK_ACCESS_SHADER_WRITE_BIT;
		}
	}
	else {
		GPU_Check(false);
	}
	DS_ProfExit();
}

static void GPU_InsertBarriers(GPU_Graph *graph, GPU_ResourceAccess *accesses, uint32_t accesses_count) {
	VkPipelineStageFlags src_stage_mask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; // By default, this barrier depends on nothing
	VkPipelineStageFlags dst_stage_mask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT; // By default, nothing depends on this barrier

	DS_Vec(VkMemoryBarrier) mem_barriers = {0};
	mem_barriers.arena = &graph->arena;
		
	DS_Vec(VkImageMemoryBarrier) img_barriers = {0};
	img_barriers.arena = &graph->arena;

	for (uint32_t access_i = 0; access_i < accesses_count; access_i++) {
		const GPU_ResourceAccess *access = &accesses[access_i];

		switch (access->resource_kind) {
		case GPU_ResourceKind_StorageImage: // fallthrough
		case GPU_ResourceKind_Texture: {
			GPU_TextureImpl *texture = (GPU_TextureImpl*)access->resource;
			VkAccessFlags aspect = GPU_GetImageAspectFlags(texture->base.format);

			GPU_TextureState *state = texture->temp;
			if (state == NULL) {
				state = DS_New(GPU_TextureState, &graph->arena);
				state->sub_states = (GPU_SubresourceState*)DS_ArenaPush(&graph->arena, sizeof(GPU_SubresourceState) * texture->base.mip_level_count * texture->base.layer_count);

				for (uint32_t layer = 0; layer < texture->base.layer_count; layer++) {
					for (uint32_t level = 0; level < texture->base.mip_level_count; level++) {
						GPU_SubresourceState *sub_state = &state->sub_states[level + layer*texture->base.mip_level_count];
						sub_state->layout =  texture->idle_layout_;
						sub_state->stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
						sub_state->access_flags = 0;
					}
				}

				texture->temp = state;
				DS_VecPush(&graph->builder_state.textures, texture);
			}

			VkImageLayout dst_layout;
			VkAccessFlags dst_access_flags = 0;
			GPU_GetVkStageAccessLayout(access, &dst_stage_mask, &dst_access_flags, &dst_layout);

			// Transfer all the accessed texture views into the new layout.

			uint32_t layers_hi = access->first_layer + access->layer_count;
			uint32_t levels_hi = access->first_mip_level + access->mip_level_count;

			for (uint32_t layer = access->first_layer; layer < layers_hi; layer++) {
				for (uint32_t level = access->first_mip_level; level < levels_hi; level++) {
					GPU_SubresourceState *sub_state = &state->sub_states[level + layer*texture->base.mip_level_count];

					// Add the barrier if the new layout or access flags are different
					if (sub_state->layout != dst_layout || sub_state->access_flags != dst_access_flags) {
						src_stage_mask |= sub_state->stage; // Make the barrier wait for the previous stage

						// if (prev_view_access.layout == VK_IMAGE_LAYOUT_UNDEFINED && dst_layout == VK_IMAGE_LAYOUT_GENERAL) BP();
						VkImageMemoryBarrier img_barrier = GPU_ImageBarrier(texture->vk_handle, aspect, layer, 1, level, 1, sub_state->layout, dst_layout, sub_state->access_flags, dst_access_flags);
						DS_VecPush(&img_barriers, img_barrier);

						sub_state->layout = dst_layout;
						sub_state->stage = dst_stage_mask;
						sub_state->access_flags = dst_access_flags;
					}
				}
			}
		} break;

		case GPU_ResourceKind_Buffer: {
			GPU_BufferImpl *buffer = (GPU_BufferImpl*)access->resource;

			GPU_BufferState *state = buffer->temp;
			if (state == NULL) {
				state = DS_New(GPU_BufferState, &graph->arena);
				state->stage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
				state->access_flags = 0;
				buffer->temp = state;
				DS_VecPush(&graph->builder_state.buffers, buffer);
			}

			VkAccessFlags dst_access_flags = 0;
			VkImageLayout _img_layout;
			GPU_GetVkStageAccessLayout(access, &dst_stage_mask, &dst_access_flags, &_img_layout);

			if (state->access_flags != dst_access_flags) {
				src_stage_mask |= state->stage; // Make the barrier wait for the previous stage

				VkMemoryBarrier mem_barrier = {VK_STRUCTURE_TYPE_MEMORY_BARRIER};
				mem_barrier.srcAccessMask = state->access_flags;
				mem_barrier.dstAccessMask = dst_access_flags;
				DS_VecPush(&mem_barriers, mem_barrier);

				state->stage = dst_stage_mask;
				state->access_flags = dst_access_flags;
			}
		} break;

		case GPU_ResourceKind_Sampler: break;
		}
	}

	if (mem_barriers.length > 0 || img_barriers.length > 0) {
		vkCmdPipelineBarrier(graph->cmd_buffer, src_stage_mask, dst_stage_mask, 0, (uint32_t)mem_barriers.length, mem_barriers.data, 0, NULL, (uint32_t)img_barriers.length, img_barriers.data);
	}
}

GPU_API GPU_DescriptorArena *GPU_MakeDescriptorArena() {
	GPU_DescriptorArena *descriptor_arena = &GPU_NewEntity()->descriptor_arena;
	DS_InitArena(&descriptor_arena->arena, 256);
		
	VkDescriptorPoolSize pool_sizes[4];
	pool_sizes[0].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
	pool_sizes[0].descriptorCount = 256; // does this mean the maximum number of descriptors per one descriptor set, or maximum number of descriptors in total for all descriptor sets in this pool? It seems to be the latter.
	pool_sizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLER;
	pool_sizes[1].descriptorCount = 256;
	pool_sizes[2].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
	pool_sizes[2].descriptorCount = 256;
	pool_sizes[3].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	pool_sizes[3].descriptorCount = 256;

	VkDescriptorPoolCreateInfo pool_info = { VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO };
	pool_info.poolSizeCount = GPU_ArrayCount(pool_sizes);
	pool_info.pPoolSizes = pool_sizes;
	pool_info.maxSets = 512;
	GPU_CheckVK(vkCreateDescriptorPool(FGPU.device, &pool_info, NULL, &descriptor_arena->pool));

	return descriptor_arena;
}

GPU_API void GPU_ResetDescriptorArena(GPU_DescriptorArena *descriptor_arena) {
	vkResetDescriptorPool(FGPU.device, descriptor_arena->pool, 0);
		
	// descriptor_arena->last_pool = &descriptor_arena->first_pool;
	DS_ArenaReset(&descriptor_arena->arena);
}

GPU_API void GPU_DestroyDescriptorArena(GPU_DescriptorArena *descriptor_arena) {
	if (descriptor_arena) {
		vkDestroyDescriptorPool(FGPU.device, descriptor_arena->pool, NULL);
		// for (GPU_DescriptorPool *pool = &descriptor_arena->first_pool; pool;) {
		//	 GPU_DescriptorPool *next = pool->next;
		// 	// GPU_FreeEntity((GPU_Entity*)pool);
		//	 if (next) GPU_TODO();
		//	 pool = next;
		// }
		DS_DestroyArena(&descriptor_arena->arena);
		GPU_FreeEntity((GPU_Entity*)descriptor_arena);
	}
}

static void GPU_GraphBegin(GPU_Graph *graph) {
	DS_VecInitA(&graph->builder_state.prepared_draw_params, &graph->arena);
	DS_VecInitA(&graph->builder_state.textures, &graph->arena);
	DS_VecInitA(&graph->builder_state.buffers, &graph->arena);

	if (!graph->has_began_cmd_buffer) {
		graph->has_began_cmd_buffer = true;
		
		VkCommandBufferBeginInfo begin_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO };
		begin_info.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		GPU_CheckVK(vkBeginCommandBuffer(graph->cmd_buffer, &begin_info));

		GPU_TextureImpl *backbuffer = NULL;
		if (graph->frame.img_index != GPU_NOT_A_SWAPCHAIN_GRAPH && graph->frame.backbuffer != NULL) {
			backbuffer = &FGPU.swapchain.textures[graph->frame.img_index];
		
			if (graph->frame.img_index != GPU_NOT_A_SWAPCHAIN_GRAPH) {
				// Transition backbuffer to COLOR_ATTACHMENT_OPTIMAL
				VkImageMemoryBarrier to_rt = GPU_ImageBarrier(backbuffer->vk_handle, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1,
					backbuffer->idle_layout_, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
					0, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT);

				vkCmdPipelineBarrier(graph->cmd_buffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0, 0, NULL, 0, NULL, 1, &to_rt);
				backbuffer->idle_layout_ = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
			}
		}
	}
}

GPU_API void GPU_DestroyGraph(GPU_Graph *graph) {
	GPU_GraphWait(graph); // Wait for the graph to complete first

	if (graph->frame.img_index != GPU_NOT_A_SWAPCHAIN_GRAPH) {
		vkDestroySemaphore(FGPU.device, graph->frame.img_available_semaphore, NULL);
		vkDestroySemaphore(FGPU.device, graph->frame.img_finished_rendering_semaphore, NULL);
	}

	vkDestroyFence(FGPU.device, graph->gpu_finished_working_fence, NULL);
	vkFreeCommandBuffers(FGPU.device, FGPU.cmd_pool, 1, &graph->cmd_buffer);
	// GPU_DestroyDescriptorArena(graph->descriptor_arena);
	DS_DestroyArena(&graph->arena);
}

GPU_API GPU_Graph *GPU_MakeGraph(void) {
	DS_Arena _arena;
	// here it'd be nice to have some way to reuse arenas using slot allocators for the backing memory.
	// I think I should add that feature to DS_Arena (still use malloc or VirtualAlloc as backing for too-large blocks).
	// That would be pretty awesome. Then we could have lots of arenas without feeling bad about it,
	// and even ditch malloc and free completely and just go to VirtualAlloc directly.
	// ... but how to make it deterministic? I guess that would be up to FOS to provide a deterministic VirtualAlloc.
	DS_InitArena(&_arena, DS_KIB(1));

	GPU_Graph *graph = DS_New(GPU_Graph, &_arena);
	graph->arena = _arena;
	graph->arena_begin_mark = DS_ArenaGetMark(&graph->arena);
	graph->frame.img_index = GPU_NOT_A_SWAPCHAIN_GRAPH;
		
	// if (flags & GPU_GraphFlag_HasDescriptorArena) {
	//	 graph->descriptor_arena = GPU_MakeDescriptorArena();
	// }

	VkCommandBufferAllocateInfo cmd_buffer_info = { VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO };
	cmd_buffer_info.commandPool = FGPU.cmd_pool;
	cmd_buffer_info.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	cmd_buffer_info.commandBufferCount = 1;
	GPU_CheckVK(vkAllocateCommandBuffers(FGPU.device, &cmd_buffer_info, &graph->cmd_buffer));

	VkFenceCreateInfo fence_info = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	fence_info.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start as signaled, so that calling Wait is OK
	GPU_CheckVK(vkCreateFence(FGPU.device, &fence_info, NULL, &graph->gpu_finished_working_fence));

	GPU_GraphBegin(graph);

	//graph->lifetime = GPU_LifetimeBeginEx(arena, GPU_LifetimeFlag_CreateDescriptorPool);
	return graph;
}

GPU_API void GPU_MakeSwapchainGraphs(uint32_t count, GPU_Graph **out_graphs) {
	GPU_Check(count == 2); // I don't think there's any reason not to have 2 frames in-flight

	for (uint32_t i = 0; i < count; i++) {
		GPU_Graph *graph = GPU_MakeGraph();
		graph->frame.img_index = GPU_SWAPCHAIN_GRAPH_HASNT_CALLED_WAIT;
		
		VkSemaphore s[2];
		VkSemaphoreCreateInfo semaphore_info = {VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
		GPU_CheckVK(vkCreateSemaphore(FGPU.device, &semaphore_info, NULL, &s[0]));
		GPU_CheckVK(vkCreateSemaphore(FGPU.device, &semaphore_info, NULL, &s[1]));
		graph->frame.img_available_semaphore = s[0];
		graph->frame.img_finished_rendering_semaphore = s[1];

		out_graphs[i] = graph;
	}
}

GPU_API GPU_Texture *GPU_GetBackbuffer(GPU_Graph *graph) {
	GPU_Check(graph->frame.img_index != GPU_NOT_A_SWAPCHAIN_GRAPH);
	return graph->frame.backbuffer;
}

GPU_API void GPU_GraphWait(GPU_Graph *graph) {
	DS_ProfEnter();
		
	DS_ArenaSetMark(&graph->arena, graph->arena_begin_mark);
	memset(&graph->builder_state, 0, sizeof(graph->builder_state));

	GPU_Check(graph->cmd_buffer != 0); // did you call GPU_GraphSubmit?
	GPU_CheckVK(vkWaitForFences(FGPU.device, 1, &graph->gpu_finished_working_fence, VK_TRUE, ~(uint64_t)0));
		
	// if (graph->descriptor_arena) {
	//	 GPU_ResetDescriptorArena(graph->descriptor_arena);
	// }

	if (graph->frame.img_index != GPU_NOT_A_SWAPCHAIN_GRAPH) {
		GPU_MaybeRecreateSwapchain();

		VkResult result = vkAcquireNextImageKHR(FGPU.device, FGPU.swapchain.vk_handle, ~(uint64_t)0, graph->frame.img_available_semaphore, 0, &graph->frame.img_index);
		graph->frame.backbuffer = &FGPU.swapchain.textures[graph->frame.img_index].base;

		if (result == VK_ERROR_OUT_OF_DATE_KHR) {
			graph->frame.backbuffer = NULL;
			// TODO(); // maybe_recreate_swapchain();
			// OLD_Print("RECREATE SWAPCHAIN FROM ACQUIRE\n");
			// reset_fence = false;
		} else if (result != VK_SUBOPTIMAL_KHR) {
			GPU_CheckVK(result);
		}
	}

	GPU_GraphBegin(graph);

	DS_ProfExit();
}

GPU_API void GPU_GraphSubmit(GPU_Graph *graph) {
	DS_ProfEnter();

	// Make sure each texture has a single layout for all of its layers and levels.
	{
		DS_Vec(VkImageMemoryBarrier) img_barriers = {0};
		img_barriers.arena = &graph->arena;
		VkPipelineStageFlags src_stage_mask = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT; // By default, this barrier depends on nothing

		DS_ForVecEach(GPU_BufferImpl*, &graph->builder_state.buffers, it) {
			(*it.elem)->temp = NULL;
		}
		DS_ForVecEach(GPU_TextureImpl*, &graph->builder_state.textures, it) {
			GPU_TextureImpl *texture = *it.elem;
			VkAccessFlags aspect = GPU_GetImageAspectFlags(texture->base.format);

			// Let's default idle_layout to SHADER_READ_ONLY_OPTIMAL for all textures (except for swapchain textures). This makes it so that you can do some arbitrary storage image computations with a texture, and it shouldn't affect performance when you're not doing writes anymore. But this might just be a dumb heuristic. If we have a storage image that we always write to at the start of a new frame/graph, then we would like to keep it as GENERAL. One solution would be to keep track of `idle_layout` separately for every view, and not even try to transition them to anything at the end of a graph. Another idea would be to add a texture flag for "preferred idle layout". Idk.

			VkImageLayout dst_layout = texture->base.flags & GPU_TextureFlag_SwapchainTarget ?
				VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

			GPU_TextureState *state = texture->temp;
			for (uint32_t layer = 0; layer < texture->base.layer_count; layer++) {
				for (uint32_t level = 0; level < texture->base.mip_level_count; level++) {
					GPU_SubresourceState *sub_state = &state->sub_states[level + layer*texture->base.mip_level_count];

					if (sub_state->layout != dst_layout) {
						src_stage_mask |= sub_state->stage; // Make the barrier wait for the previous stage

						VkImageMemoryBarrier img_barrier = GPU_ImageBarrier(texture->vk_handle, aspect, layer, 1, level, 1, sub_state->layout, dst_layout, sub_state->access_flags, 0);
						DS_VecPush(&img_barriers, img_barrier);
					}
				}
			}
			texture->temp = NULL;
			texture->idle_layout_ = dst_layout;
		}

		if (img_barriers.length > 0) {
			vkCmdPipelineBarrier(graph->cmd_buffer, src_stage_mask, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, (uint32_t)img_barriers.length, img_barriers.data);
		}
	}
		
	GPU_TextureImpl *backbuffer = NULL;
	if (graph->frame.img_index != GPU_NOT_A_SWAPCHAIN_GRAPH) {
		// You may only call GPU_GraphSubmit on a swapchain graph if there exists a valid backbuffer. You can check for this by calling GPU_GetBackbuffer.
		GPU_Check(graph->frame.backbuffer != NULL);
		GPU_Check(graph->frame.img_index != GPU_SWAPCHAIN_GRAPH_HASNT_CALLED_WAIT); // Did you forget to call GPU_GraphWait? Swapchaing graphs require it to be always called first.
		backbuffer = &FGPU.swapchain.textures[graph->frame.img_index];
	}

	if (graph->frame.img_index != GPU_NOT_A_SWAPCHAIN_GRAPH) {
		// Transition backbuffer to PRESENT_SRC_OPTIMAL
		VkImageMemoryBarrier to_present = GPU_ImageBarrier(backbuffer->vk_handle, VK_IMAGE_ASPECT_COLOR_BIT, 0, 1, 0, 1,
			backbuffer->idle_layout_, VK_IMAGE_LAYOUT_PRESENT_SRC_KHR,
			VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT, 0);

		vkCmdPipelineBarrier(graph->cmd_buffer, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT, 0, 0, NULL, 0, NULL, 1, &to_present);
		backbuffer->idle_layout_ = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
	}

	GPU_CheckVK(vkEndCommandBuffer(graph->cmd_buffer));
	graph->has_began_cmd_buffer = false;

	GPU_CheckVK(vkResetFences(FGPU.device, 1, &graph->gpu_finished_working_fence));

	VkSubmitInfo submit_info = { VK_STRUCTURE_TYPE_SUBMIT_INFO };
	submit_info.commandBufferCount = 1;
	submit_info.pCommandBuffers = &graph->cmd_buffer;

	VkPipelineStageFlags wait_dst_stage_mask = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};

	if (graph->frame.img_index != GPU_NOT_A_SWAPCHAIN_GRAPH) {
		submit_info.waitSemaphoreCount = 1;
		submit_info.pWaitSemaphores = &graph->frame.img_available_semaphore;
		submit_info.pWaitDstStageMask = &wait_dst_stage_mask;
		submit_info.signalSemaphoreCount = 1;
		submit_info.pSignalSemaphores = &graph->frame.img_finished_rendering_semaphore;
	}

	GPU_CheckVK(vkQueueSubmit(FGPU.queue, 1, &submit_info, graph->gpu_finished_working_fence));

	if (graph->frame.img_index != GPU_NOT_A_SWAPCHAIN_GRAPH) {
		VkPresentInfoKHR present_info = { VK_STRUCTURE_TYPE_PRESENT_INFO_KHR };
		present_info.waitSemaphoreCount = 1;
		present_info.pWaitSemaphores = &graph->frame.img_finished_rendering_semaphore;
		present_info.swapchainCount = 1;
		present_info.pSwapchains = &FGPU.swapchain.vk_handle;
		present_info.pImageIndices = &graph->frame.img_index;

		DS_ProfEnterN(vkQueuePresent);
		VkResult result = vkQueuePresentKHR(FGPU.queue, &present_info);
		DS_ProfExitN(vkQueuePresent);

		if (result != VK_ERROR_OUT_OF_DATE_KHR && result != VK_SUBOPTIMAL_KHR) GPU_CheckVK(result);
	}

	DS_ProfExit();
}

GPU_API void GPU_WaitUntilIdle() {
	DS_ProfEnter();
	vkDeviceWaitIdle(FGPU.device);
	DS_ProfExit();
}

GPU_API void GPU_OpBindVertexBuffer(GPU_Graph *graph, GPU_Buffer *buffer) {
	VkDeviceSize zero = {0};
	vkCmdBindVertexBuffers(graph->cmd_buffer, 0, 1, &((GPU_BufferImpl*)buffer)->vk_handle, &zero);
}

GPU_API void GPU_OpBindIndexBuffer(GPU_Graph *graph, GPU_Buffer *buffer) {
	vkCmdBindIndexBuffer(graph->cmd_buffer, ((GPU_BufferImpl*)buffer)->vk_handle, 0, VK_INDEX_TYPE_UINT32);
}

GPU_API void GPU_OpBindComputePipeline(GPU_Graph *graph, GPU_ComputePipeline *pipeline) {
	if (pipeline != graph->builder_state.compute_pipeline) {
		graph->builder_state.compute_pipeline = pipeline;
		vkCmdBindPipeline(graph->cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, pipeline->vk_handle);
	}
}

GPU_API void GPU_OpBindComputeDescriptorSet(GPU_Graph *graph, GPU_DescriptorSet *set) {
	if (set != graph->builder_state.compute_descriptor_set) {
		graph->builder_state.compute_descriptor_set = set;
		vkCmdBindDescriptorSets(graph->cmd_buffer, VK_PIPELINE_BIND_POINT_COMPUTE, set->pipeline_layout->vk_handle, 0, 1, &set->vk_handle, 0, NULL);
	}
}

GPU_API void GPU_OpBeginRenderPass(GPU_Graph *graph) {
	GPU_Check(graph->builder_state.preparing_render_pass != NULL);

	GPU_RenderPass *render_pass = graph->builder_state.preparing_render_pass;
	graph->builder_state.render_pass = render_pass;
	graph->builder_state.preparing_render_pass = NULL;
	
	bool render_to_swapchain = render_pass->depends_on_swapchain_gen_id != 0;
	if (render_to_swapchain && render_pass->depends_on_swapchain_gen_id != FGPU.swapchain.gen_id) {
		GPU_RenderPassRebuildSwapchainFramebuffers(render_pass);
		render_pass->depends_on_swapchain_gen_id = FGPU.swapchain.gen_id;
	}

	// Collect all resource accesses that happen during this renderpass.
	DS_Vec(GPU_ResourceAccess) accesses = {0};
	accesses.arena = &graph->arena;

	GPU_TextureView backbuffer_or_null = {0};
	if (graph->frame.img_index != GPU_NOT_A_SWAPCHAIN_GRAPH) {
		backbuffer_or_null.texture = &FGPU.swapchain.textures[graph->frame.img_index].base;
	}

	for (uint32_t i = 0; i < render_pass->color_targets_count; i++) {
		GPU_TextureView target = render_to_swapchain ? backbuffer_or_null : render_pass->color_targets[i];

		GPU_ResourceAccess access = {target.texture, GPU_ResourceKind_Texture, GPU_ResourceAccessFlag_ColorTargetRead|GPU_ResourceAccessFlag_ColorTargetWrite, 0, 1, target.mip_level, 1};
		GPU_Check(access.resource);
		DS_VecPush(&accesses, access);

		GPU_TextureView resolve_target = render_to_swapchain ? backbuffer_or_null : render_pass->resolve_targets[i];
		if (resolve_target.texture) {
			GPU_ResourceAccess resolve_access = {resolve_target.texture, GPU_ResourceKind_Texture, GPU_ResourceAccessFlag_ColorTargetRead|GPU_ResourceAccessFlag_ColorTargetWrite, 0, 1, resolve_target.mip_level, 1};
			GPU_Check(resolve_access.resource);
			DS_VecPush(&accesses, resolve_access);
		}
	}
		
	if (render_pass->depth_stencil_target) {
		GPU_Check(!(render_pass->depth_stencil_target->flags & GPU_TextureFlag_HasMipmaps));
		// TODO: add support for rendering into a specific level/mip?

		GPU_ResourceAccess access = {render_pass->depth_stencil_target, GPU_ResourceKind_Texture, GPU_ResourceAccessFlag_DepthStencilTargetWrite, 0, 1, 0, 1};
		GPU_Check(access.resource);
		DS_VecPush(&accesses, access);
	}

	DS_ForVecEach(GPU_DrawParams, &graph->builder_state.prepared_draw_params, it) {
		GPU_GraphicsPipeline *pipeline = it.elem->pipeline;
		GPU_DescriptorSet *desc_set = it.elem->desc_set;

		for (int access_i = 0; access_i < pipeline->accesses.length; access_i++) {
			GPU_Access *binding_access = &pipeline->accesses.data[access_i];
			GPU_BindingInfo binding_info = DS_VecGet(pipeline->layout->bindings, binding_access->binding);
			GPU_BindingValue binding_value = DS_VecGet(desc_set->bindings, binding_access->binding);

			GPU_ResourceAccess access = {binding_value.ptr, binding_info.kind, 0, 0, 0, 0, 0};

			GPU_TextureImpl *texture = (GPU_TextureImpl*)binding_value.ptr;

			switch (binding_info.kind) {
			case GPU_ResourceKind_StorageImage: {
				if (binding_access->flags & GPU_AccessFlag_Read)  access.access_flags |= GPU_ResourceAccessFlag_StorageImageRead;
				if (binding_access->flags & GPU_AccessFlag_Write) access.access_flags |= GPU_ResourceAccessFlag_StorageImageWrite;
				access.layer_count = 1;
				access.first_mip_level = binding_value.mip_level;
				access.mip_level_count = 1;
			} break;
			case GPU_ResourceKind_Buffer: {
				if (binding_access->flags & GPU_AccessFlag_Read)  access.access_flags |= GPU_ResourceAccessFlag_BufferRead;
				if (binding_access->flags & GPU_AccessFlag_Write) access.access_flags |= GPU_ResourceAccessFlag_BufferWrite;
			} break;
			case GPU_ResourceKind_Texture: {
				GPU_Check(binding_access->flags == GPU_AccessFlag_Read); // Textures can only be read
				access.access_flags |= GPU_ResourceAccessFlag_TextureRead;
				access.layer_count = texture->base.layer_count;
				
				if (binding_value.mip_level == GPU_MIP_LEVEL_ALL) {
					access.mip_level_count = texture->base.mip_level_count;
				}
				else {
					access.first_mip_level = binding_value.mip_level;
					access.mip_level_count = 1;
				}
			} break;
			case GPU_ResourceKind_Sampler: break;
			}
			DS_VecPush(&accesses, access);
		}
	}
	GPU_InsertBarriers(graph, accesses.data, (uint32_t)accesses.length);

	GPU_Check(graph->frame.img_index != GPU_NOT_A_SWAPCHAIN_GRAPH);
	uint32_t framebuffer_idx = render_to_swapchain ? graph->frame.img_index : 0;
	uint32_t width = render_to_swapchain ? FGPU.swapchain.width : render_pass->color_targets[0].texture->width;
	uint32_t height = render_to_swapchain ? FGPU.swapchain.height : render_pass->color_targets[0].texture->height;

	VkRenderPassBeginInfo render_info = {VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO};
	render_info.renderPass = render_pass->vk_handle;
	render_info.framebuffer = render_pass->framebuffers[framebuffer_idx];
	render_info.renderArea.extent.width = width;
	render_info.renderArea.extent.height = height;

	vkCmdBeginRenderPass(graph->cmd_buffer, &render_info, VK_SUBPASS_CONTENTS_INLINE);

	VkViewport viewport = {0.f, 0.f, (float)width, (float)height, GPU_REVERSE_DEPTH ? 1.f : 0.f, GPU_REVERSE_DEPTH ? 0.f : 1.f};
	VkRect2D scissor = {0, 0, width, height};
	vkCmdSetViewport(graph->cmd_buffer, 0, 1, &viewport);
	vkCmdSetScissor(graph->cmd_buffer, 0, 1, &scissor);
}

GPU_API void GPU_OpDispatch(GPU_Graph *graph, uint32_t group_count_x, uint32_t group_count_y, uint32_t group_count_z) {
	GPU_ComputePipeline *pipeline = graph->builder_state.compute_pipeline;
	GPU_DescriptorSet *desc_set = graph->builder_state.compute_descriptor_set;
	GPU_Check(pipeline != NULL && desc_set != NULL);

	// TODO: if you do multiple dispatches in a row with the same pipeline & descriptor set, we can skip this step. Though, InsertBarriers() should currently do nothing in that case anyway.
	DS_Vec(GPU_ResourceAccess) accesses = {0};
	accesses.arena = &graph->arena;

	DS_ForVecEach(GPU_Access, &pipeline->accesses, it) {
		GPU_BindingInfo binding_info = DS_VecGet(pipeline->layout->bindings, it.elem->binding);
		GPU_BindingValue binding_value = DS_VecGet(desc_set->bindings, it.elem->binding);
		
		uint32_t first_layer = 0, layer_count = 0;
		uint32_t first_mip_level = 0, mip_level_count = 1;

		GPU_ResourceAccessFlags access_flags = 0;
		switch (binding_info.kind) {
		case GPU_ResourceKind_StorageImage: {
			GPU_Texture *texture = (GPU_Texture*)binding_value.ptr;
			if (it.elem->flags & GPU_AccessFlag_Read)  access_flags |= GPU_ResourceAccessFlag_StorageImageRead;
			if (it.elem->flags & GPU_AccessFlag_Write) access_flags |= GPU_ResourceAccessFlag_StorageImageWrite;
			first_mip_level = binding_value.mip_level;
			layer_count = texture->layer_count;
		} break;
		case GPU_ResourceKind_Buffer: {
			if (it.elem->flags & GPU_AccessFlag_Read)  access_flags |= GPU_ResourceAccessFlag_BufferRead;
			if (it.elem->flags & GPU_AccessFlag_Write) access_flags |= GPU_ResourceAccessFlag_BufferWrite;
		} break;
		case GPU_ResourceKind_Sampler: break;
		case GPU_ResourceKind_Texture: {
			GPU_Texture *texture = (GPU_Texture*)binding_value.ptr;
			GPU_Check(it.elem->flags == GPU_AccessFlag_Read); // Textures can only be read from
			access_flags |= GPU_ResourceAccessFlag_TextureRead;
			
			if (binding_value.mip_level == GPU_MIP_LEVEL_ALL) {
				mip_level_count = texture->mip_level_count;
			}
			else {
				first_mip_level = binding_value.mip_level;
				mip_level_count = 1;
			}
			layer_count = texture->layer_count;
		} break;
		}

		GPU_ResourceAccess access = {binding_value.ptr, binding_info.kind, access_flags, first_layer, layer_count, first_mip_level, mip_level_count};
		DS_VecPush(&accesses, access);
	}
	GPU_InsertBarriers(graph, accesses.data, (uint32_t)accesses.length);

	vkCmdDispatch(graph->cmd_buffer, group_count_x, group_count_y, group_count_z);
}

GPU_API void GPU_OpDrawIndexed(GPU_Graph *graph, uint32_t index_count, uint32_t instance_count, uint32_t first_index, uint32_t vertex_offset, uint32_t first_instance) {
	vkCmdDrawIndexed(graph->cmd_buffer, index_count, instance_count, first_index, vertex_offset, first_instance);
}

GPU_API void GPU_OpDraw(GPU_Graph *graph, uint32_t vertex_count, uint32_t instance_count, uint32_t first_vertex, uint32_t first_instance) {
	vkCmdDraw(graph->cmd_buffer, vertex_count, instance_count, first_vertex, first_instance);
}

GPU_API void GPU_OpEndRenderPass(GPU_Graph *graph) {
	GPU_Check(graph->builder_state.render_pass != NULL);
	vkCmdEndRenderPass(graph->cmd_buffer);
	graph->builder_state.render_pass = NULL;
}

GPU_API void GPU_OpBlit(GPU_Graph *graph, const GPU_OpBlitInfo *info) {
	GPU_Check(graph->builder_state.render_pass == NULL); // You can't do this operation when inside OpBegin/EndRenderPass scope.
		
	if (info->dst_texture == info->src_texture) {
		// You may not blit from a texture into itself, unless either the layer or mip-level differs in the source / destination.
		// This decision is subject to change, as in Vulkan it would be technically allowed. However, the image layout would have to be made GENERAL rather than TRANSFER_SRC_OPTIMAL/TRANSFER_DST_OPTIMAL.
		GPU_Check(info->dst_mip_level != info->src_mip_level || info->src_layer != info->dst_layer);
	}

	GPU_ResourceAccess accesses[] = {
		{info->src_texture, GPU_ResourceKind_Texture, GPU_ResourceAccessFlag_TransferRead, info->src_layer, 1, info->src_mip_level, 1},
		{info->dst_texture, GPU_ResourceKind_Texture, GPU_ResourceAccessFlag_TransferWrite, info->dst_layer, 1, info->dst_mip_level, 1},
	};
	GPU_InsertBarriers(graph, accesses, GPU_ArrayCount(accesses));

	VkOffset3D src_offsets[2] = {{info->src_area[0].x, info->src_area[0].y, info->src_area[0].z}, {info->src_area[1].x, info->src_area[1].y, info->src_area[1].z}};
	VkOffset3D dst_offsets[2] = {{info->dst_area[0].x, info->dst_area[0].y, info->dst_area[0].z}, {info->dst_area[1].x, info->dst_area[1].y, info->dst_area[1].z}};

	VkImageBlit blit = {0};
	blit.srcOffsets[0] = src_offsets[0];
	blit.srcOffsets[1] = src_offsets[1];
	blit.srcSubresource.aspectMask = GPU_GetImageAspectFlags(info->src_texture->format);
	blit.srcSubresource.mipLevel = info->src_mip_level;
	blit.srcSubresource.baseArrayLayer = info->src_layer;
	blit.srcSubresource.layerCount = 1;
	blit.dstOffsets[0] = dst_offsets[0];
	blit.dstOffsets[1] = dst_offsets[1];
	blit.dstSubresource.aspectMask = GPU_GetImageAspectFlags(info->dst_texture->format);
	blit.dstSubresource.mipLevel = info->dst_mip_level;
	blit.dstSubresource.baseArrayLayer = 0;
	blit.dstSubresource.baseArrayLayer = info->dst_layer;
	blit.dstSubresource.layerCount = 1;

	VkFilter filter;
	switch (info->filter) {
	case GPU_Filter_Nearest: { filter = VK_FILTER_NEAREST; } break;
	case GPU_Filter_Linear: { filter = VK_FILTER_LINEAR; } break;
	}
	vkCmdBlitImage(graph->cmd_buffer, ((GPU_TextureImpl*)info->src_texture)->vk_handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
		((GPU_TextureImpl*)info->dst_texture)->vk_handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &blit, filter);
}

GPU_API void GPU_OpClearColorF(GPU_Graph *graph, GPU_Texture *dst, uint32_t mip_level, float r, float g, float b, float a) {
	GPU_Check(graph->builder_state.render_pass == NULL); // You can't do this operation when inside OpBegin/EndRenderPass scope.
	VkClearColorValue color;
	color.float32[0] = r; color.float32[1] = g; color.float32[2] = b; color.float32[3] = a;
		
	uint32_t first_level = 0, level_count = dst->mip_level_count;
	if (mip_level != GPU_MIP_LEVEL_ALL) {
		first_level = mip_level, level_count = 1;
	}

	GPU_ResourceAccess accesses[] = {{dst, GPU_ResourceKind_Texture, GPU_ResourceAccessFlag_TransferWrite, 0, 1, first_level, level_count}};
	GPU_InsertBarriers(graph, accesses, GPU_ArrayCount(accesses));

	VkImageSubresourceRange range = {0};
	range.aspectMask = GPU_GetImageAspectFlags(dst->format);
	range.layerCount = VK_REMAINING_ARRAY_LAYERS;
	range.baseMipLevel = first_level;
	range.levelCount = level_count;

	vkCmdClearColorImage(graph->cmd_buffer, ((GPU_TextureImpl*)dst)->vk_handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1, &range);
}

GPU_API void GPU_OpClearColorI(GPU_Graph *graph, GPU_Texture *dst, uint32_t mip_level, uint32_t r, uint32_t g, uint32_t b, uint32_t a) {
	GPU_Check(graph->builder_state.render_pass == NULL); // You can't do this operation when inside OpBegin/EndRenderPass scope.
	VkClearColorValue color;
	color.uint32[0] = r; color.uint32[1] = g; color.uint32[2] = b; color.uint32[3] = a;

	uint32_t first_level = 0, level_count = dst->mip_level_count;
	if (mip_level != GPU_MIP_LEVEL_ALL) {
		first_level = mip_level, level_count = 1;
	}

	GPU_ResourceAccess accesses[] = {{dst, GPU_ResourceKind_Texture, GPU_ResourceAccessFlag_TransferWrite, 0, 1, first_level, level_count}};
	GPU_InsertBarriers(graph, accesses, GPU_ArrayCount(accesses));

	VkImageSubresourceRange range = {0};
	range.aspectMask = GPU_GetImageAspectFlags(dst->format);
	range.layerCount = VK_REMAINING_ARRAY_LAYERS;
	range.baseMipLevel = first_level;
	range.levelCount = level_count;

	vkCmdClearColorImage(graph->cmd_buffer, ((GPU_TextureImpl*)dst)->vk_handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &color, 1, &range);
}

GPU_API void GPU_OpClearDepthStencil(GPU_Graph *graph, GPU_Texture *dst, uint32_t mip_level) {
	GPU_Check(graph->builder_state.render_pass == NULL); // You can't do this operation when inside OpBegin/EndRenderPass scope.
	GPU_FormatInfo format_info = GPU_GetFormatInfo(dst->format);
	GPU_Check(format_info.depth_target);

	uint32_t first_level = 0, level_count = dst->mip_level_count;
	if (mip_level != GPU_MIP_LEVEL_ALL) {
		first_level = mip_level, level_count = 1;
	}

	GPU_ResourceAccess accesses[] = {{dst, GPU_ResourceKind_Texture, GPU_ResourceAccessFlag_TransferWrite, 0, 1, first_level, level_count}};
	GPU_InsertBarriers(graph, accesses, GPU_ArrayCount(accesses));

	VkImageSubresourceRange range = {0};
	range.aspectMask = GPU_GetImageAspectFlags(dst->format);
	range.layerCount = VK_REMAINING_ARRAY_LAYERS;
	range.baseMipLevel = first_level;
	range.levelCount = level_count;

	VkClearDepthStencilValue depth_stencil = {1.f - GPU_REVERSE_DEPTH, 0};
	vkCmdClearDepthStencilImage(graph->cmd_buffer, ((GPU_TextureImpl*)dst)->vk_handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, &depth_stencil, 1, &range);
}

GPU_API void GPU_OpPushGraphicsConstants(GPU_Graph *graph, GPU_PipelineLayout *pipeline_layout, void *data, uint32_t size) {
	vkCmdPushConstants(graph->cmd_buffer, pipeline_layout->vk_handle, VK_SHADER_STAGE_ALL, 0, size, data);
}

GPU_API void GPU_OpPushComputeConstants(GPU_Graph *graph, GPU_PipelineLayout *pipeline_layout, void *data, uint32_t size) {
	vkCmdPushConstants(graph->cmd_buffer, pipeline_layout->vk_handle, VK_SHADER_STAGE_ALL, 0, size, data);
}

GPU_API void GPU_OpCopyBufferToBuffer(GPU_Graph *graph, GPU_Buffer *src, GPU_Buffer *dst, uint32_t dst_offset, uint32_t src_offset, uint32_t size) {
	GPU_Check(graph->builder_state.render_pass == NULL); // You can't do this operation when inside OpBegin/EndRenderPass scope.

	GPU_ResourceAccess accesses[] = {
		{src, GPU_ResourceKind_Buffer, GPU_ResourceAccessFlag_TransferRead, 0, 1, 0, 1},
		{dst, GPU_ResourceKind_Buffer, GPU_ResourceAccessFlag_TransferWrite, 0, 1, 0, 1},
	};
	GPU_InsertBarriers(graph, accesses, GPU_ArrayCount(accesses));

	VkBufferCopy region = { src_offset, dst_offset, size };
	vkCmdCopyBuffer(graph->cmd_buffer, ((GPU_BufferImpl*)src)->vk_handle, ((GPU_BufferImpl*)dst)->vk_handle, 1, &region);
}

GPU_API void GPU_OpCopyBufferToTexture(GPU_Graph *graph, GPU_Buffer *src, GPU_Texture *dst, uint32_t dst_first_layer, uint32_t dst_layer_count, uint32_t dst_mip_level) {
	GPU_Check(graph->builder_state.render_pass == NULL); // You can't do this operation when inside OpBegin/EndRenderPass scope.
		
	GPU_ResourceAccess accesses[] = {
		{src, GPU_ResourceKind_Buffer, GPU_ResourceAccessFlag_TransferRead, 0, 1, 0, 1},
		{dst, GPU_ResourceKind_Texture, GPU_ResourceAccessFlag_TransferWrite, dst_first_layer, dst_layer_count, dst_mip_level, 1},
	};
	GPU_InsertBarriers(graph, accesses, GPU_ArrayCount(accesses));
		
	VkExtent3D extent = {dst->width, dst->height, dst->depth};
	VkBufferImageCopy region = {0};
	region.imageSubresource.aspectMask = GPU_GetImageAspectFlags(dst->format);
	region.imageSubresource.mipLevel = dst_mip_level;
	region.imageSubresource.baseArrayLayer = dst_first_layer;
	region.imageSubresource.layerCount = dst_layer_count;
	region.imageExtent = extent;
	vkCmdCopyBufferToImage(graph->cmd_buffer, ((GPU_BufferImpl*)src)->vk_handle, ((GPU_TextureImpl*)dst)->vk_handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
}

GPU_API void GPU_OpCopyTextureToBuffer(GPU_Graph *graph, GPU_Texture *src, GPU_Buffer *dst) {
	GPU_Check(graph->builder_state.render_pass == NULL); // You can't do this operation when inside OpBegin/EndRenderPass scope.

	GPU_ResourceAccess accesses[] = {
		{src, GPU_ResourceKind_Texture, GPU_ResourceAccessFlag_TransferRead, 0, 1, 0, 1},
		{dst, GPU_ResourceKind_Buffer, GPU_ResourceAccessFlag_TransferWrite, 0, 1, 0, 1},
	};
	GPU_InsertBarriers(graph, accesses, GPU_ArrayCount(accesses));
	//if (src->flags & GPU_TextureFlag_Cubemap) BP();

	VkExtent3D extent = {src->width, src->height, src->depth};
	VkBufferImageCopy region = {0};
	region.imageSubresource.aspectMask = GPU_GetImageAspectFlags(src->format);
	region.imageSubresource.mipLevel = 0, // TODO: parametriz;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = src->flags & GPU_TextureFlag_Cubemap ? 6 : 1;
	region.imageExtent = extent;
	vkCmdCopyImageToBuffer(graph->cmd_buffer,
		((GPU_TextureImpl*)src)->vk_handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, ((GPU_BufferImpl*)dst)->vk_handle, 1, &region);
}

GPU_API void GPU_OpPrepareRenderPass(GPU_Graph *graph, GPU_RenderPass *render_pass) {
	GPU_Check(graph->builder_state.preparing_render_pass == NULL && graph->builder_state.render_pass == NULL);
		
	graph->builder_state.preparing_render_pass = render_pass;
	DS_VecClear(&graph->builder_state.prepared_draw_params);
}

GPU_API uint32_t GPU_OpPrepareDrawParams(GPU_Graph *graph, GPU_GraphicsPipeline *pipeline, GPU_DescriptorSet *descriptor_set) {
	GPU_Check(graph->builder_state.preparing_render_pass != NULL);

	uint32_t idx = (uint32_t)graph->builder_state.prepared_draw_params.length;
	GPU_DrawParams draw_params = {pipeline, descriptor_set};
	DS_VecPush(&graph->builder_state.prepared_draw_params, draw_params);
	return idx;
}

GPU_API void GPU_OpBindDrawParams(GPU_Graph *graph, uint32_t draw_params) {
	GPU_DrawParams dt = DS_VecGet(graph->builder_state.prepared_draw_params, draw_params);
		
	if (dt.pipeline != graph->builder_state.draw_pipeline) {
		vkCmdBindPipeline(graph->cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, dt.pipeline->vk_handle);
		graph->builder_state.draw_pipeline = dt.pipeline;
	}

	if (dt.desc_set != graph->builder_state.draw_descriptor_set) {
		vkCmdBindDescriptorSets(graph->cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, dt.desc_set->pipeline_layout->vk_handle, 0, 1, &dt.desc_set->vk_handle, 0, NULL);
		graph->builder_state.draw_descriptor_set = dt.desc_set;
	}
}
