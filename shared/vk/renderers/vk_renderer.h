#ifndef __VK_RENDERER_H__
#define __VK_RENDERER_H__

#include <defines.h>

typedef struct
t_vk_renderer
{
  // vulkan_render_context* ctx;

  VkFramebuffer framebuffer;
	// vulkan_render_pass render_pass;

	u32 processing_width;
	u32 processing_height;

  VkDescriptorSetLayout descriptor_set_layout;
	VkDescriptorPool descriptor_pool;
	cvec(VkDescriptorSet) descriptor_sets;

	VkPipelineLayout pipeline_layout;
	VkPipeline graphics_pipeline;

	cvec(vulkan_buffer) uniforms;

  void(*fill_command_buffer)(void*, VkCommandBuffer, u64, VkFramebuffer /*= VK_NULL_HANDLE*/, VkRenderPass /*= VK_NULL_HANDLE*/);
  void(*update_buffers)(void*, u64);
}
t_vk_renderer;

// create

// update_uniform_buffers

// init_pipeline

// init_render_pass

// begin_render_pass

// update_texture

// destroy

#endif