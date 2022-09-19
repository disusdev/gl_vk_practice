#ifndef __CBITMAP_H__
#define __CBITMAP_H__

#include "../defines.h"

#ifndef __CVEC_H__
#include "../containers/cvec.h"
#endif

#include "../math/mathm.h"

#define MEM_TAG_CBITMAP -1

enum
{
  BITMAP_FORMAT_U8,
  BITMAP_FORMAT_F32,
};

enum
{
  BITMAP_TYPE_2D,
  BITMAP_TYPE_CUBE
};

typedef struct
t_cbitmap
{
  cvec(u8) data;
  i32 w, h, d, comp;
  int format;
  int type;
}
t_cbitmap;

t_cbitmap* cbitmap_create(int w,
                          int h,
                          int d,
                          int comp,
                          int format);

t_cbitmap* cbitmap_create_from_data(int w,
                                    int h,
                                    int d,
                                    int comp,
                                    int format,
                                    const void* ptr);

void cbitmap_set_pixel(t_cbitmap* bitmap,
                       int x,
                       int y,
                       const vec4* c);

vec4 cbitmap_get_pixel(t_cbitmap* bitmap,
                       int x,
                       int y);

t_cbitmap* convert_equirectangular_map_to_vertical_cross(const t_cbitmap* b);                       

// make vscode happy
#if defined(__INTELLISENSE__)
#define CBITMAP_IMPLEMENTATION
#endif

#ifdef CBITMAP_IMPLEMENTATION

#ifdef CBITMAP_STDLIB
#include <malloc.h>
#include <string.h>
#include <assert.h>

#ifndef CBITMAP_ALLOCATE
#define CBITMAP_ALLOCATE(size, mem_tag) malloc(size)
#endif
#ifndef CBITMAP_FREE
#define CBITMAP_FREE(ptr, size, mem_tag) free(ptr)
#endif
#ifndef CBITMAP_MEMSET
#define CBITMAP_MEMSET(ptr, value, size) memset(ptr, value, size)
#endif
#ifndef CBITMAP_MEMCPY
#define CBITMAP_MEMCPY(dst, src, size) memcpy(dst, src, size)
#endif
#ifndef CBITMAP_MEMCMP
#define CBITMAP_MEMCMP(b1, b2, size) memcmp(b1, b2, size)
#endif
#ifndef CBITMAP_ASSERT
#define CBITMAP_ASSERT(condition) assert(condition)
#endif
#else
#ifndef CBITMAP_ALLOCATE
#define CBITMAP_ALLOCATE(size, mem_tag) no_allocate_defined[-1]
#endif
#ifndef CBITMAP_FREE
#define CBITMAP_FREE(ptr, size, mem_tag) no_free_defined[-1]
#endif
#ifndef CBITMAP_MEMSET
#define CBITMAP_MEMSET(ptr, value, size) no_memset_defined[-1]
#endif
#ifndef CBITMAP_MEMCPY
#define CBITMAP_MEMCPY(dst, src, size) no_memcpy_defined[-1]
#endif
#ifndef CBITMAP_MEMCMP
#define CBITMAP_MEMCMP(b1, b2, size) no_memcmp_defined[-1]
#endif
#ifndef CBITMAP_ASSERT
#define CBITMAP_ASSERT(condition)
#endif
#endif

t_cbitmap* cbitmap_create(int w,
                          int h,
                          int d,
                          int comp,
                          int format)
{
  t_cbitmap* bitmap = CBITMAP_ALLOCATE(sizeof(t_cbitmap), MEM_TAG_CBITMAP);
  bitmap->w = w;
  bitmap->h = h;
  bitmap->d = d;
  bitmap->type = BITMAP_TYPE_2D;
  bitmap->comp = comp;
  bitmap->format = format;
	if (format)
	{
		bitmap->data = cvec_ncreate(f32, w * h * d * comp);
	}
	else
	{
		bitmap->data = cvec_ncreate(u8, w * h * d * comp);
	}
  cvec_resize(bitmap->data, cvec_capacity(bitmap->data));
  return bitmap;
}

t_cbitmap* cbitmap_create_from_data(int w,
                                    int h,
                                    int d,
                                    int comp,
                                    int format,
                                    const void* ptr)
{
  t_cbitmap* bitmap = CBITMAP_ALLOCATE(sizeof(t_cbitmap), MEM_TAG_CBITMAP);
  bitmap->w = w;
  bitmap->h = h;
  bitmap->d = d;
  bitmap->type = BITMAP_TYPE_2D;
  bitmap->comp = comp;
  bitmap->format = format;
	if (format)
	{
		bitmap->data = cvec_ncreate(f32, (w * h * d * comp));
		CBITMAP_MEMCPY(bitmap->data, ptr, w * h * d * comp * sizeof(f32));
	}
	else
	{
		bitmap->data = cvec_ncreate(u8, (w * h * d * comp));
		CBITMAP_MEMCPY(bitmap->data, ptr, w * h * d * comp);
	}
  return bitmap;
}

void cbitmap_set_pixel(t_cbitmap* bitmap,
                       int x,
                       int y,
                       const vec4* c)
{
  switch (bitmap->format)
  {
  case BITMAP_FORMAT_U8:
  {
    const int ofs = bitmap->comp * (y * bitmap->w + x);
		u8* data_ptr = (u8*)bitmap->data;
		if (bitmap->comp > 0) data_ptr[ofs + 0] = (u8)(c->x * 255.0f);
		if (bitmap->comp > 1) data_ptr[ofs + 1] = (u8)(c->y * 255.0f);
		if (bitmap->comp > 2) data_ptr[ofs + 2] = (u8)(c->z * 255.0f);
		if (bitmap->comp > 3) data_ptr[ofs + 3] = (u8)(c->w * 255.0f);
  } break;
  case BITMAP_FORMAT_F32:
  {
    const int ofs = bitmap->comp * (y * bitmap->w + x);
		f32* data = (f32*)(bitmap->data);
		if (bitmap->comp > 0) data[ofs + 0] = c->x;
		if (bitmap->comp > 1) data[ofs + 1] = c->y;
		if (bitmap->comp > 2) data[ofs + 2] = c->z;
		if (bitmap->comp > 3) data[ofs + 3] = c->w;
  } break;
  }
}

vec4 cbitmap_get_pixel(t_cbitmap* bitmap,
                       int x,
                       int y)
{
  switch (bitmap->format)
  {
  case BITMAP_FORMAT_U8:
  {
    const int ofs = bitmap->comp * (y * bitmap->w + x);
		u8* data_ptr = (u8*)bitmap->data;
    return vec4_create(bitmap->comp > 0 ? (f32)(data_ptr[ofs + 0]) / 255.0f : 0.0f,
			                 bitmap->comp > 1 ? (f32)(data_ptr[ofs + 1]) / 255.0f : 0.0f,
			                 bitmap->comp > 2 ? (f32)(data_ptr[ofs + 2]) / 255.0f : 0.0f,
			                 bitmap->comp > 3 ? (f32)(data_ptr[ofs + 3]) / 255.0f : 0.0f);
  }
  case BITMAP_FORMAT_F32:
  {
    const int ofs = bitmap->comp * (y * bitmap->w + x);
		f32* data_ptr = (f32*)bitmap->data;
		return vec4_create(bitmap->comp > 0 ? data_ptr[ofs + 0] : 0.0f,
			                 bitmap->comp > 1 ? data_ptr[ofs + 1] : 0.0f,
			                 bitmap->comp > 2 ? data_ptr[ofs + 2] : 0.0f,
			                 bitmap->comp > 3 ? data_ptr[ofs + 3] : 0.0f);
  }
  }

  return vec4_create(0.0f, 0.0f, 0.0f, 1.0f);
}

vec3 faceCoordsToXYZ(int i, int j, int faceID, int faceSize)
{
	const float A = 2.0f * (f32)(i) / faceSize;
	const float B = 2.0f * (f32)(j) / faceSize;

	if (faceID == 0) return vec3_create(-1.0f, A - 1.0f, B - 1.0f);
	if (faceID == 1) return vec3_create(A - 1.0f, -1.0f, 1.0f - B);
	if (faceID == 2) return vec3_create(1.0f, A - 1.0f, 1.0f - B);
	if (faceID == 3) return vec3_create(1.0f - A, 1.0f, 1.0f - B);
	if (faceID == 4) return vec3_create(B - 1.0f, A - 1.0f, 1.0f);
	if (faceID == 5) return vec3_create(1.0f - B, A - 1.0f, -1.0f);

	return vec3_create(0.0f, 0.0f, 0.0f);
}

t_cbitmap* convert_equirectangular_map_to_vertical_cross(const t_cbitmap* b)
{
	if (b->type != BITMAP_TYPE_2D) return NULL;

	const int faceSize = b->w / 4;

	const int w = faceSize * 3;
	const int h = faceSize * 4;

	t_cbitmap* result = cbitmap_create(w, h, 1, b->comp, b->format);

	const vec2 kFaceOffsets[6] =
	{
		vec2_create(faceSize, faceSize * 3),
		vec2_create(0, faceSize),
		vec2_create(faceSize, faceSize),
		vec2_create(faceSize * 2, faceSize),
		vec2_create(faceSize, 0),
		vec2_create(faceSize, faceSize * 2)
	};

	const int clampW = b->w - 1;
	const int clampH = b->h - 1;

	for (int face = 0; face != 6; face++)
	{
		for (int i = 0; i != faceSize; i++)
		{
			for (int j = 0; j != faceSize; j++)
			{
				const vec3 P = faceCoordsToXYZ(i, j, face, faceSize);
				const float R = math_hypot(P.x, P.y);
				const float theta = math_atan2(P.y, P.x);
				const float phi = math_atan2(P.z, R);
				//	float point source coordinates
				const float Uf = (f32)(2.0f * faceSize * (theta + PI) / PI);
				const float Vf = (f32)(2.0f * faceSize * (PI / 2.0f - phi) / PI);
				// 4-samples for bilinear interpolation
				const int U1 = CLAMP((i32)(math_floor(Uf)), 0, clampW);
				const int V1 = CLAMP((i32)(math_floor(Vf)), 0, clampH);
				const int U2 = CLAMP(U1 + 1, 0, clampW);
				const int V2 = CLAMP(V1 + 1, 0, clampH);
				// fractional part
				const float s = Uf - U1;
				const float t = Vf - V1;
				// fetch 4-samples
				const vec4 A = cbitmap_get_pixel(b, U1, V1);
				const vec4 B = cbitmap_get_pixel(b, U2, V1);
				const vec4 C = cbitmap_get_pixel(b, U1, V2);
				const vec4 D = cbitmap_get_pixel(b, U2, V2);
				// bilinear interpolation
				const vec4 color = vec4_add( vec4_add(vec4_mul_scalar(A, (1 - s) * (1 - t)), vec4_mul_scalar(B, (s) * (1 - t))), vec4_add(vec4_mul_scalar(C, (1 - s) * t), vec4_mul_scalar(D, (s) * (t))));
        cbitmap_set_pixel(result, i + kFaceOffsets[face].x, j + kFaceOffsets[face].y, &color);
			}
		};
	}

	return result;
}

t_cbitmap* convert_vertical_cross_to_cube_map_faces(const t_cbitmap* b)
{
	const int faceWidth = b->w / 3;
	const int faceHeight = b->h / 4;

	t_cbitmap* cubemap = cbitmap_create(faceWidth, faceHeight, 6, b->comp, b->format);
	cubemap->type = BITMAP_TYPE_CUBE;

	const uint8_t* src = b->data;
	uint8_t* dst = cubemap->data;

	/*
			------
			| +Y |
	 ----------------
	 | -X | -Z | +X |
	 ----------------
			| -Y |
			------
			| +Z |
			------
	*/

	const int pixelSize = cubemap->comp * (cubemap->format ? 4 : 1);

	for (int face = 0; face != 6; ++face)
	{
		for (int j = 0; j != faceHeight; ++j)
		{
			for (int i = 0; i != faceWidth; ++i)
			{
				int x = 0;
				int y = 0;

				switch (face)
				{
					// GL_TEXTURE_CUBE_MAP_POSITIVE_X
				case 0:
					x = i;
					y = faceHeight + j;
					break;

					// GL_TEXTURE_CUBE_MAP_NEGATIVE_X
				case 1:
					x = 2 * faceWidth + i;
					y = 1 * faceHeight + j;
					break;

					// GL_TEXTURE_CUBE_MAP_POSITIVE_Y
				case 2:
					x = 2 * faceWidth - (i + 1);
					y = 1 * faceHeight - (j + 1);
					break;

					// GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
				case 3:
					x = 2 * faceWidth - (i + 1);
					y = 3 * faceHeight - (j + 1);
					break;

					// GL_TEXTURE_CUBE_MAP_POSITIVE_Z
				case 4:
					x = 2 * faceWidth - (i + 1);
					y = b->h - (j + 1);
					break;

					// GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
				case 5:
					x = faceWidth + i;
					y = faceHeight + j;
					break;
				}

				memcpy(dst, src + (y * b->w + x) * pixelSize, pixelSize);

				dst += pixelSize;
			}
		}
	}

	return cubemap;
}

#endif

#endif