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
#include <vk/renderers/vk_quad_renderer.h>

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

// t_multi_mesh_renderer multi_mesh_renderer;
t_quad_renderer quad_renderer;
t_clear_renderer clear_renderer;
t_final_renderer final_renderer;
t_canvas_renderer canvas2d_renderer;

typedef struct
t_animation_state
{
	vec2 position;// = vec2(0);
	double startTime;// = 0;
	u32 textureIndex;// = 0;
	u32 flipbookOffset;// = 0;
}
t_animation_state;

u32 animation_fps = 60;
u32 animations_count = 3;
u32 frames_count = 100;
cvec(t_animation_state) animations;

void
updateAnimations()
{
	u64 anim_count = cvec_size(animations);
	for (size_t i = 0; i < anim_count;)
	{
		if (cvec_empty(animations)) return;

		animations[i].textureIndex = animations[i].flipbookOffset + (u32)(animation_fps * ((glfwGetTime() - animations[i].startTime)));
		if (animations[i].textureIndex - animations[i].flipbookOffset > (frames_count - 1))
		{
			cvec_erase(animations, i);
		}
		else
		{
			i++;
		}
	}
}

void
fillQuadsBuffer(vulkan_render_device* vkDev,
								t_quad_renderer* renderer,
								u64 currentImage)
{
	const float aspect = (float)vkDev->framebuffer_width / (float)vkDev->framebuffer_height;
	const float quadSize = 0.5f;

	quad_renderer_clear(renderer);
	quad_renderer_draw(renderer, -quadSize, -quadSize * aspect, quadSize, quadSize * aspect);
	quad_renderer_update_buffers(renderer, vkDev, currentImage);
}

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

	VkPhysicalDeviceDescriptorIndexingFeaturesEXT physicalDeviceDescriptorIndexingFeatures =
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_DESCRIPTOR_INDEXING_FEATURES_EXT,
		.shaderSampledImageArrayNonUniformIndexing = VK_TRUE,
		.descriptorBindingVariableDescriptorCount = VK_TRUE,
		.runtimeDescriptorArray = VK_TRUE,
	};

	const VkPhysicalDeviceFeatures deviceFeatures =
	{
		.shaderSampledImageArrayDynamicIndexing = VK_TRUE
	};

	const VkPhysicalDeviceFeatures2 deviceFeatures2 =
	{
		.sType = VK_STRUCTURE_TYPE_PHYSICAL_DEVICE_FEATURES_2,
		.pNext = &physicalDeviceDescriptorIndexingFeatures,
		.features = deviceFeatures
	};

	if (!vulkan_init_render_device2(&vk,
																	&vk_dev,
																	g_screen_width,
																	g_screen_height,
																	isDeviceSuitable,
																	deviceFeatures2))
	{
		exit(EXIT_FAILURE);
	}
  
	animations = cvec_ncreate(t_animation_state, frames_count);
	cvec_clear(animations);

	cvec(char*) textureFiles = cvec_ncreate(char*, animations_count * frames_count);

	for (u32 j = 0; j < animations_count; j++)
	{
		for (u32 i = 0; i != frames_count; i++)
		{
			u32 buffer_size = 1024;
			textureFiles[j * frames_count + i] = malloc(buffer_size);

			snprintf(textureFiles[j * frames_count + i], buffer_size, "deps/src/explosion%01u/explosion%02u-frame%03u.tga", j, j, i + 1);
		}
	}

	for (u32 j = 0; j < animations_count * frames_count; j++)
	{
		printf("%s\n", textureFiles[j]);
	}

	quad_renderer = quad_renderer_create(&vk_dev,
																		   &textureFiles,
																			 cvec_size(textureFiles));

	for(u64 i = 0; i < cvec_size(vk_dev.swapchain_images); i++)
	{
		fillQuadsBuffer(&vk_dev, &quad_renderer, i);
	}

	vulkan_image depth_zero = (vulkan_image)
	{
		VK_NULL_HANDLE,
		VK_NULL_HANDLE,
		VK_NULL_HANDLE
	};

	clear_renderer = clear_renderer_create(&vk_dev,
																				 depth_zero);

	final_renderer = final_renderer_create(&vk_dev, depth_zero);


	canvas2d_renderer = canvas_renderer_create(&vk_dev, depth_zero);

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
		canvas_renderer_update_uniform_buffer(&canvas2d_renderer, &vk_dev, &ortho, 0.0f, imageIndex);
    // multi_mesh_renderer_update_uniform_buffers(&multi_mesh_renderer, imageIndex, &p_v);
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
	updateAnimations();

  clear_renderer.base->fill_command_buffer(&clear_renderer, cmd, image_index);

	for (u64 i = 0; i < cvec_size(animations); i++)
	{
		quad_renderer.base->push_constants(&quad_renderer, cmd, animations[i].textureIndex, &animations[i].position);
		quad_renderer.base->fill_command_buffer(&quad_renderer, cmd, image_index);
	}

	canvas2d_renderer.base->fill_command_buffer(&canvas2d_renderer, cmd, image_index);
	final_renderer.base->fill_command_buffer(&final_renderer, cmd, image_index);
}

void vulkan_term()
{
	final_renderer_destroy(&final_renderer);

	canvas_renderer_destroy(&canvas2d_renderer);

	clear_renderer_destroy(&clear_renderer);

	quad_renderer_destroy(&quad_renderer);

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
			t_animation_state anim_state =
			{
				.position = (vec2) { g_input.center_position.x, g_input.center_position.y },
				.startTime = glfwGetTime(),
				.textureIndex = 0,
				.flipbookOffset = frames_count * (u32)(rand() % 3)
			};

			cvec_push(animations, anim_state);
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