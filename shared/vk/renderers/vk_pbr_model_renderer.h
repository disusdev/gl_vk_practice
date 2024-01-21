#ifndef __VK_PBR_MODEL_RENDERER_H__
#define __VK_PBR_MODEL_RENDERER_H__

#include <defines.h>

#include "vk_base_renderer.h"

typedef struct
t_pbr_model_renderer
{
  t_base_renderer* base;
	vulkan_render_device* vk_dev;

  u64 vertex_buffer_size;
	u64 index_buffer_size;

	// 6. Storage Buffer with index and vertex data
	VkBuffer storage_buffer;
	VkDeviceMemory storage_buffer_memory;

	vulkan_texture tex_ao;
	vulkan_texture tex_emissive;
	vulkan_texture tex_albedo;
	vulkan_texture tex_me_r;
	vulkan_texture tex_normal;

	vulkan_texture env_map_irradiance;
	vulkan_texture env_map;

	vulkan_texture brdf_lut;
}
t_pbr_model_renderer;

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
                          vulkan_image depth_texture);

void
pbr_model_renderer_destroy(t_pbr_model_renderer* renderer);

#endif