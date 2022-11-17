uniform uvec2 screen_dimensions;
uniform ivec2 clip_min;
uniform ivec2 clip_max;

void set_vertex_data(vec2 position) {
  vec2 v = 2. * position / vec2(screen_dimensions) - 1.;
  vec2 render_min = 2. * vec2(clip_min) / vec2(screen_dimensions) - 1.;
  vec2 render_max = 2. * vec2(clip_max) / vec2(screen_dimensions) - 1.;

  gl_ClipDistance[0] = v.x - render_min.x;
  gl_ClipDistance[1] = render_max.x - v.x;
  gl_ClipDistance[2] = v.y - render_min.y;
  gl_ClipDistance[3] = render_max.y - v.y;
  gl_Position = vec4(v.x, -v.y, 0., 1.);
}