// 0_tutorial_cubemap

#include <defines.h>

#include <glad.h>

#include <assert.h>

#include <gl/gl_app.h>

#if defined(_WIN32) && defined(__TINYC__)
  #ifdef __int64
    #undef __int64
  #endif
  #define __int64 long
  #define _ftelli64 ftell
#endif

#define CGLTF_IMPLEMENTATION
#define CGLTF_VALIDATE_ENABLE_ASSERTS 1
#include <cgltf.h>

#if defined(_WIN32) && defined(__TINYC__)
  #undef __int64
  #define __int64 long long
  #undef _ftelli64
#endif

#include <debug.h>

#include <math/mathm.c>

#define CVEC_IMPLEMENTATION
#define CVEC_STDLIB
#include <containers/cvec.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#define CBITMAP_STDLIB
#define CBITMAP_IMPLEMENTATION
#include <resource/cbitmap.h>

#define TINYOBJ_MALLOC malloc
#define TINYOBJ_CALLOC calloc
#define TINYOBJ_REALLOC realloc
#define TINYOBJ_FREE free

#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include <tinyobj_loader_c.h>

#include <resource/model_loader.h>

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

vec2 relative_point;
gl_app g_app;

void set_mouse_pos(f32 x, f32 y)
{
	glfwSetCursorPos(g_app.window, x, y);
}

void set_mouse_pos_relative(f32 x, f32 y)
{
	glfwSetCursorPos(g_app.window, relative_point.x + x, relative_point.y + y);
}

#include <core/input.h>
#include <camera.h>

typedef struct
PerFrameData
{
	mat4 view;
	mat4 proj;
	vec4 cameraPos;
}
PerFrameData;

t_button_state g_button_state = { 0 };
t_input_state g_input = { 0 };

t_camera g_camera;


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

int main()
{
	g_app = gl_app_create();

	g_camera = camera_create();
	g_camera.position = (vec3) { 0.0f, 0.0f, 0.0f };
	g_camera.forward = vec3_forward();

  gl_shader shdGridVertex = gl_shader_create("data/shaders/chapter05/GL01_grid.vert");
	gl_shader shdGridFragment = gl_shader_create("data/shaders/chapter05/GL01_grid.frag");
	gl_program progGrid = gl_program_2s_create(&shdGridVertex, &shdGridFragment);

  const GLsizeiptr kUniformBufferSize = sizeof(PerFrameData);

	GLuint perFrameDataBuffer;
	glCreateBuffers(1, &perFrameDataBuffer);
	glNamedBufferStorage(perFrameDataBuffer, kUniformBufferSize, NULL, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, perFrameDataBuffer, 0, kUniformBufferSize);

	GLuint vao;
	glCreateVertexArrays(1, &vao);
	glBindVertexArray(vao);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glfwSetMouseButtonCallback
	(
		g_app.window,
		gltf_set_mouse_button_callback
	);

	glfwSetKeyCallback(
		g_app.window,
		gltf_key_callback
	);

	// loop
	while (!glfwWindowShouldClose(g_app.window))
	{
		f64 xpos, ypos;
		glfwGetCursorPos(g_app.window, &xpos, &ypos);
		g_input.current_mouse_position.x = xpos;
		g_input.current_mouse_position.y = ypos;

		input_update(&g_input, *(i32*)&g_button_state);

		if (input_get_key_down(&g_input, KEY_CODE_MLB))
		{
			relative_point = g_input.current_mouse_position;
			g_input.center_position = g_input.current_mouse_position;
			glfwSetInputMode(g_app.window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
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
			glfwSetInputMode(g_app.window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}

		camera_update(&g_camera, &g_input, g_app.deltaSeconds);

		int width, height;
		glfwGetFramebufferSize(g_app.window, &width, &height);
		const float ratio = width / (float)height;

		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		{
			const PerFrameData perFrameData =
			{
				.view = camera_get_view_matrix(&g_camera),
				.proj = camera_get_projection_matrix(&g_camera, ratio, false, false),
				.cameraPos = vec4_create(g_camera.position.x, g_camera.position.y, g_camera.position.z, 1.0f)
			};
			glNamedBufferSubData(perFrameDataBuffer, 0, kUniformBufferSize, &perFrameData);

			gl_program_use(progGrid);
			glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 6, 1, 0);
		}

		gl_app_swap_buffers(&g_app);
	}

	glDeleteBuffers(1, &perFrameDataBuffer);
	glDeleteVertexArrays(1, &vao);

	gl_app_destroy(&g_app);

  return(0);
}