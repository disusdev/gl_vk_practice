#include "vk_app.h"

t_vulkan_render_context
vulkan_render_context_create(void* window,
														 u32 screenWidth,
														 u32 screenHeight,
														 t_vulkan_context_features ctx_features /*= VulkanContextFeatures()*/)
{
  t_vulkan_render_context ctx = { 0 };

  vulkan_create_instance(&ctx.vk.instance);

	if (!vulkan_setup_debug_callbacks(ctx.vk.instance, &ctx.vk.messenger, &ctx.vk.report_callback))
  {
		exit(0);
  }

	if (glfwCreateWindowSurface(ctx.vk.instance, (GLFWwindow*)window, NULL, &ctx.vk.surface))
  {
		exit(0);
  }

	if (!vulkan_init_render_device3(&ctx.vk, &ctx.vk_dev, screenWidth, screenHeight, ctx_features))
  {
		exit(0);
  }

  return ctx;
}

void
vulkan_render_context_destroy(t_vulkan_render_context* ctx)
{
	vulkan_destroy_render_device(&ctx->vk_dev);
  vulkan_destroy_instance(&ctx->vk);
}

// void(*update_buffer_func)(u32),
// void(*compose_frame_func)(VkCommandBuffer, u32)

void
vulkan_render_context_update_buffers(t_vulkan_render_context* ctx,
																		 u32 image_index)
{
  // for (auto& r : onScreenRenderers_)
	// 	if (r.enabled_)
	// 		r.renderer_.updateBuffers(imageIndex);

	for (u64 i = 0; i < cvec_size(ctx->on_screen_renderers); i++)
	{
		if (ctx->on_screen_renderers[i].enabled)
		{
			t_vk_renderer* base = *(t_vk_renderer**)ctx->on_screen_renderers[i].renderer;
			base->update_buffers(ctx->on_screen_renderers[i].renderer, image_index);
		}
	}
}

void
vulkan_render_context_compose_frame(t_vulkan_render_context* ctx,
																		VkCommandBuffer command_buffer,
																		u32 image_index)
{
  const VkRect2D defaultScreenRect =
	{
		.offset = { 0, 0 },
		.extent = {.width = ctx->vk_dev.framebuffer_width, .height = ctx->vk_dev.framebuffer_height }
	};

	/*static*/ const VkClearValue defaultClearValues[2] =
	{
		(VkClearValue) { .color = { 1.0f, 1.0f, 1.0f, 1.0f } },
		(VkClearValue) { .depthStencil = { 1.0f, 0 } }
	};

	// beginRenderPass(commandBuffer, clearRenderPass.handle, imageIndex, defaultScreenRect, VK_NULL_HANDLE, 2u, defaultClearValues);
	// vkCmdEndRenderPass( commandBuffer );

	// for (auto& r : onScreenRenderers_)
	// 	if (r.enabled_)
	// 	{
	// 		RenderPass rp = r.useDepth_ ? screenRenderPass : screenRenderPass_NoDepth;
	// 		VkFramebuffer fb = (r.useDepth_ ? swapchainFramebuffers : swapchainFramebuffers_NoDepth)[imageIndex];

	// 		if (r.renderer_.renderPass_.handle != VK_NULL_HANDLE)
	// 			rp = r.renderer_.renderPass_;
	// 		if (r.renderer_.framebuffer_ != VK_NULL_HANDLE)
	// 			fb = r.renderer_.framebuffer_;

	// 		r.renderer_.fillCommandBuffer(commandBuffer, imageIndex, fb, rp.handle);
	// 	}

	// beginRenderPass(commandBuffer, finalRenderPass.handle, imageIndex, defaultScreenRect);
	// vkCmdEndRenderPass( commandBuffer );
}

void
vulkan_render_context_begin_render_pass(t_vulkan_render_context* ctx,
																				VkCommandBuffer cmdBuffer,
																				VkRenderPass pass,
																				size_t currentImage,
																				const VkRect2D area,
																				VkFramebuffer fb /*= VK_NULL_HANDLE*/,
																				uint32_t clearValueCount /*= 0*/,
																				const VkClearValue* clearValues /*= nullptr*/)
{
  const VkRenderPassBeginInfo renderPassInfo =
  {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = pass,
		.framebuffer = (fb != VK_NULL_HANDLE) ? fb : ctx->swapchain_framebuffers[currentImage],
		.renderArea = area,
		.clearValueCount = clearValueCount,
		.pClearValues = clearValues
	};

	vkCmdBeginRenderPass( cmdBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE );
}

t_resolution
detect_resolution(int width,
                  int height)
{
	GLFWmonitor* monitor = glfwGetPrimaryMonitor();
	const int code = glfwGetError(NULL);

	if (code != 0)
	{
		printf("Monitor: %p; error = %x / %d\n", monitor, code, code);
		exit(255);
	}

	const GLFWvidmode* info = glfwGetVideoMode(monitor);

	const u32 windowW = width  > 0 ? width : (u32)(info->width * width / -100);
	const u32 windowH = height > 0 ? height : (u32)(info->height * height / -100);

	return (t_resolution) { .width = windowW, .height = windowH };
}

t_vk_app
vk_app_create(int width,
              int height,
              vulkan_context_features ctx_features)
{
  t_vk_app vk_app = {0};

  volkInitialize();

  if (!glfwInit())
		exit(EXIT_FAILURE);

	if (!glfwVulkanSupported())
		exit(EXIT_FAILURE);

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

	vk_app.resolution = detect_resolution(width, height);
	width  = resolution->width;
	height = resolution->height;

  vk_app.window = glfwCreateWindow(width, height, "VulkanApp", NULL, NULL);

	if (!vk_app.window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

  vk_app.timeStamp = glfwGetTime();
  vk_app.ctx = vulkan_render_context_create(vk_app.window, width, height, ctx_features);

  vk_app.on_screen_renderers = cvec_ncreate_copy(t_render_item,
                                                 cvec_size(vk_app.ctx.on_screen_renderers),
                                                 vk_app.ctx.on_screen_renderers,
                                                 cvec_size(vk_app.ctx.on_screen_renderers));

  // glfwSetWindowUserPointer(vk_app.window, this);
	// assignCallbacks();

	return vk_app;
}

b8
draw_frame(vulkan_render_device* vk_dev,
           void(*update_buffer_func)(u32),
           void(*compose_frame_func)(VkCommandBuffer, u32))
{
  u32 imageIndex = 0;
	if (vkAcquireNextImageKHR(vk_dev->device, vk_dev->swapchain, 0, vk_dev->semaphore, VK_NULL_HANDLE, &imageIndex) != VK_SUCCESS)
		return false;

	VK_CHECK(vkResetCommandPool(vk_dev->device, vk_dev->command_pool, 0));

	// update3D(imageIndex);
	// update2D(imageIndex);

	if (update_buffer_func)
		update_buffer_func(imageIndex);

	VkCommandBuffer commandBuffer = vk_dev->command_buffers[imageIndex];

	VkCommandBufferBeginInfo bi = (VkCommandBufferBeginInfo)
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = NULL,
		.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
		.pInheritanceInfo = NULL
	};

	VK_CHECK(vkBeginCommandBuffer(commandBuffer, &bi));

	// for (u64 i = 0; i < renderers_count; i++)
	// {
	// 	t_base_renderer* base = *(t_base_renderer**)renderers[i];
	// 	base->fill_command_buffer(renderers[i], commandBuffer, imageIndex);
	// }

	if(compose_frame_func)
		compose_frame_func(commandBuffer, imageIndex);

	VK_CHECK(vkEndCommandBuffer(commandBuffer));

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }; // or even VERTEX_SHADER_STAGE

	VkSubmitInfo si = (VkSubmitInfo)
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = NULL,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vk_dev->semaphore,
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &vk_dev->command_buffers[imageIndex],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &vk_dev->render_semaphore
	};

	VK_CHECK(vkQueueSubmit(vk_dev->graphics_queue, 1, &si, NULL));

	VkPresentInfoKHR pi =
	{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = NULL,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vk_dev->render_semaphore,
		.swapchainCount = 1,
		.pSwapchains = &vk_dev->swapchain,
		.pImageIndices = &imageIndex
	};

	VK_CHECK(vkQueuePresentKHR(vk_dev->graphics_queue, &pi));
	VK_CHECK(vkDeviceWaitIdle(vk_dev->device));

	return true;
}