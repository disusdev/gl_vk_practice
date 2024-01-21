#ifndef __VK_CANVAS_RENDERER_H__
#define __VK_CANVAS_RENDERER_H__

#include <defines.h>

typedef struct
canvas_vertex_data
{
	vec3 position;
	vec4 color;
}
canvas_vertex_data;

typedef struct
canvas_uniform_buffer
{
	mat4 mvp;
	f32 time;
}
canvas_uniform_buffer;

const u32 MAX_LINES_COUNT = 65536;
const u32 MAX_LINES_DATA_SIZE = MAX_LINES_COUNT * sizeof(canvas_vertex_data) * 2;

typedef struct
t_canvas_renderer
{
  t_base_renderer* base;
  cvec(canvas_vertex_data) lines;
  cvec(VkBuffer) storage_buffer;
  cvec(VkDeviceMemory) storage_buffer_memory;
}
t_canvas_renderer;

b8
canvas_renderer_create_descriptor_set(t_canvas_renderer* canvas_renderer,
                                      vulkan_render_device* vk_device)
{
  const VkDescriptorSetLayoutBinding bindings[] =
  {
		descriptorSetLayoutBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1),
		descriptorSetLayoutBinding(1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_SHADER_STAGE_VERTEX_BIT, 1)
	};

	const VkDescriptorSetLayoutCreateInfo layoutInfo =
  {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.pNext = NULL,
		.flags = 0,
		.bindingCount = (u32)ARRAY_SIZE(bindings),
		.pBindings = bindings
	};

	VK_CHECK(vkCreateDescriptorSetLayout(vk_device->device, &layoutInfo, NULL, &canvas_renderer->base->descriptor_set_layout));

	cvec(VkDescriptorSetLayout) layouts = cvec_ncreate_set(VkDescriptorSetLayout, cvec_size(vk_device->swapchain_images), &canvas_renderer->base->descriptor_set_layout);

	const VkDescriptorSetAllocateInfo allocInfo =
  {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		.pNext = NULL,
		.descriptorPool = canvas_renderer->base->descriptor_pool,
		.descriptorSetCount = (u32)cvec_size(vk_device->swapchain_images),
		.pSetLayouts = layouts
	};

  canvas_renderer->base->descriptor_sets = cvec_ncreate(VkDescriptorSet, cvec_size(vk_device->swapchain_images));

	VK_CHECK(vkAllocateDescriptorSets(vk_device->device, &allocInfo, canvas_renderer->base->descriptor_sets));

	for (size_t i = 0; i < cvec_size(vk_device->swapchain_images); i++)
	{
		VkDescriptorSet ds = canvas_renderer->base->descriptor_sets[i];

		const VkDescriptorBufferInfo bufferInfo  = { canvas_renderer->base->uniform_buffers[i], 0, sizeof(canvas_uniform_buffer) };
		const VkDescriptorBufferInfo bufferInfo2 = { canvas_renderer->storage_buffer[i], 0, MAX_LINES_DATA_SIZE };

		const VkWriteDescriptorSet descriptorWrites[] =
    {
			bufferWriteDescriptorSet(ds, &bufferInfo,	0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER),
			bufferWriteDescriptorSet(ds, &bufferInfo2, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER)
		};

		vkUpdateDescriptorSets(vk_device->device, (u32)ARRAY_SIZE(descriptorWrites), descriptorWrites, 0, NULL);
	}

	return true;
}

void
canvas_renderer_fill_command_buffer(t_canvas_renderer* canvas_renderer,
                                    VkCommandBuffer commandBuffer,
                                    u64 currentImage)
{
  if (cvec_empty(canvas_renderer->lines))
  {
		return;
  }

  base_renderer_begin_render_pass(canvas_renderer->base, commandBuffer, currentImage);

	vkCmdDraw( commandBuffer, (u32)(cvec_size(canvas_renderer->lines)), 1, 0, 0 );
	vkCmdEndRenderPass( commandBuffer );
}

void
canvas_renderer_update_buffer(t_canvas_renderer* canvas_renderer,
                              vulkan_render_device* vk_device,
                              u64 currentImage)
{
	if (cvec_empty(canvas_renderer->lines))
  {
		return;
  }

	const VkDeviceSize bufferSize = cvec_size(canvas_renderer->lines) * sizeof(canvas_vertex_data);

  vulkan_upload_buffer_data(vk_device, &canvas_renderer->storage_buffer_memory[currentImage], 0, canvas_renderer->lines, bufferSize);
}

void
canvas_renderer_update_uniform_buffer(t_canvas_renderer* canvas_renderer,
                                      vulkan_render_device* vk_device,
                                      mat4* modelViewProj,
                                      f32 time,
                                      u32 currentImage)
{
  const canvas_uniform_buffer ubo =
  {
		.mvp = *modelViewProj,
		.time = time
	};

  vulkan_upload_buffer_data(vk_device, &canvas_renderer->base->uniform_buffers_memory[currentImage], 0, &ubo, sizeof(ubo));
}

t_canvas_renderer
canvas_renderer_create(vulkan_render_device* vk_device,
                       vulkan_image depth_texture)
{
  t_canvas_renderer canvas_renderer = { 0 };
  canvas_renderer.base = base_renderer_create(vk_device, depth_texture);
  canvas_renderer.base->fill_command_buffer = &canvas_renderer_fill_command_buffer;

  t_base_renderer* base = canvas_renderer.base;

  canvas_renderer.lines = cvec_ncreate(canvas_vertex_data, MAX_LINES_COUNT);

  const u64 imgCount = cvec_size(vk_device->swapchain_images);
  canvas_renderer.storage_buffer = cvec_ncreate(VkBuffer, imgCount);
  canvas_renderer.storage_buffer_memory = cvec_ncreate(VkDeviceMemory, imgCount);

  for(u64 i = 0; i < imgCount; i++)
	{
    if (!vulkan_create_buffer(vk_device->device,
                              vk_device->physical_device,
                              MAX_LINES_DATA_SIZE,
                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              &canvas_renderer.storage_buffer[i],
                              &canvas_renderer.storage_buffer_memory[i]))
    {
      printf("VaulkanCanvas: createBuffer() failed\n");
			exit(EXIT_FAILURE);
    }
	}

  RenderPassCreateInfo render_pass_info =
  {
    .clearColor_ = false,
    .clearDepth_ = false,
    .flags_ = 0
  };

  const char* shaders[] =
  {
    "data/shaders/chapter04/Lines.vert",
    "data/shaders/chapter04/Lines.frag"
  };

  if (!vulkan_create_color_and_depth_render_pass(vk_device,
                                                 (depth_texture.handle != VK_NULL_HANDLE),
                                                 &base->render_pass,
                                                 &render_pass_info,
                                                 VK_FORMAT_B8G8R8A8_UNORM) ||
      !base_renderer_create_uniform_buffers(base,
                                            vk_device,
                                            sizeof(canvas_uniform_buffer)) ||
      !vulkan_create_color_and_depth_framebuffers(vk_device,
																						      base->render_pass,
																						      base->depth_texture.view,
																						      &base->swapchain_framebuffers) ||
      !vulkan_create_descriptor_pool(vk_device, 1, 1, 0, &base->descriptor_pool) ||
      !canvas_renderer_create_descriptor_set(&canvas_renderer, vk_device) ||
      !vulkan_create_pipeline_layout(vk_device->device, base->descriptor_set_layout, &base->pipeline_layout) ||
      !vulkan_create_graphics_pipeline(vk_device, base->render_pass,
                                       base->pipeline_layout,
                                       shaders,
                                       ARRAY_SIZE(shaders),
                                       &base->graphics_pipeline,
                                       VK_PRIMITIVE_TOPOLOGY_LINE_LIST,
                                       (depth_texture.handle != VK_NULL_HANDLE),
                                       true, false, -1, -1, 0))
	{                                                                                               
		printf("ModelRenderer: failed to create pipeline\n");
		exit(EXIT_FAILURE);
	}

  return canvas_renderer;
}

void
canvas_renderer_clear(t_canvas_renderer* canvas_renderer)
{
  cvec_clear(canvas_renderer->lines);
}

void
canvas_renderer_line(t_canvas_renderer* canvas_renderer,
                     vec3 p1,
                     vec3 p2,
                     vec4* c)
{
  canvas_vertex_data vd1 = {.position = p1, .color = *c};
  cvec_push(canvas_renderer->lines, vd1);
  canvas_vertex_data vd2 = { .position = p2, .color = *c };
  cvec_push(canvas_renderer->lines, vd2);
}

void
canvas_renderer_plane3d(t_canvas_renderer* canvas_renderer,
                        vec3* o,
                        vec3* v1,
                        vec3* v2,
                        int n1,
                        int n2,
                        f32 s1,
                        f32 s2,
                        vec4* color,
                        vec4* outline_color)
{
  canvas_renderer_line(canvas_renderer, vec3_sub( vec3_sub(*o, vec3_mul_scalar(*v1, s1 * 0.5f)), vec3_mul_scalar(*v2, s2 * 0.5f)),
                       vec3_add( vec3_sub(*o, vec3_mul_scalar(*v1, s1 * 0.5f)), vec3_mul_scalar(*v2, s2 * 0.5f)), outline_color);
  canvas_renderer_line(canvas_renderer, vec3_sub( vec3_add(*o, vec3_mul_scalar(*v1, s1 * 0.5f)), vec3_mul_scalar(*v2, s2 * 0.5f)),
                       vec3_add( vec3_add(*o, vec3_mul_scalar(*v1, s1 * 0.5f)), vec3_mul_scalar(*v2, s2 * 0.5f)), outline_color);

  canvas_renderer_line(canvas_renderer, vec3_add( vec3_sub(*o, vec3_mul_scalar(*v1, s1 * 0.5f)), vec3_mul_scalar(*v2, s2 * 0.5f)),
                       vec3_add( vec3_add(*o, vec3_mul_scalar(*v1, s1 * 0.5f)), vec3_mul_scalar(*v2, s2 * 0.5f)), outline_color);

  canvas_renderer_line(canvas_renderer, vec3_sub( vec3_sub(*o, vec3_mul_scalar(*v1, s1 * 0.5f)), vec3_mul_scalar(*v2, s2 * 0.5f)),
                       vec3_sub( vec3_add(*o, vec3_mul_scalar(*v1, s1 * 0.5f)), vec3_mul_scalar(*v2, s2 * 0.5f)), outline_color);

	for (int i = 1; i < n1; i++)
	{
		float t = ((float)i - (float)n1 / 2.0f) * s1 / (float)n1;
		const vec3 o1 = vec3_add(*o, vec3_mul_scalar(*v1, t));

    canvas_renderer_line(canvas_renderer, vec3_sub(o1, vec3_mul_scalar(*v2, s2 * 0.5f)),
                         vec3_add(o1, vec3_mul_scalar(*v2, s2 * 0.5f)),
                         color);
	}

	for (int i = 1; i < n2; i++)
	{
		const float t = ((float)i - (float)n2 / 2.0f) * s2 / (float)n2;
    const vec3 o2 = vec3_add(*o, vec3_mul_scalar(*v2, t));

    canvas_renderer_line(canvas_renderer, vec3_sub(o2, vec3_mul_scalar(*v1, s1 * 0.5f)),
                         vec3_add(o2, vec3_mul_scalar(*v1, s1 * 0.5f)),
                         color);
	}
}

void
canvas_renderer_destroy(t_canvas_renderer* canvas_renderer)
{
  for (u64 i = 0; i < cvec_size(canvas_renderer->storage_buffer); i++)
  {
    vkDestroyBuffer(canvas_renderer->base->device, canvas_renderer->storage_buffer[i], NULL);
  }

  for (u64 i = 0; i < cvec_size(canvas_renderer->storage_buffer_memory); i++)
  {
    vkFreeMemory(canvas_renderer->base->device, canvas_renderer->storage_buffer_memory[i], NULL);
  }

  base_renderer_destory(canvas_renderer->base);
  canvas_renderer->base = NULL;
}

#endif