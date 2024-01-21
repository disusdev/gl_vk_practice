#ifndef __MATH_TYPES_H__
#define __MATH_TYPES_H__

#include "../defines.h"

typedef union
vec2_u
{
  f32 data[2];
  struct
  {
    union
    {
      f32 x, r, s, u;
    };
    union
    {
      f32 y, g, t, v;
    };
  };
}
vec2;

typedef union
vec3_u
{
  f32 data[3];
  struct
  {
    union
    {
      f32 x, r, s, u;
    };
    union
    {
      f32 y, g, t, v;
    };
    union
    {
      f32 z, b, p, w;
    };
  };
}
vec3;

typedef union
vec4_u
{
  f32 data[4];
  struct
  {
    union
    {
      f32 x, r, s;
    };
    union
    {
      f32 y, g, t;
    };
    union
    {
      f32 z, b, p;
    };
    union
    {
      f32 w, a, q;
    };
  };
}
vec4;

typedef vec4 quat;

typedef union
mat4_u
{
  f32 data[16];
}
mat4;

typedef struct
vertex_3d
{
  vec3 position;
  vec3 normal;
  vec2 texcoord;
  vec4 color;
  vec4 tangent;
}
vertex_3d;

typedef struct
vertex_2d
{
  vec2 position;
  vec2 texcoord;
}
vertex_2d;

typedef struct
transform
{
  vec3 position;
  quat rotation;
  vec3 scale;
  b8 is_dirty;
  mat4 local;
  struct transform* parent;
}
transform;

typedef struct
bbox
{
  vec3 min;
  vec3 max;
}
bbox;

#endif
