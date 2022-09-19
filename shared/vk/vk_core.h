#ifndef __CK_CORE_H__
#define __CK_CORE_H__

#include <defines.h>
#include <stdlib.h>

#include "vk_types.h"

VkDescriptorSetLayoutBinding
descriptorSetLayoutBinding(u32 binding,
													 VkDescriptorType descriptorType,
													 VkShaderStageFlags stageFlags,
													 u32 descriptorCount/* = 1 */)
{
	return (VkDescriptorSetLayoutBinding)
	{
		.binding = binding,
		.descriptorType = descriptorType,
		.descriptorCount = descriptorCount,
		.stageFlags = stageFlags,
		.pImmutableSamplers = NULL
	};
}

VkPipelineShaderStageCreateInfo
shaderStageInfo(VkShaderStageFlagBits shaderStage,
								ShaderModule* shader_module,
								const char* entryPoint)
{
	return (VkPipelineShaderStageCreateInfo)
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.stage = shaderStage,
		.module = shader_module->shaderModule,
		.pName = entryPoint,
		.pSpecializationInfo = NULL
	};
}

VkResult
createShaderModule(VkDevice device, ShaderModule* shader, const char* fileName)
{
	compile_shader(fileName, &shader->SPIRV);
	//if (compileShaderFile(fileName, *shader) < 1)
	//	return VK_NOT_READY;

	VkShaderModuleCreateInfo createInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = cvec_size(shader->SPIRV) * sizeof(u32),
		.pCode = shader->SPIRV,
	};

	return vkCreateShaderModule(device, &createInfo, NULL, &shader->shaderModule);
}

void
vulkan_create_instance();


// implementations
void CHECK(b8 check, const char* fileName, int lineNumber, u32 value)
{
	if (!check)
	{
		printf("CHECK() failed at %s:%i, value: %d\n", fileName, lineNumber, value);
		assert(false);
		exit(EXIT_FAILURE);
	}
}

void
vulkan_create_instance(VkInstance* out_instance)
{
  const char* validation_layers[] = 
  {
    "VK_LAYER_KHRONOS_validation"
  };  

  const char* exts[] =
  {
    "VK_KHR_surface",
#if defined (_WIN32)
    "VK_KHR_win32_surface",
#endif
    VK_EXT_DEBUG_UTILS_EXTENSION_NAME,
    VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
    VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME
  };

  VkApplicationInfo appinfo =
	{
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pNext = NULL,
		.pApplicationName = "Vulkan",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "No Engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_1
	};

  VkInstanceCreateInfo createInfo =
	{
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.pApplicationInfo = &appinfo,
		.enabledLayerCount = ARRAY_SIZE(validation_layers),
		.ppEnabledLayerNames = validation_layers,
		.enabledExtensionCount = ARRAY_SIZE(exts),
		.ppEnabledExtensionNames = exts
	};

  VK_CHECK(vkCreateInstance(&createInfo, NULL, out_instance));

  volkLoadInstance(*out_instance);
}

void
vulkan_destroy_instance(vulkan_instance* vk)
{
  vkDestroySurfaceKHR(vk->instance, vk->surface, NULL);

	vkDestroyDebugReportCallbackEXT(vk->instance, vk->report_callback, NULL);
	vkDestroyDebugUtilsMessengerEXT(vk->instance, vk->messenger, NULL);

	vkDestroyInstance(vk->instance, NULL);
}

static VKAPI_ATTR VkBool32 VKAPI_CALL
vulkan_debug_callback(VkDebugUtilsMessageSeverityFlagBitsEXT Severity,
	                    VkDebugUtilsMessageTypeFlagsEXT Type,
	                    const VkDebugUtilsMessengerCallbackDataEXT* CallbackData,
	                    void* UserData)
{
	printf("Validation layer: %s\n", CallbackData->pMessage);
	return VK_FALSE;
}

static VKAPI_ATTR VkBool32 VKAPI_CALL
vulkan_debug_report_callback(VkDebugReportFlagsEXT flags,
	                           VkDebugReportObjectTypeEXT objectType,
	                           u64 object,
	                           u64 location,
	                           i32 messageCode,
	                           const char* pLayerPrefix,
	                           const char* pMessage,
	                           void* UserData)
{
	// https://github.com/zeux/niagara/blob/master/src/device.cpp   [ignoring performance warnings]
	// This silences warnings like "For optimal performance image layout should be VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL instead of GENERAL."
	if (flags & VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT)
		return VK_FALSE;
	
	if (flags & VK_DEBUG_REPORT_DEBUG_BIT_EXT)
		return VK_FALSE;

	printf("Debug callback (%s): %s\n", pLayerPrefix, pMessage);

	if (flags & VK_DEBUG_REPORT_ERROR_BIT_EXT)
		assert(false);

	return VK_FALSE;
}

b8
vulkan_setup_debug_callbacks(VkInstance instance,
                             VkDebugUtilsMessengerEXT* messenger,
                             VkDebugReportCallbackEXT* reportCallback)
{
  {
    VkDebugUtilsMessengerCreateInfoEXT ci = 
    {
      .sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
      .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
				                 VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
      .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
				             VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
				             VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
      .pfnUserCallback = &vulkan_debug_callback,
      .pUserData = NULL
    };

		VK_CHECK(vkCreateDebugUtilsMessengerEXT(instance, &ci, NULL, messenger));
	}

	{
    VkDebugReportCallbackCreateInfoEXT ci = 
    {
      .sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
      .flags = VK_DEBUG_REPORT_WARNING_BIT_EXT |
				       VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT |
				       VK_DEBUG_REPORT_ERROR_BIT_EXT |
				       VK_DEBUG_REPORT_DEBUG_BIT_EXT,
      .pfnCallback = &vulkan_debug_report_callback,
      .pUserData = NULL
    };

		VK_CHECK(vkCreateDebugReportCallbackEXT(instance, &ci, NULL, reportCallback));
	}

	return true;
}

VkResult find_suitable_physical_device(VkInstance instance, b8(*selector)(VkPhysicalDevice), VkPhysicalDevice* physicalDevice)
{
	uint32_t deviceCount = 0;
	VK_CHECK_RET(vkEnumeratePhysicalDevices(instance, &deviceCount, NULL));

	if (!deviceCount) return VK_ERROR_INITIALIZATION_FAILED;

  cvec(VkPhysicalDevice) devices = cvec_ncreate(VkPhysicalDevice, deviceCount);
	VK_CHECK_RET(vkEnumeratePhysicalDevices(instance, &deviceCount, devices));

	for (u64 i = 0; i != cvec_size(devices); i++)
	{
		if (selector(devices[i]))
		{
			*physicalDevice = devices[i];
			return VK_SUCCESS;
		}
	}

	return VK_ERROR_INITIALIZATION_FAILED;
}

u32
find_queue_families(VkPhysicalDevice device,
                    VkQueueFlags desiredFlags)
{
	u32 familyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, NULL);

  cvec(VkQueueFamilyProperties) families = cvec_ncreate(VkQueueFamilyProperties, familyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &familyCount, families);

	for (u64 i = 0; i != cvec_size(families); i++)
		if (families[i].queueCount > 0 && families[i].queueFlags & desiredFlags)
			return i;

	return 0;
}

VkResult
create_device(VkPhysicalDevice physicalDevice,
              VkPhysicalDeviceFeatures deviceFeatures,
              u32 graphicsFamily,
              VkDevice* device)
{
  const char* extensions[2] =
  {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME,
    VK_KHR_SHADER_DRAW_PARAMETERS_EXTENSION_NAME
  };

	float queuePriority = 1.0f;

	VkDeviceQueueCreateInfo qci =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.queueFamilyIndex = graphicsFamily,
		.queueCount = 1,
		.pQueuePriorities = &queuePriority
	};

	VkDeviceCreateInfo ci =
	{
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.queueCreateInfoCount = 1,
		.pQueueCreateInfos = &qci,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = NULL,
		.enabledExtensionCount = 2,
		.ppEnabledExtensionNames = extensions,
		.pEnabledFeatures = &deviceFeatures
	};

	return vkCreateDevice(physicalDevice, &ci, NULL, device);
}

swapchain_support_details
querySwapchainSupport(VkPhysicalDevice device,
                      VkSurfaceKHR surface)
{
	swapchain_support_details details = 
  {
    .formats = NULL,
    .preset_modes = NULL
  };

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

	u32 formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, NULL);

	if (formatCount)
	{
		details.formats = cvec_ncreate(VkSurfaceFormatKHR, formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats);
	}

	u32 presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, NULL);

	if (presentModeCount)
	{
		details.preset_modes = cvec_ncreate(VkPresentModeKHR, formatCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.preset_modes);
	}

	return details;
}

VkSurfaceFormatKHR
chooseSwapSurfaceFormat(cvec(VkSurfaceFormatKHR) availableFormats)
{
	return (VkSurfaceFormatKHR){ VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
}

VkPresentModeKHR
chooseSwapPresentMode(cvec(VkPresentModeKHR) availablePresentModes)
{
  for (u64 i = 0; i != cvec_size(availablePresentModes); i++)
  {
    if (availablePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR)
    {
      return availablePresentModes[i];
    }
  }

	// FIFO will always be supported
	return VK_PRESENT_MODE_FIFO_KHR;
}

u32
chooseSwapImageCount(VkSurfaceCapabilitiesKHR* capabilities)
{
	u32 imageCount = capabilities->minImageCount + 1;

	b8 imageCountExceeded = capabilities->maxImageCount > 0 && imageCount > capabilities->maxImageCount;

	return imageCountExceeded ? capabilities->maxImageCount : imageCount;
}

VkResult
create_swapchain(VkDevice device,
                 VkPhysicalDevice physicalDevice,
                 VkSurfaceKHR surface,
                 u32 graphicsFamily,
                 u32 width,
                 u32 height,
                 VkSwapchainKHR* swapchain,
                 b8 supportScreenshots)
{
	swapchain_support_details swapchainSupport = querySwapchainSupport(physicalDevice, surface);
	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.preset_modes);

	VkSwapchainKHR old_one;

	VkSwapchainCreateInfoKHR ci =
	{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.flags = 0,
		.surface = surface,
		.minImageCount = chooseSwapImageCount(&swapchainSupport.capabilities),
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = {.width = width, .height = height },
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | (supportScreenshots ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0u),
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices = &graphicsFamily,
		.preTransform = swapchainSupport.capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = presentMode,
		.clipped = VK_TRUE,
		.oldSwapchain = old_one
	};

	return vkCreateSwapchainKHR(device, &ci, NULL, swapchain);
}

// For volumes use the call
// createImageView(device, image, /* whatever the format is */ VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, VkImageView* imageView, VK_IMAGE_VIEW_TYPE_3D, 1)

b8
createImageView(VkDevice device,
                VkImage image,
                VkFormat format,
                VkImageAspectFlags aspectFlags,
                VkImageView* imageView,
                VkImageViewType viewType,
                u32 layerCount,
                u32 mipLevels)
{
	const VkImageViewCreateInfo viewInfo =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.image = image,
		.viewType = viewType,
		.format = format,
		.subresourceRange =
		{
			.aspectMask = aspectFlags,
			.baseMipLevel = 0,
			.levelCount = mipLevels,
			.baseArrayLayer = 0,
			.layerCount = layerCount
		}
	};

	return (vkCreateImageView(device, &viewInfo, NULL, imageView) == VK_SUCCESS);
}

VkResult
createSwapchain(VkDevice device,
                VkPhysicalDevice physicalDevice,
                VkSurfaceKHR surface,
                u32 graphicsFamily,
                u32 width,
                u32 height,
                VkSwapchainKHR* swapchain,
                b8 supportScreenshots)
{
	swapchain_support_details swapchainSupport = querySwapchainSupport(physicalDevice, surface);
	VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapchainSupport.formats);
	VkPresentModeKHR presentMode = chooseSwapPresentMode(swapchainSupport.preset_modes);

	VkSwapchainCreateInfoKHR ci =
	{
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.flags = 0,
		.surface = surface,
		.minImageCount = chooseSwapImageCount(&swapchainSupport.capabilities),
		.imageFormat = surfaceFormat.format,
		.imageColorSpace = surfaceFormat.colorSpace,
		.imageExtent = {.width = width, .height = height },
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | (supportScreenshots ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0u),
		.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 1,
		.pQueueFamilyIndices = &graphicsFamily,
		.preTransform = swapchainSupport.capabilities.currentTransform,
		.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
		.presentMode = presentMode,
		.clipped = VK_TRUE,
		.oldSwapchain = VK_NULL_HANDLE
	};

	return vkCreateSwapchainKHR(device, &ci, NULL, swapchain);
}

u64
createSwapchainImages(VkDevice device,
                      VkSwapchainKHR swapchain,
	                    cvec(VkImage)* swapchainImages,
	                    cvec(VkImageView)* swapchainImageViews)
{
	u32 imageCount = 0;
	VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, NULL));
	*swapchainImages = cvec_ncreate(VkImage, imageCount);
	VK_CHECK(vkGetSwapchainImagesKHR(device, swapchain, &imageCount, *swapchainImages));

	*swapchainImageViews = cvec_ncreate(VkImageView, imageCount);
	for (u32 i = 0; i < imageCount; i++)
		if (!createImageView(device, (*swapchainImages)[i], VK_FORMAT_B8G8R8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &((*swapchainImageViews)[i]), VK_IMAGE_VIEW_TYPE_2D, 1, 1))
			exit(0);

	return (u64)(imageCount);
}

VkResult
createSemaphore(VkDevice device,
                VkSemaphore* outSemaphore)
{
	const VkSemaphoreCreateInfo ci =
	{
		.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO
	};

	return vkCreateSemaphore(device, &ci, NULL, outSemaphore);
}

b8 vulkan_init_render_device(vulkan_instance* vk,
                               vulkan_render_device* vkDev,
                               u32 width,
                               u32 height,
                               b8(*selector)(VkPhysicalDevice),
                               VkPhysicalDeviceFeatures deviceFeatures)
{
	vkDev->framebuffer_width = width;
	vkDev->framebuffer_height = height;

	VK_CHECK(find_suitable_physical_device(vk->instance, selector, &vkDev->physical_device));
	
	vkDev->graphics_family = find_queue_families(vkDev->physical_device, VK_QUEUE_GRAPHICS_BIT);
	VK_CHECK(create_device(vkDev->physical_device, deviceFeatures, vkDev->graphics_family, &vkDev->device));

	vkGetDeviceQueue(vkDev->device, vkDev->graphics_family, 0, &vkDev->graphics_queue);
	if (vkDev->graphics_queue == NULL)
		exit(EXIT_FAILURE);

	VkBool32 presentSupported = 0;
	vkGetPhysicalDeviceSurfaceSupportKHR(vkDev->physical_device, vkDev->graphics_family, vk->surface, &presentSupported);
	if (!presentSupported)
		exit(EXIT_FAILURE);

	VK_CHECK(createSwapchain(vkDev->device, vkDev->physical_device, vk->surface, vkDev->graphics_family, width, height, &vkDev->swapchain, false));

	u64 imageCount = createSwapchainImages(vkDev->device, vkDev->swapchain, &vkDev->swapchain_images, &vkDev->swapchain_image_views);
  
	vkDev->command_buffers = cvec_ncreate(VkCommandBuffer, imageCount);

	VK_CHECK(createSemaphore(vkDev->device, &vkDev->semaphore));
	VK_CHECK(createSemaphore(vkDev->device, &vkDev->render_semaphore));

	VkCommandPoolCreateInfo cpi =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = 0,
		.queueFamilyIndex = vkDev->graphics_family
	};

	VK_CHECK(vkCreateCommandPool(vkDev->device, &cpi, NULL, &vkDev->command_pool));

	VkCommandBufferAllocateInfo ai =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = NULL,
		.commandPool = vkDev->command_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = (u32)(cvec_size(vkDev->swapchain_images)),
	};

	VK_CHECK(vkAllocateCommandBuffers(vkDev->device, &ai, &vkDev->command_buffers[0]));
	return true;
}

void vulkan_destroy_render_device(vulkan_render_device* vkDev)
{
	for (size_t i = 0; i < cvec_size(vkDev->swapchain_images); i++)
		vkDestroyImageView(vkDev->device, vkDev->swapchain_image_views[i], NULL);

	vkDestroySwapchainKHR(vkDev->device, vkDev->swapchain, NULL);

	vkDestroyCommandPool(vkDev->device, vkDev->command_pool, NULL);

	vkDestroySemaphore(vkDev->device, vkDev->semaphore, NULL);
	vkDestroySemaphore(vkDev->device, vkDev->render_semaphore, NULL);

	if (vkDev->use_compute)
	{
		vkDestroyCommandPool(vkDev->device, vkDev->compute_command_pool, NULL);
	}

	vkDestroyDevice(vkDev->device, NULL);
}

u32
findMemoryType(VkPhysicalDevice device,
							u32 typeFilter,
							VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(device,
																			&memProperties);

	for (u32 i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	return 0xFFFFFFFF;
}

b8 vulkan_create_buffer(VkDevice device,
												VkPhysicalDevice physicalDevice,
												VkDeviceSize size,
												VkBufferUsageFlags usage,
												VkMemoryPropertyFlags properties,
												VkBuffer* buffer,
												VkDeviceMemory* bufferMemory)
{
	VkBufferCreateInfo bufferInfo =
	{
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.size = size,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = NULL
	};

	VK_CHECK(vkCreateBuffer(device, &bufferInfo, NULL, buffer));

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, *buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo =
	{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = NULL,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties)
	};

	VK_CHECK(vkAllocateMemory(device, &allocInfo, NULL, bufferMemory));

	vkBindBufferMemory(device, *buffer, *bufferMemory, 0);

	return true;
}

VkCommandBuffer
vulkan_begin_single_time_commands(vulkan_render_device* vkDev)
{
	VkCommandBuffer commandBuffer;

	VkCommandBufferAllocateInfo allocInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
		.pNext = NULL,
		.commandPool = vkDev->command_pool,
		.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
		.commandBufferCount = 1
	};

	vkAllocateCommandBuffers(vkDev->device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo =
	{
		.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
		.pNext = NULL,
		.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
		.pInheritanceInfo = NULL
	};

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void
vulkan_end_single_time_commands(vulkan_render_device* vkDev,
																VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.pNext = NULL,
		.waitSemaphoreCount = 0,
		.pWaitSemaphores = NULL,
		.pWaitDstStageMask = NULL,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandBuffer,
		.signalSemaphoreCount = 0,
		.pSignalSemaphores = NULL
	};

	vkQueueSubmit(vkDev->graphics_queue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(vkDev->graphics_queue);

	vkFreeCommandBuffers(vkDev->device, vkDev->command_pool, 1, &commandBuffer);
}

void vulkan_copy_buffer(vulkan_render_device* vkDev,
												VkBuffer srcBuffer,
												VkBuffer dstBuffer,
												VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = vulkan_begin_single_time_commands(vkDev);

	VkBufferCopy copyRegion =
	{
		.srcOffset = 0,
		.dstOffset = 0,
		.size = size
	};

	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	vulkan_end_single_time_commands(vkDev, commandBuffer);
}

u64 vulkan_allocate_vertex_buffer(vulkan_render_device* vkDev,
																	VkBuffer* storageBuffer,
																	VkDeviceMemory* storageBufferMemory,
																	u64 vertexDataSize,
																	const void* vertexData,
																	u64 indexDataSize,
																	const void* indexData)
{
	VkDeviceSize bufferSize = vertexDataSize + indexDataSize;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	vulkan_create_buffer(vkDev->device,
											 vkDev->physical_device,
											 bufferSize,
											 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
											 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
											 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
											 &stagingBuffer,
											 &stagingBufferMemory);

	void* data;
	vkMapMemory(vkDev->device,
							stagingBufferMemory,
							0,
							bufferSize,
							0,
							&data);
	memcpy(data,
				 vertexData,
				 vertexDataSize);
	memcpy((unsigned char *)data + vertexDataSize,
				 indexData,
				 indexDataSize);
	vkUnmapMemory(vkDev->device,
								stagingBufferMemory);

	vulkan_create_buffer(vkDev->device,
											 vkDev->physical_device,
											 bufferSize,
											 VK_BUFFER_USAGE_TRANSFER_DST_BIT |
											 VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
											 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
											 storageBuffer,
											 storageBufferMemory);

	vulkan_copy_buffer(vkDev,
						 				 stagingBuffer,
						 				 *storageBuffer,
						 				 bufferSize);

	vkDestroyBuffer(vkDev->device,
									stagingBuffer, 
									NULL);
	vkFreeMemory(vkDev->device,
							 stagingBufferMemory,
							 NULL);

	return bufferSize;
}

b8 vulkan_create_textured_vertex_buffer(vulkan_render_device* vkDev,
																				const char* file_path,
																				VkBuffer* storageBuffer,
																				VkDeviceMemory* storageBufferMemory,
																				u64* vertexBufferSize,
																				u64* indexBufferSize)
{
	vkDev->model = LoadGLTF(file_path);

	vkDev->mesh_idx = 0;

	*vertexBufferSize = sizeof(VertexData) * vkDev->model->meshes[vkDev->mesh_idx].vertexCount;
	*indexBufferSize = sizeof(u32) * vkDev->model->meshes[vkDev->mesh_idx].triangleCount * 3;

	t_mesh* mesh = &vkDev->model->meshes[vkDev->mesh_idx];

	mesh->vn_vertices = cvec_ncreate(VertexData, mesh->vertexCount);

	for (u64 i = 0; i < mesh->vertexCount; i++)
	{
		mesh->vn_vertices[i].pos.x = mesh->v_vertices[i * 3];
		mesh->vn_vertices[i].pos.y = mesh->v_vertices[i * 3 + 1];
		mesh->vn_vertices[i].pos.z = mesh->v_vertices[i * 3 + 2];

		//mesh->vn_vertices[i].n.x = mesh->v_normals[i * 3];
		//mesh->vn_vertices[i].n.y = mesh->v_normals[i * 3 + 1];
		//mesh->vn_vertices[i].n.z = mesh->v_normals[i * 3 + 2];

		mesh->vn_vertices[i].tc.x = mesh->v_texcoords[i * 2];
		mesh->vn_vertices[i].tc.y = mesh->v_texcoords[i * 2 + 1];
	}

	vulkan_allocate_vertex_buffer(vkDev,
																storageBuffer,
																storageBufferMemory,
																*vertexBufferSize,
																vkDev->model->meshes[vkDev->mesh_idx].vn_vertices,
																*indexBufferSize,
																vkDev->model->meshes[vkDev->mesh_idx].v_indices);

	return true;
}

b8
vulkan_create_image(VkDevice device,
										VkPhysicalDevice physicalDevice,
										u32 width,
										u32 height,
										VkFormat format,
										VkImageTiling tiling,
										VkImageUsageFlags usage,
										VkMemoryPropertyFlags properties,
										VkImage* image,
										VkDeviceMemory* imageMemory,
										VkImageCreateFlags flags,
										u32 mipLevels)
{
	VkImageCreateInfo imageInfo =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.pNext = NULL,
		.flags = flags,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = format,
		.extent = (VkExtent3D) { .width = width, .height = height, .depth = 1 },
		.mipLevels = mipLevels,
		.arrayLayers = (u32)((flags == VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT) ? 6 : 1),
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = tiling,
		.usage = usage,
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.queueFamilyIndexCount = 0,
		.pQueueFamilyIndices = NULL,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED
	};

	VK_CHECK(vkCreateImage(device, &imageInfo, NULL, image));

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, *image, &memRequirements);

	VkMemoryAllocateInfo allocInfo =
	{
		.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
		.pNext = NULL,
		.allocationSize = memRequirements.size,
		.memoryTypeIndex = findMemoryType(physicalDevice, memRequirements.memoryTypeBits, properties)
	};

	VK_CHECK(vkAllocateMemory(device, &allocInfo, NULL, imageMemory));

	vkBindImageMemory(device, *image, *imageMemory, 0);
	return true;
}

void
vulkan_upload_buffer_data(vulkan_render_device* vkDev,
													VkDeviceMemory* bufferMemory,
													VkDeviceSize deviceOffset,
													void* data,
													u64 dataSize)
{
	void* mappedData = NULL;
	vkMapMemory(vkDev->device, *bufferMemory, deviceOffset, dataSize, 0, &mappedData);
	memcpy(mappedData, data, dataSize);
	vkUnmapMemory(vkDev->device, *bufferMemory);
}

b8
hasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

u32
bytesPerTexFormat(VkFormat fmt)
{
	switch (fmt)
	{
	case VK_FORMAT_R8_SINT:
	case VK_FORMAT_R8_UNORM:
		return 1;
	case VK_FORMAT_R16_SFLOAT:
		return 2;
	case VK_FORMAT_R16G16_SFLOAT:
		return 4;
	case VK_FORMAT_R16G16_SNORM:
		return 4;
	case VK_FORMAT_B8G8R8A8_UNORM:
		return 4;
	case VK_FORMAT_R8G8B8A8_UNORM:
		return 4;
	case VK_FORMAT_R16G16B16A16_SFLOAT:
		return 4 * sizeof(u16);
	case VK_FORMAT_R32G32B32A32_SFLOAT:
		return 4 * sizeof(float);
	default:
		break;
	}
	return 0;
}

void
vulkan_transit_image_layout_cmd(VkCommandBuffer commandBuffer,
																VkImage image,
																VkFormat format,
																VkImageLayout oldLayout,
																VkImageLayout newLayout,
																u32 layerCount,
																u32 mipLevels)
{
	VkImageMemoryBarrier barrier =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = NULL,
		.srcAccessMask = 0,
		.dstAccessMask = 0,
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = (VkImageSubresourceRange)
		{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = mipLevels,
			.baseArrayLayer = 0,
			.layerCount = layerCount
		}
	};

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
		(format == VK_FORMAT_D16_UNORM) ||
		(format == VK_FORMAT_X8_D24_UNORM_PACK32) ||
		(format == VK_FORMAT_D32_SFLOAT) ||
		(format == VK_FORMAT_S8_UINT) ||
		(format == VK_FORMAT_D16_UNORM_S8_UINT) ||
		(format == VK_FORMAT_D24_UNORM_S8_UINT))
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (hasStencilComponent(format)) {
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
		if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL) {
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		}
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	/* Convert back from read-only to updateable */
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	/* Convert from updateable texture to shader read-only */
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	/* Convert depth texture from undefined state to depth-stencil buffer */
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}

	/* Wait for render pass to complete */
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = 0; // VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = 0;
		/*
				sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		///		destinationStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
				destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		*/
		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}

	/* Convert back from read-only to color attachment */
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	/* Convert from updateable texture to shader read-only */
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}

	/* Convert back from read-only to depth attachment */
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	}
	/* Convert from updateable depth texture to shader read-only */
	else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, NULL,
		0, NULL,
		1, &barrier
	);
}

void
vulkan_transit_image_layout(vulkan_render_device* vkDev,
														VkImage image,
														VkFormat format,
														VkImageLayout oldLayout,
														VkImageLayout newLayout,
														u32 layerCount,
														u32 mipLevels)
{
	VkCommandBuffer commandBuffer = vulkan_begin_single_time_commands(vkDev);

	vulkan_transit_image_layout_cmd(commandBuffer, image, format, oldLayout, newLayout, layerCount, mipLevels);

	vulkan_end_single_time_commands(vkDev, commandBuffer);
}

void
vulkan_copy_buffer_to_image(vulkan_render_device* vkDev,
														VkBuffer buffer,
														VkImage image,
														u32 width,
														u32 height,
														u32 layerCount)
{
	VkCommandBuffer commandBuffer = vulkan_begin_single_time_commands(vkDev);

	const VkBufferImageCopy region = {
		.bufferOffset = 0,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource = (VkImageSubresourceLayers)
		{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = layerCount
		},
		.imageOffset = (VkOffset3D) {.x = 0, .y = 0, .z = 0 },
		.imageExtent = (VkExtent3D) {.width = width, .height = height, .depth = 1 }
	};

	vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

	vulkan_end_single_time_commands(vkDev, commandBuffer);
}

b8
vulkan_update_texture_image(vulkan_render_device* vkDev,
														VkImage* textureImage,
														VkDeviceMemory* textureImageMemory,
														u32 texWidth,
														u32 texHeight,
														VkFormat texFormat,
														u32 layerCount,
														void* imageData,
														VkImageLayout sourceImageLayout)
{
	u32 bytesPerPixel = bytesPerTexFormat(texFormat);

	VkDeviceSize layerSize = texWidth * texHeight * bytesPerPixel;
	VkDeviceSize imageSize = layerSize * layerCount;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	vulkan_create_buffer(vkDev->device,
											 vkDev->physical_device,
											 imageSize,
											 VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
											 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
											 VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
											 &stagingBuffer,
											 &stagingBufferMemory);

	vulkan_upload_buffer_data(vkDev, &stagingBufferMemory, 0, imageData, imageSize);

	vulkan_transit_image_layout(vkDev, *textureImage, texFormat, sourceImageLayout, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, layerCount, 1);
	vulkan_copy_buffer_to_image(vkDev, stagingBuffer, *textureImage, (u32)texWidth, (u32)texHeight, layerCount);
	vulkan_transit_image_layout(vkDev, *textureImage, texFormat, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, layerCount, 1);

	vkDestroyBuffer(vkDev->device, stagingBuffer, NULL);
	vkFreeMemory(vkDev->device, stagingBufferMemory, NULL);

	return true;
}

b8
vulkan_create_texture_image_from_data(vulkan_render_device* vkDev,
																			VkImage* textureImage,
																			VkDeviceMemory* textureImageMemory,
																			void* imageData,
																			u32 texWidth,
																			u32 texHeight,
																			VkFormat texFormat,
																			u32 layerCount,
																			VkImageCreateFlags flags)
{
	vulkan_create_image(vkDev->device,
											vkDev->physical_device,
											texWidth,
											texHeight,
											texFormat,
											VK_IMAGE_TILING_OPTIMAL,
											VK_IMAGE_USAGE_TRANSFER_DST_BIT |
											VK_IMAGE_USAGE_SAMPLED_BIT,
											VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
											textureImage,
											textureImageMemory,
											flags,
											1);

	return vulkan_update_texture_image(vkDev, textureImage, textureImageMemory, texWidth, texHeight, texFormat, layerCount, imageData, VK_IMAGE_LAYOUT_UNDEFINED);
}

b8
vulkan_create_texture_image(vulkan_render_device* vkDev,
														const char* filename,
														VkImage* textureImage,
														VkDeviceMemory* textureImageMemory,
														u32* outTexWidth,
														u32* outTexHeight)
{
	int texWidth, texHeight, texChannels;
	stbi_uc* pixels = stbi_load(filename, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);

	if (!pixels)
	{
		printf("Failed to load [%s] texture\n", filename); fflush(stdout);
		return false;
	}

	b8 result = vulkan_create_texture_image_from_data(vkDev,
																										textureImage,
																										textureImageMemory,
																										pixels,
																										texWidth,
																										texHeight,
																										VK_FORMAT_R8G8B8A8_UNORM,
																										1,
																										0);

	stbi_image_free(pixels);

	if (outTexWidth && outTexHeight)
	{
		*outTexWidth = (u32)texWidth;
		*outTexHeight = (u32)texHeight;
	}

	return result;
}

void
vulkan_destroy_vulkan_image(VkDevice device, vulkan_image* image)
{
	vkDestroyImageView(device, image->view, NULL);
	vkDestroyImage(device, image->handle, NULL);
	vkFreeMemory(device, image->memory, NULL);
}

b8
vulkan_create_image_view(VkDevice device,
												 VkImage image,
												 VkFormat format,
												 VkImageAspectFlags aspectFlags,
												 VkImageView* imageView,
												 VkImageViewType viewType,
												 u32 layerCount,
												 u32 mipLevels)
{
	VkImageViewCreateInfo viewInfo =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.image = image,
		.viewType = viewType,
		.format = format,
		.subresourceRange =
		{
			.aspectMask = aspectFlags,
			.baseMipLevel = 0,
			.levelCount = mipLevels,
			.baseArrayLayer = 0,
			.layerCount = layerCount
		}
	};

	return (vkCreateImageView(device, &viewInfo, NULL, imageView) == VK_SUCCESS);
}

b8
vulkan_create_texture_sampler(VkDevice device,
															VkSampler* sampler,
															VkFilter minFilter,
															VkFilter maxFilter,
															VkSamplerAddressMode addressMode)
{
	VkSamplerCreateInfo samplerInfo =
	{
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.addressModeU = addressMode, // VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeV = addressMode, // VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.addressModeW = addressMode, // VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE VK_SAMPLER_ADDRESS_MODE_REPEAT,
		.mipLodBias = 0.0f,
		.anisotropyEnable = VK_FALSE,
		.maxAnisotropy = 1,
		.compareEnable = VK_FALSE,
		.compareOp = VK_COMPARE_OP_ALWAYS,
		.minLod = 0.0f,
		.maxLod = 0.0f,
		.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		.unnormalizedCoordinates = VK_FALSE
	};

	return (vkCreateSampler(device, &samplerInfo, NULL, sampler) == VK_SUCCESS);
}

VkFormat
findSupportedFormat(VkPhysicalDevice device,
										VkFormat* candidates,
										int candidates_count,
										VkImageTiling tiling,
										VkFormatFeatureFlags features)
{
	for (u64 i = 0; i < candidates_count; i++)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(device, candidates[i], &props);

		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return candidates[i];
		}
		else
		if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return candidates[i];
		}
	}

	printf("failed to find supported format!\n");
	exit(0);
}

VkFormat
findDepthFormat(VkPhysicalDevice device)
{
	VkFormat formats[] =
	{
		VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT
	};
	return findSupportedFormat(
		device,
		formats,
		ARRAY_SIZE(formats),
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

void
transitionImageLayoutCmd(VkCommandBuffer commandBuffer,
												 VkImage image,
												 VkFormat format,
												 VkImageLayout oldLayout,
												 VkImageLayout newLayout,
												 u32 layerCount,
												 u32 mipLevels)
{
	VkImageMemoryBarrier barrier =
	{
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.pNext = NULL,
		.srcAccessMask = 0,
		.dstAccessMask = 0,
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image,
		.subresourceRange = (VkImageSubresourceRange)
		{
			.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
			.baseMipLevel = 0,
			.levelCount = mipLevels,
			.baseArrayLayer = 0,
			.layerCount = layerCount
		}
	};

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL ||
		(format == VK_FORMAT_D16_UNORM) ||
		(format == VK_FORMAT_X8_D24_UNORM_PACK32) ||
		(format == VK_FORMAT_D32_SFLOAT) ||
		(format == VK_FORMAT_S8_UINT) ||
		(format == VK_FORMAT_D16_UNORM_S8_UINT) ||
		(format == VK_FORMAT_D24_UNORM_S8_UINT))
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (hasStencilComponent(format))
		{
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else
	{
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL)
	{
			barrier.srcAccessMask = 0;
			barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

			sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	/* Convert back from read-only to updateable */
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	/* Convert from updateable texture to shader read-only */
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	/* Convert depth texture from undefined state to depth-stencil buffer */
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	/* Wait for render pass to complete */
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = 0; // VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = 0;
		/*
				sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		///		destinationStage = VK_PIPELINE_STAGE_ALL_GRAPHICS_BIT;
				destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		*/
		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	/* Convert back from read-only to color attachment */
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	}
	/* Convert from updateable texture to shader read-only */
	else if (oldLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	/* Convert back from read-only to depth attachment */
	else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
	}
	/* Convert from updateable depth texture to shader read-only */
	else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
	{
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, NULL,
		0, NULL,
		1, &barrier
	);
}

void
transitionImageLayout(vulkan_render_device* vkDev,
											VkImage image,
											VkFormat format,
											VkImageLayout oldLayout,
											VkImageLayout newLayout,
											u32 layerCount,
											u32 mipLevels)
{
	VkCommandBuffer commandBuffer = vulkan_begin_single_time_commands(vkDev);

	transitionImageLayoutCmd(commandBuffer, image, format, oldLayout, newLayout, layerCount, mipLevels);

	vulkan_end_single_time_commands(vkDev, commandBuffer);
}

// b8
// vulkan_create_depth_resources(vulkan_render_device* vkDev,
// 															u32 width,
// 															u32 height,
// 															vulkan_image* depth)
// {
// 	VkFormat depthFormat = findDepthFormat(vkDev->physical_device);

// 	if (!vulkan_create_image(vkDev->device, vkDev->physical_device, width, height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, &depth->handle, &depth->memory, 0, 1))
// 		return false;

// 	if (!vulkan_create_image_view(vkDev->device, depth->handle, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, &depth->view, VK_IMAGE_VIEW_TYPE_2D, 1, 1))
// 		return false;

// 	transitionImageLayout(vkDev, depth->handle, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, 1);

// 	return true;
// }

b8
vulkan_create_descriptor_pool(vulkan_render_device* vkDev,
															u32 uniformBufferCount,
															u32 storageBufferCount,
															u32 samplerCount,
															VkDescriptorPool* descriptorPool)
{
	u32 imageCount = (u32) cvec_size(vkDev->swapchain_images);

	cvec(VkDescriptorPoolSize) poolSizes = cvec_create(VkDescriptorPoolSize);

	if (uniformBufferCount)
	{
		VkDescriptorPoolSize pool_size = (VkDescriptorPoolSize)
		{
			.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
			.descriptorCount = imageCount * uniformBufferCount
		};

		cvec_push(poolSizes, pool_size);
	}

	if (storageBufferCount)
	{
		VkDescriptorPoolSize pool_size = (VkDescriptorPoolSize)
		{
			.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
			.descriptorCount = imageCount * storageBufferCount
		};
		cvec_push(poolSizes, pool_size);
	}

	if (samplerCount)
	{
		VkDescriptorPoolSize pool_size = (VkDescriptorPoolSize)
		{
			.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
			.descriptorCount = imageCount * samplerCount
		};
		cvec_push(poolSizes, pool_size);
	}

	VkDescriptorPoolCreateInfo poolInfo = (VkDescriptorPoolCreateInfo)
	{
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.maxSets = (u32)imageCount,
		.poolSizeCount = (u32) cvec_size(poolSizes),
		.pPoolSizes = cvec_empty(poolSizes) ? NULL : poolSizes
	};

	VkResult res = vkCreateDescriptorPool(vkDev->device, &poolInfo, NULL, descriptorPool);

	return (res == VK_SUCCESS);
}

b8
vulkan_create_color_and_depth_render_pass(vulkan_render_device* vkDev,
																					b8 useDepth,
																					VkRenderPass* renderPass,
																					RenderPassCreateInfo* ci,
																					VkFormat colorFormat /*= VK_FORMAT_B8G8R8A8_UNORM*/)
{
	b8 offscreenInt = ci->flags_ & eRenderPassBit_OffscreenInternal;
	b8 first = ci->flags_ & eRenderPassBit_First;
	b8 last = ci->flags_ & eRenderPassBit_Last;

	VkAttachmentDescription colorAttachment =
	{
		.flags = 0,
		.format = colorFormat,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = offscreenInt ? VK_ATTACHMENT_LOAD_OP_LOAD : (ci->clearColor_ ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD),
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = first ? VK_IMAGE_LAYOUT_UNDEFINED : (offscreenInt ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL),
		.finalLayout = last ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkAttachmentReference colorAttachmentRef =
	{
		.attachment = 0,
		.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL
	};

	VkAttachmentDescription depthAttachment =
	{
		.flags = 0,
		.format = useDepth ? findDepthFormat(vkDev->physical_device) : VK_FORMAT_D32_SFLOAT,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.loadOp = offscreenInt ? VK_ATTACHMENT_LOAD_OP_LOAD : (ci->clearDepth_ ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD),
		.storeOp = VK_ATTACHMENT_STORE_OP_STORE,
		.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE,
		.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE,
		.initialLayout = ci->clearDepth_ ? VK_IMAGE_LAYOUT_UNDEFINED : (offscreenInt ? VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL : VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL),
		.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	VkAttachmentReference depthAttachmentRef =
	{
		.attachment = 1,
		.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL
	};

	if (ci->flags_ & eRenderPassBit_Offscreen)
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	cvec(VkSubpassDependency) dependencies = cvec_create(VkSubpassDependency);
	{
		VkSubpassDependency dep = (VkSubpassDependency)
		{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = 0,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = 0
		};
		cvec_push(dependencies, dep);
	}

	if (ci->flags_ & eRenderPassBit_Offscreen)
	{
		colorAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		depthAttachment.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

		// Use subpass dependencies for layout transitions
		// dependencies.resize(2);

		dependencies[0] = (VkSubpassDependency)
		{
			.srcSubpass = VK_SUBPASS_EXTERNAL,
			.dstSubpass = 0,
			.srcStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.srcAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		};

		VkSubpassDependency dep = (VkSubpassDependency)
		{
			.srcSubpass = 0,
			.dstSubpass = VK_SUBPASS_EXTERNAL,
			.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT,
			.dstStageMask = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT,
			.srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT,
			.dstAccessMask = VK_ACCESS_SHADER_READ_BIT,
			.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT
		};
		cvec_push(dependencies, dep);
	}

	VkSubpassDescription subpass =
	{
		.flags = 0,
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.inputAttachmentCount = 0,
		.pInputAttachments = NULL,
		.colorAttachmentCount = 1,
		.pColorAttachments = &colorAttachmentRef,
		.pResolveAttachments = NULL,
		.pDepthStencilAttachment = useDepth ? &depthAttachmentRef : NULL,
		.preserveAttachmentCount = 0,
		.pPreserveAttachments = NULL
	};

	VkAttachmentDescription attachments[] =
	{
		colorAttachment,
		depthAttachment
	};

	VkRenderPassCreateInfo renderPassInfo =
	{
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.attachmentCount = (u32)(useDepth ? 2 : 1),
		.pAttachments = attachments,
		.subpassCount = 1,
		.pSubpasses = &subpass,
		.dependencyCount = (u32)(cvec_size(dependencies)),
		.pDependencies = dependencies
	};

	return (vkCreateRenderPass(vkDev->device, &renderPassInfo, NULL, renderPass) == VK_SUCCESS);
}

b8
vulkan_create_pipeline_layout(VkDevice device,
															VkDescriptorSetLayout dsLayout,
															VkPipelineLayout* pipelineLayout)
{
	VkPipelineLayoutCreateInfo pipelineLayoutInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.setLayoutCount = 1,
		.pSetLayouts = &dsLayout,
		.pushConstantRangeCount = 0,
		.pPushConstantRanges = NULL
	};

	return (vkCreatePipelineLayout(device, &pipelineLayoutInfo, NULL, pipelineLayout) == VK_SUCCESS);
}

VkShaderStageFlagBits vulkanShaderStageFromFileName(const char* fileName)
{
	if (ends_with(fileName, ".vert"))
		return VK_SHADER_STAGE_VERTEX_BIT;

	if (ends_with(fileName, ".frag"))
		return VK_SHADER_STAGE_FRAGMENT_BIT;

	if (ends_with(fileName, ".geom"))
		return VK_SHADER_STAGE_GEOMETRY_BIT;

	if (ends_with(fileName, ".comp"))
		return VK_SHADER_STAGE_COMPUTE_BIT;

	if (ends_with(fileName, ".tesc"))
		return VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT;

	if (ends_with(fileName, ".tese"))
		return VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT;

	return VK_SHADER_STAGE_VERTEX_BIT;
}

b8
vulkan_create_graphics_pipeline(vulkan_render_device* vkDev,
																VkRenderPass renderPass,
																VkPipelineLayout pipelineLayout,
																char** shaderFiles,
																i32 shaders_count,
																VkPipeline* pipeline,
																VkPrimitiveTopology topology,
																b8 useDepth,
																b8 useBlending,
																b8 dynamicScissorState,
																i32 customWidth,
																i32 customHeight,
																u32 numPatchControlPoints)
{
	// std::vector<ShaderModule> shaderModules;
	cvec(ShaderModule) shaderModules = cvec_ncreate(ShaderModule, shaders_count);
	//std::vector<VkPipelineShaderStageCreateInfo> shaderStages;
	cvec(VkPipelineShaderStageCreateInfo) shaderStages = cvec_ncreate(VkPipelineShaderStageCreateInfo, shaders_count);

	//shaderStages.resize(shaderFiles.size());
	//shaderModules.resize(shaderFiles.size());

	for (size_t i = 0; i < shaders_count; i++)
	{
		const char* file = shaderFiles[i];
		VK_CHECK(createShaderModule(vkDev->device, &shaderModules[i], file));

		VkShaderStageFlagBits stage = vulkanShaderStageFromFileName(file);

		shaderStages[i] = shaderStageInfo(stage, &shaderModules[i], "main");
	}

	VkPipelineVertexInputStateCreateInfo vertexInputInfo =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO
	};

	VkPipelineInputAssemblyStateCreateInfo inputAssembly =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		/* The only difference from createGraphicsPipeline() */
		.topology = topology,
		.primitiveRestartEnable = VK_FALSE
	};

	b8 is_fliped = true;

	VkViewport viewport =
	{
		.x = 0.0f,
		.y = is_fliped ? (f32)(customHeight > 0 ? customHeight : vkDev->framebuffer_height) : 0.0f,
		.width = (f32)(customWidth > 0 ? customWidth : vkDev->framebuffer_width),
		.height = (is_fliped ? -1.0f : 1.0f) * (f32)(customHeight > 0 ? customHeight : vkDev->framebuffer_height),
		.minDepth = 0.0f,
		.maxDepth = 1.0f
	};

	VkRect2D scissor =
	{
		.offset = { 0, 0 },
		.extent = { customWidth > 0 ? customWidth : vkDev->framebuffer_width, customHeight > 0 ? customHeight : vkDev->framebuffer_height }
	};

	VkPipelineViewportStateCreateInfo viewportState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.pViewports = &viewport,
		.scissorCount = 1,
		.pScissors = &scissor
	};

	VkPipelineRasterizationStateCreateInfo rasterizer =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_NONE,
		.frontFace = VK_FRONT_FACE_CLOCKWISE,
		.lineWidth = 1.0f
	};

	VkPipelineMultisampleStateCreateInfo multisampling =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 1.0f
	};

	VkPipelineColorBlendAttachmentState colorBlendAttachment =
	{
		.blendEnable = VK_TRUE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
		.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
		.colorBlendOp = VK_BLEND_OP_ADD,
		.srcAlphaBlendFactor = useBlending ? VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA : VK_BLEND_FACTOR_ONE,
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
		.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT
	};

	VkPipelineColorBlendStateCreateInfo colorBlending =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY,
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachment,
		.blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f }
	};

	VkPipelineDepthStencilStateCreateInfo depthStencil =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = (VkBool32)(useDepth ? VK_TRUE : VK_FALSE),
		.depthWriteEnable = (VkBool32)(useDepth ? VK_TRUE : VK_FALSE),
		.depthCompareOp = VK_COMPARE_OP_LESS,
		.depthBoundsTestEnable = VK_FALSE,
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 1.0f
	};

	VkDynamicState dynamicStateElt = VK_DYNAMIC_STATE_SCISSOR;

	VkPipelineDynamicStateCreateInfo dynamicState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.dynamicStateCount = 1,
		.pDynamicStates = &dynamicStateElt
	};

	VkPipelineTessellationStateCreateInfo tessellationState =
	{
		.sType = VK_STRUCTURE_TYPE_PIPELINE_TESSELLATION_STATE_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.patchControlPoints = numPatchControlPoints
	};

	VkGraphicsPipelineCreateInfo pipelineInfo =
	{
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = (u32)(cvec_size(shaderStages)),
		.pStages = shaderStages,
		.pVertexInputState = &vertexInputInfo,
		.pInputAssemblyState = &inputAssembly,
		.pTessellationState = (topology == VK_PRIMITIVE_TOPOLOGY_PATCH_LIST) ? &tessellationState : NULL,
		.pViewportState = &viewportState,
		.pRasterizationState = &rasterizer,
		.pMultisampleState = &multisampling,
		.pDepthStencilState = useDepth ? &depthStencil : NULL,
		.pColorBlendState = &colorBlending,
		.pDynamicState = dynamicScissorState ? &dynamicState : NULL,
		.layout = pipelineLayout,
		.renderPass = renderPass,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE,
		.basePipelineIndex = -1
	};

	VK_CHECK(vkCreateGraphicsPipelines(vkDev->device, VK_NULL_HANDLE, 1, &pipelineInfo, NULL, pipeline));

	for (u64 i = 0; i < cvec_size(shaderModules); i++)
	{
		vkDestroyShaderModule(vkDev->device, shaderModules[i].shaderModule, NULL);
	}

	return true;
}

b8
vulkan_create_color_and_depth_framebuffers(vulkan_render_device* vkDev,
																					 VkRenderPass renderPass,
																					 VkImageView depthImageView,
																					 cvec(VkFramebuffer)* swapchainFramebuffers)
{
	*swapchainFramebuffers = cvec_ncreate(VkFramebuffer, cvec_size(vkDev->swapchain_image_views));

	for (u64 i = 0; i < cvec_size(vkDev->swapchain_images); i++)
	{
		VkImageView attachments[] =
		{
			vkDev->swapchain_image_views[i],
			depthImageView
		};

		VkFramebufferCreateInfo framebufferInfo =
		{
			.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
			.pNext = NULL,
			.flags = 0,
			.renderPass = renderPass,
			.attachmentCount = (u32)((depthImageView == VK_NULL_HANDLE) ? 1 : 2),
			.pAttachments = attachments,
			.width = vkDev->framebuffer_width,
			.height = vkDev->framebuffer_height,
			.layers = 1
		};

		VK_CHECK(vkCreateFramebuffer(vkDev->device, &framebufferInfo, NULL, &(*swapchainFramebuffers)[i]));
	}

	return true;
}

void
vulkan_destroy_image(VkDevice device,
  									 vulkan_image* image)
{
	vkDestroyImageView(device, image->view, NULL);
	vkDestroyImage(device, image->handle, NULL);
	vkFreeMemory(device, image->memory, NULL);
}

b8
vulkan_create_uniform_buffer(vulkan_render_device* vkDev,
														 VkBuffer* buffer,
														 VkDeviceMemory* bufferMemory,
														 VkDeviceSize bufferSize)
{
	return vulkan_create_buffer(vkDev->device,
															vkDev->physical_device,
															bufferSize,
															VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
															VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
															buffer, bufferMemory);
}

b8
vulkan_create_depth_resources(vulkan_render_device* vkDev,
															u32 width,
															u32 height,
															vulkan_image* depth)
{
	VkFormat depthFormat = findDepthFormat(vkDev->physical_device);

	if (!vulkan_create_image(vkDev->device,
													 vkDev->physical_device,
													 width,
													 height,
													 depthFormat,
													 VK_IMAGE_TILING_OPTIMAL,
													 VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
													 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
													 &depth->handle,
													 &depth->memory,
													 0,
													 1))
	{
		return false;
	}

	if (!vulkan_create_image_view(vkDev->device, depth->handle, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, &depth->view, VK_IMAGE_VIEW_TYPE_2D, 1, 1))
	{
		return false;
	}

	vulkan_transit_image_layout(vkDev, depth->handle, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1, 1);

	return true;
}

VkWriteDescriptorSet
bufferWriteDescriptorSet(VkDescriptorSet ds,
												 const VkDescriptorBufferInfo* bi,
												 u32 bindIdx,
												 VkDescriptorType dType)
{
	return (VkWriteDescriptorSet)
	{
		VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		NULL,
		ds,
		bindIdx,
		0,
		1,
		dType,
		NULL,
		bi,
		NULL
	};
}

VkWriteDescriptorSet
imageWriteDescriptorSet(VkDescriptorSet ds,
												const VkDescriptorImageInfo* ii,
												u32 bindIdx) 
{
	return (VkWriteDescriptorSet)
	{
		VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
		NULL,
		ds,
		bindIdx,
		0,
		1,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		ii,
		NULL,
		NULL
	};
}

#endif