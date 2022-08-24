#ifndef __CLST_H__
#define __CLST_H__

typedef unsigned long long u64;
typedef _Bool b8;

// + MEM_TAG for allocation tracker +
// |-> (-1) mean no tag was defined |
#define MEM_TAG_CLST -1

typedef struct
t_clst_node
{
  void* data;
  struct t_clst_node* prev;
  struct t_clst_node* next;
}
t_clst_node;

typedef struct
t_clst
{
  u64 size;
  u64 stride;
  t_clst_node* begin;
  t_clst_node* end;
}
t_clst;

t_clst _clst_create(const u64 stride);
#define clst_create(type)\
  _clst_create(sizeof(type))

void _clst_push(t_clst* lst, const void* value_ptr);
#define clst_push(lst, value)do{\
  typeof(value) temp = value;\
  _clst_push(lst, &temp);}while(0)

void _clst_destroy(t_clst* lst);
#define clst_destroy(lst)\
  _clst_destroy(lst)

t_clst_node* clst_find(t_clst* lst, const void* value_ptr);

#endif

/*
* End of API part.
* Lower you will find implementation.
*/

// make vscode happy
#if defined(__INTELLISENSE__)
#define CLST_IMPLEMENTATION
#endif

#ifdef CLST_IMPLEMENTATION

#ifdef CLST_STDLIB
#include <malloc.h>
#include <string.h>
#include <assert.h>

#ifndef CLST_ALLOCATE
#define CLST_ALLOCATE(size, mem_tag) malloc(size)
#endif
#ifndef CLST_FREE
#define CLST_FREE(ptr, size, mem_tag) free(ptr)
#endif
#ifndef CLST_MEMSET
#define CLST_MEMSET(ptr, value, size) memset(ptr, value, size)
#endif
#ifndef CLST_MEMCPY
#define CLST_MEMCPY(src, dst, size) memcpy(src, dst, size)
#endif
#ifndef CLST_MEMCMP
#define CLST_MEMCMP(b1, b2, size) memcmp(b1, b2, size)
#endif
#ifndef CLST_ASSERT
#define CLST_ASSERT(condition) assert(condition)
#endif
#else
#ifndef CLST_ALLOCATE
#define CLST_ALLOCATE(size, mem_tag) no_allocate_defined[-1]
#endif
#ifndef CLST_FREE
#define CLST_FREE(ptr, size, mem_tag) no_free_defined[-1]
#endif
#ifndef CLST_MEMSET
#define CLST_MEMSET(ptr, value, size) no_memset_defined[-1]
#endif
#ifndef CLST_MEMCPY
#define CLST_MEMCPY(src, dst, size) no_memcpy_defined[-1]
#endif
#ifndef CLST_MEMCMP
#define CLST_MEMCMP(b1, b2, size) no_memcmp_defined[-1]
#endif
#ifndef CLST_ASSERT
#define CLST_ASSERT(condition)
#endif
#endif

t_clst _clst_create(const u64 stride)
{
  t_clst lst;
  CLST_MEMSET(&lst, 0, sizeof(t_clst));
  lst.stride = stride;

  lst.begin = CLST_ALLOCATE(sizeof(t_clst_node), MEM_TAG_CLST);
  CLST_MEMSET(lst.begin, 0, sizeof(t_clst_node));
  lst.end = lst.begin;

  return lst;
}

void _clst_destroy(t_clst* lst)
{
  for (t_clst_node* it = lst->begin; it != lst->end; it = it->next)
  {
    CLST_FREE(it->data, lst->stride, MEM_TAG_CLST);
  }

  t_clst_node* node = lst->end;
  while (node != NULL)
  {
    t_clst_node* node_to_delete = node;
    node = node->prev;
    CLST_FREE(node_to_delete, sizeof(t_clst_node), MEM_TAG_CLST);
  }

  CLST_MEMSET(&lst, 0, sizeof(t_clst));
}

void _clst_push(t_clst* lst, const void* value_ptr)
{
  t_clst_node* node = lst->end;

  node->data = CLST_ALLOCATE(lst->stride, MEM_TAG_CLST);
  CLST_MEMCPY(node->data, value_ptr, lst->stride);

  lst->end = CLST_ALLOCATE(sizeof(t_clst_node), MEM_TAG_CLST);
  CLST_MEMSET(lst->end, 0, sizeof(t_clst_node));

  node->next = lst->end;
  lst->end->prev = node;
}

t_clst_node* clst_find(t_clst* lst, const void* value_ptr)
{
  for (t_clst_node* it = lst->begin; it != lst->end; it = it->next)
  {
    if (CLST_MEMCMP(value_ptr, it->data, lst->stride) == 0)
    {
      return it;
    }
  }
  return lst->end;
}

#endif