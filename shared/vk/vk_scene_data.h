#ifndef __VK_SCENE_DATA_H__
#define __VK_SCENE_DATA_H__

#include <defines.h>

#include <scene/vtx_data.h>

typedef struct
t_vk_scene_data
{
  vulkan_texture env_map_irr;
  vulkan_texture env_map;
  vulkan_texture brdf_lut;

  vulkan_buffer material;
  vulkan_buffer transforms;

  // vulkan_render_context* ctx;

  // vulkan_texture_array_attachment all_material_textures;

  // vulkan_buffer_attachment index_buffer;
  // vulkan_buffer_attachment vertex_buffer;

  t_mesh_data mesh_data;

  // t_scene scene;
  // cvec(t_material_description) materials;

  cvec(mat4) shape_transforms;

  cvec(t_draw_data) shapes;

  cvec(const char*) texture_files;
  cvec(t_loaded_image_data) loaded_files;
  // mutex loaded_files_mutex

}
t_vk_scene_data;

#endif