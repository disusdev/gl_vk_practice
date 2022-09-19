#ifndef __VK_MODEL_RENDERER__
#define __VK_MODEL_RENDERER__

#include <defines.h>

#include "vk_base_renderer.h"

static VkClearColorValue CLEAR_VALUE_COLOR = (VkClearColorValue) { 1.0f, 1.0f, 1.0f, 1.0f };

typedef struct
t_model_renderer
{
  t_base_renderer* base_renderer;

  u64 vertex_buffer_size;
  u64 index_buffer_size;

  VkBuffer storage_buffer;
  VkDeviceMemory storage_buffer_memory;

  VkSampler texture_sampler;
  vulkan_image texture;

  b8 use_general_texture_layout;// = false
	b8 is_external_depth;// = false
	b8 delete_mesh_data;// = true
}
t_model_renderer;

b8
model_renderer_create_descriptor_set(t_model_renderer* model_renderer,
                                     vulkan_render_device* vk_device,
                                     u64 uniform_data_size)
{
  VkDescriptorSetLayoutBinding bindings[4] =
  {
		descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1),
		descriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1),
		descriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1),
		descriptorSetLayoutBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
	};

	VkDescriptorSetLayoutCreateInfo layoutInfo = (VkDescriptorSetLayoutCreateInfo)
  {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.bindingCount = (u32)ARRAY_SIZE(bindings),
		.pBindings = bindings
	};

	VK_CHECK(vkCreateDescriptorSetLayout(vk_device->device, &layoutInfo, NULL, &model_renderer->base_renderer->descriptor_set_layout));

	cvec(VkDescriptorSetLayout) layouts = cvec_ncreate_set(VkDescriptorSetLayout, cvec_size(vk_device->swapchain_images), &model_renderer->base_renderer->descriptor_set_layout);

	VkDescriptorSetAllocateInfo allocInfo = (VkDescriptorSetAllocateInfo)
  {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = NULL,
		.descriptorPool = model_renderer->base_renderer->descriptor_pool,
		.descriptorSetCount = (u32)cvec_size(vk_device->swapchain_images),
		.pSetLayouts = layouts
	};

  model_renderer->base_renderer->descriptor_sets = cvec_ncreate(VkDescriptorSet, cvec_size(vk_device->swapchain_images));

	VK_CHECK(vkAllocateDescriptorSets(vk_device->device, &allocInfo, model_renderer->base_renderer->descriptor_sets));

	for (u64 i = 0; i < cvec_size(vk_device->swapchain_images); i++)
	{
		VkDescriptorSet ds = model_renderer->base_renderer->descriptor_sets[i];

		VkDescriptorBufferInfo bufferInfo = (VkDescriptorBufferInfo)
    {
      model_renderer->base_renderer->uniform_buffers[i],
      0,
      uniform_data_size
    };
		VkDescriptorBufferInfo bufferInfo2 = (VkDescriptorBufferInfo)
    {
      model_renderer->storage_buffer,
      0,
      model_renderer->vertex_buffer_size
    };
		VkDescriptorBufferInfo bufferInfo3 = (VkDescriptorBufferInfo)
    {
      model_renderer->storage_buffer,
      model_renderer->vertex_buffer_size,
      model_renderer->index_buffer_size
    };
		VkDescriptorImageInfo  imageInfo = (VkDescriptorImageInfo)
    {
      model_renderer->texture_sampler,
      model_renderer->texture.view,
      model_renderer->use_general_texture_layout ? VK_IMAGE_LAYOUT_GENERAL : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
    };

		VkWriteDescriptorSet descriptorWrites[4] =
    {
			bufferWriteDescriptorSet(ds, &bufferInfo, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
			bufferWriteDescriptorSet(ds, &bufferInfo2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
			bufferWriteDescriptorSet(ds, &bufferInfo3, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
			imageWriteDescriptorSet(ds, &imageInfo, 3)
		};

		vkUpdateDescriptorSets(vk_device->device, (u32)ARRAY_SIZE(descriptorWrites), descriptorWrites, 0, NULL);
	}

	return true;
}

t_model_renderer*
model_renderer_create_default(vulkan_render_device* vkDev,
                              const char* modelFile,
                              const char* textureFile,
                              u32 uniformDataSize)
{
  t_model_renderer* model_renderer = malloc(sizeof(t_model_renderer));
  memset(model_renderer, 0, sizeof(t_model_renderer));
  model_renderer->base_renderer = base_renderer_create(vkDev, (vulkan_image) { 0, 0, 0 });
  model_renderer->delete_mesh_data = true;

  if (!vulkan_create_textured_vertex_buffer(vkDev,
																						modelFile,
                                            &model_renderer->storage_buffer,
																						&model_renderer->storage_buffer_memory,
																						&model_renderer->vertex_buffer_size,
																						&model_renderer->index_buffer_size))
	{
		printf("ModelRenderer: createTexturedVertexBuffer() failed\n");
		exit(EXIT_FAILURE);
	}

  vulkan_create_texture_image(vkDev, textureFile, &model_renderer->texture.handle, &model_renderer->texture.memory, NULL, NULL);
	vulkan_create_image_view(vkDev->device, model_renderer->texture.handle, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &model_renderer->texture.view, VK_IMAGE_VIEW_TYPE_2D, 1, 1);
	vulkan_create_texture_sampler(vkDev->device, &model_renderer->texture_sampler, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);

  const char* shaders[] =
  {
    "data/shaders/chapter03/VK02.vert",
    "data/shaders/chapter03/VK02.frag",
    "data/shaders/chapter03/VK02.geom"
  };

  RenderPassCreateInfo render_pass_info = 
  {
    .clearColor_ = false,
    .clearDepth_ = false,
    .flags_ = eRenderPassBit_Last 
  };

  if (!vulkan_create_depth_resources(vkDev,
                                     vkDev->framebuffer_width,
                                     vkDev->framebuffer_height,
                                     &model_renderer->base_renderer->depth_texture) ||
      !vulkan_create_color_and_depth_render_pass(vkDev,
                                                 true,
                                                 &model_renderer->base_renderer->render_pass,
                                                 &render_pass_info,
                                                 VK_FORMAT_B8G8R8A8_UNORM) ||
      !base_renderer_create_uniform_buffers(model_renderer->base_renderer,
                                            vkDev,
                                            uniformDataSize) ||
      !vulkan_create_color_and_depth_framebuffers(vkDev,
																						      model_renderer->base_renderer->render_pass,
																						      model_renderer->base_renderer->depth_texture.view,
																						      &model_renderer->base_renderer->swapchain_framebuffers) ||
      !vulkan_create_descriptor_pool(vkDev,
                                     1,
                                     2,
                                     1,
                                     &model_renderer->base_renderer->descriptor_pool) ||
      !model_renderer_create_descriptor_set(model_renderer,
                                            vkDev,
                                            uniformDataSize) ||
      !vulkan_create_pipeline_layout(vkDev->device,
                                     model_renderer->base_renderer->descriptor_set_layout,
                                     &model_renderer->base_renderer->pipeline_layout) ||
      !vulkan_create_graphics_pipeline(vkDev,
                                       model_renderer->base_renderer->render_pass,
                                       model_renderer->base_renderer->pipeline_layout,
                                       shaders,
                                       ARRAY_SIZE(shaders),
                                       &model_renderer->base_renderer->graphics_pipeline,
                                       VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, true, true, false, -1, -1, 0))
  {
    printf("ModelRenderer: failed to create pipeline\n");
		exit(EXIT_FAILURE);
  }

  return model_renderer;
}

t_model_renderer*
model_renderer_create(vulkan_render_device* vkDev,
                      b8 useDepth,
                      VkBuffer storageBuffer,
                      VkDeviceMemory storageBufferMemory,
                      u32 vertexBufferSize,
                      u32 indexBufferSize,
                      vulkan_image texture,
                      VkSampler textureSampler,
                      cvec(const char*)* shaderFiles,
                      u32 uniformDataSize,
                      b8 useGeneralTextureLayout,
                      vulkan_image externalDepth,
                      b8 deleteMeshData)
{
  t_model_renderer* model_renderer = malloc(sizeof(t_model_renderer));
  memset(model_renderer, 0, sizeof(t_model_renderer));

  model_renderer->use_general_texture_layout = useGeneralTextureLayout;
  model_renderer->vertex_buffer_size = vertexBufferSize;
  model_renderer->index_buffer_size = indexBufferSize;
  model_renderer->storage_buffer = storageBuffer;
  model_renderer->storage_buffer_memory = storageBufferMemory;
  model_renderer->texture = texture;
  model_renderer->texture_sampler = textureSampler;
  model_renderer->delete_mesh_data = deleteMeshData;

  model_renderer->base_renderer = base_renderer_create(vkDev, (vulkan_image) { 0, 0, 0 });

  if (useDepth)
	{
		model_renderer->is_external_depth = (externalDepth.handle != VK_NULL_HANDLE);

		if (model_renderer->is_external_depth)
    {
			model_renderer->base_renderer->depth_texture = externalDepth;
    }
		else
    {
      vulkan_create_depth_resources(vkDev, vkDev->framebuffer_width, vkDev->framebuffer_height, &model_renderer->base_renderer->depth_texture);
    }
	}

  RenderPassCreateInfo render_pass_info =
  {
    .clearColor_ = false,
    .clearDepth_ = false,
    .flags_ = eRenderPassBit_Last
  };

	if (!vulkan_create_color_and_depth_render_pass(&vkDev,
                                                 useDepth,
                                                 &model_renderer->base_renderer->render_pass,
                                                 &render_pass_info,
                                                 VK_FORMAT_B8G8R8A8_UNORM) ||
      !base_renderer_create_uniform_buffers(model_renderer->base_renderer,
                                            vkDev,
                                            uniformDataSize) ||
      !vulkan_create_color_and_depth_framebuffers(vkDev,
																						      model_renderer->base_renderer->render_pass,
																						      model_renderer->base_renderer->depth_texture.view,
																						      &model_renderer->base_renderer->swapchain_framebuffers) ||
      !vulkan_create_descriptor_pool(vkDev, 1, 2, 1, &model_renderer->base_renderer->descriptor_pool) ||
      !model_renderer_create_descriptor_set(&model_renderer, vkDev, uniformDataSize) ||
      !vulkan_create_pipeline_layout(vkDev->device, model_renderer->base_renderer->descriptor_set_layout, &model_renderer->base_renderer->pipeline_layout) ||
      !vulkan_create_graphics_pipeline(vkDev, model_renderer->base_renderer->render_pass,
                                       model_renderer->base_renderer->pipeline_layout,
                                       *shaderFiles,
                                       cvec_size(*shaderFiles),
                                       &model_renderer->base_renderer->graphics_pipeline,
                                       VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, true, true, false, -1, -1, 0))
	{                                                                                               
		printf("ModelRenderer: failed to create pipeline\n");
		exit(EXIT_FAILURE);
	}

  return model_renderer;
}

void
model_renderer_destroy(t_model_renderer* model_renderer)
{
  if (model_renderer->delete_mesh_data)
	{
		vkDestroyBuffer(model_renderer->base_renderer->device, model_renderer->storage_buffer, NULL);
		vkFreeMemory(model_renderer->base_renderer->device, model_renderer->storage_buffer_memory, NULL);
	}

	if (model_renderer->texture_sampler != VK_NULL_HANDLE)
	{
		vkDestroySampler(model_renderer->base_renderer->device, model_renderer->texture_sampler, NULL);
    vulkan_destroy_image(model_renderer->base_renderer->device, &model_renderer->texture);
	}

	if (!model_renderer->is_external_depth)
  {
    vulkan_destroy_image(model_renderer->base_renderer->device, &model_renderer->base_renderer->depth_texture);
  }

  base_renderer_destory(model_renderer->base_renderer);
  model_renderer->base_renderer = NULL;
}

void
model_renderer_fill_command_buffer(t_model_renderer* model_renderer,
                                   VkCommandBuffer commandBuffer,
                                   u64 currentImage)
{
  base_renderer_begin_render_pass(model_renderer->base_renderer, commandBuffer, currentImage);

	vkCmdDraw(commandBuffer, (u32)(model_renderer->index_buffer_size / (sizeof(u32))), 1, 0, 0);
	vkCmdEndRenderPass(commandBuffer);
}

void
model_renderer_update_uniform_buffer(t_model_renderer* model_renderer,
                                     vulkan_render_device* vkDev,
                                     u32 currentImage,
                                     const void* data,
                                     u64 dataSize)
{
	vulkan_upload_buffer_data(vkDev, &model_renderer->base_renderer->uniform_buffers_memory[currentImage], 0, data, dataSize);
}

#endif