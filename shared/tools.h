
#ifndef __TOOLS_H__
#define __TOOLS_H__

#include "defines.h"

#include <stdio.h>
#include <string.h>
#include <malloc.h>

void
print_shader_src(const char* text)
{
  int line = 1;
  
	printf("\n(%3i) ", line);
  
	while (text && *text++)
	{
		if (*text == '\n')
		{
			printf("\n(%3i) ", ++line);
		}
		else if (*text == '\r')
		{
		}
		else
		{
			printf("%c", *text);
		}
	}
  
	printf("\n");
}

int
ends_with(const char* s,
          const char* part)
{
  return (strstr(s,
                 part) - s) == (strlen(s) - strlen(part));
}

typedef struct
raw_text
{
  char* ptr;
  int length;
}
raw_text;

raw_text read_text_file(const char* file_path)
{
  raw_text text;
  
  FILE* f = fopen(file_path, "rb");
  fseek(f, 0, SEEK_END);
  text.length = ftell(f);
  fseek(f, 0, SEEK_SET);

  text.ptr = (char*)malloc((text.length + 1));
  fread(text.ptr, sizeof(char), text.length, f);

  fclose(f);

  //text.ptr += 3;
  //text.length -= 3;
  text.ptr[text.length] = '\0';

  return text;
}

unsigned char *LoadFileData(const char *fileName, unsigned int *bytesRead)
{
    unsigned char *data = NULL;
    *bytesRead = 0;

    if (fileName != NULL)
    {
        // if (loadFileData)
        // {
        //     data = loadFileData(fileName, bytesRead);
        //     return data;
        // }
        
        FILE *file = fopen(fileName, "rb");

        if (file != NULL)
        {
            // WARNING: On binary streams SEEK_END could not be found,
            // using fseek() and ftell() could not work in some (rare) cases
            fseek(file, 0, SEEK_END);
            int size = ftell(file);
            fseek(file, 0, SEEK_SET);

            if (size > 0)
            {
                data = (unsigned char *)malloc(size*sizeof(unsigned char));

                // NOTE: fread() returns number of read elements instead of bytes, so we read [1 byte, size elements]
                unsigned int count = (unsigned int)fread(data, sizeof(unsigned char), size, file);
                *bytesRead = count;
            }

            fclose(file);
        }
    }

    return data;
}

#endif