#include "game/render/shaders/lib/position_fragment.glsl"

uniform vec2 clip_min;
uniform vec2 clip_max;
uniform vec2 coordinate_offset;

float render_depth() {
  // Vertex z-range is [-128, 128].
  return (128. + clamp(gl_in[0].gl_Position.z, -128., 128.)) / 256.;
}

vec4 render_coords(vec2 position) {
  vec2 v = aspect_scale * (2. * position / vec2(render_dimensions) - 1.);
  return vec4(v.x, -v.y, render_depth(), 1.);
}

vec4 render_position(vec2 position) {
  return render_coords(position + coordinate_offset);
}

void set_vertex_data(vec4 position) {
  vec4 render_min = render_coords(clip_min);
  vec4 render_max = render_coords(clip_max);
  gl_ClipDistance[0] = position.x - render_min.x;
  gl_ClipDistance[1] = render_max.x - position.x;
  gl_ClipDistance[2] = -(position.y - render_min.y);
  gl_ClipDistance[3] = -(render_max.y - position.y);
  gl_Position = position;
}

float game_distance(vec4 render_a, vec4 render_b) {
  vec2 a = (render_a.xy * vec2(render_dimensions) / 2. + 1.) / aspect_scale;
  vec2 b = (render_b.xy * vec2(render_dimensions) / 2. + 1.) / aspect_scale;
  return distance(a, b);
}