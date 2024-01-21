#include <defines.h>

#include <stdlib.h>
#include <stdio.h>
#include <float.h>

#include <math/mathm.c>

#define CVEC_IMPLEMENTATION
#define CVEC_STDLIB
#include <containers/cvec.h>

#define CQUE_IMPLEMENTATION
#define CQUE_STDLIB
#include <containers/cque.h>

#define CGLTF_IMPLEMENTATION
#include <cgltf.h>

#define TINYOBJ_MALLOC malloc
#define TINYOBJ_CALLOC calloc
#define TINYOBJ_REALLOC realloc
#define TINYOBJ_FREE free

#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include <tinyobj_loader_c.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define CBITMAP_STDLIB
#define CBITMAP_IMPLEMENTATION
#include <resource/cbitmap.h>

#include <perf/fps_counter.h>

#include <tools.h>
#include <resource/model_loader.h>
#include <vk/vk_shader_compiler.h>
#include <vk/vk_core.h>

#include <scene/vtx_data.h>

#include <vk/vk_app.h>

#include <vk/renderers/vk_base_renderer.h>

#include <vk/renderers/vk_model_renderer.h>
#include <vk/renderers/vk_clear_renderer.h>
#include <vk/renderers/vk_cubemap_renderer.h>
#include <vk/renderers/vk_final_renderer.h>
#include <vk/renderers/vk_canvas_renderer.h>
#include <vk/renderers/vk_multi_mesh_renderer.h>

#include <perf/fps_graph.h>

typedef struct
t_button_state
{
	u8 w : 1;
	u8 s : 1;
	u8 a : 1;
	u8 d : 1;
	u8 q : 1;
	u8 e : 1;
	u8 space : 1;
	u8 shift : 1;
	u8 mlb;
	u16 __padding;
}
t_button_state;

const u32 g_screen_width = 1280;
const u32 g_screen_height = 720;

t_vk_app vk_app;
vec2 relative_point;

void set_mouse_pos(f32 x, f32 y)
{
	glfwSetCursorPos(vk_app.window, x, y);
}

void set_mouse_pos_relative(f32 x, f32 y)
{
	glfwSetCursorPos(vk_app.window, relative_point.x + x, relative_point.y + y);
}

#include <core/input.h>
#include <camera.h>

t_button_state g_button_state = { 0 };
t_input_state g_input = { 0 };

t_camera g_camera;

t_fps_counter g_fps_counter;
t_fps_graph g_fps_graph;

typedef struct
t_uniform_buffer
{
  mat4 mvp;
}
t_uniform_buffer;

u64 vertex_buffer_size;
u64 index_buffer_size;

vulkan_instance vk;
vulkan_render_device vk_dev;

// _model_renderer model_renderer;
t_multi_mesh_renderer multi_mesh_renderer;
t_clear_renderer clear_renderer;
// t_cubemap_renderer cube_renderer;
t_final_renderer final_renderer;
t_canvas_renderer canvas2d_renderer;
// t_canvas_renderer canvas_renderer;

typedef struct
t_vulkan_state
{
  // 1.
  VkDescriptorSetLayout descriptor_set_layout;
  VkDescriptorPool descriptor_pool;
  cvec(VkDescriptorSet) descriptor_sets;

  // 2.
  cvec(VkFramebuffer) swapchain_framebuffers;

  // 3.
  VkRenderPass render_pass;
  VkPipelineLayout pipeline_layout;
  VkPipeline graphics_pipeline;

  // 4.
  cvec(VkBuffer) uniform_buffers;
  cvec(VkDeviceMemory) uniform_buffers_memory;

  // 5.
  VkBuffer storage_buffer;
  VkDeviceMemory storage_buffer_memory;

  // 6.
  vulkan_image depth_texture;

  VkSampler texture_sampler;
  vulkan_image texture;
}
t_vulkan_state;

void
gltf_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	b8 pressed = action != GLFW_RELEASE;
	if (key == GLFW_KEY_ESCAPE && pressed)
	{
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
	if (key == GLFW_KEY_W)
	{
		g_button_state.w = pressed;
	}
	if (key == GLFW_KEY_S)
	{
		g_button_state.s = pressed;
	}
	if (key == GLFW_KEY_A)
	{
		g_button_state.a = pressed;
	}
	if (key == GLFW_KEY_D)
	{
		g_button_state.d = pressed;
	}
	if (key == GLFW_KEY_Q)
	{
		g_button_state.q = pressed;
	}
	if (key == GLFW_KEY_E)
	{
		g_button_state.e = pressed;
	}
	if (key == GLFW_KEY_LEFT_SHIFT)
	{
		g_button_state.shift = pressed;
	}
	if (key == GLFW_KEY_SPACE)
	{
		g_button_state.space = pressed;
	}
}

void
gltf_set_mouse_button_callback(GLFWwindow* window, i32 button, i32 action, i32 mods)
{
	if (button == GLFW_MOUSE_BUTTON_LEFT)
	{
		g_button_state.mlb = action != GLFW_RELEASE;
	}
}

b8 isDeviceSuitable(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties deviceProperties;
	vkGetPhysicalDeviceProperties(device, &deviceProperties);

	VkPhysicalDeviceFeatures deviceFeatures;
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	b8 isDiscreteGPU = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU;
	b8 isIntegratedGPU = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU;
	b8 isGPU = isDiscreteGPU || isIntegratedGPU;

	return isGPU && deviceFeatures.geometryShader;
}

b8 vulkan_init()
{
  vulkan_create_instance(&vk.instance);

	if (!vulkan_setup_debug_callbacks(vk.instance, &vk.messenger, &vk.report_callback))
		exit(EXIT_FAILURE);

	if (glfwCreateWindowSurface(vk.instance, vk_app.window, NULL, &vk.surface))
		exit(EXIT_FAILURE);

  if (!vulkan_init_render_device(&vk,
																 &vk_dev,
																 g_screen_width,
																 g_screen_height,
																 isDeviceSuitable,
																 (VkPhysicalDeviceFeatures)
																 { .multiDrawIndirect = VK_TRUE,
																	 .drawIndirectFirstInstance = VK_TRUE
																	 /*.geometryShader = VK_TRUE*/ } ))
    exit(EXIT_FAILURE);
  
  multi_mesh_renderer = multi_mesh_renderer_create(&vk_dev,
                                                   "data/meshes/test.meshes",
                                                   "data/meshes/test.meshes.drawdata",
                                                   "",
                                                   "data/shaders/chapter05/VK01.vert",
                                                   "data/shaders/chapter05/VK01.frag");

	// model_renderer = model_renderer_create_default(&vk_dev,
	// 																							 "data/rubber_duck/scene.gltf",
	// 																							 "data/rubber_duck/textures/Duck_baseColor.png",
	// 																							 (u32)sizeof(mat4));

	vulkan_image depth_zero = (vulkan_image)
	{
		VK_NULL_HANDLE,
		VK_NULL_HANDLE,
		VK_NULL_HANDLE
	};

	clear_renderer = clear_renderer_create(&vk_dev,
																				 depth_zero);

	//cube_renderer = cubemap_renderer_create(&vk_dev,
	//																				depth_zero,
	//																				"data/SnowPano_4k_Ref.hdr");

	final_renderer = final_renderer_create(&vk_dev, depth_zero);


	canvas2d_renderer = canvas_renderer_create(&vk_dev, depth_zero);

	// canvas_renderer = canvas_renderer_create(&vk_dev, depth_zero);

	return VK_SUCCESS;
}

void
update3D(u32 imageIndex)
{
	int width, height;
	glfwGetFramebufferSize(vk_app.window, &width, &height);
	const float ratio = width / (float)height;

	float time = (float)glfwGetTime();
	vec3 pos = (vec3) { 0.0f, -0.5f, -4.0f - math_sin(time) };
	vec3 axis = (vec3) { 0.0f, 1.0f, 0.0f };
	quat q = quat_from_axis_angle(axis, -time, false);
	// mat4 m1 = mat4_mul(vk_dev.model->transforms[vk_dev.mesh_idx], mat4_mul(quat_to_mat4(q), mat4_translation(pos)));
	// mat4 m_cube = mat4_translation(g_camera.position);// mat4_mul(quat_to_mat4(q), mat4_translation(pos));
	mat4 m_cube = mat4_mul(mat4_scale((vec3) { 50.0f, 50.0f, 50.0f }), mat4_translation(g_camera.position));

	//mat4 p = mat4_persp(DEG2RAD * 90.0f, ratio, 0.1f, 1000.0f);
	mat4 p = camera_get_projection_matrix(&g_camera, ratio, false, true);

	mat4 view = camera_get_view_matrix(&g_camera);
	//mat4 mtx = mat4_mul(mat4_mul(m1, view), p);
	//mat4 mtx_cube = mat4_mul(mat4_mul(m_cube, view), p);

	mat4 ortho = mat4_ortho(0.0f, 1.0f, 1.0f, 0.0f, -100.0f, 100.0f);

	mat4 p_v = mat4_mul(view, p);

	{
		//cubemap_renderer_update_uniform_buffer(&cube_renderer, &vk_dev, imageIndex, &mtx_cube);
		canvas_renderer_update_uniform_buffer(&canvas2d_renderer, &vk_dev, &ortho, 0.0f, imageIndex);
		// canvas_renderer_update_uniform_buffer(&canvas_renderer, &vk_dev, &p_v, 0.0f, imageIndex);
    multi_mesh_renderer_update_uniform_buffers(&multi_mesh_renderer, imageIndex, &p_v);
		// model_renderer_update_uniform_buffer(&model_renderer, &vk_dev, imageIndex, (void*)&mtx, sizeof(mat4));
	}
}

void
update2D(u32 imageIndex)
{
	canvas_renderer_clear(&canvas2d_renderer);
	vec4 color = (vec4)
	{
		0.0f, 0.0f, 1.0f, 1.0f
	};
	fps_graph_render(&g_fps_graph, &canvas2d_renderer, &color);
	canvas_renderer_update_buffer(&canvas2d_renderer, &vk_dev, imageIndex);
}

void update_buffer(u32 image_index)
{
  update2D(image_index);
  update3D(image_index);
}

void update_frame(VkCommandBuffer cmd,
                  u32 image_index)
{
  clear_renderer.base->fill_command_buffer(&clear_renderer, cmd, image_index);
	//cube_renderer.base->fill_command_buffer(&cube_renderer, cmd, image_index);
	// canvas_renderer.base->fill_command_buffer(&canvas_renderer, cmd, image_index);
  multi_mesh_renderer.base->fill_command_buffer(&multi_mesh_renderer, cmd, image_index);
	// model_renderer.base->fill_command_buffer(&model_renderer, cmd, image_index);
	canvas2d_renderer.base->fill_command_buffer(&canvas2d_renderer, cmd, image_index);
	final_renderer.base->fill_command_buffer(&final_renderer, cmd, image_index);
}

void vulkan_term()
{
	final_renderer_destroy(&final_renderer);

	canvas_renderer_destroy(&canvas2d_renderer);

	// canvas_renderer_destroy(&canvas_renderer);

	//cubemap_renderer_destroy(&cube_renderer);

	clear_renderer_destroy(&clear_renderer);

  multi_mesh_renderer_destroy(&multi_mesh_renderer);

  vulkan_destroy_render_device(&vk_dev);

  vulkan_destroy_instance(&vk);
}

int main()
{
  vk_app = vk_app_create(g_screen_width, g_screen_height, NULL);

	glfwSetKeyCallback
	(
		vk_app.window,
		gltf_key_callback
	);

	glfwSetMouseButtonCallback
	(
		vk_app.window,
		gltf_set_mouse_button_callback
	);

	g_camera = camera_create();
	g_camera.position = (vec3){ 0.0f, 0.0f, 0.0f };
	g_camera.forward = vec3_forward();

	g_fps_counter = fps_counter_create(0.01666f);
	// g_fps_counter.print_fps = true;
	g_fps_graph = fps_graph_create(256);

  vulkan_init();

	{
		//vec3 o = (vec3){ 0.0f, -0.5f, -4.0f };
		//vec3 v1 = (vec3){ 1.0f, 0.0f, 0.0f };
		//vec3 v2 = (vec3){ 0.0f, 0.0f, 1.0f };
		//vec4 color = (vec4){ 1.0f, 0.0f, 0.0f, 1.0f };
		//vec4 outline = (vec4){ 0.0f, 1.0f, 0.0f, 1.0f };

		// canvas_renderer_plane3d(&canvas_renderer, &o, &v1, &v2, 40, 40, 10.0f, 10.0f, &color, &outline);

		//for (size_t i = 0; i < cvec_size(vk_dev.swapchain_images); i++)
		//{
		//	canvas_renderer_update_buffer(&canvas_renderer, &vk_dev, i);
		//}
	}

	f64 timeStamp = glfwGetTime();
	f32 deltaSeconds = 0.0f;

  while (!glfwWindowShouldClose(vk_app.window))
	{
		glfwPollEvents();

		f64 newTimeStamp = glfwGetTime();
		deltaSeconds = (f32)(newTimeStamp - timeStamp);
		timeStamp = newTimeStamp;

		f64 xpos, ypos;
		glfwGetCursorPos(vk_app.window, &xpos, &ypos);
		g_input.current_mouse_position.x = xpos;
		g_input.current_mouse_position.y = ypos;

		input_update(&g_input, *(i32*)&g_button_state);

		if (input_get_key_down(&g_input, KEY_CODE_MLB))
		{
			relative_point = g_input.current_mouse_position;
			g_input.center_position = g_input.current_mouse_position;
			glfwSetInputMode(vk_app.window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
		}
		else
		if (input_get_key(&g_input, KEY_CODE_MLB))
		{
			g_input.rotation_delta = vec2_sub(g_input.current_mouse_position, g_input.center_position);

			g_input.rotation_delta = vec2_mul_scalar(g_input.rotation_delta, 10.0f);

			set_mouse_pos_relative(0.0f, 0.0f);
		}
		else
		if (input_get_key_up(&g_input, KEY_CODE_MLB))
		{
			g_input.rotation_delta = vec2_zero();
			g_input.center_position = vec2_zero();
			set_mouse_pos_relative(0.0f, 0.0f);
			glfwSetInputMode(vk_app.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}

		if (input_get_key_down(&g_input, KEY_CODE_SPACE))
		{
			// hide next mesh
		}

		camera_update(&g_camera, &g_input, deltaSeconds);

		const b8 frameRendered = draw_frame(&vk_dev, &update_buffer, &update_frame);

		if (fps_counter_update(&g_fps_counter, deltaSeconds, frameRendered))
		{
			fps_graph_add_point(&g_fps_graph, g_fps_counter.fps);
		}
	}

  vulkan_term();
	glfwTerminate();

  return 0;
}