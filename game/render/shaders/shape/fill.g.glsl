#include "game/render/shaders/shape/data.glsl"
#include "game/render/shaders/shape/geometry.glsl"

layout(points) in;
layout(triangle_strip, max_vertices = 85) out;

in v_out_t {
  shape_data data;
}
v_in[];

out vec4 g_colour;

void emit_polygon(vec2 position, shape_data data) {
  polygon_data d = convert_polygon(position, data);
  for (uint i = 0; i <= d.segments; ++i) {
    set_vertex_data(polygon_outer_v(d, i));
    EmitVertex();
    if (i != d.segments) {
      set_vertex_data(render_position(d.position));
      EmitVertex();
    }
  }
  EndPrimitive();
}

void emit_box(vec2 position, shape_data data) {
  box_data d = convert_box(position, data);
  set_vertex_data(d.a_outer);
  EmitVertex();
  set_vertex_data(d.b_outer);
  EmitVertex();
  set_vertex_data(d.d_outer);
  EmitVertex();
  set_vertex_data(d.c_outer);
  EmitVertex();
  EndPrimitive();
}

void main() {
  vec2 position = gl_in[0].gl_Position.xy;

  g_colour = v_in[0].data.colour;
  switch (v_in[0].data.style) {
  case kStyleNgonPolygon:
    emit_polygon(position, v_in[0].data);
    break;
  case kStyleBox:
    emit_box(position, v_in[0].data);
    break;
  }
}