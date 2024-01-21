#include "vk_pbr_model_renderer.h"

#include <containers/chtb.h>

static void
fill_command_buffer(t_pbr_model_renderer* renderer,
                    VkCommandBuffer commandBuffer,
                    u64 currentImage)
{
  base_renderer_begin_render_pass(renderer->base, commandBuffer, currentImage);
	/* For CountKHR (Vulkan 1.1) we may use indirect rendering with GPU-based object counter */
	/// vkCmdDrawIndirectCountKHR(commandBuffer, indirectBuffers_[currentImage], 0, countBuffers_[currentImage], 0, maxShapes_, sizeof(VkDrawIndirectCommand));
	/* For Vulkan 1.0 vkCmdDrawIndirect is enough */
	// vkCmdDrawIndirect(commandBuffer, renderer->indirect_buffers[currentImage], 0, renderer->max_shapes, sizeof(VkDrawIndirectCommand));

  vkCmdDraw(commandBuffer, (u32)(renderer->index_buffer_size / (sizeof(u32))), 1, 0, 0);

	vkCmdEndRenderPass(commandBuffer);
}

void
pbr_model_update_uniform_buffer(t_pbr_model_renderer* renderer, u32 currentImage, const void* data, const u64 dataSize)
{
  vulkan_upload_buffer_data(renderer->vk_dev, &renderer->base->uniform_buffers_memory[currentImage], 0, data, dataSize);
}

static void
load_cube_map(vulkan_render_device* vk_dev, const char* file_name, vulkan_texture* cubemap, u32 mipLevels /*= 1*/)
{
	if (mipLevels > 1)
		vulkan_create_mip_cube_texture_image(vk_dev, file_name, mipLevels, &cubemap->image.handle, &cubemap->image.memory, NULL, NULL);
	else
		vulkan_create_cube_texture_image(vk_dev, file_name, &cubemap->image.handle, &cubemap->image.memory, NULL, NULL);

	vulkan_create_image_view(vk_dev->device, cubemap->image.handle, VK_FORMAT_R32G32B32A32_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, &cubemap->image.view, VK_IMAGE_VIEW_TYPE_CUBE, 6, mipLevels);
	vulkan_create_texture_sampler(vk_dev->device, &cubemap->sampler, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);
}

static b8
create_descriptor_set(t_pbr_model_renderer* renderer,
                      u32 uniform_buffer_size)
{
  const VkDescriptorSetLayoutBinding bindings[] =
  {
		descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 1),
		descriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1),
		descriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1),

		descriptorSetLayoutBinding(3, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),  // AO
		descriptorSetLayoutBinding(4, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),  // Emissive
		descriptorSetLayoutBinding(5, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),  // Albedo
		descriptorSetLayoutBinding(6, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),  // MeR
		descriptorSetLayoutBinding(7, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),  // Normal

		descriptorSetLayoutBinding(8, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),  // env
		descriptorSetLayoutBinding(9, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1),  // env_IRR

		descriptorSetLayoutBinding(10, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)  // brdfLUT
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

  cvec(VkDescriptorSetLayout) layouts = cvec_ncreate_set(VkDescriptorSetLayout, cvec_size(renderer->vk_dev->swapchain_images), &renderer->base->descriptor_set_layout);

	const VkDescriptorSetAllocateInfo allocInfo =
  {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = NULL,
		.descriptorPool = renderer->base->descriptor_pool,
		.descriptorSetCount = (u32)cvec_size(renderer->vk_dev->swapchain_images),
		.pSetLayouts = layouts
	};

  renderer->base->descriptor_sets = cvec_ncreate(VkDescriptorSet, cvec_size(renderer->vk_dev->swapchain_images));

	VK_CHECK(vkAllocateDescriptorSets(renderer->vk_dev->device, &allocInfo, renderer->base->descriptor_sets));

	for (u64 i = 0; i < cvec_size(renderer->vk_dev->swapchain_images); i++)
	{
		VkDescriptorSet ds = renderer->base->descriptor_sets[i];

		const VkDescriptorBufferInfo bufferInfo  = { renderer->base->uniform_buffers[i], 0, uniform_buffer_size };
		const VkDescriptorBufferInfo bufferInfo2 = { renderer->storage_buffer, 0, renderer->vertex_buffer_size };
		const VkDescriptorBufferInfo bufferInfo3 = { renderer->storage_buffer, renderer->vertex_buffer_size, renderer->index_buffer_size };
		const VkDescriptorImageInfo  imageInfoAO       = { renderer->tex_ao.sampler, renderer->tex_ao.image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		const VkDescriptorImageInfo  imageInfoEmissive = { renderer->tex_emissive.sampler, renderer->tex_emissive.image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		const VkDescriptorImageInfo  imageInfoAlbedo   = { renderer->tex_albedo.sampler, renderer->tex_albedo.image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		const VkDescriptorImageInfo  imageInfoMeR      = { renderer->tex_me_r.sampler, renderer->tex_me_r.image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		const VkDescriptorImageInfo  imageInfoNormal   = { renderer->tex_normal.sampler, renderer->tex_normal.image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

		const VkDescriptorImageInfo  imageInfoEnv      = { renderer->env_map.sampler, renderer->env_map.image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		const VkDescriptorImageInfo  imageInfoEnvIrr   = { renderer->env_map_irradiance.sampler, renderer->env_map_irradiance.image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };
		const VkDescriptorImageInfo  imageInfoBRDF     = { renderer->brdf_lut.sampler, renderer->brdf_lut.image.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL };

		const VkWriteDescriptorSet descriptorWrites[] =
    {
			bufferWriteDescriptorSet(ds, &bufferInfo,  0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
			bufferWriteDescriptorSet(ds, &bufferInfo2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
			bufferWriteDescriptorSet(ds, &bufferInfo3, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
			imageWriteDescriptorSet( ds, &imageInfoAO,       3),
			imageWriteDescriptorSet( ds, &imageInfoEmissive, 4),
			imageWriteDescriptorSet( ds, &imageInfoAlbedo,   5),
			imageWriteDescriptorSet( ds, &imageInfoMeR,      6),
			imageWriteDescriptorSet( ds, &imageInfoNormal,   7),

			imageWriteDescriptorSet( ds, &imageInfoEnv,      8),
			imageWriteDescriptorSet( ds, &imageInfoEnvIrr,   9),
			imageWriteDescriptorSet( ds, &imageInfoBRDF,     10)
		};

		vkUpdateDescriptorSets(renderer->vk_dev->device, (u32)ARRAY_SIZE(descriptorWrites), descriptorWrites, 0, NULL);
	}

	return true;
}

static hashtable models_cache_table = { 0 };

t_pbr_model_renderer
pbr_model_renderer_create(vulkan_render_device* vk_dev,
                          u32 uniform_buffer_size,
                          const char* model_file,
                          const char* tex_ao_file,
                          const char* tex_emmisive_file,
                          const char* tex_albedo_file,
                          const char* tex_me_r_file,
                          const char* tex_normal_file,
                          const char* tex_env_map_file,
                          const char* tex_irr_map_file,
                          vulkan_image depth_texture)
{
  t_pbr_model_renderer renderer = {0};
  renderer.base = base_renderer_create(vk_dev, depth_texture);
  renderer.base->fill_command_buffer = &fill_command_buffer;
  renderer.vk_dev = vk_dev;

  if (models_cache_table.element_size == 0)
  {
    void* mem = malloc(sizeof(t_model*) * 1024);
    hashtable_create(sizeof(t_model*), 1024, mem, true, &models_cache_table);
  }

  RenderPassCreateInfo render_pass_info = 
  {
    .clearColor_ = false,
    .clearDepth_ = false,
    .flags_ = 0 
  };

  t_model* model = NULL;
  hashtable_get_ptr(&models_cache_table, model_file, &model);

  if (!model)
  {
    model = LoadGLTF(model_file);
    hashtable_set_ptr(&models_cache_table, model_file, &model);
  }

  // Resource loading part
	if (!create_pbr_vertex_buffer(vk_dev,
                                model,
                                &renderer.storage_buffer,
                                &renderer.storage_buffer_memory,
                                &renderer.vertex_buffer_size,
                                &renderer.index_buffer_size))
	{
		printf("ModelRenderer: createPBRVertexBuffer() failed\n");
		exit(EXIT_FAILURE);
	}

  vulkan_load_texture(vk_dev, tex_ao_file, &renderer.tex_ao);
	vulkan_load_texture(vk_dev, tex_emmisive_file, &renderer.tex_emissive);
	vulkan_load_texture(vk_dev, tex_albedo_file, &renderer.tex_albedo);
	vulkan_load_texture(vk_dev, tex_me_r_file, &renderer.tex_me_r);
	vulkan_load_texture(vk_dev, tex_normal_file, &renderer.tex_normal);

  load_cube_map(vk_dev, tex_env_map_file, &renderer.env_map, 1);
  load_cube_map(vk_dev, tex_irr_map_file, &renderer.env_map_irradiance, 1);

  gli_texture* tex = gli_load_ktx("data/brdfLUT.ktx");
  vec2 extent = { 0.0f };
  gli_get_extent(tex, &extent.x, &extent.y);

  if (!vulkan_create_texture_image_from_data(vk_dev,
                                             &renderer.brdf_lut.image.handle,
                                             &renderer.brdf_lut.image.memory,
                                             gli_get_data(tex),
                                             extent.x, extent.y,
                                             VK_FORMAT_R16G16_SFLOAT,
                                             1, 0))
  {
    printf("ModelRenderer: failed to load BRDF LUT texture \n");
		exit(EXIT_FAILURE);
  }

  vulkan_create_image_view(vk_dev->device, renderer.brdf_lut.image.handle, VK_FORMAT_R16G16_SFLOAT, VK_IMAGE_ASPECT_COLOR_BIT, &renderer.brdf_lut.image.view, VK_IMAGE_VIEW_TYPE_2D, 1, 1);
  vulkan_create_texture_sampler(vk_dev->device, &renderer.brdf_lut.sampler, VK_FILTER_LINEAR, VK_FILTER_LINEAR, VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE);

  const char* shaders[] =
  {
    "data/shaders/chapter06/VK05_mesh.vert",
    "data/shaders/chapter06/VK05_mesh.frag"
  };

  if (!vulkan_create_color_and_depth_render_pass(vk_dev,
                                                 true,
                                                 &renderer.base->render_pass,
                                                 &render_pass_info,
                                                 VK_FORMAT_B8G8R8A8_UNORM) ||
      !base_renderer_create_uniform_buffers(renderer.base,
                                            vk_dev,
                                            uniform_buffer_size) ||
      !vulkan_create_color_and_depth_framebuffers(vk_dev,
																						      renderer.base->render_pass,
																						      depth_texture.view,
																						      &renderer.base->swapchain_framebuffers) ||
      !vulkan_create_descriptor_pool(vk_dev,
                                     1,
                                     2,
                                     8,
                                     &renderer.base->descriptor_pool) ||
      !create_descriptor_set(&renderer, uniform_buffer_size) ||
      !vulkan_create_pipeline_layout(vk_dev->device,
                                     renderer.base->descriptor_set_layout,
                                     &renderer.base->pipeline_layout) ||
      !vulkan_create_graphics_pipeline(vk_dev,
                                       renderer.base->render_pass,
                                       renderer.base->pipeline_layout,
                                       shaders,
                                       ARRAY_SIZE(shaders),
                                       &renderer.base->graphics_pipeline,
                                       VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, true, true, false, -1, -1, 0))
  {
    printf("PBRModelRenderer: failed to create pipeline\n");
		exit(EXIT_FAILURE);
  }

  return renderer;
}

void
pbr_model_renderer_destroy(t_pbr_model_renderer* renderer)
{
  base_renderer_destory(renderer->base);

  vkDestroyBuffer(renderer->vk_dev->device, renderer->storage_buffer, NULL);
	vkFreeMemory(renderer->vk_dev->device, renderer->storage_buffer_memory, NULL);

	vulkan_destroy_texture(renderer->vk_dev->device, &renderer->tex_ao);
	vulkan_destroy_texture(renderer->vk_dev->device, &renderer->tex_emissive);
	vulkan_destroy_texture(renderer->vk_dev->device, &renderer->tex_albedo);
	vulkan_destroy_texture(renderer->vk_dev->device, &renderer->tex_me_r);
	vulkan_destroy_texture(renderer->vk_dev->device, &renderer->tex_normal);

	vulkan_destroy_texture(renderer->vk_dev->device,  &renderer->env_map);
	vulkan_destroy_texture(renderer->vk_dev->device,  &renderer->env_map_irradiance);

	vulkan_destroy_texture(renderer->vk_dev->device,  &renderer->brdf_lut);

  memset(renderer, 0, sizeof(t_pbr_model_renderer));
}