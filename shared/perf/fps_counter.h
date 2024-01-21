#ifndef __FPS_COUNTER_H__
#define __FPS_COUNTER_H__

#include <defines.h>

typedef struct
t_fps_counter
{
  f64 accumulate_time;
  f32 avg_interval;
  f32 fps;
  u32 num_frames;
  b8 print_fps;
}
t_fps_counter;

t_fps_counter
fps_counter_create(f32 avg/* = 0.5f*/)
{
  assert(avg > 0.0f);

  t_fps_counter counter = { 0 };
  counter.avg_interval = avg;

  return counter;
}

b8
fps_counter_update(t_fps_counter* counter,
                   f32 deltaSeconds,
                   b8 frameRendered/* = true*/)
{
  if (frameRendered)
  {
		counter->num_frames++;
  }

	counter->accumulate_time += deltaSeconds;

	if (counter->accumulate_time > counter->avg_interval)
	{
		counter->fps = (f32)(counter->num_frames / counter->accumulate_time);
		if (counter->print_fps)
    {
			printf("FPS: %.1f\n", counter->fps);
    }
		counter->num_frames = 0;
		counter->accumulate_time = 0;
		return true;
	}

	return false;
}

#endif