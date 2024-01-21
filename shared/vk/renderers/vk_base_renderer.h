#ifndef __VK_BASE_RENDERER_H__
#define __VK_BASE_RENDERER_H__

#include <defines.h>

typedef struct
t_base_renderer
{
  VkDevice device;
  i32 framebuffer_width;
  i32 framebuffer_height;
  vulkan_image depth_texture;
  
  // Descriptor set (layout + pool + sets) -> uses uniform buffers, textures, framebuffers
  VkDescriptorSetLayout descriptor_set_layout;
  VkDescriptorPool descriptor_pool;
  cvec(VkDescriptorSet) descriptor_sets;

  // Framebuffers (one for each command buffer)
  cvec(VkFramebuffer) swapchain_framebuffers;

  // 4. Pipeline & render pass (using DescriptorSets & pipeline state options)
  VkRenderPass render_pass;
  VkPipelineLayout pipeline_layout;
  VkPipeline graphics_pipeline;

  // 5. Uniform buffer
  cvec(VkBuffer) uniform_buffers;
  cvec(VkDeviceMemory) uniform_buffers_memory;

  void(*fill_command_buffer)(void*, VkCommandBuffer, u64);
  void(*push_constants)(void*, VkCommandBuffer, u32, vec2*);
}
t_base_renderer;

t_base_renderer*
base_renderer_create(vulkan_render_device* vk_device,
                     vulkan_image depth_texture)
{
  t_base_renderer* base_renderer = malloc(sizeof(t_base_renderer));
  memset(base_renderer, 0, sizeof(t_base_renderer));

  base_renderer->device = vk_device->device;
  base_renderer->depth_texture = depth_texture;
  base_renderer->framebuffer_width = vk_device->framebuffer_width;
  base_renderer->framebuffer_height = vk_device->framebuffer_height;

  return base_renderer;
}

void
base_renderer_begin_render_pass(t_base_renderer* base_renderer,
                                VkCommandBuffer cmd_buffer,
                                u64 current_image)
{
  VkRect2D screenRect =
  {
		.offset = (VkOffset2D) { 0, 0 },
		.extent = (VkExtent2D) { .width = base_renderer->framebuffer_width, .height = base_renderer->framebuffer_height }
	};

	VkRenderPassBeginInfo renderPassInfo =
  {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = NULL,
		.renderPass = base_renderer->render_pass,
		.framebuffer = base_renderer->swapchain_framebuffers[current_image],
		.renderArea = screenRect
	};

	vkCmdBeginRenderPass(cmd_buffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdBindPipeline(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, base_renderer->graphics_pipeline);
	vkCmdBindDescriptorSets(cmd_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, base_renderer->pipeline_layout, 0, 1, &base_renderer->descriptor_sets[current_image], 0, NULL);
}

b8
base_renderer_create_uniform_buffers(t_base_renderer* base_renderer,
                                     vulkan_render_device* vk_device,
                                     u64 uniform_data_size)
{
  base_renderer->uniform_buffers = cvec_ncreate(VkBuffer, cvec_size(vk_device->swapchain_images));
  base_renderer->uniform_buffers_memory = cvec_ncreate(VkDeviceMemory, cvec_size(vk_device->swapchain_images));

  for (u64 i = 0; i < cvec_size(vk_device->swapchain_images); i++)
	{
		if (!vulkan_create_uniform_buffer(vk_device, &base_renderer->uniform_buffers[i], &base_renderer->uniform_buffers_memory[i], uniform_data_size))
		{
			printf("Cannot create uniform buffer\n");
			fflush(stdout);
			return false;
		}
	}
	return true;
}

void
base_renderer_destory(t_base_renderer* base_renderer)
{
  for (u64 i = 0; i < cvec_size(base_renderer->uniform_buffers); i++)
  {
		vkDestroyBuffer(base_renderer->device, base_renderer->uniform_buffers[i], NULL);
  }

  for (u64 i = 0; i < cvec_size(base_renderer->uniform_buffers_memory); i++)
  {
		vkFreeMemory(base_renderer->device, base_renderer->uniform_buffers_memory[i], NULL);
  }

	vkDestroyDescriptorSetLayout(base_renderer->device, base_renderer->descriptor_set_layout, NULL);
	vkDestroyDescriptorPool(base_renderer->device, base_renderer->descriptor_pool, NULL);

  for (u64 i = 0; i < cvec_size(base_renderer->swapchain_framebuffers); i++)
  {
		vkDestroyFramebuffer(base_renderer->device, base_renderer->swapchain_framebuffers[i], NULL);
  }

	vkDestroyRenderPass(base_renderer->device, base_renderer->render_pass, NULL);
	vkDestroyPipelineLayout(base_renderer->device, base_renderer->pipeline_layout, NULL);
	vkDestroyPipeline(base_renderer->device, base_renderer->graphics_pipeline, NULL);
}

#endif