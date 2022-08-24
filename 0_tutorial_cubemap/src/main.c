// 0_tutorial_cubemap

#include <shared/defines.h>

#include <external/glad.h>

#define RGLFW_IMPLEMENTATION
#include <external/rglfw.h>

#define GLAD_MALLOC malloc
#define GLAD_FREE free
#define GLAD_GL_IMPLEMENTATION
#include <external/glad.h>

#define CGLTF_IMPLEMENTATION
#include <external/cgltf.h>

#include <shared/debug.h>

#include <shared/math/mathm.h>

#define GL_SHADER_IMPLEMENTATION
#include <shared/gl/gl_shader.h>

typedef struct
VertexData
{
	vec3 pos;
	vec3 n;
	vec2 tc;
}
VertexData;

typedef struct
PerFrameData
{
	mat4 model;
	mat4 mvp;
	vec4 cameraPos;
}
PerFrameData;

//typedef struct
//t_model
//{
//	mat4 transform;       // Local transform matrix
//
//	int meshCount;          // Number of meshes
//	int materialCount;      // Number of materials
//	Mesh* meshes;           // Meshes array
//	Material* materials;    // Materials array
//	int* meshMaterial;      // Mesh material number
//
//	// Animation data
//	int boneCount;          // Number of bones
//	BoneInfo* bones;        // Bones information (skeleton)
//	Transform* bindPose;    // Bones base transformation (pose)
//}
//t_model;

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
  gl_shader shdModelVertex = gl_shader_create("../../data/shaders/chapter03/GL03_duck.vert");
	gl_shader shdModelFragment = gl_shader_create("../../data/shaders/chapter03/GL03_duck.frag");
	gl_program progModel = gl_program_2s_create(&shdModelVertex, &shdModelFragment);

	gl_shader shdCubeVertex = gl_shader_create("../../data/shaders/chapter03/GL03_cube.vert");
	gl_shader shdCubeFragment = gl_shader_create("../../data/shaders/chapter03/GL03_cube.frag");
	gl_program progCube = gl_program_2s_create(&shdCubeVertex, &shdCubeFragment);

  const GLsizeiptr kUniformBufferSize = sizeof(PerFrameData);

	GLuint perFrameDataBuffer;
	glCreateBuffers(1, &perFrameDataBuffer);
	glNamedBufferStorage(perFrameDataBuffer, kUniformBufferSize, NULL, GL_DYNAMIC_STORAGE_BIT);
	glBindBufferRange(GL_UNIFORM_BUFFER, 0, perFrameDataBuffer, 0, kUniformBufferSize);

	glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
	glEnable(GL_DEPTH_TEST);

	// model load gltf
	{
		#define LOAD_ATTRIBUTE(accesor, numComp, dataType, dstPtr) \
    { \
        int n = 0; \
        dataType *buffer = (dataType *)accesor->buffer_view->buffer->data + accesor->buffer_view->offset/sizeof(dataType) + accesor->offset/sizeof(dataType); \
        for (unsigned int k = 0; k < accesor->count; k++) \
        {\
            for (int l = 0; l < numComp; l++) \
            {\
                dstPtr[numComp*k + l] = buffer[n + l];\
            }\
            n += (int)(accesor->stride/sizeof(dataType));\
        }\
    }

		// glTF file loading
		unsigned int dataSize = 0;
		unsigned char* fileData = LoadFileData("../../data/rubber_duck/scene.gltf", &dataSize);

		// glTF data loading
		cgltf_options options = { 0 };
		cgltf_data* data = NULL;
		cgltf_result result = cgltf_parse(&options, fileData, dataSize, &data);

		if (result == cgltf_result_success)
		{
			result = cgltf_load_buffers(&options, data, "../../data/rubber_duck/scene.gltf");
			if (result != cgltf_result_success) assert(false);

			int primitivesCount = 0;
			// NOTE: We will load every primitive in the glTF as a separate raylib mesh
			for (unsigned int i = 0; i < data->meshes_count; i++) primitivesCount += (int)data->meshes[i].primitives_count;

			// Load our model data: meshes and materials
			// model.meshCount = primitivesCount;
			// model.meshes = calloc(model.meshCount, sizeof(Mesh));

			for (unsigned int i = 0, meshIndex = 0; i < primitivesCount; i++)
			{
				for (unsigned int p = 0; p < data->meshes[i].primitives_count; p++)
				{
					if (data->meshes[i].primitives[p].type != cgltf_primitive_type_triangles) continue;

					for (unsigned int j = 0; j < data->meshes[i].primitives[p].attributes_count; j++)
					{
						if (data->meshes[i].primitives[p].attributes[j].type == cgltf_attribute_type_position)      // POSITION
						{
							cgltf_accessor* attribute = data->meshes[i].primitives[p].attributes[j].data;

							// WARNING: SPECS: POSITION accessor MUST have its min and max properties defined.

							if ((attribute->component_type == cgltf_component_type_r_32f) && (attribute->type == cgltf_type_vec3))
							{
								// Init raylib mesh vertices to copy glTF attribute data
								//model.meshes[meshIndex].vertexCount = (int)attribute->count;
								//model.meshes[meshIndex].vertices = malloc(attribute->count * 3 * sizeof(float));

								// Load 3 components of float data type into mesh.vertices
								//LOAD_ATTRIBUTE(attribute, 3, float, model.meshes[meshIndex].vertices)
							}
							// else TRACELOG(LOG_WARNING, "MODEL: [%s] Vertices attribute data format not supported, use vec3 float", fileName);
						}
					}
				}
			}
		}
	}

	// loop

	glfwDestroyWindow(window);
	glfwTerminate();

  return(0);
}