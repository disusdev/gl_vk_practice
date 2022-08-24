#ifndef __MEM_H__
#define __MEM_H__

#include "../defines.h"

typedef enum
mem_tag
{
  MEM_TAG_NONE,
  MEM_TAG_GAME,
  MEM_TAG_APP,
  MEM_TAG_LINEAR_ALLOCATOR,
  MEM_TAG_DARRAY,
  MEM_TAG_STRING,
  
  MEM_TAG_COUNT
}
mem_tag;

typedef struct
mem_system_cfg
{
  u64 total_alloc_size;
}
mem_system_cfg;

void
mem_system_init(mem_system_cfg config);

void
mem_system_term();

void*
mem_alloc(u64 size,
          mem_tag tag);

void
mem_free(void* block,
         u64 size,
         mem_tag tag);

void*
mem_zero(void* block,
         u64 size);

void*
mem_copy(void* dst,
         const void* src,
         u64 size);

void*
mem_set(void* dst,
        i32 value,
        u64 size);

//char*
//get_mem_usage_str();
//
//u64
//get_mem_alloc_count();

/*
 *
 * Stop now, if you are only interested in the API.
 * Below, you find the implementation.
 *
 */

#ifdef MEM_IMPLEMENTATION

struct
mem_stats
{
  u64 total_allocated;
  u64 tagged_allocations[MEM_TAG_COUNT];
};

typedef struct
mem_system_state
{
  mem_system_cfg config;
  struct mem_stats stats;
  u64 alloc_count;
  u64 allocator_mem_req;
  dynamic_allocator allocator;
  void* allocator_block;
}
mem_system_state;

static mem_system_state* state_ptr;

void
mem_system_init(mem_system_cfg config)
{
  u64 state_mem_req = sizeof(mem_system_state);
  
  u64 alloc_req = 0;
  dynamic_allocator_create(config.total_alloc_size, &alloc_req, 0, 0);
  
  void* block = platform_mem_allocate(state_mem_req + alloc_req, false);
  // ASSERT(block)
  
  state_ptr = (mem_system_state*)block;
  state_ptr->config = config;
  state_ptr->alloc_count = 0;
  state_ptr->allocator_mem_req = alloc_req;
  platform_mem_zero(&state_ptr->stats, sizeof(state_ptr->stats));
  state_ptr->allocator_block = ((u8*)block + state_mem_req);
  
  dynamic_allocator_create(config.total_alloc_size,
                           &state_ptr->allocator_mem_req,
                           state_ptr->allocator_block,
                           &state_ptr->allocator);
}

#endif

#endif