#ifndef __VK_MULTI_MESH_RENDERER_H__
#define __VK_MULTI_MESH_RENDERER_H__

#include <defines.h>

#include "vk_base_renderer.h"

typedef struct
t_multi_mesh_renderer
{
  t_base_renderer* base;

  u32 vertex_buffer_size;
  u32 index_buffer_size;

  vulkan_render_device* vk_dev;

  u32 max_vertex_buffer_size;
  u32 max_index_buffer_size;

  u32 max_shapes;

  u32 max_draw_data_size;
  u32 max_material_size;

  VkBuffer storage_buffer;
  VkDeviceMemory storage_buffer_memory;

  VkBuffer material_buffer;
  VkDeviceMemory material_buffer_memory;

  cvec(VkBuffer) indirect_buffers;
  cvec(VkDeviceMemory) indirect_buffers_memory;

  cvec(VkBuffer) draw_data_buffers;
  cvec(VkDeviceMemory) draw_data_buffers_memory;

  cvec(VkBuffer) count_buffers;
  cvec(VkDeviceMemory) count_buffers_memory;

  cvec(t_draw_data) shapes;
  t_mesh_data mesh_data;
}
t_multi_mesh_renderer;

static void
load_draw_data(t_multi_mesh_renderer* renderer,
               const char* drawDataFile)
{
  FILE* f = fopen(drawDataFile, "rb");

	if (!f)
  {
		printf("Unable to open draw data file. Run MeshConvert first\n");
		exit(255);
	}

	fseek(f, 0, SEEK_END);
	u64 fsize = ftell(f);
	fseek(f, 0, SEEK_SET);
	renderer->max_shapes = (u32)(fsize / sizeof(t_draw_data));

	printf("Reading draw data items: %d\n", (int)renderer->max_shapes); fflush(stdout);

  renderer->shapes = cvec_ncreate(t_draw_data, renderer->max_shapes);

	if (fread(renderer->shapes, sizeof(t_draw_data), renderer->max_shapes, f) != renderer->max_shapes)
  {
		printf("Unable to read draw data\n");
		exit(255);
	}

	fclose(f);
}

static void
fill_command_buffer(t_multi_mesh_renderer* renderer,
                    VkCommandBuffer commandBuffer,
                    u64 currentImage)
{
  base_renderer_begin_render_pass(renderer->base, commandBuffer, currentImage);
	/* For CountKHR (Vulkan 1.1) we may use indirect rendering with GPU-based object counter */
	/// vkCmdDrawIndirectCountKHR(commandBuffer, indirectBuffers_[currentImage], 0, countBuffers_[currentImage], 0, maxShapes_, sizeof(VkDrawIndirectCommand));
	/* For Vulkan 1.0 vkCmdDrawIndirect is enough */
	vkCmdDrawIndirect(commandBuffer, renderer->indirect_buffers[currentImage], 0, renderer->max_shapes, sizeof(VkDrawIndirectCommand));

	vkCmdEndRenderPass(commandBuffer);
}

static b8
create_descriptor_set(t_multi_mesh_renderer* renderer)
{
  const VkDescriptorSetLayoutBinding bindings[] =
  {
		descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1),
		/* vertices [part of this.storageBuffer] */
		descriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1),
		/* indices [part of this.storageBuffer] */
		descriptorSetLayoutBinding(2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1),
		/* draw data [this.drawDataBuffer] */
		descriptorSetLayoutBinding(3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1),
		/* material data [this.materialBuffer] */
		descriptorSetLayoutBinding(4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_FRAGMENT_BIT, 1)
	};

	const VkDescriptorSetLayoutCreateInfo layoutInfo =
  {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.bindingCount = (u32)(ARRAY_SIZE(bindings)),
		.pBindings = bindings
	};

	VK_CHECK(vkCreateDescriptorSetLayout(renderer->vk_dev->device, &layoutInfo, NULL, &renderer->base->descriptor_set_layout));

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

		const VkDescriptorBufferInfo bufferInfo  = { renderer->base->uniform_buffers[i], 0, sizeof(mat4) };
		const VkDescriptorBufferInfo bufferInfo2 = { renderer->storage_buffer, 0, renderer->max_vertex_buffer_size };
		const VkDescriptorBufferInfo bufferInfo3 = { renderer->storage_buffer, renderer->max_vertex_buffer_size, renderer->max_index_buffer_size };
		const VkDescriptorBufferInfo bufferInfo4 = { renderer->draw_data_buffers[i], 0, renderer->max_draw_data_size };
		const VkDescriptorBufferInfo bufferInfo5 = { renderer->material_buffer, 0, renderer->max_material_size };

		const VkWriteDescriptorSet descriptorWrites[] =
    {
			bufferWriteDescriptorSet(ds, &bufferInfo,  0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
			bufferWriteDescriptorSet(ds, &bufferInfo2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
			bufferWriteDescriptorSet(ds, &bufferInfo3, 2, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
			bufferWriteDescriptorSet(ds, &bufferInfo4, 3, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
			bufferWriteDescriptorSet(ds, &bufferInfo5, 4, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER),
      // imageWriteDescriptorSet( ds, &imageInfo,   3)
		};

		vkUpdateDescriptorSets(renderer->vk_dev->device, (u32)ARRAY_SIZE(descriptorWrites), descriptorWrites, 0, NULL);
	}

	return true;
}

void
multi_mesh_renderer_update_geometry_buffers(t_multi_mesh_renderer* renderer,
                                            u32 vertexCount,
                                            u32 indexCount,
                                            const void* vertices,
                                            const void* indices)
{
  vulkan_upload_buffer_data(renderer->vk_dev, &renderer->storage_buffer_memory, 0, vertices, vertexCount);
  vulkan_upload_buffer_data(renderer->vk_dev, &renderer->storage_buffer_memory, renderer->max_vertex_buffer_size, indices, indexCount);
}

void
multi_mesh_renderer_update_indirect_buffers(t_multi_mesh_renderer* renderer,
                                            u64 currentImage,
                                            b8* visibility) // NULL
{
  VkDrawIndirectCommand* data = NULL;
	vkMapMemory(renderer->vk_dev->device, renderer->indirect_buffers_memory[currentImage], 0, 2 * sizeof(VkDrawIndirectCommand), 0, (void **)&data);

	for (u32 i = 0 ; i < renderer->max_shapes ; i++)
	{
		const u32 j = renderer->shapes[i].mesh_index;
		const u32 lod = renderer->shapes[i].lod;
		data[i] = (VkDrawIndirectCommand)
    {
			.vertexCount = get_lod_indices_count(&renderer->mesh_data.meshes[j], lod),
			.instanceCount = visibility ? (visibility[i] ? 1u : 0u) : 1u,
			.firstVertex = 0,
			.firstInstance = i
		};
	}

	vkUnmapMemory(renderer->vk_dev->device, renderer->indirect_buffers_memory[currentImage]);
}

void
multi_mesh_renderer_update_material_buffers(t_multi_mesh_renderer* renderer,
                                            u32 materialSize,
                                            const void* materialData)
{

}

void
multi_mesh_renderer_update_uniform_buffers(t_multi_mesh_renderer* renderer,
                                           u64 currentImage,
                                           const mat4* m)
{
  vulkan_upload_buffer_data(renderer->vk_dev, &renderer->base->uniform_buffers_memory[currentImage], 0, m, sizeof(mat4));
}

void
multi_mesh_renderer_update_draw_data_buffers(t_multi_mesh_renderer* renderer,
                                             u64 currentImage,
                                             u32 draw_data_size,
                                             const void* draw_data)
{
  vulkan_upload_buffer_data(renderer->vk_dev, &renderer->draw_data_buffers_memory[currentImage], 0, draw_data, draw_data_size);
}

void
multi_mesh_renderer_update_count_buffers(t_multi_mesh_renderer* renderer,
                                         u64 currentImage,
                                         u32 item_count)
{
  vulkan_upload_buffer_data(renderer->vk_dev, &renderer->count_buffers_memory[currentImage], 0, &item_count, sizeof(u32));
}

t_multi_mesh_renderer
multi_mesh_renderer_create(vulkan_render_device* vkDev,
		                       const char* meshFile,
		                       const char* drawDataFile,
		                       const char* materialFile,
		                       const char* vtxShaderFile,
		                       const char* fragShaderFile)
{
  t_multi_mesh_renderer renderer = { 0 };
  renderer.base = base_renderer_create(vkDev, (vulkan_image) { 0, 0, 0 });
  renderer.base->fill_command_buffer = &fill_command_buffer;
  renderer.vk_dev = vkDev;

  RenderPassCreateInfo render_pass_info = 
  {
    .clearColor_ = false,
    .clearDepth_ = false,
    .flags_ = 0 
  };

  if (!vulkan_create_color_and_depth_render_pass(vkDev,
                                                 false,
                                                 &renderer.base->render_pass,
                                                 &render_pass_info,
                                                 VK_FORMAT_B8G8R8A8_UNORM))
  {
    printf("Failed to create render pass\n");
		exit(EXIT_FAILURE);
  }

  vulkan_create_depth_resources(vkDev,
                                vkDev->framebuffer_width,
                                vkDev->framebuffer_height,
                                &renderer.base->depth_texture);

  load_draw_data(&renderer, drawDataFile);

  t_mesh_file_header header = load_mesh_data(meshFile, &renderer.mesh_data);

  const u32 indirectDataSize = renderer.max_shapes * sizeof(VkDrawIndirectCommand);
	renderer.max_draw_data_size = renderer.max_shapes * sizeof(t_draw_data);
	renderer.max_material_size = 1024;

  renderer.count_buffers = cvec_ncreate(VkBuffer, cvec_size(vkDev->swapchain_images));
  renderer.count_buffers_memory = cvec_ncreate(VkDeviceMemory, cvec_size(vkDev->swapchain_images));

  renderer.draw_data_buffers = cvec_ncreate(VkBuffer, cvec_size(vkDev->swapchain_images));
  renderer.draw_data_buffers_memory = cvec_ncreate(VkDeviceMemory, cvec_size(vkDev->swapchain_images));

  renderer.indirect_buffers = cvec_ncreate(VkBuffer, cvec_size(vkDev->swapchain_images));
  renderer.indirect_buffers_memory = cvec_ncreate(VkDeviceMemory, cvec_size(vkDev->swapchain_images));

  if (!vulkan_create_buffer(vkDev->device,
											      vkDev->physical_device,
											      renderer.max_material_size,
											      VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
											      VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
											      VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
											      &renderer.material_buffer,
											      &renderer.material_buffer_memory))
  {
    printf("Cannot create material buffer\n");
		fflush(stdout);
		exit(EXIT_FAILURE);
  }

  renderer.max_vertex_buffer_size = header.vertex_data_size;
  renderer.max_index_buffer_size = header.index_data_size;

  VkPhysicalDeviceProperties devProps;
    vkGetPhysicalDeviceProperties(vkDev->physical_device, &devProps);
  const u32 offsetAlignment = (u32)(devProps.limits.minStorageBufferOffsetAlignment);
	if ((renderer.max_vertex_buffer_size & (offsetAlignment - 1)) != 0)
	{
		i32 floats = (offsetAlignment - (renderer.max_vertex_buffer_size & (offsetAlignment - 1))) / sizeof(f32);
		for (i32 ii = 0; ii < floats; ii++)
			cvec_push(renderer.mesh_data.vertex_data, 0);// renderer.mesh_data.vertex_data meshData_.vertexData_.push_back(0);
		renderer.max_vertex_buffer_size = (renderer.max_vertex_buffer_size + offsetAlignment) & ~(offsetAlignment - 1);
	}

	if (!vulkan_create_buffer(vkDev->device,
                            vkDev->physical_device,
                            renderer.max_vertex_buffer_size + renderer.max_index_buffer_size,
		                        VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		                        VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		                        &renderer.storage_buffer,
                            &renderer.storage_buffer_memory))
	{
		printf("Cannot create vertex/index buffer\n"); fflush(stdout);
		exit(EXIT_FAILURE);
	}

  multi_mesh_renderer_update_geometry_buffers(&renderer,
                                              header.vertex_data_size,
                                              header.index_data_size,
                                              renderer.mesh_data.vertex_data,
                                              renderer.mesh_data.index_data);

  for (u64 i = 0; i < cvec_size(vkDev->swapchain_images); i++)
	{
    if (!vulkan_create_buffer(vkDev->device,
                              vkDev->physical_device,
                              indirectDataSize,
		                          VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
		                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		                          &renderer.indirect_buffers[i],
                              &renderer.indirect_buffers_memory[i]))
	  {
	  	printf("Cannot create indirect buffer\n"); fflush(stdout);
			exit(EXIT_FAILURE);
	  }

    multi_mesh_renderer_update_indirect_buffers(&renderer, i, NULL);

    if (!vulkan_create_buffer(vkDev->device,
                              vkDev->physical_device,
                              renderer.max_draw_data_size,
		                          VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
		                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		                          &renderer.draw_data_buffers[i],
                              &renderer.draw_data_buffers_memory[i]))
	  {
	  	printf("Cannot create draw data buffer\n"); fflush(stdout);
			exit(EXIT_FAILURE);
	  }

    multi_mesh_renderer_update_draw_data_buffers(&renderer, i, renderer.max_draw_data_size, renderer.shapes);

    if (!vulkan_create_buffer(vkDev->device,
                              vkDev->physical_device,
                              sizeof(u32),
		                          VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
		                          VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		                          &renderer.count_buffers[i],
                              &renderer.count_buffers_memory[i]))
	  {
	  	printf("Cannot create count buffer\n");
			fflush(stdout);
			exit(EXIT_FAILURE);
	  }

    multi_mesh_renderer_update_count_buffers(&renderer, i, renderer.max_shapes);
	}

  const char* shaders[] =
  {
    vtxShaderFile,
    fragShaderFile
  };

  if (!base_renderer_create_uniform_buffers(renderer.base,
                                            vkDev,
                                            sizeof(mat4)) ||
      !vulkan_create_color_and_depth_framebuffers(vkDev,
																						      renderer.base->render_pass,
																						      VK_NULL_HANDLE,
																						      &renderer.base->swapchain_framebuffers) ||
      !vulkan_create_descriptor_pool(vkDev,
                                     1,
                                     4,
                                     0,
                                     &renderer.base->descriptor_pool) ||
      !create_descriptor_set(&renderer) ||
      !vulkan_create_pipeline_layout(vkDev->device,
                                     renderer.base->descriptor_set_layout,
                                     &renderer.base->pipeline_layout) ||
      !vulkan_create_graphics_pipeline(vkDev,
                                       renderer.base->render_pass,
                                       renderer.base->pipeline_layout,
                                       shaders,
                                       ARRAY_SIZE(shaders),
                                       &renderer.base->graphics_pipeline,
                                       VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST, true, true, false, -1, -1, 0))
  {
    printf("Failed to create pipeline\n"); fflush(stdout);
		exit(EXIT_FAILURE);
  }

  return renderer;
}

void
multi_mesh_renderer_destroy(t_multi_mesh_renderer* renderer)
{
  VkDevice device = renderer->vk_dev->device;

	vkDestroyBuffer(device, renderer->storage_buffer, NULL);
	vkFreeMemory(device, renderer->storage_buffer_memory, NULL);

	vkDestroyBuffer(device, renderer->material_buffer, NULL);
	vkFreeMemory(device, renderer->material_buffer_memory, NULL);

	for (u64 i = 0; i < cvec_size(renderer->base->swapchain_framebuffers); i++)
	{
		vkDestroyBuffer(device, renderer->indirect_buffers[i], NULL);
		vkFreeMemory(device, renderer->indirect_buffers_memory[i], NULL);

		vkDestroyBuffer(device, renderer->draw_data_buffers[i], NULL);
		vkFreeMemory(device, renderer->draw_data_buffers_memory[i], NULL);

		vkDestroyBuffer(device, renderer->count_buffers[i], NULL);
		vkFreeMemory(device, renderer->count_buffers_memory[i], NULL);
	}

	vulkan_destroy_image(device, &renderer->base->depth_texture);

  base_renderer_destory(renderer->base);

  memset(renderer, 0, sizeof(t_multi_mesh_renderer));
}


#endif