#ifndef __VK_CUBEMAP_RENDERER_H__
#define __VK_CUBEMAP_RENDERER_H__

#include <defines.h>

#include <resource/cbitmap.h>

typedef struct
t_cubemap_renderer
{
  t_base_renderer* base;
  vulkan_image texture;
  VkSampler texture_sampler;
}
t_cubemap_renderer;

b8
create_cube_texture_image(vulkan_render_device* vkDev,
                          const char* filename,
                          VkImage* handle,
                          VkDeviceMemory* memory,
                          u32* width,
                          u32* height)
{
  int w, h, comp;
	const f32* img = stbi_loadf(filename, &w, &h, &comp, 3);
	cvec(f32) img32 = cvec_ncreate(f32, (w * h * 4));

	float24to32(w, h, img, img32);

	if (!img)
  {
		printf("Failed to load [%s] texture\n", filename); fflush(stdout);
		return false;
	}

	stbi_image_free((void*)img);

  t_cbitmap* in = cbitmap_create_from_data(w, h, 1, 4, BITMAP_FORMAT_F32, img32);
	t_cbitmap* out = convert_equirectangular_map_to_vertical_cross(in);

  t_cbitmap* cubemap = convert_vertical_cross_to_cube_map_faces(out);

	if (width && height)
	{
		*width = w;
		*height = h;
	}

  return vulkan_create_texture_image_from_data(vkDev,
                                               handle,
                                               memory,
                                               cubemap->data,
                                               cubemap->w,
                                               cubemap->h,
                                               VK_FORMAT_R32G32B32A32_SFLOAT,
                                               6,
                                               VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT);
}

b8
cubemap_renderer_create_descriptor_set(t_cubemap_renderer* cubemap_renderer,
                                       vulkan_render_device* vk_device,
                                       u64 uniform_data_size)
{
  VkDescriptorSetLayoutBinding bindings[] =
  {
		descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1),
		descriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
	};

	VkDescriptorSetLayoutCreateInfo layoutInfo =
  {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.bindingCount = (u32)ARRAY_SIZE(bindings),
		.pBindings = bindings
	};

	VK_CHECK(vkCreateDescriptorSetLayout(vk_device->device, &layoutInfo, NULL, &cubemap_renderer->base->descriptor_set_layout));

  cvec(VkDescriptorSetLayout) layouts = cvec_ncreate_set(VkDescriptorSetLayout, cvec_size(vk_device->swapchain_images), &cubemap_renderer->base->descriptor_set_layout);

	const VkDescriptorSetAllocateInfo allocInfo =
  {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = NULL,
		.descriptorPool = cubemap_renderer->base->descriptor_pool,
		.descriptorSetCount = (u32)cvec_size(vk_device->swapchain_images),
		.pSetLayouts = layouts
	};

  cubemap_renderer->base->descriptor_sets = cvec_ncreate(VkDescriptorSet, cvec_size(vk_device->swapchain_images));

	VK_CHECK(vkAllocateDescriptorSets(vk_device->device, &allocInfo, cubemap_renderer->base->descriptor_sets));

	for (size_t i = 0; i < cvec_size(vk_device->swapchain_images); i++)
	{
		VkDescriptorSet ds = cubemap_renderer->base->descriptor_sets[i];

		const VkDescriptorBufferInfo bufferInfo  = { cubemap_renderer->base->uniform_buffers[i], 0, sizeof(mat4) };
		const VkDescriptorImageInfo  imageInfo   = { cubemap_renderer->texture_sampler, cubemap_renderer->texture.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

		const VkWriteDescriptorSet descriptorWrites[] =
    {
			bufferWriteDescriptorSet(ds, &bufferInfo, 0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
			imageWriteDescriptorSet(ds, &imageInfo, 1)
		};

    vkUpdateDescriptorSets(vk_device->device, (u32)ARRAY_SIZE(descriptorWrites), descriptorWrites, 0, NULL);
	}

	return true;
}

static void
cubemap_renderer_fill_command_buffer(t_cubemap_renderer* cube_renderer,
                                     VkCommandBuffer commandBuffer,
                                     u64 currentImage)
{
  base_renderer_begin_render_pass(cube_renderer->base, commandBuffer, currentImage);

	vkCmdDraw(commandBuffer, 36, 1, 0, 0);

	vkCmdEndRenderPass(commandBuffer);
}

t_cubemap_renderer
cubemap_renderer_create(vulkan_render_device* vkDev,
                        vulkan_image inDepthTexture,
                        const char* textureFile)
{
  t_cubemap_renderer cubemap_renderer = { 0 };

  cubemap_renderer.base = base_renderer_create(vkDev, inDepthTexture);
  cubemap_renderer.base->fill_command_buffer = &cubemap_renderer_fill_command_buffer;

  t_base_renderer* base = cubemap_renderer.base;

  // Resource loading
  create_cube_texture_image(vkDev, textureFile, &cubemap_renderer.texture.handle, &cubemap_renderer.texture.memory, NULL, NULL);

  vulkan_create_image_view(vkDev->device, cubemap_renderer.texture.handle, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, &cubemap_renderer.texture.view, VK_IMAGE_VIEW_TYPE_CUBE, 6, 1);
  vulkan_create_texture_sampler(vkDev->device, &cubemap_renderer.texture_sampler, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_REPEAT);

  RenderPassCreateInfo render_pass_info =
  {
    .clearColor_ = false,
    .clearDepth_ = false,
    .flags_ = 0
  };

  const char* shaders[] =
  {
    "data/shaders/chapter04/VKCube.vert",
    "data/shaders/chapter04/VKCube.frag"
  };

  if (!vulkan_create_color_and_depth_render_pass(vkDev,
                                                 true,
                                                 &base->render_pass,
                                                 &render_pass_info,
                                                 VK_FORMAT_B8G8R8A8_UNORM) ||
      !base_renderer_create_uniform_buffers(base,
                                            vkDev,
                                            sizeof(mat4)) ||
      !vulkan_create_color_and_depth_framebuffers(vkDev,
																						      base->render_pass,
																						      base->depth_texture.view,
																						      &base->swapchain_framebuffers) ||
      !vulkan_create_descriptor_pool(vkDev, 1, 0, 1, &base->descriptor_pool) ||
      !cubemap_renderer_create_descriptor_set(&cubemap_renderer, vkDev, sizeof(mat4)) ||
      !vulkan_create_pipeline_layout(vkDev->device, base->descriptor_set_layout, &base->pipeline_layout) ||
      !vulkan_create_graphics_pipeline(vkDev, base->render_pass,
                                       base->pipeline_layout,
                                       shaders,
                                       ARRAY_SIZE(shaders),
                                       &base->graphics_pipeline,
                                       VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, true, true, false, -1, -1, 0))
	{                                                                                               
		printf("ModelRenderer: failed to create pipeline\n");
		exit(EXIT_FAILURE);
	}

  return cubemap_renderer;
}

void
cubemap_renderer_update_uniform_buffer(t_cubemap_renderer* cube_renderer,
                                       vulkan_render_device* vkDev,
                                       u32 currentImage,
                                       mat4* mtx)
{
  vulkan_upload_buffer_data(vkDev, &cube_renderer->base->uniform_buffers_memory[currentImage], 0, (void*)mtx, sizeof(mat4));
}

void
cubemap_renderer_destroy(t_cubemap_renderer* cube_renderer)
{
  if (cube_renderer->texture_sampler != VK_NULL_HANDLE)
	{
		vkDestroySampler(cube_renderer->base->device, cube_renderer->texture_sampler, NULL);
    vulkan_destroy_image(cube_renderer->base->device, &cube_renderer->texture);
	}

  base_renderer_destory(cube_renderer->base);
  cube_renderer->base = NULL;
}

#endif