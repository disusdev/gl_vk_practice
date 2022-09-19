// 0_tutorial_cubemap

#include <defines.h>

#include <glad.h>

#define RGLFW_IMPLEMENTATION
#include <rglfw.h>

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
PerFrameData
{
	mat4 model;
	mat4 mvp;
	vec4 cameraPos;
}
PerFrameData;

void gltf_error_callback(int error, const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

void gltf_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GLFW_TRUE);
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

	GLFWwindow* window = glfwCreateWindow(1024, 768, "Simple example", NULL, NULL);
	if (!window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

	glfwSetKeyCallback(
		window,
		gltf_key_callback
	);

	glfwMakeContextCurrent(window);
	gladLoadGL(glfwGetProcAddress);
	glfwSwapInterval(1);
  debug_init();

  // init scene settings
  gl_shader shdModelVertex = gl_shader_create("data/shaders/chapter03/GL03_duck.vert");
	gl_shader shdModelFragment = gl_shader_create("data/shaders/chapter03/GL03_duck.frag");
	gl_program progModel = gl_program_2s_create(&shdModelVertex, &shdModelFragment);

	gl_shader shdCubeVertex = gl_shader_create("data/shaders/chapter03/GL03_cube.vert");
	gl_shader shdCubeFragment = gl_shader_create("data/shaders/chapter03/GL03_cube.frag");
	gl_program progCube = gl_program_2s_create(&shdCubeVertex, &shdCubeFragment);

  const GLsizeiptr kUniformBufferSize = sizeof(PerFrameData);

	GLuint perFrameDataBuffer;
	glCreateBuffers(1, &perFrameDataBuffer);
	glNamedBufferStorage(perFrameDataBuffer, kUniformBufferSize, NULL, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, perFrameDataBuffer, 0, kUniformBufferSize);

	glClearColor(0.3f, 0.3f, 0.3f, 1.0f);
	glEnable(GL_DEPTH_TEST);
	// glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

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

	// loop
	while (!glfwWindowShouldClose(window))
	{
		int width, height;
		glfwGetFramebufferSize(window, &width, &height);
		const float ratio = width / (float)height;

		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		const mat4 p = mat4_persp(DEG2RAD * 90.0f, ratio, 0.1f, 1000.0f);

		{
			const float time = (float)glfwGetTime();

			const vec3 pos = (vec3){ 0.0f, -0.5f, -4.0f - math_sin(time /** 0.5*/) };

			const vec3 axis = (vec3){ 0.0f, 1.0f, 0.0f };
			const quat q = quat_from_axis_angle(axis, -time /** 0.5*/, false);
			mat4 tr = mat4_mul(model->transforms[mesh_idx], mat4_mul(quat_to_mat4(q), mat4_translation(pos)));

			PerFrameData perFrameData;
			perFrameData.model = tr;
			perFrameData.mvp = mat4_mul(tr, p);
			perFrameData.cameraPos = vec4_zero();

			glNamedBufferSubData(perFrameDataBuffer, 0, kUniformBufferSize, &perFrameData);
			gl_program_use(progModel);

			glDrawElements(GL_TRIANGLES, (u32)(mesh->triangleCount * 3), GL_UNSIGNED_INT, NULL);
		}

		{
			const mat4 m = mat4_scale_scalar(100.0f);
			PerFrameData perFrameData;
			perFrameData.model = m;
			perFrameData.mvp = mat4_mul(m, p);
			perFrameData.cameraPos = vec4_zero();
			glNamedBufferSubData(perFrameDataBuffer, 0, kUniformBufferSize, &perFrameData);
			gl_program_use(progCube);
			glDrawArrays(GL_TRIANGLES, 0, 36);
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	glDeleteBuffers(1, &dataIndices);
	glDeleteBuffers(1, &dataVertices);
	glDeleteBuffers(1, &perFrameDataBuffer);
	glDeleteVertexArrays(1, &vao);

	glfwDestroyWindow(window);
	glfwTerminate();

  return(0);
}