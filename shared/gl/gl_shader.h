#ifndef __GL_SHADER_H__
#define __GL_SHADER_H__

#include "../tools.h"
#include <assert.h>

typedef struct
gl_shader
{
  GLenum type;
	GLuint handle;
}
gl_shader;

typedef struct
gl_program
{
	GLuint handle;
}
gl_program;

typedef struct
gl_buffer
{
	GLuint handle;
}
gl_buffer;

gl_shader gl_shader_create_from_source(GLenum type, const char* text, const char* debugFileName);
gl_shader gl_shader_create(const char* file_name);
void gl_shader_destroy(gl_shader shader);

//gl_program gl_program_1s_create(const gl_shader& a);
gl_program gl_program_2s_create(const gl_shader* a, const gl_shader* b);
gl_program gl_program_3s_create(const gl_shader* a, const gl_shader* b, const gl_shader* c);
//gl_program gl_program_5s_create(const gl_shader& a, const gl_shader& b, const gl_shader& c, const gl_shader& d, const gl_shader& e);
void gl_program_use(gl_program program);
void gl_program_destroy(gl_program program);

gl_buffer gl_buffer_create(GLsizeiptr size, const void* data, GLbitfield flags);
void gl_buffer_destroy(gl_program program);

// make vscode happy
#if defined(__INTELLISENSE__)
#define GL_SHADER_IMPLEMENTATION
#endif

#ifdef GL_SHADER_IMPLEMENTATION

static GLenum
shader_get_type(const char* file_name)
{
  if (ends_with(file_name, ".vert"))
  {
    return GL_VERTEX_SHADER;
  }
  
  if (ends_with(file_name, ".frag"))
  {
    return GL_FRAGMENT_SHADER;
  }
  
  if (ends_with(file_name, ".geom"))
  {
    return GL_GEOMETRY_SHADER;
  }
  
  if (ends_with(file_name, ".tesc"))
  {
    return GL_TESS_CONTROL_SHADER;
  }
  
  if (ends_with(file_name, ".tese"))
  {
    return GL_TESS_EVALUATION_SHADER;
  }
  
  if (ends_with(file_name, ".comp"))
  {
    return GL_COMPUTE_SHADER;
  }
  
  assert(false);
  return 0;
}

gl_shader
gl_shader_create_from_source(GLenum type,
                             const char* text,
                             const char* debug_name)
{
  gl_shader s;
  s.type = type;
  s.handle = glCreateShader(type);
  

  glShaderSource(s.handle, 1, &text, NULL);
  glCompileShader(s.handle);

  // @xander: this shouldn't be here!
  // free((char*)(text-3));
  
  char buffer[8192];
  GLsizei length = 0;
  glGetShaderInfoLog(s.handle, sizeof(buffer), &length, buffer);
  
  if (length)
  {
    printf("%s (File: %s)\n", buffer, debug_name);
    print_shader_src(text);
    assert(false);
  }
  
  return s;
}

gl_shader
gl_shader_create(const char* file_path)
{
  return gl_shader_create_from_source(shader_get_type(file_path),
                                      read_text_file(file_path).ptr,
                                      file_path);
}

void
gl_shader_destroy(gl_shader shader)
{
  glDeleteProgram(shader.handle);
  shader.type = 0;
  shader.handle = 0;
}

gl_program
gl_program_2s_create(const gl_shader* s1,
                     const gl_shader* s2)
{
  gl_program p = { glCreateProgram() };
  glAttachShader(p.handle, s1->handle);
  glAttachShader(p.handle, s2->handle);
  glLinkProgram(p.handle);
  // program_print_info(p.handle);
  return p;
}

gl_program
gl_program_3s_create(const gl_shader* s1,
                     const gl_shader* s2,
                     const gl_shader* s3)
{
  gl_program p;
  p.handle = glCreateProgram();
  glAttachShader(p.handle, s1->handle);
  glAttachShader(p.handle, s2->handle);
  glAttachShader(p.handle, s3->handle);
  glLinkProgram(p.handle);
  // program_print_info(p.handle);
  return p;
}

void
gl_program_use(gl_program program)
{
  glUseProgram(program.handle);
}

void
gl_program_destroy(gl_program program)
{
  glDeleteProgram(program.handle);
  program.handle = 0;
}


#endif

#endif