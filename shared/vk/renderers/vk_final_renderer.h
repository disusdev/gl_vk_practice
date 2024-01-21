#ifndef __VK_FINAL_RENDERER_H__
#define __VK_FINAL_RENDERER_H__

#include <defines.h>

typedef struct
t_final_renderer
{
  t_base_renderer* base;
}
t_final_renderer;

static void
final_renderer_fill_command_buffer(t_final_renderer* final_renderer,
                                   VkCommandBuffer commandBuffer,
                                   u64 currentImage)
{
	VkRect2D screenRect = (VkRect2D)
	{
		.offset = { 0, 0 },
		.extent = { .width = final_renderer->base->framebuffer_width, .height = final_renderer->base->framebuffer_height }
	};

  VkRenderPassBeginInfo renderPassInfo = (VkRenderPassBeginInfo)
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = NULL,
		.renderPass = final_renderer->base->render_pass,
		.framebuffer = final_renderer->base->swapchain_framebuffers[currentImage],
		.renderArea = screenRect
	};

	vkCmdBeginRenderPass( commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );
	vkCmdEndRenderPass( commandBuffer );
}

t_final_renderer
final_renderer_create(vulkan_render_device* vkDev,
                      vulkan_image depthTexture)
{
  t_final_renderer final_renderer = { 0 };
  final_renderer.base = base_renderer_create(vkDev, depthTexture);
  final_renderer.base->fill_command_buffer = &final_renderer_fill_command_buffer;

  RenderPassCreateInfo render_pass_info =
  {
    .clearColor_ = false,
    .clearDepth_ = false,
    .flags_ = eRenderPassBit_Last
  };

  if (!vulkan_create_color_and_depth_render_pass(vkDev,
                                                 (depthTexture.handle != VK_NULL_HANDLE),
                                                 &final_renderer.base->render_pass,
                                                 &render_pass_info,
                                                 VK_FORMAT_B8G8R8A8_UNORM))
	{
		printf("VulkanFinal: failed to create render pass\n");
		exit(EXIT_FAILURE);
	}

  vulkan_create_color_and_depth_framebuffers(vkDev,
																						 final_renderer.base->render_pass,
																						 final_renderer.base->depth_texture.view,
																						 &final_renderer.base->swapchain_framebuffers);

  return final_renderer;
}

void
final_renderer_destroy(t_final_renderer* final_renderer)
{
  base_renderer_destory(final_renderer->base);
	final_renderer->base = NULL;
}

#endif