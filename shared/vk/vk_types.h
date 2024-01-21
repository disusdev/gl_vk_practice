#ifndef __VK_TYPES_H__
#define __VK_TYPES_H__

// #define VK_USE_PLATFORM_WIN32_KHR
#define VK_NO_PROTOTYPES
#include <volk/volk.h>

#include <defines.h>

#define VK_CHECK(value) do{ VkResult res = value;  CHECK(res == VK_SUCCESS, __FILE__, __LINE__, res); }while(0)
#define VK_CHECK_RET(value) do{ VkResult __res = value; if ( __res != VK_SUCCESS ) { CHECK(false, __FILE__, __LINE__, __res); return __res; }}while(0)
#define BL_CHECK(value) CHECK(value, __FILE__, __LINE__, value);

typedef struct
vulkan_instance
{
  VkInstance instance;
  VkSurfaceKHR surface;
  VkDebugUtilsMessengerEXT messenger;
  VkDebugReportCallbackEXT report_callback;
}
vulkan_instance;

typedef struct
vulkan_render_device
{
  u32 framebuffer_width;
  u32 framebuffer_height;

  VkDevice device;
  VkQueue graphics_queue;
  VkPhysicalDevice physical_device;

  u32 graphics_family;

  VkSwapchainKHR swapchain;
  VkSemaphore semaphore;
  VkSemaphore render_semaphore;

  cvec(VkImage) swapchain_images;
  cvec(VkImageView) swapchain_image_views;

  VkCommandPool command_pool;
  cvec(VkCommandBuffer) command_buffers;

  b8 use_compute;

  u32 compute_family;
  VkQueue compute_queue;

  cvec(u32) device_queue_indices;
  cvec(VkQueue) device_queues;

  VkCommandBuffer compute_command_buffer;
  VkCommandPool compute_command_pool;

  // t_model* model;
  // int mesh_idx;
}
vulkan_render_device;

typedef struct
vulkan_context_features
{
  b8 support_screenshots;

  b8 geometry_shader;
  b8 tesselation_shader;

  b8 vertex_pipeline_stores_and_atomics;
  b8 fragment_pipeline_stores_and_atomics;
}
vulkan_context_features;

typedef struct
swapchain_support_details
{
  VkSurfaceCapabilitiesKHR capabilities;
  cvec(VkSurfaceFormatKHR) formats;
  cvec(VkPresentModeKHR) preset_modes;
}
swapchain_support_details;

typedef struct
shader_module
{
  cvec(u32) raw_spirv;
  VkShaderModule handle;
}
shader_module;

typedef struct
vulkan_buffer
{
  VkBuffer handle;
  VkDeviceSize size;
  VkDeviceMemory memory;
  void* ptr;
}
vulkan_buffer;

typedef struct
vulkan_image
{
  VkImage handle;
  VkImageView view;
  VkDeviceMemory memory;
}
vulkan_image;

typedef struct
vulkan_texture
{
  vulkan_image image;

  VkSampler sampler;
  VkFormat format;
  VkImageLayout desired_layout;
  
  u32 width;
  u32 height;
  u32 depth;
}
vulkan_texture;

enum eRenderPassBit
{
  eRenderPassBit_First = 0x01,
  eRenderPassBit_Last = 0x02,
  eRenderPassBit_Offscreen = 0x04,
  eRenderPassBit_OffscreenInternal = 0x08,
};

typedef struct
RenderPassCreateInfo
{
  b8 clearColor_/* = false*/;
  b8 clearDepth_/* = false*/;
  u8 flags_/* = 0*/;
}
RenderPassCreateInfo;

typedef struct
ShaderModule
{
  cvec(u32) SPIRV;
  VkShaderModule shaderModule/* = nullptr */;
}
ShaderModule;

typedef struct
t_vulkan_context_features
{
	b8 support_screenshots /*= false*/;

	b8 geometry_shader /*= true*/;
	b8 tessellation_shader /*= false*/;

	b8 vertexPipeline_stores_and_atomics /*= false*/;
	b8 fragment_stores_and_atomics /*= false*/;
}
t_vulkan_context_features;

void vulkan_create_context(const vulkan_instance* vk,
                           const vulkan_render_device* dev,
                           void* window,
                           int screen_width,
                           int screen_height,
                           const vulkan_context_features* ctx_features);

#endif