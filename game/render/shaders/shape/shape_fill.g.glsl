#include "game/render/shaders/shape/data.glsl"
#include "game/render/shaders/shape/geometry.glsl"

layout(points) in;
layout(triangle_strip, max_vertices = 113) out;

in v_out_t {
  shape_vertex_data data;
}
v_in[];

flat out uint g_buffer_index;

void emit_polygon(vec2 position, shape_vertex_data data) {
  polygon_data d = convert_polygon(position, data);
  for (uint i = 0; i <= d.segments; ++i) {
    set_vertex_data(polygon_outer_v(d, i));
    EmitVertex();
    set_vertex_data(polygon_inner_outer_v(d, i));
    EmitVertex();
  }
  EndPrimitive();
}

void emit_box(vec2 position, shape_vertex_data data) {
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

void emit_ball(vec2 position, shape_vertex_data data) {
  ball_data d = convert_ball(position, data);
  set_vertex_data(d.v_min);
  EmitVertex();
  set_vertex_data(vec4(d.v_max.x, d.v_min.yzw));
  EmitVertex();
  set_vertex_data(vec4(d.v_min.x, d.v_max.yzw));
  EmitVertex();
  set_vertex_data(d.v_max);
  EmitVertex();
  EndPrimitive();
}

void main() {
  vec2 position = gl_in[0].gl_Position.xy;

  g_buffer_index = v_in[0].data.buffer_index;
  switch (v_in[0].data.style) {
  case kStyleNgonPolygon:
    emit_polygon(position, v_in[0].data);
    break;
  case kStyleBox:
    emit_box(position, v_in[0].data);
    break;
  case kStyleBall:
    emit_ball(position, v_in[0].data);
    break;
  }
}