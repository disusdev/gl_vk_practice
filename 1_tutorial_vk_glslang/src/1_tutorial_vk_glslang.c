

#include <defines.h>

#include <stdlib.h>
#include <stdio.h>

#define VK_USE_PLATFORM_WIN32_KHR
//#define VOLK_IMPLEMENTATION
#include <volk/volk.h>

#include <glfw/include/GLFW/glfw3.h>
#include <glfw/include/GLFW/glfw3native.h>

#include <math/mathm.c>

#define CVEC_IMPLEMENTATION
#define CVEC_STDLIB
#include <containers/cvec.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#define TINYOBJ_MALLOC malloc
#define TINYOBJ_CALLOC calloc
#define TINYOBJ_REALLOC realloc
#define TINYOBJ_FREE free

#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include <tinyobj_loader_c.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <tools.h>
#include <resource/model_loader.h>
#include <vk/vk_shader_compiler.h>
#include <vk/vk_core.h>

const u32 g_screen_width = 1280;
const u32 g_screen_height = 720;

struct GLFWwindow* window;

typedef struct
t_uniform_buffer
{
  mat4 mvp;
}
t_uniform_buffer;

VkClearColorValue clear_value_color = (VkClearColorValue) { 0.3f, 0.3f, 0.3f, 1.0f };

u64 vertex_buffer_size;
u64 index_buffer_size;

vulkan_instance vk;
vulkan_render_device vk_dev;

typedef struct
t_vulkan_state
{
  // 1.
  VkDescriptorSetLayout descriptor_set_layout;
  VkDescriptorPool descriptor_pool;
  cvec(VkDescriptorSet) descriptor_sets;

  // 2.
  cvec(VkFramebuffer) swapchain_framebuffers;

  // 3.
  VkRenderPass render_pass;
  VkPipelineLayout pipeline_layout;
  VkPipeline graphics_pipeline;

  // 4.
  cvec(VkBuffer) uniform_buffers;
  cvec(VkDeviceMemory) uniform_buffers_memory;

  // 5.
  VkBuffer storage_buffer;
  VkDeviceMemory storage_buffer_memory;

  // 6.
  vulkan_image depth_texture;

  VkSampler texture_sampler;
  vulkan_image texture;
}
t_vulkan_state;

static t_vulkan_state vk_state;

void keyboard_callback(struct GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
}

b8 isDeviceSuitable(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	b8 isDiscreteGPU = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
	b8 isIntegratedGPU = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
	b8 isGPU = isDiscreteGPU || isIntegratedGPU;

	return isGPU && deviceFeatures.geometryShader;
}

b8 vulkan_create_uniform_buffers()
{
	VkDeviceSize bufferSize = sizeof(t_uniform_buffer);

	vk_state.uniform_buffers = cvec_ncreate(VkBuffer, cvec_size(vk_dev.swapchain_images));

	vk_state.uniform_buffers_memory = cvec_ncreate(VkDeviceMemory, cvec_size(vk_dev.swapchain_images));

	for (size_t i = 0; i < cvec_size(vk_dev.swapchain_images); i++)
	{
		if (!vulkan_create_buffer(vk_dev.device,
															vk_dev.physical_device,
															bufferSize,
															VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
															VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
															VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
															&vk_state.uniform_buffers[i],
															&vk_state.uniform_buffers_memory[i]))
		{
			printf("Fail: buffers\n");
			return false;
		}
	}

	return true;
}

b8
createDescriptorSet()
{
	VkDescriptorSetLayoutBinding bindings[] =
	{
		descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1),
		descriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1),
		descriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1),
		descriptorSetLayoutBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
	};

	VkDescriptorSetLayoutCreateInfo layoutInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.bindingCount = (u32)ARRAY_SIZE(bindings),
		.pBindings = bindings
	};

	VK_CHECK(vkCreateDescriptorSetLayout(vk_dev.device, &layoutInfo, NULL, &vk_state.descriptor_set_layout));

	// std::vector<VkDescriptorSetLayout> layouts(vkDev.swapchainImages.size(), vkState.descriptorSetLayout);
	cvec(VkDescriptorSetLayout) layouts = cvec_ncreate(VkDescriptorSetLayout, cvec_size(vk_dev.swapchain_images));

	for (u64 i = 0; i < cvec_size(layouts); i++)
	{
		layouts[i] = vk_state.descriptor_set_layout;
	}

	VkDescriptorSetAllocateInfo allocInfo =
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = NULL,
		.descriptorPool = vk_state.descriptor_pool,
		.descriptorSetCount = (u32)cvec_size(vk_dev.swapchain_images),
		.pSetLayouts = layouts
	};

	vk_state.descriptor_sets = cvec_ncreate(VkDescriptorSet, cvec_size(vk_dev.swapchain_images));
	// cvec_resize(vk_state.descriptor_sets, cvec_size(vk_dev.swapchain_images));

	VK_CHECK(vkAllocateDescriptorSets(vk_dev.device, &allocInfo, vk_state.descriptor_sets));

	for (size_t i = 0; i < cvec_size(vk_dev.swapchain_images); i++)
	{
		VkDescriptorBufferInfo bufferInfo =
		{
			.buffer = vk_state.uniform_buffers[i],
			.offset = 0,
			.range = sizeof(t_uniform_buffer)
		};
		VkDescriptorBufferInfo bufferInfo2 =
		{
			.buffer = vk_state.storage_buffer,
			.offset = 0,
			.range = vertex_buffer_size
		};
		VkDescriptorBufferInfo bufferInfo3 =
		{
			.buffer = vk_state.storage_buffer,
			.offset = vertex_buffer_size,
			.range = index_buffer_size
		};
		VkDescriptorImageInfo imageInfo =
		{
			.sampler = vk_state.texture_sampler,
			.imageView = vk_state.texture.view,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};

		VkWriteDescriptorSet descriptorWrites[] =
		{
			(VkWriteDescriptorSet)
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = vk_state.descriptor_sets[i],
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.pBufferInfo = &bufferInfo
			},
			(VkWriteDescriptorSet)
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = vk_state.descriptor_sets[i],
				.dstBinding = 1,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.pBufferInfo = &bufferInfo2
			},
			(VkWriteDescriptorSet)
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = vk_state.descriptor_sets[i],
				.dstBinding = 2,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.pBufferInfo = &bufferInfo3
			},
			(VkWriteDescriptorSet)
			{
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = vk_state.descriptor_sets[i],
				.dstBinding = 3,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = &imageInfo
			},
		};

		vkUpdateDescriptorSets(vk_dev.device, (u32)ARRAY_SIZE(descriptorWrites), descriptorWrites, 0, NULL);
	}

	return true;
}

b8 vulkan_init()
{
  vulkan_create_instance(&vk.instance);

	if (!vulkan_setup_debug_callbacks(vk.instance, &vk.messenger, &vk.report_callback))
		exit(EXIT_FAILURE);

	if (glfwCreateWindowSurface(vk.instance, window, NULL, &vk.surface))
		exit(EXIT_FAILURE);

  if (!vulkan_init_render_device(&vk, &vk_dev, g_screen_width, g_screen_height, isDeviceSuitable, (VkPhysicalDeviceFeatures){ .geometryShader = VK_TRUE } ))
    exit(EXIT_FAILURE);

	
	if (!vulkan_create_textured_vertex_buffer(&vk_dev,
																						"data/rubber_duck/scene.gltf",
																						&vk_state.storage_buffer,
																						&vk_state.storage_buffer_memory,
																						&vertex_buffer_size,
																						&index_buffer_size) ||
		  !vulkan_create_uniform_buffers())
	{
		printf("Cannot create data buffers\n"); fflush(stdout);
		exit(1);
	}

	vulkan_create_texture_image(&vk_dev, "data/rubber_duck/textures/Duck_baseColor.png", &vk_state.texture.handle, &vk_state.texture.memory, NULL, NULL);

	vulkan_create_image_view(vk_dev.device, vk_state.texture.handle, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &vk_state.texture.view, VK_IMAGE_VIEW_TYPE_2D, 1, 1);
	vulkan_create_texture_sampler(vk_dev.device, &vk_state.texture_sampler, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);

	vulkan_create_depth_resources(&vk_dev, g_screen_width, g_screen_height, &vk_state.depth_texture);

	const char* shaders[] =
	{
		"data/shaders/chapter03/VK02.vert",
		"data/shaders/chapter03/VK02.frag",
		"data/shaders/chapter03/VK02.geom"
	};

	RenderPassCreateInfo render_pass_info =
	{
		.clearColor_ = true, .clearDepth_ = true, .flags_ = eRenderPassBit_First | eRenderPassBit_Last
	};

	if (!vulkan_create_descriptor_pool(&vk_dev, 1, 2, 1, &vk_state.descriptor_pool) ||
	 	  !createDescriptorSet() ||
	 	  !vulkan_create_color_and_depth_render_pass(&vk_dev, true, &vk_state.render_pass, &render_pass_info, VK_FORMAT_B8G8R8A8_UNORM) ||
	 	  !vulkan_create_pipeline_layout(vk_dev.device, vk_state.descriptor_set_layout, &vk_state.pipeline_layout) ||
	 	  !vulkan_create_graphics_pipeline(&vk_dev,
																		 vk_state.render_pass,
																		 vk_state.pipeline_layout,
																		 shaders,
																		 ARRAY_SIZE(shaders),
																		 &vk_state.graphics_pipeline,
																		 VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
																		 true, true, false, -1, -1, 0))
	{
		printf("Failed to create pipeline\n"); fflush(stdout);
		exit(0);
	}

	vulkan_create_color_and_depth_framebuffers(&vk_dev,
																						 vk_state.render_pass,
																						 vk_state.depth_texture.view,
																						 &vk_state.swapchain_framebuffers);

	return VK_SUCCESS;
}

void
updateUniformBuffer(u32 currentImage,
										const void* uboData,
										u64 uboSize)
{
	void* data = NULL;
	vkMapMemory(vk_dev.device, vk_state.uniform_buffers_memory[currentImage], 0, uboSize, 0, &data);
	memcpy(data, uboData, uboSize);
	vkUnmapMemory(vk_dev.device, vk_state.uniform_buffers_memory[currentImage]);
}

b8
fillCommandBuffers(u64 i)
{
	VkCommandBufferBeginInfo bi = (VkCommandBufferBeginInfo)
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = NULL,
		.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT,
		.pInheritanceInfo = NULL
	};

	VkClearValue clearValues[] =
	{
		(VkClearValue) {.color = clear_value_color },
		(VkClearValue) {.depthStencil = { 1.0f, 0 } }
	};

	VkRect2D screenRect = (VkRect2D)
	{
		.offset = { 0, 0 },
		.extent = {.width = g_screen_width, .height = g_screen_height }
	};

	VK_CHECK(vkBeginCommandBuffer(vk_dev.command_buffers[i], &bi));

	VkRenderPassBeginInfo renderPassInfo = (VkRenderPassBeginInfo)
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.pNext = NULL,
		.renderPass = vk_state.render_pass,
		.framebuffer = vk_state.swapchain_framebuffers[i],
		.renderArea = screenRect,
		.clearValueCount = (u32)ARRAY_SIZE(clearValues),
		.pClearValues = clearValues
	};

	vkCmdBeginRenderPass(vk_dev.command_buffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

	vkCmdBindPipeline(vk_dev.command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vk_state.graphics_pipeline);

	vkCmdBindDescriptorSets(vk_dev.command_buffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, vk_state.pipeline_layout, 0, 1, &vk_state.descriptor_sets[i], 0, NULL);

	vkCmdDraw(vk_dev.command_buffers[i], (u32)(index_buffer_size / (sizeof(u32))), 1, 0, 0);

	vkCmdEndRenderPass(vk_dev.command_buffers[i]);

	VK_CHECK(vkEndCommandBuffer(vk_dev.command_buffers[i]));

	return true;
}

b8
drawOverlay()
{
	u32 imageIndex = 0;
	if (vkAcquireNextImageKHR(vk_dev.device, vk_dev.swapchain, 0, vk_dev.semaphore, VK_NULL_HANDLE, &imageIndex) != VK_SUCCESS)
		return false;

	VK_CHECK(vkResetCommandPool(vk_dev.device, vk_dev.command_pool, 0));

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	const float ratio = width / (float)height;

	//const mat4 m1 = glm::rotate(
	//	glm::translate(mat4(1.0f), vec3(0.f, 0.5f, -1.5f)) * glm::rotate(mat4(1.f), glm::pi<float>(),
	//		vec3(1, 0, 0)),
	//	(float)glfwGetTime(),
	//	vec3(0.0f, 1.0f, 0.0f)
	//);


	mat4 p = mat4_persp(DEG2RAD * 90.0f, ratio, 0.1f, 1000.0f);

	float time = (float)glfwGetTime();
	vec3 pos = (vec3){ 0.0f, -0.5f, -4.0f - math_sin(time /** 0.5*/) };

	vec3 axis = (vec3){ 0.0f, 1.0f, 0.0f };
	quat q = quat_from_axis_angle(axis, -time /** 0.5*/, false);
	mat4 tr = mat4_mul(vk_dev.model->transforms[vk_dev.mesh_idx], mat4_mul(quat_to_mat4(q), mat4_translation(pos)));
	// const mat4 p = glm::perspective(45.0f, ratio, 0.1f, 1000.0f);

	t_uniform_buffer ubo = (t_uniform_buffer) { .mvp = mat4_mul(tr, p) };


	updateUniformBuffer(imageIndex, &ubo, sizeof(ubo));

	fillCommandBuffers(imageIndex);

	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT }; // or even VERTEX_SHADER_STAGE

	VkSubmitInfo si = (VkSubmitInfo)
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = NULL,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vk_dev.semaphore,
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &vk_dev.command_buffers[imageIndex],
		.signalSemaphoreCount = 1,
		.pSignalSemaphores = &vk_dev.render_semaphore
	};

	VK_CHECK(vkQueueSubmit(vk_dev.graphics_queue, 1, &si, NULL));

	VkPresentInfoKHR pi =
	{
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.pNext = NULL,
		.waitSemaphoreCount = 1,
		.pWaitSemaphores = &vk_dev.render_semaphore,
		.swapchainCount = 1,
		.pSwapchains = &vk_dev.swapchain,
		.pImageIndices = &imageIndex
	};

	VK_CHECK(vkQueuePresentKHR(vk_dev.graphics_queue, &pi));
	VK_CHECK(vkDeviceWaitIdle(vk_dev.device));

	return true;
}

void vulkan_term()
{
	vkDestroyBuffer(vk_dev.device, vk_state.storage_buffer, NULL);
	vkFreeMemory(vk_dev.device, vk_state.storage_buffer_memory, NULL);

	for (size_t i = 0; i < cvec_size(vk_dev.swapchain_images); i++)
	{
	 	vkDestroyBuffer(vk_dev.device, vk_state.uniform_buffers[i], NULL);
	 	vkFreeMemory(vk_dev.device, vk_state.uniform_buffers_memory[i], NULL);
	}

	vkDestroyDescriptorSetLayout(vk_dev.device, vk_state.descriptor_set_layout, NULL);
	vkDestroyDescriptorPool(vk_dev.device, vk_state.descriptor_pool, NULL);

	for (u64 i = 0; i < cvec_size(vk_state.swapchain_framebuffers); i++)
	{
		vkDestroyFramebuffer(vk_dev.device, vk_state.swapchain_framebuffers[i], NULL);
	}

	vkDestroySampler(vk_dev.device, vk_state.texture_sampler, NULL);
	vulkan_destroy_vulkan_image(vk_dev.device, &vk_state.texture);

	vulkan_destroy_image(vk_dev.device, &vk_state.depth_texture);

	vkDestroyRenderPass(vk_dev.device, vk_state.render_pass, NULL);

	vkDestroyPipelineLayout(vk_dev.device, vk_state.pipeline_layout, NULL);
	vkDestroyPipeline(vk_dev.device, vk_state.graphics_pipeline, NULL);

  vulkan_destroy_render_device(&vk_dev);

  vulkan_destroy_instance(&vk);
}

int main()
{
  VkResult result = volkInitialize();

  if (result != VK_SUCCESS)
  {
    printf("bad result: %d\n", result);
    return result;
  }

 if (!glfwInit())
		exit(EXIT_FAILURE);

	if (!glfwVulkanSupported())
		exit(EXIT_FAILURE);

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(g_screen_width, g_screen_height, "VulkanApp", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetKeyCallback(
		window,
		keyboard_callback
	);

  vulkan_init();

  printf("vulkan_inited();\n");

  while (!glfwWindowShouldClose(window))
	{
		drawOverlay();
		glfwPollEvents();
	}

  vulkan_term();
	glfwTerminate();

  return 0;
}