#include "mathm.h"

#include <math.h>

// set rand seed function
// static u8 rand_seeded = false;

#define BIT_NOSIE1 0xB5297A4D;
#define BIT_NOSIE2 0x68E31DA4;
#define BIT_NOSIE3 0x1B56C4E9;

u32
math_rng(i32 pos,
         u32 seed)
{
  u32 mangled = pos;
  mangled *= BIT_NOSIE1;
  mangled += seed;
  mangled ^= (mangled >> 8);
  mangled += BIT_NOSIE2;
  mangled ^= (mangled << 8);
  mangled *= BIT_NOSIE3;
  mangled ^= (mangled >> 8);
  return mangled;
}

i32
math_rng_range(i32 min,
               i32 max,
               i32 pos,
               u32 seed)
{
  int range = max - min + 1;
  return math_rng(pos, seed) % range + min;
}

float
math_sin(float x)
{
  return sinf(x);
}

float
math_cos(float x)
{
  return cosf(x);
}

float
math_tan(float x)
{
  return tanf(x);
}

float
math_atan2(float x, float y)
{
  return atan2f(x, y);
}

float
math_hypot(float p, float b)
{
  return hypotf(p, b);
}

float
math_floor(float f)
{
  return floorf(f);
}

float
math_acos(float x)
{
  return acosf(x);
}

float
math_sqrt(float x)
{
  return sqrtf(x);
}

float
math_abs(float x)
{
  return fabsf(x);
}