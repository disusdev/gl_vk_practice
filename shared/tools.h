
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

char *substring(const char *start, const char* end)
{
  char* new_str = malloc(end - start);
  memset(new_str, 0, end - start);

  new_str = strncat(new_str, start, end - start);

  return new_str;
}

char* strreplace(char* string, const char* start, const char* end, const char* replaceWith)
{
  int replace_length = end - start;
  int full_length = strlen(string) - replace_length + strlen(replaceWith);

  char* newString = malloc(full_length + 1);
  memset(newString, 0, full_length + 1);

  const char* str_ptr = string;

  newString = strncat(newString, str_ptr, start - str_ptr);
  newString = strncat(newString, replaceWith, strlen(replaceWith));
  str_ptr += (start - str_ptr) + replace_length;
  newString = strcat(newString, str_ptr);

  free(string);
  string = newString;
  return newString;
}

raw_text read_text_file(const char* file_path)
{
  raw_text text;
  
  FILE* f = fopen(file_path, "rb");

  assert(f);

  fseek(f, 0, SEEK_END);
  text.length = ftell(f);
  fseek(f, 0, SEEK_SET);

  text.ptr = (char*)malloc((text.length + 1));
  fread(text.ptr, sizeof(char), text.length, f);

  fclose(f);

  //text.ptr += 3;
  //text.length -= 3;
  text.ptr[text.length] = '\0';

  char* code_ptr = text.ptr;

  while (strstr(code_ptr, "#include ") != NULL)
  {
    char* pos = strstr(code_ptr, "#include ");
    char* p1 = strstr(code_ptr, "<");
    char* p2 = strstr(code_ptr, ">");

		if (p1 == NULL || p2 == NULL || p2 <= p1)
		{
			printf("Error while loading shader program: %s\n", text.ptr);
			return text;
		}
		char* name = substring(p1 + 1, p2);
		raw_text include = read_text_file(name);
    //free(name);
    text.ptr = strreplace(text.ptr, pos, p2 + 1, include.ptr);
    free(include.ptr);
    code_ptr = text.ptr;
  }

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
                data = (unsigned char *)malloc(size*sizeof(unsigned char) + 1);

                // NOTE: fread() returns number of read elements instead of bytes, so we read [1 byte, size elements]
                unsigned int count = (unsigned int)fread(data, sizeof(unsigned char), size, file);
                *bytesRead = count;

                data[size] = '\0';
            }

            fclose(file);
        }
    }

    return data;
}

void UnloadFileData(unsigned char* data)
{
  free(data);
}

typedef union
uif32
{
	float f;
	unsigned int i;
}
uif32;

f32 overflow()
{
	volatile f32 f = 1e10;
	for(int i = 0; i < 10; ++i)
		f *= f; // this will overflow before the for loop terminates
	return f;
}

u16 toFloat16(float f)
{
	uif32 Entry;
	Entry.f = f;
	int i = (i32)(Entry.i);
	//
	// Our floating point number, f, is represented by the bit
	// pattern in integer i.  Disassemble that bit pattern into
	// the sign, s, the exponent, e, and the significand, m.
	// Shift s into the position where it will go in the
	// resulting half number.
	// Adjust e, accounting for the different exponent bias
	// of float and half (127 versus 15).
	//
	int s =  (i >> 16) & 0x00008000;
	int e = ((i >> 23) & 0x000000ff) - (127 - 15);
	int m =   i        & 0x007fffff;
	//
	// Now reassemble s, e and m into a half:
	//
	if(e <= 0)
	{
		if(e < -10)
		{
			//
			// E is less than -10.  The absolute value of f is
			// less than half_MIN (f may be a small normalized
			// float, a denormalized float or a zero).
			//
			// We convert f to a half zero.
			//
			return (u16)(s);
		}
		//
		// E is between -10 and 0.  F is a normalized float,
		// whose magnitude is less than __half_NRM_MIN.
		//
		// We convert f to a denormalized half.
		//
		m = (m | 0x00800000) >> (1 - e);
		//
		// Round to nearest, round "0.5" up.
		//
		// Rounding may cause the significand to overflow and make
		// our number normalized.  Because of the way a half's bits
		// are laid out, we don't have to treat this case separately;
		// the code below will handle it correctly.
		//
		if(m & 0x00001000)
			m += 0x00002000;
		//
		// Assemble the half from s, e (zero) and m.
		//
		return (u16)(s | (m >> 13));
	}
	else if(e == 0xff - (127 - 15))
	{
		if(m == 0)
		{
			//
			// F is an infinity; convert f to a half
			// infinity with the same sign as f.
			//
			return (u16)(s | 0x7c00);
		}
		else
		{
			//
			// F is a NAN; we produce a half NAN that preserves
			// the sign bit and the 10 leftmost bits of the
			// significand of f, with one exception: If the 10
			// leftmost bits are all zero, the NAN would turn
			// into an infinity, so we have to set at least one
			// bit in the significand.
			//
			m >>= 13;
			return (u16)(s | 0x7c00 | m | (m == 0));
		}
	}
	else
	{
		//
		// E is greater than zero.  F is a normalized float.
		// We try to convert f to a normalized half.
		//
		//
		// Round to nearest, round "0.5" up
		//
		if(m &  0x00001000)
		{
			m += 0x00002000;
			if(m & 0x00800000)
			{
				m =  0;     // overflow in significand,
				e += 1;     // adjust exponent
			}
		}
		//
		// Handle exponent overflow
		//
		if (e > 30)
		{
			overflow();        // Cause a hardware floating point overflow;
			return (u16)(s | 0x7c00);
			// if this returns, the half becomes an
		}   // infinity with the same sign as f.
		//
		// Assemble the half from s, e and m.
		//
		return (u16)(s | (e << 10) | (m >> 13));
	}
}

u32 packHalf2x16(vec2 v)
{
	union
	{
		i16 in[2];
		u32 out;
	} u;
	u.in[0] = toFloat16(v.x);
	u.in[1] = toFloat16(v.y);
	return u.out;
}

#endif