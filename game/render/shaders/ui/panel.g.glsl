#include "game/render/shaders/ui/common.glsl"
#include "game/render/shaders/ui/data.glsl"

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in v_out_t {
  panel_data data;
}
v_in[];

flat out vec2 g_render_dimensions;
flat out vec4 g_panel_colour;
flat out uint g_style;
out vec2 g_texture_coords;

void emit(vec2 position, vec2 dimensions, vec2 coords) {
  g_texture_coords = coords;
  set_vertex_data(position + coords * dimensions);
  EmitVertex();
}

void main() {
  g_render_dimensions = vec2(v_in[0].data.render_dimensions);
  g_panel_colour = v_in[0].data.panel_colour;
  g_style = v_in[0].data.style;

  vec2 position = gl_in[0].gl_Position.xy;
  vec2 screen_dimensions = vec2(v_in[0].data.screen_dimensions);

  emit(position, screen_dimensions, vec2(0., 0.));
  emit(position, screen_dimensions, vec2(1., 0.));
  emit(position, screen_dimensions, vec2(0., 1.));
  emit(position, screen_dimensions, vec2(1., 1.));
}