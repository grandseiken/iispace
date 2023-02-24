#include "game/render/shaders/fx/data.glsl"
#include "game/render/shaders/lib/math.glsl"
#include "game/render/shaders/lib/position.glsl"

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

in v_out_t {
  fx_vertex_data data;
}
v_in[];

flat out float g_time;
flat out float g_rotation;
flat out uint g_shape;
flat out uint g_style;
flat out vec4 g_colour;
flat out vec2 g_position;
flat out vec2 g_dimensions;
flat out vec2 g_seed;

void main() {
  g_time = v_in[0].data.time;
  g_rotation = v_in[0].data.rotation;
  g_shape = v_in[0].data.shape;
  g_style = v_in[0].data.style;
  g_colour = v_in[0].data.colour;
  g_position = gl_in[0].gl_Position.xy;
  g_dimensions = v_in[0].data.dimensions;
  g_seed = v_in[0].data.seed;

  switch (v_in[0].data.shape) {
  case kFxShapeBall:
    set_vertex_data(render_position(g_position + vec2(g_dimensions.x)));
    EmitVertex();
    set_vertex_data(render_position(g_position + vec2(-g_dimensions.x, g_dimensions.x)));
    EmitVertex();
    set_vertex_data(render_position(g_position + vec2(g_dimensions.x, -g_dimensions.x)));
    EmitVertex();
    set_vertex_data(render_position(g_position + vec2(-g_dimensions.x)));
    EmitVertex();
    EndPrimitive();
    break;

  case kFxShapeBox:
    set_vertex_data(render_position(g_position + rotate(g_dimensions, g_rotation)));
    EmitVertex();
    set_vertex_data(
        render_position(g_position + rotate(vec2(-g_dimensions.x, g_dimensions.y), g_rotation)));
    EmitVertex();
    set_vertex_data(
        render_position(g_position + rotate(vec2(g_dimensions.x, -g_dimensions.y), g_rotation)));
    EmitVertex();
    set_vertex_data(render_position(g_position + rotate(-g_dimensions, g_rotation)));
    EmitVertex();
    EndPrimitive();
    break;
  }
}