#ifndef __GL_APP_H__
#define __GL_APP_H__

#include <defines.h>

#include <glad.h>

#include <glfw/include/GLFW/glfw3.h>
#include <glfw/include/GLFW/glfw3native.h>

#define GLAD_MALLOC malloc
#define GLAD_FREE free
#define GLAD_GL_IMPLEMENTATION
#include <glad.h>

#define GL_SHADER_IMPLEMENTATION
#include <gl/gl_shader.h>

#include <math/mathm.h>

#include <debug.h>

typedef struct
gl_app
{
  GLFWwindow* window;
	f64 timeStamp;// = glfwGetTime();
	f32 deltaSeconds;// = 0;
}
gl_app;

static void
gltf_error_callback(int error,
                    const char* description)
{
	fprintf(stderr, "Error: %s\n", description);
}

gl_app
gl_app_create()
{
  gl_app app = { 0 };
  app.timeStamp = glfwGetTime();

  glfwSetErrorCallback(&gltf_error_callback);

	if (!glfwInit())
		exit(EXIT_FAILURE);

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GLFW_TRUE);

	const GLFWvidmode* info = glfwGetVideoMode(glfwGetPrimaryMonitor());

	app.window = glfwCreateWindow(info->width, info->height, "Simple example", NULL, NULL);

	if (!app.window)
	{
		glfwTerminate();
		exit(EXIT_FAILURE);
	}

  glfwMakeContextCurrent(app.window);
	gladLoadGL(glfwGetProcAddress);
	glfwSwapInterval(0);
  debug_init();

	return app;
}

void
gl_app_swap_buffers(gl_app* app)
{
  glfwSwapBuffers(app->window);
	glfwPollEvents();
	assert(glGetError() == GL_NO_ERROR);

	const f64 newTimeStamp = glfwGetTime();
	app->deltaSeconds = (f32)(newTimeStamp - app->timeStamp);
	app->timeStamp = newTimeStamp;
}

void
gl_app_destroy(gl_app* app)
{
  glfwDestroyWindow(app->window);
	glfwTerminate();
}

#endif