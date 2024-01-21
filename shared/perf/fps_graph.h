#ifndef __FPS_GRAPH_H__
#define __FPS_GRAPH_H__

#include <defines.h>

typedef struct
t_fps_graph
{
  u64 max_points;
  cque(f32) points;
  b8 visible;
}
t_fps_graph;

t_fps_graph
fps_graph_create(u64 max_graph_points)
{
  t_fps_graph fps_graph = { 0 };

  fps_graph.max_points = max_graph_points;
  fps_graph.points = cque_create(f32);

  return fps_graph;
}

void
fps_graph_add_point(t_fps_graph* fps_graph,
                    f32 point)
{
  cque_push(fps_graph->points, point);

  if (cque_size(fps_graph->points) > fps_graph->max_points)
  {
    cque_pop(fps_graph->points, NULL);
  }
}

void
fps_graph_render(t_fps_graph* fps_graph,
                 t_canvas_renderer* c,
                 vec4* color/* = vec4(1.0)*/)
{
  if (!fps_graph->visible) return;

	f32 minfps = FLT_MAX;
	f32 maxfps = FLT_MIN;

  int que_length = cque_size(fps_graph->points);

  for (u64 i = 0; i < que_length; i++)
  {
    float f = 0.0f;
    cque_at(fps_graph->points, i, &f);

    if (f < minfps) minfps = f;
		if (f > maxfps) maxfps = f;
  }

	const f32 range = maxfps - minfps;

	f32 x = 0.0f;

	vec3 p1 = (vec3) { 0.0f, 0.0f, 0.0f };

  for (u64 i = 0; i < que_length; i++)
  {
    float f = 0.0f;
    cque_at(fps_graph->points, i, &f);

    const float val = (f - minfps) / range;
		vec3 p2 = (vec3){ x, val * 0.15f, 0.0f };
		x += 1.0f / fps_graph->max_points;

    b8 met60 = f > 58.0f;
    b8 met30 = (f > 28.0f) && (f <= 58.0f);
    b8 let30 = f <= 28.0f;

    color->r = let30 * 1.0f;
    color->g = (met60 || met30) * 1.0f;
    color->b = met30 * 1.0f;

    canvas_renderer_line(c, p1, p2, color);
		p1 = p2;
  }
}

#endif