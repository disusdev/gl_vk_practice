#ifndef __VK_APP_H__
#define __VK_APP_H__

#include <defines.h>

#define VK_USE_PLATFORM_WIN32_KHR
#include <volk/volk.h>

#include <glfw/include/GLFW/glfw3.h>
#include <glfw/include/GLFW/glfw3native.h>

#include <math/mathm.h>

#include <tools.h>
#include <resource/model_loader.h>
#include <vk/vk_core.h>

// #include "vk_shaders.h"

#include "vk_resources.h"

#include <vk/renderers/vk_renderer.h>

// #include <perf/fps_counter.h>
// #include <perf/fps_graph.h>

typedef struct
t_resolution
{
  u32 width;
  u32 height;
}
t_resolution;

typedef struct
t_render_item
{
	t_vk_renderer* renderer;
	b8 enabled;
	b8 use_depth;
}
t_render_item;

typedef struct
t_vulkan_render_context
{
	vulkan_instance vk;
	vulkan_render_device vk_dev;
	t_vk_resources resources;
	
	// vulkan_context_creator ctx_creator;

	cvec(t_render_item) on_screen_renderers;

	vulkan_texture depth_texture;

	// vulkan_render_pass screen_render_pass;
	// vulkan_render_pass screen_render_pass_no_depth;

	// vulkan_render_pass clear_render_pass;
	// vulkan_render_pass final_render_pass;

	cvec(VkFramebuffer) swapchain_framebuffers;
	cvec(VkFramebuffer) swapchain_framebuffers_no_depth;

	// void(*update_buffer)(u32);
  // void(*compose_frame)(VkCommandBuffer, u32);
}
t_vulkan_render_context;

t_vulkan_render_context
vulkan_render_context_create(void* window,
														 u32 screenWidth,
														 u32 screenHeight,
														 t_vulkan_context_features ctx_features /*= VulkanContextFeatures()*/);

void
vulkan_render_context_update_buffers(t_vulkan_render_context* ctx,
																		 u32 image_index);

void
vulkan_render_context_compose_frame(t_vulkan_render_context* ctx,
																		VkCommandBuffer command_buffer,
																		u32 image_index);

void
vulkan_render_context_destroy(t_vulkan_render_context* ctx);

// vulkan_pipeline_info pipeline_parametrs_for_outputs(cvec(vulkan_texture)* outputs);

void
vulkan_render_context_begin_render_pass(t_vulkan_render_context* ctx,
																				VkCommandBuffer cmdBuffer,
																				VkRenderPass pass,
																				size_t currentImage,
																				const VkRect2D area,
																				VkFramebuffer fb /*= VK_NULL_HANDLE*/,
																				uint32_t clearValueCount /*= 0*/,
																				const VkClearValue* clearValues /*= nullptr*/);

typedef struct
t_vk_app
{
	t_resolution resolution;
  GLFWwindow* window;
	t_vulkan_render_context* ctx;
	cvec(t_render_item*) on_screen_renderer;

	f64 timeStamp;// = glfwGetTime();
	f32 deltaSeconds;// = 0;
}
t_vk_app;

t_resolution
detect_resolution(int width,
                  int height);

t_vk_app
vk_app_create(int width,
              int height,
							vulkan_context_features ctx_features);

b8
draw_frame(vulkan_render_device* vk_dev,
           void(*update_buffer_func)(u32),
           void(*compose_frame_func)(VkCommandBuffer, u32));

typedef struct
t_camera_app
{
  t_vk_app* vk_app;	
}
t_camera_app;

t_camera_app
camera_app_create(i32 width,
									i32 height);

#endif