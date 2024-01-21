#ifndef __VK_MULTI_RENDERER_H__
#define __VK_MULTI_RENDERER_H__

#include <defines.h>

#include "../vk_scene_data.h"

typedef struct
t_vk_multi_renderer
{
  t_vk_renderer* base;

  t_vk_scene_data* scene_data;

  cvec(vulkan_buffer) indirect;
  cvec(vulkan_buffer) shape;
}
t_vk_multi_renderer;

t_vk_multi_renderer
multi_renderer_create();



#endif