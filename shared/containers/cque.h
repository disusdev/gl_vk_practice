#ifndef __CQUE_H__
#define __CQUE_H__

typedef unsigned long long u64;

// + MEM_TAG for allocation tracker +
// |-> (-1) mean no tag was defined |
#define MEM_TAG_CQUE -1

// 4 elements, 3 size, offset 0
// [2][3][1][x]

// pop()

// 4 elements, 2 size, offset 1
// [x][3][1][x]

// push(5)

// 4 elements, 3 size, offset 1
// [x][3][1][5]

// push(7)

// 4 elements, 4 size, offset 1
// [7][3][1][5]

// push(2)

// 8 elements, 5 size, offset 1
// [x][3][1][5][7][2][x][x]
// 4 + 1 % 8

enum
{
  CQUE_CAPACITY         = -1,
  CQUE_SIZE             = -2,
  CQUE_STRIDE           = -3,
  CQUE_OFFSET           = -4,
  CQUE_FIELD_LENGTH     = 4,
  CQUE_DEFAULT_CAPACITY = 1,
  CQUE_RESIZE_FACTOR    = 2,
};

#define cque(type) type*

#define cque_capacity(que)\
  ((que) ? ((u64*)que)[CQUE_CAPACITY] : (u64)0)

#define cque_size(que)\
  ((que) ? ((u64*)que)[CQUE_SIZE] : (u64)0)

#define cque_stride(que)\
  ((que) ? ((u64*)que)[CQUE_STRIDE] : (u64)0)

#define cque_offset(que)\
  ((que) ? ((u64*)que)[CQUE_OFFSET] : (u64)0)

#define cque_front(que)\
  que[((u64*)que)[CQUE_OFFSET]]

#define cque_empty(que)\
  (que_size(que) == 0)

void _cque_at(void* que, u64 index, const void* value_ptr);
#define cque_at(que, index, value_ptr)\
  _cque_at(que, index, value_ptr)

void* _cque_push(void* que, const void* value_ptr);
#define cque_push(que, value)do{\
  CQUE_ASSERT(sizeof(TYPEOF(value)) == cque_stride(que));\
  TYPEOF(value) temp = value;\
  que = _cque_push(que, &temp);}while(0)

void* _cque_create(const u64 length, const u64 stride);
#define cque_create(type)\
  (type*)_cque_create(CQUE_DEFAULT_CAPACITY, sizeof(type))

void _cque_destroy(void* que);
#define cque_destroy(que)\
  _cque_destroy(que)

void _cque_pop(void* que, void* value_ptr);
#define cque_pop(que, value_ptr)\
  _cque_pop(que, value_ptr)

void* _cque_resize(void* que);

#endif

/*
* End of API part.
* Lower you will find implementation.
*/

// make vscode happy
#if defined(__INTELLISENSE__)
#define CQUE_IMPLEMENTATION
#endif

#ifdef CQUE_IMPLEMENTATION

#ifdef CQUE_STDLIB
#include <malloc.h>
#include <string.h>
#include <assert.h>

#ifndef CQUE_ALLOCATE
#define CQUE_ALLOCATE(size, mem_tag) malloc(size)
#endif
#ifndef CQUE_FREE
#define CQUE_FREE(ptr, size, mem_tag) free(ptr)
#endif
#ifndef CQUE_MEMSET
#define CQUE_MEMSET(ptr, value, size) memset(ptr, value, size)
#endif
#ifndef CQUE_MEMCPY
#define CQUE_MEMCPY(src, dst, size) memcpy(src, dst, size)
#endif
#ifndef CQUE_ASSERT
#define CQUE_ASSERT(condition) assert(condition)
#endif
#else
#ifndef CQUE_ALLOCATE
#define CQUE_ALLOCATE(size, mem_tag) no_allocate_defined[-1]
#endif
#ifndef CQUE_FREE
#define CQUE_FREE(ptr, size, mem_tag) no_free_defined[-1]
#endif
#ifndef CQUE_MEMSET
#define CQUE_MEMSET(ptr, value, size) no_memset_defined[-1]
#endif
#ifndef CQUE_MEMCPY
#define CQUE_MEMCPY(src, dst, size) no_memcpy_defined[-1]
#endif
#ifndef CQUE_ASSERT
#define CQUE_ASSERT(condition)
#endif
#endif

void* _cque_create(const u64 length, const u64 stride)
{
  const u64 header_size = CQUE_FIELD_LENGTH * sizeof(u64);
  const u64 que_size = length * stride;
  u64* new_que = CQUE_ALLOCATE(header_size + que_size, MEM_TAG_CQUE);
  CQUE_MEMSET(new_que, 0, header_size + que_size);
  new_que += CQUE_FIELD_LENGTH;
  new_que[CQUE_CAPACITY] = length;
  new_que[CQUE_SIZE] = 0;
  new_que[CQUE_STRIDE] = stride;
  new_que[CQUE_OFFSET] = 0;
  return (void*)new_que;
}

void _cque_destroy(void* que)
{
  CQUE_ASSERT(que);
  u64* header = (u64*)que - CQUE_FIELD_LENGTH;
  const u64 header_size = CQUE_FIELD_LENGTH * sizeof(u64);
  const u64 total_size = header_size + header[CQUE_CAPACITY] * header[CQUE_STRIDE];
  CQUE_FREE(header, total_size, MEM_TAG_CQUE);
}

void*
_cque_push(void* que,
           const void* value_ptr)
{
  CQUE_ASSERT(que);
  const u64 length = cque_size(que);
  const u64 stride = cque_stride(que);
  const u64 offset = cque_offset(que);
  if (length >= cque_capacity(que))
  {
    que = _cque_resize(que);
  }
  u64 addr = (u64)que;
  addr += (((length + offset) % cque_capacity(que)) * stride);
  CQUE_MEMCPY((void*)addr, value_ptr, stride);
  ((u64*)que)[CQUE_SIZE] = length + 1;
  return que;
}

void _cque_pop(void* que, void* value_ptr)
{
  CQUE_ASSERT(que);
  CQUE_ASSERT(cque_size(que) != 0);
  const u64 length = cque_size(que);
  const u64 stride = cque_stride(que);
  const u64 offset = cque_offset(que);
  u64 addr = (u64)que;
  addr += (offset * stride);
  ((u64*)que)[CQUE_SIZE] = length - 1;
  ((u64*)que)[CQUE_OFFSET] = (offset + 1) % cque_capacity(que);
  if(value_ptr) CQUE_MEMCPY(value_ptr, (void*)addr, stride);
}

void _cque_clear(void* que)
{
  CQUE_ASSERT(que);
  u64* header = (u64*)que - CQUE_FIELD_LENGTH;
  const u64 header_size = CQUE_FIELD_LENGTH * sizeof(u64);
  const u64 total_size = header_size + header[CQUE_CAPACITY] * header[CQUE_STRIDE];
  CQUE_MEMSET(header, 0, total_size);
}

void* _cque_resize(void* que)
{
  CQUE_ASSERT(que);
  const u64 length = cque_size(que);
  const u64 stride = cque_stride(que);
  const u64 offset = cque_offset(que);
  void* temp = _cque_create((CQUE_RESIZE_FACTOR *
                            cque_capacity(que)),
                            stride);
  CQUE_MEMCPY(((char*)temp) + cque_capacity(que) * stride, que, offset * stride);
  CQUE_MEMCPY(((char*)temp) + offset * stride, ((char*)que) + offset * stride, (length - offset) * stride);
  ((u64*)temp)[CQUE_SIZE] = length;
  ((u64*)temp)[CQUE_OFFSET] = offset;
  _cque_destroy(que);
  return temp;
}

void _cque_at(void* que, u64 index, const void* value_ptr)
{
  CQUE_ASSERT(que);
  const u64 length = cque_size(que);
  const u64 stride = cque_stride(que);
  const u64 offset = cque_offset(que);

  u64 new_inedx = (offset + index) % length;

  u64 addr = (u64)que;
  addr += (new_inedx * stride);

  if(value_ptr) CQUE_MEMCPY(value_ptr, (void*)addr, stride);
}

#endif