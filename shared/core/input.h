#ifndef __INPUT_H__
#define __INPUT_H__

#include <defines.h>

#include <math/mathm.h>

typedef enum
e_key_code
{
	KEY_CODE_NONE = 0x00,
	KEY_CODE_W = BIT(0),
	KEY_CODE_S = BIT(1),
	KEY_CODE_A = BIT(2),
	KEY_CODE_D = BIT(3),
	KEY_CODE_Q = BIT(4),
	KEY_CODE_E = BIT(5),
	KEY_CODE_SPACE = BIT(6),
	KEY_CODE_SHIFT = BIT(7),
  KEY_CODE_MLB = BIT(8)
}
e_key_code;

typedef struct
t_input_state
{
	vec2 center_position;
	vec2 current_mouse_position;
	vec2 rotation_delta;

	i32 keys_hold;
	i32 keys_prev_hold;
	i32 keys_down;
	i32 keys_up;

	b8 work_in_background;
}
t_input_state;

b8
input_get_key(t_input_state* ptr_state, i32 key_code)
{
  return ptr_state->keys_hold & key_code;
}

b8
input_get_key_down(t_input_state* ptr_state, i32 key_code)
{
  return ptr_state->keys_down & key_code;
}

b8
input_get_key_up(t_input_state* ptr_state, i32 key_code)
{
  return ptr_state->keys_up & key_code;
}

void
input_add_hold_button(t_input_state* ptr_state, i32 key_code)
{
  ptr_state->keys_hold |= key_code;
}

void
input_update(t_input_state* ptr_state, i32 button_states)
{
  ptr_state->keys_hold = 0x00;
  ptr_state->rotation_delta.x = 0.0f;
  ptr_state->rotation_delta.y = 0.0f;

  ptr_state->keys_hold |= button_states;

  ptr_state->keys_down = (ptr_state->keys_hold ^ ptr_state->keys_prev_hold) & ptr_state->keys_hold;
  ptr_state->keys_up = (ptr_state->keys_hold ^ ptr_state->keys_prev_hold) & ptr_state->keys_prev_hold;
  ptr_state->keys_prev_hold = ptr_state->keys_hold;
}

#endif