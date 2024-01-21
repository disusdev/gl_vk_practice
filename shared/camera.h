#ifndef __CAMERA_H__
#define __CAMERA_H__

#include <defines.h>

#include <math/math_types.h>

typedef struct
t_camera
{
	mat4 view_mat;
	quat orientation;

	vec3 position;
	vec3 velocity;

	vec3 forward;
	vec3 right;
	vec3 up;

	f32 pitch;
	f32 yaw;

	b8 locked;
}
t_camera;

t_camera
camera_create()
{
	t_camera camera = { 0 };
	return camera;
}

mat4
camera_get_rotation_matrix(t_camera* ptr_camera)
{
	quat rot = quat_from_euler((vec3) { ptr_camera->pitch, ptr_camera->yaw, 0.0f });
	return quat_to_mat4(rot);
}

void
camera_update(t_camera* ptr_camera, t_input_state* ptr_input_state, f64 delta)
{
	f32 cam_vel = 10.0f + input_get_key(ptr_input_state, KEY_CODE_SHIFT) * 10.0f;

	ptr_camera->yaw -= ptr_input_state->rotation_delta.x * DEG2RAD * 0.006f;// * (f32)delta;
	ptr_camera->pitch -= ptr_input_state->rotation_delta.y * DEG2RAD * 0.004f;// * (f32)delta;

	ptr_camera->pitch = CLAMP(ptr_camera->pitch, -90 * DEG2RAD, 90 * DEG2RAD);

	quat q_pitch = quat_from_axis_angle((vec3) { 1.0f, 0.0f, 0.0f }, ptr_camera->pitch, false);
	quat q_yaw = quat_from_axis_angle((vec3) { 0.0f, 1.0f, 0.0f }, ptr_camera->yaw, false);

	ptr_camera->orientation = quat_mul(q_yaw, q_pitch);

	mat4 rotate = quat_to_mat4(ptr_camera->orientation);

	ptr_camera->forward = mat4_forward(rotate);
	ptr_camera->right = mat4_right(rotate);
	ptr_camera->up = mat4_up(rotate);

	vec3 inputAxis = (vec3)
	{
		(f32)input_get_key(ptr_input_state, KEY_CODE_D) - (f32)input_get_key(ptr_input_state, KEY_CODE_A),
		(f32)input_get_key(ptr_input_state, KEY_CODE_Q) - (f32)input_get_key(ptr_input_state, KEY_CODE_E),
		(f32)input_get_key(ptr_input_state, KEY_CODE_W) - (f32)input_get_key(ptr_input_state, KEY_CODE_S)
	};

	ptr_camera->velocity = vec3_add(vec3_add(vec3_mul_scalar(ptr_camera->forward, inputAxis.z), vec3_mul_scalar(ptr_camera->right, inputAxis.x)), vec3_mul_scalar(ptr_camera->up, inputAxis.y));

	ptr_camera->position = vec3_add(ptr_camera->position, vec3_mul_scalar(ptr_camera->velocity, cam_vel * (f32)delta));
}

mat4
camera_get_view_matrix(t_camera* ptr_camera)
{
	mat4 rotate = quat_to_mat4(ptr_camera->orientation);

	mat4 translate = mat4_identity();
	translate = mat4_translation(vec3_mul_scalar(ptr_camera->position, -1.0f));

	ptr_camera->view_mat = mat4_mul(translate, rotate);

	// ptr_camera->view_mat = mat4_inverse(ptr_camera->view_mat);
	return ptr_camera->view_mat;
}

mat4
camera_get_projection_matrix(t_camera* ptr_camera,
														 f32 ratio,
														 f32 fov,
														 b8 reverse,
														 b8 inverse)
{
	if (reverse)
	{
		mat4 pro = mat4_persp(DEG2RAD * fov, ratio, 5000.0f, 0.1f);
		if (inverse)
			pro.data[5] *= -1;
		return pro;
	}
	else
	{
		mat4 pro = mat4_persp(DEG2RAD * fov, ratio, 0.1f, 5000.0f);
		if (inverse)
			pro.data[5] *= -1;
		return pro;
	}
}

#endif