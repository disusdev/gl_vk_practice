#ifndef __HASHTABLE_H__
#define __HASHTABLE_H__

#include <defines.h>

typedef struct
hashtable
{
  u64 element_size;
  u32 element_count;
  b8 is_pointer_type;
  void* memory;
}
hashtable;

LIB_API void
hashtable_create(u64 element_size,
                 u32 element_count,
                 void* memory,
                 b8 is_pointer_type,
                 hashtable* out_hashtable);

LIB_API void
hashtable_destroy(hashtable* table);

LIB_API void
hashtable_set(hashtable* table,
              const char* name,
              void* value);

LIB_API void
hashtable_set_ptr(hashtable* table,
                  const char* name,
                  void** value);

LIB_API void
hashtable_get(hashtable* table,
              const char* name,
              void* out_value);

LIB_API void
hashtable_get_ptr(hashtable* table,
                  const char* name,
                  void** out_value);

LIB_API void
hashtable_fill(hashtable* table,
               void* value);

#endif
