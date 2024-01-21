#include "chtb.h"

u64 get_hash(const char* name, u32 element_count)
{
  u64 hash = 0;
  i32 c;
  while(c = *name++)
  {
    hash = c + (hash << 6) + (hash << 16) - hash;
  }
  
  hash %= element_count;
  
  return hash;
}

void
hashtable_create(u64 element_size,
                 u32 element_count,
                 void* memory,
                 b8 is_pointer_type,
                 hashtable* out_hashtable)
{
  // ASSERT(memory)
  // ASSERT(out_hashtable)
  // ASSERT(element_count)
  // ASSERT(element_size)
  
  out_hashtable->memory = memory;
  out_hashtable->element_count = element_count;
  out_hashtable->element_size = element_size;
  out_hashtable->is_pointer_type = is_pointer_type;
  memset(out_hashtable->memory, 0, element_size * element_count);
}

void
hashtable_destroy(hashtable* table)
{
  // ASSERT(table)
  memset(table, 0, sizeof(hashtable));
}

void
hashtable_set(hashtable* table,
              const char* name,
              void* value)
{
  // ASSERT(table)
  // ASSERT(name)
  // ASSERT(value)
  // ASSERT(table->is_pointer_type)
  
  u64 hash = get_hash(name, table->element_count);
  memcpy(table->memory + (table->element_size * hash),
         value,
         table->element_size);
}

void
hashtable_set_ptr(hashtable* table,
                  const char* name,
                  void** value)
{
  // ASSERT(table)
  // ASSERT(name)
  // ASSERT(table->is_pointer_type)
  
  u64 hash = get_hash(name, table->element_count);
  ((void**)table->memory)[hash] = value ? *value : 0;
}

void
hashtable_get(hashtable* table,
              const char* name,
              void* out_value)
{
  // ASSERT(table)
  // ASSERT(name)
  // ASSERT(out_value)
  // ASSERT(table->is_pointer_type)
  
  u64 hash = get_hash(name, table->element_count);
  memcpy(out_value,
         table->memory + (table->element_size * hash),
         table->element_size);
}

void
hashtable_get_ptr(hashtable* table,
                  const char* name,
                  void** out_value)
{
  // ASSERT(table)
  // ASSERT(name)
  // ASSERT(out_value)
  // ASSERT(table->is_pointer_type)
  
  u64 hash = get_hash(name, table->element_count);

  if (hash < table->element_count)
  {
    *out_value = ((void**)table->memory)[hash];
  }
}

void
hashtable_fill(hashtable* table,
               void* value)
{
  // ASSERT(table)
  // ASSERT(value)
  // ASSERT(table->is_pointer_type)
  
  for (u32 i = 0; i < table->element_count; i++)
  {
    memcpy(table->memory + (table->element_size * i),
           value,
           table->element_size);
  }
}
