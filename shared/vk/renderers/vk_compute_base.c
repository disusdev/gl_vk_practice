#include "vk_compute_base.h"

static b8
create_descriptor_set(t_compute_base* base,
                      VkDevice device,
                      VkDescriptorSetLayout desc_set_layout)
{
  // Descriptor pool
	VkDescriptorPoolSize descriptorPoolSize =
  {
    VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 2
  };

	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo =
  {
		VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO, 0, 0, 1, 1, &descriptorPoolSize
	};

	VK_CHECK(vkCreateDescriptorPool(device, &descriptorPoolCreateInfo, 0, &base->descriptor_pool));

	// Descriptor set
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo =
  {
		VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
		0, base->descriptor_pool, 1, &desc_set_layout
	};

	VK_CHECK(vkAllocateDescriptorSets(device, &descriptorSetAllocateInfo, &base->descriptor_set));

	// Finally, update descriptor set with concrete buffer pointers
	VkDescriptorBufferInfo inBufferInfo =
  {
    base->in_buffer, 0, VK_WHOLE_SIZE
  };

	VkDescriptorBufferInfo outBufferInfo =
  {
    base->out_buffer, 0, VK_WHOLE_SIZE
  };

	VkWriteDescriptorSet writeDescriptorSet[] =
  {
		{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, 0, base->descriptor_set, 0, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0,  &inBufferInfo, 0 },
		{ VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET, 0, base->descriptor_set, 1, 0, 1, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 0, &outBufferInfo, 0 }
	};

	vkUpdateDescriptorSets(device, (u32)ARRAY_SIZE(writeDescriptorSet), writeDescriptorSet, 0, 0);

	return true;
}

t_compute_base
compute_base_create(vulkan_render_device* vk_dev,
                    const char* shader_name,
                    u32 in_size,
                    u32 out_size)
{
  t_compute_base base = {0};
  base.vk_dev = vk_dev;
  base.create_descriptor_set = &create_descriptor_set;

  vulkan_create_shared_buffer(vk_dev,
                              in_size,
                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              &base.in_buffer,
                              &base.in_buffer_memory);

  vulkan_create_shared_buffer(vk_dev,
                              out_size,
                              VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
                              VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                              VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                              &base.out_buffer,
                              &base.out_buffer_memory);

  ShaderModule shader = { 0 };
  VK_CHECK(createShaderModule(vk_dev->device, &shader, shader_name));

  vulkan_create_compute_descriptor_set_layout(vk_dev->device, &base.ds_layout);
  vulkan_create_pipeline_layout(vk_dev->device,
                                base.ds_layout,
                                &base.pipeline_layout);
  vulkan_create_compute_pipeline(vk_dev->device, shader.shaderModule, base.pipeline_layout, &base.pipeline);
  create_descriptor_set(&base, vk_dev->device, base.ds_layout);

	vkDestroyShaderModule(vk_dev->device, shader.shaderModule, NULL);

  return base;
}

void
compute_base_destroy(t_compute_base* base)
{
  vkDestroyBuffer(base->vk_dev->device, base->in_buffer, NULL);
	vkFreeMemory(base->vk_dev->device, base->in_buffer_memory, NULL);

	vkDestroyBuffer(base->vk_dev->device, base->out_buffer, NULL);
	vkFreeMemory(base->vk_dev->device, base->out_buffer_memory, NULL);

	vkDestroyPipelineLayout(base->vk_dev->device, base->pipeline_layout, NULL);
	vkDestroyPipeline(base->vk_dev->device, base->pipeline, NULL);

	vkDestroyDescriptorSetLayout(base->vk_dev->device, base->ds_layout, NULL);
	vkDestroyDescriptorPool(base->vk_dev->device, base->descriptor_pool, NULL);
}