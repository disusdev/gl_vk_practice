#ifndef __MATHM_H__
#define __MATHM_H__

#include "../defines.h"
#include "math_types.h"

#undef near
#undef far

// #include "core/mem.h"

#define PI 3.14159265358979323846f
#define PI_2 2.0f * PI
#define HALF_PI 0.5f * PI
#define QUARTER_PI 0.25f * PI
#define ONE_OVER_PI 1.0f / PI
#define ONE_OVER_TWO_PI 1.0f / PI_2

#define SQRT_TWO 1.41421356237309504880f
#define SQRT_THREE 1.73205080756887729352f
#define SQRT_ONE_OVER_TWO 0.70710678118654752440f
#define SQRT_ONE_OVER_THREE 0.57735026918962576450f

#define DEG2RAD PI / 180.0f
#define RAD2DEG 180.0f / PI

// #define SEC_TO_MS_MULTIPLIER 1000.0f
// #define MS_TO_SEC_MULTIPLIER 0.001f

// A huge number that should be larger than any valid number used.
#define M_INF 1e30f

// Smallest positive number where 1.0 + FLOAT_EPSILON != 0
#define M_EPS 1.192092896e-07f

LIB_API float math_sin(float x);
LIB_API float math_cos(float x);
LIB_API float math_tan(float x);
LIB_API float math_acos(float x);
LIB_API float math_sqrt(float x);
LIB_API float math_abs(float x);

LIB_INLINE u8 is_power_of_2(u64 value)
{
  return (value != 0) && ((value & (value - 1)) == 0);
}

LIB_API u32 math_rng(i32 pos, u32 seed);
LIB_API i32 math_rng_range(i32 min, i32 max, i32 pos, u32 seed);

// LIB_API i32 math_rnd();
// LIB_API i32 math_rnd_in_range(i32 min, i32 max);

// LIB_API float math_frnd();
// LIB_API float math_frnd_in_range(float min, float max);

LIB_INLINE vec2 vec2_create(float x, float y)
{
  vec2 out_vec;
  out_vec.x = x;
  out_vec.y = y;
  return out_vec;
}

LIB_INLINE vec2 vec2_zero()
{
  return (vec2) { { 0.0f, 0.0f } };
}

LIB_INLINE vec2 vec2_one()
{
  return (vec2) { { 1.0f, 1.0f } };
}

LIB_INLINE vec2 vec2_up()
{
  return (vec2) { { 0.0f, 1.0f } };
}

LIB_INLINE vec2 vec2_down()
{
  return (vec2) { { 0.0f, -1.0f } };
}

LIB_INLINE vec2 vec2_left()
{
  return (vec2) { { -1.0f, 0.0f } };
}

LIB_INLINE vec2 vec2_right()
{
  return (vec2) { {1.0f, 0.0f } };
}

LIB_INLINE vec2 vec2_add(vec2 vec_0, vec2 vec_1)
{
  return (vec2) { { vec_0.x + vec_1.x, vec_0.y + vec_1.y } };
}

LIB_INLINE vec2 vec2_sub(vec2 vec_0, vec2 vec_1)
{
  return (vec2) { { vec_0.x - vec_1.x, vec_0.y - vec_1.y } };
}

LIB_INLINE vec2 vec2_mul(vec2 vec_0, vec2 vec_1)
{
  return (vec2) { { vec_0.x* vec_1.x, vec_0.y* vec_1.y } };
}

LIB_INLINE vec2 vec2_div(vec2 vec_0, vec2 vec_1)
{
  return (vec2) { { vec_0.x / vec_1.x, vec_0.y / vec_1.y } };
}

LIB_INLINE float vec2_length_squared(vec2 vec)
{
  return vec.x * vec.x + vec.y * vec.y;
}

LIB_INLINE float vec2_length(vec2 vec)
{
  return math_sqrt(vec2_length_squared(vec));
}

LIB_INLINE void vec2_normalize(vec2* vec)
{
  const float length = vec2_length(*vec);
  vec->x /= length;
  vec->y /= length;
}

LIB_INLINE vec2 vec2_normalized(vec2 vec)
{
  vec2_normalize(&vec);
  return vec;
}

LIB_INLINE u8 vec2_compare(vec2 vec_0, vec2 vec_1, float tolerance)
{
  if (math_abs(vec_0.x - vec_1.x) > tolerance)
  {
    return false;
  }
  
  if (math_abs(vec_0.y - vec_1.y) > tolerance)
  {
    return false;
  }
  
  return true;
}

LIB_INLINE float vec2_distance(vec2 vec_0, vec2 vec_1)
{
  vec2 d = vec2_sub(vec_0, vec_1);
  return vec2_length(d);
}

LIB_INLINE vec3 vec3_create(float x, float y, float z)
{
  return (vec3) { { x, y, z } };
}

LIB_INLINE vec3 vec3_from_vec4(vec4 vec)
{
  return (vec3) { { vec.x, vec.y, vec.z } };
}

LIB_INLINE vec4 vec3_to_vec4(vec3 vec, float w)
{
  return (vec4) { {vec.x, vec.y, vec.z, w } };
}

LIB_INLINE vec3 vec3_zero()
{
  return (vec3) { {0.0f, 0.0f, 0.0f } };
}

LIB_INLINE vec3 vec3_one()
{
  return (vec3) { { 1.0f, 1.0f, 1.0f } };
}

LIB_INLINE vec3 vec3_up()
{
  return (vec3) { {0.0f, 1.0f, 0.0f } };
}

LIB_INLINE vec3 vec3_down()
{
  return (vec3) { {0.0f, -1.0f, 0.0f } };
}

LIB_INLINE vec3 vec3_left()
{
  return (vec3) { { -1.0f, 0.0f, 0.0f } };
}

LIB_INLINE vec3 vec3_right()
{
  return (vec3) { { 1.0f, 0.0f, 0.0f } };
}

LIB_INLINE vec3 vec3_forward()
{
  return (vec3) { { 0.0f, 0.0f, -1.0f } };
}

LIB_INLINE vec3 vec3_back()
{
  return (vec3) { {0.0f, 0.0f, 1.0f } };
}

LIB_INLINE vec3 vec3_add(vec3 vec_0, vec3 vec_1)
{
  return (vec3) { { vec_0.x + vec_1.x, vec_0.y + vec_1.y, vec_0.z + vec_1.z } };
}

LIB_INLINE vec3 vec3_sub(vec3 vec_0, vec3 vec_1)
{
  return (vec3) { { vec_0.x - vec_1.x, vec_0.y - vec_1.y, vec_0.z - vec_1.z } };
}

LIB_INLINE vec3 vec3_mul(vec3 vec_0, vec3 vec_1)
{
  return (vec3) { { vec_0.x* vec_1.x, vec_0.y* vec_1.y, vec_0.z* vec_1.z } };
}

LIB_INLINE vec3 vec3_mul_scalar(vec3 vec_0, float scalar)
{
  return (vec3) { { vec_0.x* scalar, vec_0.y* scalar, vec_0.z* scalar } };
}

LIB_INLINE vec3 vec3_div(vec3 vec_0, vec3 vec_1)
{
  return (vec3) { { vec_0.x / vec_1.x, vec_0.y / vec_1.y, vec_0.z / vec_1.z } };
}

LIB_INLINE float vec3_length_squared(vec3 vec)
{
  return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z;
}

LIB_INLINE float vec3_length(vec3 vec)
{
  return math_sqrt(vec3_length_squared(vec));
}

LIB_INLINE void vec3_normalize(vec3* vec)
{
  const float length = vec3_length(*vec);
  vec->x /= length;
  vec->y /= length;
  vec->z /= length;
}

LIB_INLINE vec3 vec3_normalized(vec3 vec)
{
  vec3_normalize(&vec);
  return vec;
}

LIB_INLINE float vec3_dot(vec3 vec_0, vec3 vec_1)
{
  float p = 0.0f;
  p += vec_0.x * vec_1.x;
  p += vec_0.y * vec_1.y;
  p += vec_0.z * vec_1.z;
  return p;
}

LIB_INLINE vec3 vec3_cross(vec3 vec_0, vec3 vec_1)
{
  return (vec3) {
    { vec_0.y* vec_1.z - vec_0.z * vec_1.y,
      vec_0.z* vec_1.x - vec_0.x * vec_1.z,
      vec_0.x* vec_1.y - vec_0.y * vec_1.x }
  };
}

LIB_INLINE u8 vec3_compare(vec3 vec_0, vec3 vec_1, float tolerance)
{
  if (math_abs(vec_0.x - vec_1.x) > tolerance)
  {
    return false;
  }
  
  if (math_abs(vec_0.y - vec_1.y) > tolerance)
  {
    return false;
  }
  
  if (math_abs(vec_0.z - vec_1.z) > tolerance)
  {
    return false;
  }
  
  return true;
}

LIB_INLINE float vec3_distance(vec3 vec_0, vec3 vec_1)
{
  vec3 d = vec3_sub(vec_0, vec_1);
  return vec3_length(d);
}

LIB_INLINE vec4 vec4_create(float x, float y, float z, float w)
{
  vec4 out_vec;
#if defined(USE_SIMD)
  out_vec.data_ = _mm_set_ps(x, y, z, w);
#else
  out_vec.x = x;
  out_vec.y = y;
  out_vec.z = z;
  out_vec.w = w;
#endif
  return out_vec;
}

LIB_INLINE vec3 vec4_to_vec3(vec4 vec)
{
  return (vec3) { { vec.x, vec.y, vec.z } };
}

LIB_INLINE vec4 vec4_from_vec3(vec3 vec, float w)
{
#if defined(USE_SIMD)
  vec4 out_vec;
  out_vec.data_ = _mm_set_ps(vec.x, vec.y, vec.z, w);
  return out_vec;
#else
  return (vec4) { { vec.x, vec.y, vec.z, w } };
#endif
}

LIB_INLINE vec4 vec4_zero()
{
  return (vec4) { {0.0f, 0.0f, 0.0f, 0.0f } };
}

LIB_INLINE vec4 vec4_one()
{
  return (vec4) { {1.0f, 1.0f, 1.0f, 1.0f } };
}

LIB_INLINE vec4 vec4_add(vec4 vec_0, vec4 vec_1)
{
  vec4 res;
  for (u64 i = 0; i < 4; i++)
  {
    res.data[i] = vec_0.data[i] + vec_1.data[i];
  }
  return res;
}

LIB_INLINE vec4 vec4_sub(vec4 vec_0, vec4 vec_1)
{
  vec4 res;
  for (u64 i = 0; i < 4; i++)
  {
    res.data[i] = vec_0.data[i] - vec_1.data[i];
  }
  return res;
}

LIB_INLINE vec4 vec4_mul(vec4 vec_0, vec4 vec_1)
{
  vec4 res;
  for (u64 i = 0; i < 4; i++)
  {
    res.data[i] = vec_0.data[i] * vec_1.data[i];
  }
  return res;
}

LIB_INLINE vec4 vec4_div(vec4 vec_0, vec4 vec_1)
{
  vec4 res;
  for (u64 i = 0; i < 4; i++)
  {
    res.data[i] = vec_0.data[i] / vec_1.data[i];
  }
  return res;
}

LIB_INLINE float vec4_length_squared(vec4 vec)
{
  return vec.x * vec.x + vec.y * vec.y + vec.z * vec.z + vec.w * vec.w;
}

LIB_INLINE float vec4_length(vec4 vec)
{
  return math_sqrt(vec4_length_squared(vec));
}

LIB_INLINE void vec4_normalize(vec4* vec)
{
  const float length = vec4_length(*vec);
  vec->x /= length;
  vec->y /= length;
  vec->z /= length;
  vec->w /= length;
}

LIB_INLINE vec4 vec4_normalized(vec4 vec)
{
  vec4_normalize(&vec);
  return vec;
}

LIB_INLINE float vec4_dot_f32(float a0, float a1, float a2, float a3,
                              float b0, float b1, float b2, float b3)
{
  float p = a0 * b0 +
    a1 * b1 +
    a2 * b2 +
    a3 * b3;
  return p;
}

LIB_INLINE mat4 mat4_identity()
{
  mat4 out_mat;
  // mem_zero(out_mat.data, sizeof(float) * 16);
  memset(out_mat.data, 0, sizeof(float) * 16);
  out_mat.data[0] = 1.0f;
  out_mat.data[5] = 1.0f;
  out_mat.data[10] = 1.0f;
  out_mat.data[15] = 1.0f;
  return out_mat;
}

LIB_INLINE mat4 mat4_mul(mat4 mat_0, mat4 mat_1)
{
  mat4 out_mat = mat4_identity();
  
  const float* m0_ptr = mat_0.data;
  const float* m1_ptr = mat_1.data;
  float* dst_ptr = out_mat.data;
  
  for (i32 i = 0; i < 4; i++)
  {
    for (i32 j = 0; j < 4; j++)
    {
      *dst_ptr =
        m0_ptr[0] * m1_ptr[0 + j] +
        m0_ptr[1] * m1_ptr[4 + j] +
        m0_ptr[2] * m1_ptr[8 + j] +
        m0_ptr[3] * m1_ptr[12 + j];
      dst_ptr++;
    }
    m0_ptr += 4;
  }
  
  return out_mat;
}

LIB_INLINE mat4 mat4_ortho(float left, float right, float bottom, float top, float near, float far)
{
  mat4 out_mat = mat4_identity();
  
  float lr = 1.0f / (left - right);
  float bt = 1.0f / (bottom - top);
  float nf = 1.0f / (near - far);
  
  out_mat.data[0] = -2.0f * lr;
  out_mat.data[5] = -2.0f * bt;
  out_mat.data[10] = -2.0f * nf;
  
  out_mat.data[12] = (left + right) * lr;
  out_mat.data[13] = (bottom + top) * bt;
  out_mat.data[14] = (near + far) * nf;
  return out_mat;
}

LIB_INLINE mat4 mat4_persp(float fov, float aspect, float near, float far)
{
  float half_tan_fov = math_tan(fov * 0.5f);
  mat4 out_mat;
  //mem_zero(out_mat.data, sizeof(float) * 16);
  memset(out_mat.data, 0, sizeof(float) * 16);
  out_mat.data[0] = 1.0f / (aspect * half_tan_fov);
  out_mat.data[0] = 1.0f / half_tan_fov;
  out_mat.data[0] = -((far + near) / (far - near));
  out_mat.data[0] = -1.0f;
  out_mat.data[0] = -((1.0f * far * near) / (far - near));
  return out_mat;
}

LIB_INLINE mat4 mat4_look_at(vec3 pos, vec3 target, vec3 up)
{
  mat4 out_mat;
  vec3 z_axis;
  z_axis.x = target.x - pos.x;
  z_axis.y = target.y - pos.y;
  z_axis.z = target.z - pos.z;
  
  z_axis = vec3_normalized(z_axis);
  vec3 x_axis = vec3_normalized(vec3_cross(z_axis, up));
  vec3 y_axis = vec3_cross(x_axis, z_axis);
  
  out_mat.data[0] = x_axis.x;
  out_mat.data[1] = y_axis.x;
  out_mat.data[2] = -z_axis.x;
  out_mat.data[3] = 0.0f;
  out_mat.data[4] = x_axis.y;
  out_mat.data[5] = y_axis.y;
  out_mat.data[6] = -z_axis.y;
  out_mat.data[7] = 0.0f;
  out_mat.data[8] = x_axis.z;
  out_mat.data[9] = y_axis.z;
  out_mat.data[10] = -z_axis.z;
  out_mat.data[11] = 0.0f;
  out_mat.data[12] = -vec3_dot(x_axis, pos);
  out_mat.data[13] = -vec3_dot(y_axis, pos);
  out_mat.data[14] = vec3_dot(z_axis, pos);
  out_mat.data[15] = 1.0f;
  
  return out_mat;
}

LIB_INLINE mat4 mat4_transposed(mat4 mat)
{
  mat4 out_mat = mat4_identity();
  out_mat.data[0] = mat.data[0];
  out_mat.data[1] = mat.data[4];
  out_mat.data[2] = mat.data[8];
  out_mat.data[3] = mat.data[12];
  out_mat.data[4] = mat.data[1];
  out_mat.data[5] = mat.data[5];
  out_mat.data[6] = mat.data[9];
  out_mat.data[7] = mat.data[13];
  out_mat.data[8] = mat.data[2];
  out_mat.data[9] = mat.data[6];
  out_mat.data[10] = mat.data[10];
  out_mat.data[11] = mat.data[14];
  out_mat.data[12] = mat.data[3];
  out_mat.data[13] = mat.data[7];
  out_mat.data[14] = mat.data[11];
  out_mat.data[15] = mat.data[15];
  return out_mat;
}

LIB_INLINE mat4 mat4_inverse(mat4 mat)
{
  const float* m = mat.data;
  
  float t0 = m[10] * m[15];
  float t1 = m[14] * m[11];
  float t2 = m[6] * m[15];
  float t3 = m[14] * m[7];
  float t4 = m[6] * m[11];
  float t5 = m[10] * m[7];
  float t6 = m[2] * m[15];
  float t7 = m[14] * m[3];
  float t8 = m[2] * m[11];
  float t9 = m[10] * m[3];
  float t10 = m[2] * m[7];
  float t11 = m[6] * m[3];
  float t12 = m[8] * m[13];
  float t13 = m[12] * m[9];
  float t14 = m[4] * m[13];
  float t15 = m[12] * m[5];
  float t16 = m[4] * m[9];
  float t17 = m[8] * m[5];
  float t18 = m[0] * m[13];
  float t19 = m[12] * m[1];
  float t20 = m[0] * m[9];
  float t21 = m[8] * m[1];
  float t22 = m[0] * m[5];
  float t23 = m[4] * m[1];
  
  mat4 out_mat;
  float* o = out_mat.data;
  
  o[0] = (t0 * m[5] + t3 * m[9] + t4 * m[13]) - (t1 * m[5] + t2 * m[9] + t5 * m[13]);
  o[1] = (t1 * m[1] + t6 * m[9] + t9 * m[13]) - (t0 * m[1] + t7 * m[9] + t8 * m[13]);
  o[2] = (t2 * m[1] + t7 * m[5] + t10 * m[13]) - (t3 * m[1] + t6 * m[5] + t11 * m[13]);
  o[3] = (t3 * m[1] + t8 * m[5] + t11 * m[9]) - (t4 * m[1] + t9 * m[5] + t10 * m[9]);
  
  float d = 1.0f / (m[0] * o[0] + m[4] * o[1] + m[8] * o[2] + m[12] * o[3]);
  
  o[0] = d * o[0];
  o[1] = d * o[1];
  o[2] = d * o[2];
  o[3] = d * o[3];
  o[4]  = d * ((t1  * m[4]  + t2  * m[8]  + t5  * m[12]) - (t0  * m[4]  + t3  * m[8]  + t4  * m[12]));
  o[5]  = d * ((t0  * m[0]  + t7  * m[8]  + t8  * m[12]) - (t1  * m[0]  + t6  * m[8]  + t9  * m[12]));
  o[6]  = d * ((t3  * m[0]  + t6  * m[4]  + t11 * m[12]) - (t2  * m[0]  + t7  * m[4]  + t10 * m[12]));
  o[7]  = d * ((t4  * m[0]  + t9  * m[4]  + t10 * m[8])  - (t5  * m[0]  + t8  * m[4]  + t11 * m[8]));
  o[8]  = d * ((t12 * m[7]  + t15 * m[11] + t16 * m[15]) - (t13 * m[7]  + t14 * m[11] + t17 * m[15]));
  o[9]  = d * ((t13 * m[3]  + t18 * m[11] + t21 * m[15]) - (t12 * m[3]  + t19 * m[11] + t20 * m[15]));
  o[10] = d * ((t14 * m[3]  + t19 * m[7]  + t22 * m[15]) - (t15 * m[3]  + t18 * m[7]  + t23 * m[15]));
  o[11] = d * ((t17 * m[3]  + t20 * m[7]  + t23 * m[11]) - (t16 * m[3]  + t21 * m[7]  + t22 * m[11]));
  o[12] = d * ((t14 * m[10] + t17 * m[14] + t13 * m[6])  - (t16 * m[14] + t12 * m[6]  + t15 * m[10]));
  o[13] = d * ((t20 * m[14] + t12 * m[2]  + t19 * m[10]) - (t18 * m[10] + t21 * m[14] + t13 * m[2]));
  o[14] = d * ((t18 * m[6]  + t23 * m[14] + t15 * m[2])  - (t22 * m[14] + t14 * m[2]  + t19 * m[6]));
  o[15] = d * ((t22 * m[10] + t16 * m[2]  + t21 * m[6])  - (t20 * m[6]  + t23 * m[10] + t17 * m[2]));
  
  return out_mat;
}

LIB_INLINE mat4 mat4_translation(vec3 position)
{
  mat4 out_matrix = mat4_identity();
  out_matrix.data[12] = position.x;
  out_matrix.data[13] = position.y;
  out_matrix.data[14] = position.z;
  return out_matrix;
}

LIB_INLINE mat4 mat4_scale(vec3 scale)
{
  mat4 out_mat = mat4_identity();
  out_mat.data[0] = scale.x;
  out_mat.data[5] = scale.y;
  out_mat.data[10] = scale.z;
  return out_mat;
}

LIB_INLINE mat4 mat4_euler_x(float angle_rad)
{
  mat4 out_mat = mat4_identity();
  float c = math_cos(angle_rad);
  float s = math_sin(angle_rad);
  
  out_mat.data[5] = c;
  out_mat.data[6] = s;
  out_mat.data[9] = -s;
  out_mat.data[10] = c;
  return out_mat;
}

LIB_INLINE mat4 mat4_euler_y(float angle_rad)
{
  mat4 out_mat = mat4_identity();
  float c = math_cos(angle_rad);
  float s = math_sin(angle_rad);
  
  out_mat.data[0] = c;
  out_mat.data[2] = -s;
  out_mat.data[8] = s;
  out_mat.data[10] = c;
  return out_mat;
}

LIB_INLINE mat4 mat4_euler_z(float angle_rad)
{
  mat4 out_mat = mat4_identity();
  float c = math_cos(angle_rad);
  float s = math_sin(angle_rad);
  
  out_mat.data[0] = c;
  out_mat.data[1] = s;
  out_mat.data[4] = -s;
  out_mat.data[5] = c;
  return out_mat;
}

LIB_INLINE mat4 mat4_euler_xyz(float x_rad, float y_rad, float z_rad)
{
  mat4 rx = mat4_euler_x(x_rad);
  mat4 ry = mat4_euler_y(y_rad);
  mat4 rz = mat4_euler_z(z_rad);
  mat4 out_mat = mat4_mul(rx, ry);
  out_mat = mat4_mul(out_mat, rz);
  return out_mat;
}

LIB_INLINE vec3 mat4_forward(mat4 mat)
{
  vec3 forward;
  forward.x = -mat.data[2];
  forward.y = -mat.data[6];
  forward.z = -mat.data[10];
  vec3_normalize(&forward);
  return forward;
}

LIB_INLINE vec3 mat4_backward(mat4 mat)
{
  vec3 backward;
  backward.x = mat.data[2];
  backward.y = mat.data[6];
  backward.z = mat.data[10];
  vec3_normalize(&backward);
  return backward;
}

LIB_INLINE vec3 mat4_up(mat4 mat)
{
  vec3 up;
  up.x = mat.data[1];
  up.y = mat.data[5];
  up.z = mat.data[9];
  vec3_normalize(&up);
  return up;
}

LIB_INLINE vec3 mat4_down(mat4 mat)
{
  vec3 down;
  down.x = -mat.data[1];
  down.y = -mat.data[5];
  down.z = -mat.data[9];
  vec3_normalize(&down);
  return down;
}

LIB_INLINE vec3 mat4_left(mat4 mat)
{
  vec3 left;
  left.x = -mat.data[0];
  left.y = -mat.data[4];
  left.z = -mat.data[8];
  vec3_normalize(&left);
  return left;
}

LIB_INLINE vec3 mat4_right(mat4 mat)
{
  vec3 right;
  right.x = mat.data[0];
  right.y = mat.data[4];
  right.z = mat.data[8];
  vec3_normalize(&right);
  return right;
}

LIB_INLINE quat quat_identity()
{
  return (quat) { {0.0f, 0.0f, 0.0f, 1.0f } };
}

LIB_INLINE float quat_normal(quat q)
{
  return math_sqrt(q.x * q.x + q.y * q.y + q.z * q.z + q.w * q.w);
}

LIB_INLINE quat quat_normalize(quat q)
{
  float normal = quat_normal(q);
  return (quat) { { q.x / normal, q.y / normal, q.z / normal, q.w / normal } };
}

LIB_INLINE quat quat_conjugate(quat q)
{
  return (quat) { { -q.x, -q.y, -q.z, q.w } };
}

LIB_INLINE quat quat_inverse(quat q)
{
  return quat_normalize(quat_conjugate(q));
}

LIB_INLINE quat quat_mul(quat q_0, quat q_1)
{
  quat out_quat;
  
  out_quat.x = q_0.x * q_1.w +
    q_0.y * q_1.z -
    q_0.z * q_1.y +
    q_0.w * q_1.y;
  
  out_quat.y = -q_0.x * q_1.z +
    q_0.y * q_1.w +
    q_0.z * q_1.x +
    q_0.w * q_1.y;
  
  out_quat.z = q_0.x * q_1.y -
    q_0.y * q_1.x +
    q_0.z * q_1.w +
    q_0.w * q_1.z;
  
  out_quat.w = -q_0.x * q_1.x -
    q_0.y * q_1.y -
    q_0.z * q_1.z +
    q_0.w * q_1.w;
  
  return out_quat;
}

LIB_INLINE float quat_dot(quat q_0, quat q_1)
{
  return q_0.x * q_1.x +
    q_0.y * q_1.y +
    q_0.z * q_1.z +
    q_0.w * q_1.w;
}

LIB_INLINE mat4 quat_to_mat4(quat q)
{
  mat4 out_mat = mat4_identity();
  
  quat n = quat_normalize(q);
  
  out_mat.data[0] = 1.0f - 2.0f * n.y * n.y - 2.0f * n.z * n.z;
  out_mat.data[1] = 2.0f * n.x * n.y - 2.0f * n.z * n.w;
  out_mat.data[2] = 2.0f * n.x * n.z + 2.0f * n.y * n.w;
  
  out_mat.data[4] = 2.0f * n.x * n.y + 2.0f * n.z * n.w;
  out_mat.data[5] = 1.0f - 2.0f * n.x * n.x - 2.0f * n.z * n.z;
  out_mat.data[6] = 2.0f * n.y * n.z - 2.0f * n.x * n.w;
  
  out_mat.data[8] = 2.0f * n.x * n.z - 2.0f * n.y * n.w;
  out_mat.data[9] = 2.0f * n.y * n.z + 2.0f * n.x * n.w;
  out_mat.data[10] = 1.0f - 2.0f * n.x * n.x - 2.0f * n.y * n.y;
  
  return out_mat;
}

LIB_INLINE mat4 quat_to_rotation_mat(quat q, vec3 center)
{
  mat4 out_mat;
  
  float* o = out_mat.data;
  o[0] = (q.x * q.x) - (q.y * q.y) - (q.z * q.z) + (q.w * q.w);
  o[1] = 2.0f * ((q.x * q.y) + (q.z * q.w));
  o[2] = 2.0f * ((q.x * q.z) - (q.y * q.w));
  o[3] = center.x - center.x * o[0] - center.y * o[1] - center.z * o[2];
  
  o[4] = 2.0f * ((q.x * q.y) - (q.z * q.w));
  o[5] = -(q.x * q.x) + (q.y * q.y) - (q.z * q.z) + (q.w * q.w);
  o[6] = 2.0f * ((q.y * q.z) + (q.x * q.w));
  o[7] = center.y - center.x * o[4] - center.y * o[5] - center.z * o[6];
  
  o[8] = 2.0f * ((q.x * q.z) + (q.y * q.w));
  o[9] = 2.0f * ((q.y * q.z) - (q.x * q.w));
  o[10] = -(q.x * q.x) - (q.y * q.y) + (q.z * q.z) + (q.w * q.w);
  o[11] = center.z - center.x * o[8] - center.y * o[9] - center.z * o[10];
  
  o[12] = 0.0f;
  o[13] = 0.0f;
  o[14] = 0.0f;
  o[15] = 1.0f;
  return out_mat;
}

LIB_INLINE quat quat_from_axis_angle(vec3 axis, float angle, u8 normalize)
{
  const float half_angle = 0.5f * angle;
  float s = math_sin(half_angle);
  float c = math_cos(half_angle);
  
  quat q = (quat){ {s * axis.x, s * axis.y, s * axis.z, c } };
  if (normalize)
  {
    return quat_normalize(q);
  }
  return q;
}

LIB_INLINE quat quat_slerp(quat q_0, quat q_1, float precentage)
{
  quat out_quat;
  
  quat v0 = quat_normalize(q_0);
  quat v1 = quat_normalize(q_1);
  
  float dot = quat_dot(v0, v1);
  
  if (dot < 0.0f)
  {
    v1.x = -v1.x;
    v1.y = -v1.y;
    v1.z = -v1.z;
    v1.w = -v1.w;
    dot = -dot;
  }
  
  const float DOT_TRESHHOLD = 0.9995f;
  
  if (dot > DOT_TRESHHOLD)
  {
    out_quat = (quat){ {
      v0.x + ((v1.x - v0.x) * precentage),
      v0.y + ((v1.y - v0.y) * precentage),
      v0.z + ((v1.z - v0.z) * precentage),
      v0.w + ((v1.w - v0.w) * precentage)
    } };
    
    return quat_normalize(out_quat);
  }
  
  float theta_0 = math_cos(dot);
  float theta = theta_0 * precentage;
  float sin_theta = math_sin(theta);
  float sin_theta_0 = math_sin(theta_0);
  
  float s0 = math_cos(theta) - dot * sin_theta / sin_theta_0;
  float s1 = sin_theta / sin_theta_0;
  
  return (quat) {
    {
      (v0.x * s0) + (v1.x * s1),
        (v0.y * s0) + (v1.y * s1),
        (v0.z * s0) + (v1.z * s1),
        (v0.w * s0) + (v1.w * s1)
    }
  };
}

LIB_INLINE float deg_to_rad(float degrees)
{
  return degrees * DEG2RAD;
}

LIB_INLINE float rad_to_deg(float radians)
{
  return radians * RAD2DEG;
}

#endif
