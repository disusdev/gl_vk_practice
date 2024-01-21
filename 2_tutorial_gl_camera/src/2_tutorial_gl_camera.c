// 0_tutorial_cubemap

#include <defines.h>

#include <glad.h>

#include <glfw/include/GLFW/glfw3.h>
#include <glfw/include/GLFW/glfw3native.h>

#define GLAD_MALLOC malloc
#define GLAD_FREE free
#define GLAD_GL_IMPLEMENTATION
#include <glad.h>

#include <assert.h>

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


#define GL_SHADER_IMPLEMENTATION
#include <gl/gl_shader.h>

#define CVEC_IMPLEMENTATION
#define CVEC_STDLIB
#include <containers/cvec.h>

#define STB_IMAGE_IMPLEMENTATION
// #define STBI_MAX_DIMENSIONS (1 << 64)
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

GLFWwindow* g_window;
vec2 relative_point;

void set_mouse_pos(f32 x, f32 y)
{
	glfwSetCursorPos(g_window, x, y);
}

void set_mouse_pos_relative(f32 x, f32 y)
{
	glfwSetCursorPos(g_window, relative_point.x + x, relative_point.y + y);
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


void gltf_error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
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

int main()
{
	glfwSetErrorCallback(
		gltf_error_callback
	);

	if (!glfwInit())
		exit(EXIT_FAILURE);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

	g_window = glfwCreateWindow(1024, 768, "Simple example", NULL, NULL);
	if (!g_window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetMouseButtonCallback
	(
		g_window,
		gltf_set_mouse_button_callback
	);

	glfwSetKeyCallback(
		g_window,
		gltf_key_callback
	);

	glfwMakeContextCurrent(g_window);
	gladLoadGL(glfwGetProcAddress);
	glfwSwapInterval(1);
  debug_init();

  gl_shader shdModelVertex = gl_shader_create("data/shaders/chapter04/GL01_duck.vert");
	gl_shader shdModelFragment = gl_shader_create("data/shaders/chapter04/GL01_duck.frag");
	gl_program progModel = gl_program_2s_create(&shdModelVertex, &shdModelFragment);

	gl_shader shdCubeVertex = gl_shader_create("data/shaders/chapter04/GL01_cube.vert");
	gl_shader shdCubeFragment = gl_shader_create("data/shaders/chapter04/GL01_cube.frag");
	gl_program progCube = gl_program_2s_create(&shdCubeVertex, &shdCubeFragment);

  const GLsizeiptr kUniformBufferSize = sizeof(PerFrameData);

	GLuint perFrameDataBuffer;
	glCreateBuffers(1, &perFrameDataBuffer);
	glNamedBufferStorage(perFrameDataBuffer, kUniformBufferSize, NULL, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, perFrameDataBuffer, 0, kUniformBufferSize);

	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	// glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	g_camera = camera_create();
	g_camera.position = (vec3) { 0.0f, 0.0f, 0.0f };
	g_camera.forward = vec3_forward();

	// model load gltf
	//t_model* model = LoadGLTF("../../data/rubber_duck/scene.gltf");
	t_model* model = LoadGLTF("data/DragonAttenuation/glTF/DragonAttenuation.gltf");
	//t_model* model = LoadGLTF("../../data/Cube/Cube.gltf");
	//t_model* model = LoadOBJ("../../data/Cube/Cube.obj");

	int mesh_idx = 1;

	t_mesh* mesh = &model->meshes[mesh_idx];

	mesh->vn_vertices = cvec_ncreate(VertexData, mesh->vertexCount);
	cvec_resize(mesh->vn_vertices, mesh->vertexCount);

	for (u64 i = 0; i < mesh->vertexCount; i++)
	{
		mesh->vn_vertices[i].pos.x = mesh->v_vertices[i * 3];
		mesh->vn_vertices[i].pos.y = mesh->v_vertices[i * 3 + 1];
		mesh->vn_vertices[i].pos.z = mesh->v_vertices[i * 3 + 2];

		mesh->vn_vertices[i].n.x = mesh->v_normals[i * 3];
		mesh->vn_vertices[i].n.y = mesh->v_normals[i * 3 + 1];
		mesh->vn_vertices[i].n.z = mesh->v_normals[i * 3 + 2];

		mesh->vn_vertices[i].tc.x = mesh->v_texcoords[i * 2];
		mesh->vn_vertices[i].tc.y = mesh->v_texcoords[i * 2 + 1];
	}

	u64 idx_size = sizeof(u32) * (mesh->triangleCount * 3);
	u64 vert_size = sizeof(VertexData) * mesh->vertexCount;

	// indices
	GLuint dataIndices;
	glCreateBuffers(1, &dataIndices);
	glNamedBufferStorage(dataIndices, idx_size, mesh->v_indices, 0);
	GLuint vao;
	glCreateVertexArrays(1, &vao);
	glBindVertexArray(vao);
	glVertexArrayElementBuffer(vao, dataIndices);

	// vertices
	GLuint dataVertices;
	glCreateBuffers(1, &dataVertices);
	glNamedBufferStorage(dataVertices, vert_size, mesh->vn_vertices, 0);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, dataVertices);

	GLuint modelMatrices;
	glCreateBuffers(1, &modelMatrices);
	glNamedBufferStorage(modelMatrices, sizeof(mat4) * 2, NULL, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, modelMatrices);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

	// texture
	GLuint texture;
	{
		int w, h, comp;
		// const uint8_t* img = stbi_load("../../data/rubber_duck/textures/Duck_baseColor.png", &w, &h, &comp, 3);
		const uint8_t* img = stbi_load("data/DragonAttenuation/glTF/Dragon_ThicknessMap.jpg", &w, &h, &comp, 3);

		glCreateTextures(GL_TEXTURE_2D, 1, &texture);
		glTextureParameteri(texture, GL_TEXTURE_MAX_LEVEL, 0);
		glTextureParameteri(texture, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(texture, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureStorage2D(texture, 1, GL_RGB8, w, h);
		glTextureSubImage2D(texture, 0, 0, 0, w, h, GL_RGB, GL_UNSIGNED_BYTE, img);
		glBindTextures(0, 1, &texture);

		stbi_image_free((void*)img);
	}

	// cube map
	GLuint cubemapTex;
	{
		int w, h, comp;

		//const float* img = stbi_loadf("../../data/piazza_bologni_1k.hdr", &w, &h, &comp, 3);
		//const float* img = stbi_loadf("../../data/Milkyway_small.hdr", &w, &h, &comp, 3);
		//const float* img = stbi_loadf("../../data/Ice_Lake_Ref.hdr", &w, &h, &comp, 3);
		const float* img = stbi_loadf("data/SnowPano_4k_Ref.hdr", &w, &h, &comp, 3);
		//const float* img = stbi_loadf("../../data/country_club_16k.hdr", &w, &h, &comp, 3);
		//const float* img = stbi_loadf("../../data/pretville_cinema_1k.hdr", &w, &h, &comp, 3);

		t_cbitmap* in = cbitmap_create_from_data(w, h, 1, comp, BITMAP_FORMAT_F32, img);
		t_cbitmap* out = convert_equirectangular_map_to_vertical_cross(in);

		//stbi_write_hdr("in_screenshot.hdr", in->w, in->h, in->comp, (const float*)in->data);
		//stbi_write_hdr("out_screenshot.hdr", out->w, out->h, out->comp, (const float*)out->data);

		stbi_image_free((void*)img);

		t_cbitmap* cubemap = convert_vertical_cross_to_cube_map_faces(out);

		glCreateTextures(GL_TEXTURE_CUBE_MAP, 1, &cubemapTex);
		glTextureParameteri(cubemapTex, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTextureParameteri(cubemapTex, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTextureParameteri(cubemapTex, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTextureParameteri(cubemapTex, GL_TEXTURE_BASE_LEVEL, 0);
		glTextureParameteri(cubemapTex, GL_TEXTURE_MAX_LEVEL, 0);
		glTextureParameteri(cubemapTex, GL_TEXTURE_MAX_LEVEL, 0);
		glTextureParameteri(cubemapTex, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTextureParameteri(cubemapTex, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTextureParameteri(cubemapTex, GL_TEXTURE_CUBE_MAP_SEAMLESS, GL_TRUE);
		glTextureStorage2D(cubemapTex, 1, GL_RGB32F, cubemap->w, cubemap->h);
		const uint8_t* data = cubemap->data;

		for (unsigned i = 0; i != 6; ++i)
		{
			glTextureSubImage3D(cubemapTex, 0, 0, 0, i, cubemap->w, cubemap->h, 1, GL_RGB, GL_FLOAT, data);
			data += cubemap->w * cubemap->h * cubemap->comp * (cubemap->format ? 4 : 1);
		}
		glBindTextures(1, 1, &cubemapTex);
	}

	f64 timeStamp = glfwGetTime();
	f32 deltaSeconds = 0.0f;

	// loop
	while (!glfwWindowShouldClose(g_window))
	{
		glfwPollEvents();

		f64 newTimeStamp = glfwGetTime();
		deltaSeconds = (f32)(newTimeStamp - timeStamp);
		timeStamp = newTimeStamp;

		f64 xpos, ypos;
		glfwGetCursorPos(g_window, &xpos, &ypos);
		g_input.current_mouse_position.x = xpos;
		g_input.current_mouse_position.y = ypos;

		input_update(&g_input, *(i32*)&g_button_state);

		if (input_get_key_down(&g_input, KEY_CODE_MLB))
		{
			relative_point = g_input.current_mouse_position;
			g_input.center_position = g_input.current_mouse_position;
			glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
		}
		else
		if (input_get_key(&g_input, KEY_CODE_MLB))
		{
			g_input.rotation_delta = vec2_sub(g_input.current_mouse_position, g_input.center_position);
			set_mouse_pos_relative(0.0f, 0.0f);
		}
		else
		if (input_get_key_up(&g_input, KEY_CODE_MLB))
		{
			g_input.rotation_delta = vec2_zero();
			g_input.center_position = vec2_zero();
			set_mouse_pos_relative(0.0f, 0.0f);
			glfwSetInputMode(g_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
		}

		camera_update(&g_camera, &g_input, deltaSeconds);

		int width, height;
		glfwGetFramebufferSize(g_window, &width, &height);
		const float ratio = width / (float)height;

		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		f32 time = (float)glfwGetTime();
		vec3 pos = (vec3){ 0.0f, -0.5f, -4.0f - math_sin(time) };
		const vec3 axis = (vec3){ 0.0f, 1.0f, 0.0f };
		const quat q = quat_from_axis_angle(axis, -time, false);

		{
			PerFrameData perFrameData;
			perFrameData.view = camera_get_view_matrix(&g_camera);
			perFrameData.proj = camera_get_projection_matrix(&g_camera, ratio, false, false);
			perFrameData.cameraPos = vec4_create(g_camera.position.x, g_camera.position.y, g_camera.position.z, 1.0f);

			glNamedBufferSubData(perFrameDataBuffer, 0, kUniformBufferSize, &perFrameData);
			gl_program_use(progModel);

			glDrawElementsInstancedBaseVertexBaseInstance(GL_TRIANGLES, (u32)(mesh->triangleCount * 3), GL_UNSIGNED_INT, NULL, 1, 0, 0);
		}

		{
			mat4 Matrices[2] =
			{
				mat4_mul(model->transforms[mesh_idx], mat4_mul(quat_to_mat4(q), mat4_translation(pos))),//mat4_mul(quat_to_mat4(q), mat4_translation(pos)),
				mat4_scale_scalar(100.0f)
			};

			glNamedBufferSubData(modelMatrices, 0, sizeof(mat4) * 2, &Matrices);

			gl_program_use(progCube);
			glDrawArraysInstancedBaseInstance(GL_TRIANGLES, 0, 36, 1, 1);
		}

		glfwSwapBuffers(g_window);
	}

	glDeleteBuffers(1, &dataIndices);
	glDeleteBuffers(1, &dataVertices);
	glDeleteBuffers(1, &perFrameDataBuffer);
	glDeleteVertexArrays(1, &vao);

	glfwDestroyWindow(g_window);
	glfwTerminate();

  return(0);
}