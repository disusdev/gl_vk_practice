#ifndef __VK_RESOURCES_H__
#define __VK_RESOURCES_H__

#include <defines.h>

typedef struct
t_vk_resources
{
  vulkan_render_device* vk_dev;

  cvec(vulkan_texture) all_textures;
  cvec(vulkan_buffer) all_buffers;

  cvec(VkFramebuffer) all_framebuffers;
  cvec(VkRenderPass) all_render_pass;

  cvec(VkPipelineLayout) all_pipeline_layout;
  cvec(VkPipeline) all_pipelines;

  cvec(VkDescriptorSetLayout) all_ds_layouts;
  cvec(VkDescriptorPool) all_d_pools;

  cvec(shader_module) shader_modules;
  // chtb()? cmap(int) shader_map;
}
t_vk_resources;

#endif