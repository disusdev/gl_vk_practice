#ifndef __VK_QUAD_RENDERER_H__
#define __VK_QUAD_RENDERER_H__

#include <defines.h>

#include "vk_base_renderer.h"

static i32 MAX_QUADS = 256;

typedef struct
t_quad_const_buffer
{
  vec2 offset;
  u32 texture_index;
}
t_quad_const_buffer;

typedef struct
t_quad_vertex_data
{
  vec3 pos;
  vec2 tc;
}
t_quad_vertex_data;

typedef struct
t_quad_renderer
{
  t_base_renderer* base;

  cvec(t_quad_vertex_data) quads;

  u64 vertex_buffer_size;
  u64 index_buffer_size;

  cvec(VkBuffer) storage_buffers;
  cvec(VkDeviceMemory) storage_buffers_memory;

  cvec(vulkan_image) textures;
  cvec(VkSampler) texture_samplers;
}
t_quad_renderer;

void quad_renderer_clear(t_quad_renderer* renderer)
{
  cvec_clear(renderer->quads);
}

void quad_renderer_draw(t_quad_renderer* renderer,
                        float x1, float y1,
                        float x2, float y2)
{
	t_quad_vertex_data v1 = { (vec3){ x1, y1, 0 }, (vec2){ 0, 0 } };
	t_quad_vertex_data v2 = { (vec3){ x2, y1, 0 }, (vec2){ 1, 0 } };
	t_quad_vertex_data v3 = { (vec3){ x2, y2, 0 }, (vec2){ 1, 1 } };
	t_quad_vertex_data v4 = { (vec3){ x1, y2, 0 }, (vec2){ 0, 1 } };

  cvec_push(renderer->quads, v1);
  cvec_push(renderer->quads, v2);
  cvec_push(renderer->quads, v3);

  cvec_push(renderer->quads, v1);
  cvec_push(renderer->quads, v3);
  cvec_push(renderer->quads, v4);
}

// FIXIT: turn to .c files
static void
quad_renderer_fill_command_buffer(t_quad_renderer* renderer,
                    VkCommandBuffer commandBuffer,
                    u64 currentImage)
{
  if (cvec_empty(renderer->quads))
  {
		return;
  }

	base_renderer_begin_render_pass(renderer->base, commandBuffer, currentImage);

	vkCmdDraw(commandBuffer, (u32)cvec_size(renderer->quads), 1, 0, 0);
	vkCmdEndRenderPass(commandBuffer);
}

// FIXIT: turn to .c files
static b8
quad_renderer_create_descriptor_set(t_quad_renderer* renderer, vulkan_render_device* vkDev)
{
  u64 images_cout = cvec_size(vkDev->swapchain_images);

  const VkDescriptorSetLayoutBinding bindings[] =
  {
		descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1),
		descriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1),
		descriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, (u32)(cvec_size(renderer->textures)))
	};

	const VkDescriptorSetLayoutCreateInfo layoutInfo =
  {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.bindingCount = (u32)ARRAY_SIZE(bindings),
		.pBindings = bindings
	};

	VK_CHECK(vkCreateDescriptorSetLayout(renderer->base->device, &layoutInfo, NULL, &renderer->base->descriptor_set_layout));
	cvec(VkDescriptorSetLayout) layouts = cvec_ncreate_set(VkDescriptorSetLayout, images_cout, &renderer->base->descriptor_set_layout);

	const VkDescriptorSetAllocateInfo allocInfo =
  {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = NULL,
		.descriptorPool = renderer->base->descriptor_pool,
		.descriptorSetCount = (u32)(images_cout),
		.pSetLayouts = layouts
	};

  renderer->base->descriptor_sets = cvec_ncreate(VkDescriptorSet, images_cout);

	VK_CHECK(vkAllocateDescriptorSets(renderer->base->device, &allocInfo, renderer->base->descriptor_sets));

	cvec(VkDescriptorImageInfo) textureDescriptors = cvec_ncreate(VkDescriptorImageInfo, cvec_size(renderer->textures));
	for (u64 i = 0; i < cvec_size(renderer->textures); i++)
  {
		textureDescriptors[i] = (VkDescriptorImageInfo)
    {
			.sampler = renderer->texture_samplers[i],
			.imageView = renderer->textures[i].view,
			.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL
		};
	}

	for (u64 i = 0; i < images_cout; i++)
	{
		const VkDescriptorBufferInfo bufferInfo =
    {
			.buffer = renderer->base->uniform_buffers[i],
			.offset = 0,
			.range = sizeof(t_quad_const_buffer)
		};

		const VkDescriptorBufferInfo bufferInfo2 =
    {
			.buffer = renderer->storage_buffers[i],
			.offset = 0,
			.range = renderer->vertex_buffer_size
		};

		const VkWriteDescriptorSet descriptorWrites[] =
    {
			(VkWriteDescriptorSet)
      {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = renderer->base->descriptor_sets[i],
				.dstBinding = 0,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
				.pBufferInfo = &bufferInfo
			},
			(VkWriteDescriptorSet)
      {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = renderer->base->descriptor_sets[i],
				.dstBinding = 1,
				.dstArrayElement = 0,
				.descriptorCount = 1,
				.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER,
				.pBufferInfo = &bufferInfo2
			},
			(VkWriteDescriptorSet)
      {
				.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET,
				.dstSet = renderer->base->descriptor_sets[i],
				.dstBinding = 2,
				.dstArrayElement = 0,
				.descriptorCount = (u32)cvec_size(renderer->textures),
				.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
				.pImageInfo = textureDescriptors
			},
		};
		vkUpdateDescriptorSets(renderer->base->device, (u32)ARRAY_SIZE(descriptorWrites), descriptorWrites, 0, NULL);
	}

	return true;
}

void
quad_renderer_update_buffers(t_quad_renderer* renderer,
                             vulkan_render_device* vkDev,
                             u64 i)
{
  vulkan_upload_buffer_data(vkDev, &renderer->storage_buffers_memory[i], 0, renderer->quads, cvec_size(renderer->quads) * sizeof(t_quad_vertex_data));
}

static void
push_constants(t_quad_renderer* renderer,
               VkCommandBuffer cmd,
               u32 textureIndex,
               vec2* offset)
{
  const t_quad_const_buffer constBuffer = { *offset, textureIndex };
	vkCmdPushConstants(cmd, renderer->base->pipeline_layout, VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(t_quad_const_buffer), &constBuffer);
}

t_quad_renderer
quad_renderer_create(vulkan_render_device* vkDev,
                     cvec(char*)* textureFiles,
                     u64 textureCount)
{
  t_quad_renderer renderer = { 0 };
  renderer.base = base_renderer_create(vkDev, (vulkan_image) { 0, 0, 0 });
  renderer.base->fill_command_buffer = &quad_renderer_fill_command_buffer;
  renderer.base->push_constants = &push_constants;

  const u64 imgCount = cvec_size(vkDev->swapchain_images);
  renderer.base->framebuffer_width = vkDev->framebuffer_width;
  renderer.base->framebuffer_height = vkDev->framebuffer_height;

  renderer.quads = cvec_ncreate(t_quad_vertex_data, MAX_QUADS * 6);
  cvec_clear(renderer.quads);

  renderer.storage_buffers = cvec_ncreate(VkBuffer, imgCount);
  renderer.storage_buffers_memory = cvec_ncreate(VkDeviceMemory, imgCount);

  renderer.vertex_buffer_size = MAX_QUADS * 6 * sizeof(t_quad_vertex_data);

  for (u64 i = 0; i < imgCount; i++)
  {
    if (!vulkan_create_buffer(vkDev->device,
											        vkDev->physical_device,
											        renderer.vertex_buffer_size,
											        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
											        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
											        VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
											        &renderer.storage_buffers[i],
											        &renderer.storage_buffers_memory[i]))
    {
      printf("Cannot create vertex buffer\n");
			fflush(stdout);
			exit(EXIT_FAILURE);
    }
  }
  
  if (!base_renderer_create_uniform_buffers(renderer.base,
                                            vkDev,
                                            sizeof(t_quad_const_buffer)))
  {
    printf("Cannot create data buffers\n");
		fflush(stdout);
		exit(EXIT_FAILURE);
  }

  const u64 numTextureFiles = textureCount;

  renderer.textures = cvec_ncreate(vulkan_image, numTextureFiles);
  renderer.texture_samplers = cvec_ncreate(VkSampler, numTextureFiles);

  for (u64 i = 0; i < numTextureFiles; i++)
  {
    printf("\rLoading texture %u...", (u32)i);
    vulkan_create_texture_image(vkDev, (*textureFiles)[i], &renderer.textures[i].handle, &renderer.textures[i].memory, NULL, NULL);
	  vulkan_create_image_view(vkDev->device, renderer.textures[i].handle, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_ASPECT_COLOR_BIT, &renderer.textures[i].view, VK_IMAGE_VIEW_TYPE_2D, 1, 1);
	  vulkan_create_texture_sampler(vkDev->device, &renderer.texture_samplers[i], VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);
  }
  printf("\n");

  const char* shaders[] =
  {
    "data/shaders/chapter06/VK02_texture_array.vert",
    "data/shaders/chapter06/VK02_texture_array.frag"
  };

  RenderPassCreateInfo render_pass_info = 
  {
    .clearColor_ = false,
    .clearDepth_ = false,
    .flags_ = 0 
  };

  if (!vulkan_create_depth_resources(vkDev,
                                     vkDev->framebuffer_width,
                                     vkDev->framebuffer_height,
                                     &renderer.base->depth_texture) ||
      !vulkan_create_descriptor_pool(vkDev,
                                     1,
                                     1,
                                     1,
                                     &renderer.base->descriptor_pool) ||
      !quad_renderer_create_descriptor_set(&renderer, vkDev) ||
      !vulkan_create_color_and_depth_render_pass(vkDev,
                                                 false,
                                                 &renderer.base->render_pass,
                                                 &render_pass_info,
                                                 VK_FORMAT_B8G8R8A8_UNORM) ||
      !vulkan_create_pipeline_layout_with_consts(vkDev->device,
                                                 renderer.base->descriptor_set_layout,
                                                 &renderer.base->pipeline_layout,
                                                 sizeof(t_quad_const_buffer), 0) ||
      !vulkan_create_graphics_pipeline(vkDev,
                                       renderer.base->render_pass,
                                       renderer.base->pipeline_layout,
                                       shaders,
                                       ARRAY_SIZE(shaders),
                                       &renderer.base->graphics_pipeline,
                                       VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, true, true, false, -1, -1, 0))
  {
    printf("ModelRenderer: failed to create pipeline\n");
		exit(EXIT_FAILURE);
  }

  vulkan_create_color_and_depth_framebuffers(vkDev,
																						 renderer.base->render_pass,
																						 VK_NULL_HANDLE, //  renderer.base->depth_texture.view,
																						 &renderer.base->swapchain_framebuffers);

  return renderer;
}

void
quad_renderer_destroy(t_quad_renderer* renderer)
{
  VkDevice device = renderer->base->device;

  for (u64 i = 0; i < cvec_size(renderer->storage_buffers); i++)
  {
    vkDestroyBuffer(device, renderer->storage_buffers[i], NULL);
		vkFreeMemory(device, renderer->storage_buffers_memory[i], NULL);
  }

  for (u64 i = 0; i < cvec_size(renderer->textures); i++)
  {
    vkDestroySampler(device, renderer->texture_samplers[i], NULL);
    vulkan_destroy_image(device, &renderer->textures[i]);  
  }

  vulkan_destroy_image(device, &renderer->base->depth_texture);
}

#endif