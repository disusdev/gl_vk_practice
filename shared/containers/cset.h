#ifndef __CSET_H__
#define __CSET_H__

#if !defined(__CLST_H__)
#include "clst.h"
#endif

typedef t_clst t_cset;
typedef t_clst_node t_cset_node;

#define cset_create(type)\
  clst_create(type)

#define cset_destroy(set)\
  clst_destroy(set)

#define cset_push(set, value)\
  if (clst_find(set, &value) == (set)->end)\
    clst_push(set, value)

#endif