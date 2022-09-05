#include "game/render/shaders/lib/hsl.glsl"

uniform uvec2 screen_dimensions;
uniform ivec2 clip_min;
uniform ivec2 clip_max;
uniform ivec2 panel_dimensions;
uniform vec4 panel_colour;
uniform float colour_cycle;

layout(location = 0) in ivec2 in_position;
layout(location = 1) in ivec2 in_texture_coords;
out vec2 v_texture_coords;
out vec4 v_panel_colour;

void main() {
  v_texture_coords = vec2(in_texture_coords) * vec2(panel_dimensions);
  v_panel_colour = hsl2rgba_cycle(panel_colour, colour_cycle);

  vec2 v = 2. * in_position / vec2(screen_dimensions) - 1.;
  vec2 render_min = 2. * vec2(clip_min) / vec2(screen_dimensions) - 1.;
  vec2 render_max = 2. * vec2(clip_max) / vec2(screen_dimensions) - 1.;

  gl_ClipDistance[0] = v.x - render_min.x;
  gl_ClipDistance[1] = v.y - render_min.y;
  gl_ClipDistance[2] = render_max.x - v.x;
  gl_ClipDistance[3] = render_max.y - v.y;
  gl_Position = vec4(v.x, -v.y, 0., 1.);
}