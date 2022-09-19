#ifndef __CVEC_H__
#define __CVEC_H__

typedef unsigned long long u64;
typedef unsigned char u8;

// + MEM_TAG for allocation tracker +
// |-> (-1) mean no tag was defined |
#define MEM_TAG_CVEC -1

enum
{
  CVEC_CAPACITY         = -1,
  CVEC_SIZE             = -2,
  CVEC_STRIDE           = -3,
  CVEC_FIELD_LENGTH     = 3,
  CVEC_DEFAULT_CAPACITY = 1,
  CVEC_RESIZE_FACTOR    = 2,
};

#define cvec(type) type*

#define cvec_capacity(vec)\
  ((vec) ? ((u64*)vec)[CVEC_CAPACITY] : (u64)0)

#define cvec_size(vec)\
  ((vec) ? ((u64*)vec)[CVEC_SIZE] : (u64)0)

#define cvec_stride(vec)\
  ((vec) ? ((u64*)vec)[CVEC_STRIDE] : (u64)0)

#define cvec_empty(vec)\
  (cvec_size(vec) == 0)

void* _cvec_resize(void* vec);

void* _cvec_push(void* vec, const void* value_ptr);
#define cvec_push(vec, value)do{\
  CVEC_ASSERT(sizeof(TYPEOF(value)) == cvec_stride(vec));\
  TYPEOF(value) temp = value;\
  vec = _cvec_push(vec, &temp);}while(0)

void* _cvec_create(const u64 length, const u64 stride);
#define cvec_create(type)\
  (type*)_cvec_create(CVEC_DEFAULT_CAPACITY, sizeof(type))

void* _cvec_ncreate(const u64 length, const u64 stride, void* ptr_values, b8 single);
#define cvec_ncreate_set(type, size, ptr)\
  (type*)_cvec_ncreate(size, sizeof(type), ptr, true)
#define cvec_ncreate_copy(type, size, ptr)\
  (type*)_cvec_ncreate(size, sizeof(type), ptr, false)
#define cvec_ncreate(type, size)\
  (type*)_cvec_ncreate(size, sizeof(type), NULL, false)

#define cvec_resize(vec, size)do{\
  ((u64*)vec)[CVEC_SIZE] = size;\
  vec = _cvec_resize(vec);}while(0)

void _cvec_destroy(void* vec);
#define cvec_destroy(vec)\
  _cvec_destroy(vec)

void _cvec_pop(void* vec, void* value_ptr);
#define cvec_pop(vec, value_ptr)\
  _cvec_pop(vec, value_ptr)

u64 cvec_find(void* vec, const void* value_ptr);

#endif

/*
* End of API part.
* Lower you will find implementation.
*/

// make vscode happy
#if defined(__INTELLISENSE__)
#define CVEC_IMPLEMENTATION
#endif

#ifdef CVEC_IMPLEMENTATION

#ifdef CVEC_STDLIB
#include <malloc.h>
#include <string.h>
#include <assert.h>

#ifndef CVEC_ALLOCATE
#define CVEC_ALLOCATE(size, mem_tag) malloc(size)
#endif
#ifndef CVEC_FREE
#define CVEC_FREE(ptr, size, mem_tag) free(ptr)
#endif
#ifndef CVEC_MEMSET
#define CVEC_MEMSET(ptr, value, size) memset(ptr, value, size)
#endif
#ifndef CVEC_MEMCPY
#define CVEC_MEMCPY(dst, src, size) memcpy(dst, src, size)
#endif
#ifndef CVEC_MEMCMP
#define CVEC_MEMCMP(b1, b2, size) memcmp(b1, b2, size)
#endif
#ifndef CVEC_ASSERT
#define CVEC_ASSERT(condition) assert(condition)
#endif
#else
#ifndef CVEC_ALLOCATE
#define CVEC_ALLOCATE(size, mem_tag) no_allocate_defined[-1]
#endif
#ifndef CVEC_FREE
#define CVEC_FREE(ptr, size, mem_tag) no_free_defined[-1]
#endif
#ifndef CVEC_MEMSET
#define CVEC_MEMSET(ptr, value, size) no_memset_defined[-1]
#endif
#ifndef CVEC_MEMCPY
#define CVEC_MEMCPY(dst, src, size) no_memcpy_defined[-1]
#endif
#ifndef CVEC_MEMCMP
#define CVEC_MEMCMP(b1, b2, size) no_memcmp_defined[-1]
#endif
#ifndef CVEC_ASSERT
#define CVEC_ASSERT(condition)
#endif
#endif

void* _cvec_create(const u64 length, const u64 stride)
{
  const u64 header_size = CVEC_FIELD_LENGTH * sizeof(u64);
  const u64 vec_size = length * stride;
  u64* new_vec = CVEC_ALLOCATE(header_size + vec_size, MEM_TAG_CVEC);
  CVEC_MEMSET(new_vec, 0, header_size + vec_size);
  new_vec += CVEC_FIELD_LENGTH;
  new_vec[CVEC_CAPACITY] = length;
  new_vec[CVEC_SIZE] = 0;
  new_vec[CVEC_STRIDE] = stride;
  return (void*)new_vec;
}

void* _cvec_ncreate(const u64 length, const u64 stride, void* ptr_values, b8 single)
{
  const u64 header_size = CVEC_FIELD_LENGTH * sizeof(u64);
  const u64 vec_size = length * stride;
  u64* new_vec = CVEC_ALLOCATE(header_size + vec_size, MEM_TAG_CVEC);
  CVEC_MEMSET(new_vec, 0, header_size + vec_size);
  new_vec += CVEC_FIELD_LENGTH;
  new_vec[CVEC_CAPACITY] = length;
  new_vec[CVEC_SIZE] = length;
  new_vec[CVEC_STRIDE] = stride;

  if (ptr_values)
  {
    if (single)
    {
      u8* ptr = (u8*)new_vec;
      for (u64 i = 0; i < length; i++)
      {
        CVEC_MEMCPY((void*)ptr, ptr_values, stride);
        ptr += stride;
      }
    }
    else
    {
      CVEC_MEMCPY((void*)new_vec, ptr_values, vec_size);
    }
  }

  return (void*)new_vec;
}

void _cvec_destroy(void* vec)
{
  CVEC_ASSERT(vec);
  u64* header = (u64*)vec - CVEC_FIELD_LENGTH;
  const u64 header_size = CVEC_FIELD_LENGTH * sizeof(u64);
  const u64 total_size = header_size + header[CVEC_CAPACITY] * header[CVEC_STRIDE];
  CVEC_FREE(header, total_size, MEM_TAG_CVEC);
}

void*
_cvec_push(void* vec,
           const void* value_ptr)
{
  CVEC_ASSERT(vec);
  const u64 length = cvec_size(vec);
  const u64 stride = cvec_stride(vec);
  if (length >= cvec_capacity(vec))
  {
    vec = _cvec_resize(vec);
  }
  u64 addr = (u64)vec;
  addr += (length * stride);
  CVEC_MEMCPY((void*)addr, value_ptr, stride);
  ((u64*)vec)[CVEC_SIZE] = length + 1;
  return vec;
}

void _cvec_pop(void* vec, void* value_ptr)
{
  CVEC_ASSERT(vec);
  CVEC_ASSERT(cvec_size(vec) != 0);
  const u64 length = cvec_size(vec);
  const u64 stride = cvec_stride(vec);
  u64 addr = (u64)vec;
  addr += ((length - 1) * stride);
  ((u64*)vec)[CVEC_SIZE] = length - 1;
  if(value_ptr) CVEC_MEMCPY(value_ptr, (void*)addr, stride);
}

void _cvec_clear(void* vec)
{
  CVEC_ASSERT(vec);
  u64* header = (u64*)vec - CVEC_FIELD_LENGTH;
  const u64 header_size = CVEC_FIELD_LENGTH * sizeof(u64);
  const u64 total_size = header_size + header[CVEC_CAPACITY] * header[CVEC_STRIDE];
  CVEC_MEMSET(header, 0, total_size);
}

void* _cvec_resize(void* vec)
{
  CVEC_ASSERT(vec);
  const u64 length = cvec_size(vec);
  const u64 stride = cvec_stride(vec);
  void* temp = _cvec_create((CVEC_RESIZE_FACTOR *
                            cvec_capacity(vec)),
                            stride);
  CVEC_MEMCPY(temp, vec, length * stride);
  ((u64*)temp)[CVEC_SIZE] = length;
  _cvec_destroy(vec);
  return temp;
}

u64 cvec_find(void* vec, const void* value_ptr)
{
  u64 index = 0;
  for (u8* ptr = vec; ptr; ptr += cvec_stride(vec))
  {
    if (CVEC_MEMCMP(ptr, value_ptr, cvec_stride(vec)) == 0)
    {
      return index;
    }
    index++;
  }
  return index;
}

#endif