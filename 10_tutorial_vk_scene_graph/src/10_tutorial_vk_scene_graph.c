#include <defines.h>

#include <vk/vk_app.h>

#include <vk/renderers/vk_multi_renderer.h>

typedef struct
t_app
{
  t_camera_app* camera_app;

  vulkan_texture env_map;
  vulkan_texture irr_map;

  t_vk_scene_data scene_data;

  // InfinitePlaneRenderer plane;
	// MultiRenderer multiRenderer;
	// GuiRenderer imgui;

  // int selectedNode = -1;
}
t_app;

t_app
app_create()
{
  t_app app = { 0 };
  app.camera_app = camera_app_create(800, 600);

  // init components

  //

  return app;
}

void
app_draw_ui()
{

}

void
app_draw_3D()
{
  
}

void
app_run(t_app* app)
{

}


int main()
{
  t_app app = app_create();
  app_run(&app);
  return 0;
}