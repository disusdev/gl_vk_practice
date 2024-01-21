#ifndef __VK_COMPUTE_BASE_H__
#define __VK_COMPUTE_BASE_H__

#include <defines.h>

typedef struct
t_compute_base
{
  vulkan_render_device* vk_dev;

  VkBuffer in_buffer;
	VkBuffer out_buffer;
	VkDeviceMemory in_buffer_memory;
	VkDeviceMemory out_buffer_memory;

	VkDescriptorSetLayout ds_layout;
	VkPipelineLayout pipeline_layout;
	VkPipeline pipeline;

	VkDescriptorPool descriptor_pool;
	VkDescriptorSet descriptor_set;

  b8(*create_descriptor_set)(struct t_compute_base*, VkDevice, VkDescriptorSetLayout);
}
t_compute_base;

t_compute_base
compute_base_create(vulkan_render_device* vk_dev,
                    const char* shader_name,
                    u32 in_size,
                    u32 out_size);

void
compute_base_destroy(t_compute_base* base);

void
compute_base_upload_input(t_compute_base* base,
                          u32 offset,
                          void* in_data,
                          u32 byte_count)
{
  vulkan_upload_buffer_data(base->vk_dev,
                            &base->in_buffer_memory,
                            offset,
                            in_data,
                            byte_count);
}

void
compute_base_download_output(t_compute_base* base,
                             u32 offset,
                             void* out_data,
                             u32 byte_count)
{
  vulkan_download_buffer_data(base->vk_dev,
                              &base->out_buffer_memory,
                              offset,
                              out_data,
                              byte_count);
}

b8
compute_base_execute(t_compute_base* base,
                     u32 xsize,
                     u32 ysize,
                     u32 zsize)
{
  return vulkan_execute_compute_shader(base->vk_dev,
                                       base->pipeline,
                                       base->pipeline_layout,
                                       base->descriptor_set,
                                       xsize,
                                       ysize,
                                       zsize);
}


#endif