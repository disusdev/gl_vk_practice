#ifndef __VK_SHADER_COMPILER_H__
#define __VK_SHADER_COMPILER_H__

// #include <defines.h>
//#include <containers/cvec.h>
//#include <string.h>
//#include <stdio.h>
//#include <stdlib.h>
#include <Windows.h>
#include <winbase.h>

#define PATH_TO_COMPILER "C:/VulkanSDK/1.3.216.0/Bin/glslc"
#define SHADERS_FOLDER "data/shaders/chapter03/"
#define COMPILED_SHADERS_FOLDER "data/compiled_shaders"

//#define compile_shader(shader_file_name)\
//system(PATH_TO_COMPILER " " SHADERS_FOLDER shader_file_name " -o " COMPILED_SHADERS_FOLDER "/" shader_file_name ".spv");

typedef enum
file_mode
{
  FILE_MODE_READ = 0,
  FILE_MODE_WRITE = BIT(0),
  FILE_MODE_READWRITE = BIT(1),
}
file_mode;

void
load_shader(const char* file_name, cvec(u32)* out_data)
{
  OFSTRUCT buffer;
  HFILE file = OpenFile(file_name,
                        &buffer,
                        OF_READ);

  assert(file != -1);

  u64 size = 0;
  BOOL res = GetFileSizeEx(file, &size);

  assert(size != 0);
  assert(res);

  *out_data = cvec_ncreate(u32, (size / sizeof(u32)));

  DWORD bytes_read = 0;

  ReadFile(file,
           *out_data,
           size,
           &bytes_read,
           NULL);

  res = CloseHandle(file);

  assert(res);
}

void
compile_shader(const char* file_path, cvec(u32)* out_data)
{
  const char* filename = strrchr(file_path, '/');
  i32 str_size = strlen(PATH_TO_COMPILER) + 1 + strlen(file_path) + 4 + strlen(COMPILED_SHADERS_FOLDER) + strlen(filename) + 5;
  char* cmd = malloc(str_size);
  strcpy(cmd, PATH_TO_COMPILER);
  cmd = strcat(cmd, " ");
  cmd = strcat(cmd, file_path);
  cmd = strcat(cmd, " -o ");
  cmd = strcat(cmd, COMPILED_SHADERS_FOLDER);
  cmd = strcat(cmd, filename);
  cmd = strcat(cmd, ".spv");
  cmd[str_size] = '\0';

  printf("Call: %s\n", cmd);

  system(cmd);

  str_size = strlen(COMPILED_SHADERS_FOLDER) + strlen(filename) + 5;
  char* compiled = malloc(str_size);
  strcpy(compiled, COMPILED_SHADERS_FOLDER);
  compiled = strcat(compiled, filename);
  compiled = strcat(compiled, ".spv");
  compiled[str_size] = '\0';

  load_shader(compiled, out_data);
}

#endif