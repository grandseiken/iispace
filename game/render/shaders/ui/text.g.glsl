#include "game/render/shaders/ui/common.glsl"
#include "game/render/shaders/ui/data.glsl"

uniform uvec2 texture_dimensions;

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in v_out_t {
  glyph_data data;
}
v_in[];

flat out vec4 g_text_colour;
out vec2 g_texture_coords;

void emit(vec2 position, vec2 texture_coords, vec2 offset) {
  g_texture_coords = (texture_coords + offset) / vec2(texture_dimensions);
  set_vertex_data(position + offset);
  EmitVertex();
}

void main() {
  g_text_colour = v_in[0].data.text_colour;

  vec2 position = gl_in[0].gl_Position.xy;
  vec2 dimensions = vec2(v_in[0].data.dimensions);
  vec2 texture_coords = vec2(v_in[0].data.texture_coords);
  emit(position, texture_coords, dimensions * vec2(0., 0.));
  emit(position, texture_coords, dimensions * vec2(1., 0.));
  emit(position, texture_coords, dimensions * vec2(0., 1.));
  emit(position, texture_coords, dimensions * vec2(1., 1.));
}