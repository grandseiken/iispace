#include "game/render/shaders/ui/common.glsl"

uniform uvec2 screen_dimensions;
uniform uvec2 texture_dimensions;
uniform ivec2 clip_min;
uniform ivec2 clip_max;

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in v_out_t {
  glyph_data data;
}
v_in[];

out vec4 g_text_colour;
out vec2 g_texture_coords;

void emit(vec2 position, vec2 texture_coords, vec2 offset) {
  vec2 v = 2. * (position + offset) / vec2(screen_dimensions) - 1.;
  vec2 render_min = 2. * vec2(clip_min) / vec2(screen_dimensions) - 1.;
  vec2 render_max = 2. * vec2(clip_max) / vec2(screen_dimensions) - 1.;

  g_texture_coords = (texture_coords + offset) / vec2(texture_dimensions);
  gl_ClipDistance[0] = v.x - render_min.x;
  gl_ClipDistance[1] = v.y - render_min.y;
  gl_ClipDistance[2] = render_max.x - v.x;
  gl_ClipDistance[3] = render_max.y - v.y;
  gl_Position = vec4(v.x, -v.y, 0., 1.);
  EmitVertex();
}

void main() {
  g_text_colour = v_in[0].data.text_colour;

  vec2 position = gl_in[0].gl_Position.xy;
  vec2 dimensions = vec2(v_in[0].data.dimensions);
  vec2 texture_coords = vec2(v_in[0].data.texture_coords);
  emit(position, texture_coords, vec2(0.));
  emit(position, texture_coords, vec2(dimensions.x, 0.));
  emit(position, texture_coords, vec2(0., dimensions.y));
  emit(position, texture_coords, vec2(dimensions.x, dimensions.y));
}