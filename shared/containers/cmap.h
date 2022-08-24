#ifndef __CMAP_H__
#define __CMAP_H__

#include "cvec.h"

typedef unsigned long long u64;
typedef unsigned char u8;

typedef struct
t_pair
{
  u64 key;
  void* value;
}
t_pair;

typedef struct
t_cmap
{
  cvec(u64) keys;
  void* values;

  //u64 stride;
  u64 size;
}
t_cmap;

t_cmap _cmap_create(u64 stride)
{
  t_cmap map;
  memset(&map, 0, sizeof(t_cmap));
  //map.stride = stride;
  map.keys = cvec_create(u64);
  map.values = _cvec_create(CVEC_DEFAULT_CAPACITY, stride);

  return map;
}

void _cmap_insert(t_cmap* map, u64 key, const void* value_ptr)
{
  u64 found_key = cvec_find(map->keys, &key);
  if (found_key == cvec_size(map->keys))
  {
    cvec_push(map->keys, key);
    map->values = _cvec_push(map->values, value_ptr);
    map->size += 1;
  }
  else
  {
    u8* ptr = (u8*)map->values;
    ptr += found_key * cvec_stride(map->values);
    CVEC_MEMCPY(ptr, value_ptr, cvec_stride(map->values));
  }
}

#endif