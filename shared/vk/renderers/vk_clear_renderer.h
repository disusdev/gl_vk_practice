#ifndef __VK_CLEAR_RENDERER_H__
#define __VK_CLEAR_RENDERER_H__

#include <defines.h>

static VkClearColorValue clear_value_color = (VkClearColorValue) { 1.0f, 1.0f, 1.0f, 1.0f };

typedef struct
t_clear_renderer
{
  t_base_renderer* base;
  b8 should_clear_depth;
}
t_clear_renderer;

static void
clear_renderer_fill_command_buffer(t_clear_renderer* clear_renderer,
                                   VkCommandBuffer commandBuffer,
                                   u64 currentImage)
{
  VkClearValue clearValues[] =
	{
		(VkClearValue) {.color = clear_value_color },
		(VkClearValue) {.depthStencil = { 1.0f, 0 } }
	};

	VkRect2D screenRect = (VkRect2D)
	{
		.offset = { 0, 0 },
		.extent = { .width = clear_renderer->base->framebuffer_width, .height = clear_renderer->base->framebuffer_height }
	};

  VkRenderPassBeginInfo renderPassInfo = (VkRenderPassBeginInfo)
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = NULL,
		.renderPass = clear_renderer->base->render_pass,
		.framebuffer = clear_renderer->base->swapchain_framebuffers[currentImage],
		.renderArea = screenRect,
		.clearValueCount = (u32) (clear_renderer->should_clear_depth ? ARRAY_SIZE(clearValues) : 1),
		.pClearValues = clearValues
	};

	vkCmdBeginRenderPass( commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );
	vkCmdEndRenderPass( commandBuffer );
}

t_clear_renderer
clear_renderer_create(vulkan_render_device* vkDev,
                      vulkan_image depthTexture)
{
  t_clear_renderer clear_renderer = { 0 };

  clear_renderer.base = base_renderer_create(vkDev, (vulkan_image) { 0, 0, 0 });
	clear_renderer.base->fill_command_buffer = &clear_renderer_fill_command_buffer;
	clear_renderer.base->depth_texture = depthTexture;

  clear_renderer.should_clear_depth = (depthTexture.handle != VK_NULL_HANDLE);

  RenderPassCreateInfo render_pass_info =
  {
    .clearColor_ = true,
    .clearDepth_ = true,
    .flags_ = eRenderPassBit_First
  };

  if (!vulkan_create_color_and_depth_render_pass(vkDev,
                                                 clear_renderer.should_clear_depth,
                                                 &clear_renderer.base->render_pass,
                                                 &render_pass_info,
                                                 VK_FORMAT_B8G8R8A8_UNORM))
	{
		printf("VulkanClear: failed to create render pass\n");
		exit(EXIT_FAILURE);
	}

  vulkan_create_color_and_depth_framebuffers(vkDev,
																						 clear_renderer.base->render_pass,
																						 clear_renderer.base->depth_texture.view,
																						 &clear_renderer.base->swapchain_framebuffers);

	return clear_renderer;
}

void
clear_renderer_destroy(t_clear_renderer* clear_renderer)
{
	base_renderer_destory(clear_renderer->base);
	clear_renderer->base = NULL;
}

#endif