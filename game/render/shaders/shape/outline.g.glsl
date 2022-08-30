#version 460
#include "game/render/shaders/shape/data.glsl"
#include "game/render/shaders/shape/geometry.glsl"

layout(points) in;
layout(triangle_strip, max_vertices = 128) out;

in v_out_t {
  shape_data data;
}
v_in[];

out vec4 g_colour;

void emit_polygon(vec2 position, shape_data data) {
  polygon_data d = convert_polygon(position, data);
  for (uint i = 0; i <= d.segments; ++i) {
    gl_Position = polygon_inner_v(d, i);
    EmitVertex();
    gl_Position = polygon_outer_v(d, i);
    EmitVertex();
  }
  EndPrimitive();
}

void emit_odd_polystar(vec2 position, shape_data data) {
  polystar_data d = convert_polystar(position, data);
  for (uint i = 0; i < d.sides; ++i) {
    polystar_outer a = polystar_outer_v(d, i);
    polystar_inner b = polystar_inner_v(d, i);

    gl_Position = b.v0;
    EmitVertex();
    gl_Position = b.v1;
    EmitVertex();
    gl_Position = a.v0;
    EmitVertex();
    gl_Position = a.v1;
    EmitVertex();
    gl_Position = a.v;
    EmitVertex();
    EndPrimitive();
  }
}

void emit_even_polystar(vec2 position, shape_data data) {
  polystar_data d = convert_polystar(position, data);
  for (uint i = 0; i < d.sides / 2; ++i) {
    polystar_outer a = polystar_outer_v(d, i);
    polystar_outer b = polystar_outer_v(d, i + d.sides / 2);

    gl_Position = a.v;
    EmitVertex();
    gl_Position = a.v0;
    EmitVertex();
    gl_Position = a.v1;
    EmitVertex();
    gl_Position = b.v1;
    EmitVertex();
    gl_Position = b.v0;
    EmitVertex();
    gl_Position = b.v;
    EmitVertex();
    EndPrimitive();
  }
}

void emit_polygram(vec2 position, shape_data data) {
  uint start = data.params.y;
  // TODO: reusing polystar data doesn't guarantee exact line width, probably fine for now.
  polystar_data d = convert_polystar(position, data);
  polystar_outer v = polystar_outer_v(d, start);
  for (uint i = start + 2; i < d.sides && i < start + d.sides - 1; ++i) {
    polystar_outer u = polystar_outer_v(d, i);

    gl_Position = u.v;
    EmitVertex();
    gl_Position = u.v0;
    EmitVertex();
    gl_Position = u.v1;
    EmitVertex();
    gl_Position = v.v1;
    EmitVertex();
    gl_Position = v.v0;
    EmitVertex();
    gl_Position = v.v;
    EmitVertex();
    EndPrimitive();
  }
}

void emit_box(vec2 position, shape_data data) {
  box_data d = convert_box(position, data);
  gl_Position = d.a_outer;
  EmitVertex();
  gl_Position = d.a_inner;
  EmitVertex();
  gl_Position = d.b_outer;
  EmitVertex();
  gl_Position = d.b_inner;
  EmitVertex();
  gl_Position = d.c_outer;
  EmitVertex();
  gl_Position = d.c_inner;
  EmitVertex();
  gl_Position = d.d_outer;
  EmitVertex();
  gl_Position = d.d_inner;
  EmitVertex();
  gl_Position = d.a_outer;
  EmitVertex();
  gl_Position = d.a_inner;
  EmitVertex();
  EndPrimitive();
}

void emit_line(vec2 position, shape_data data) {
  line_data d = convert_line(position, data);
  gl_Position = d.a;
  EmitVertex();
  gl_Position = d.a0;
  EmitVertex();
  gl_Position = d.a1;
  EmitVertex();
  gl_Position = d.b0;
  EmitVertex();
  gl_Position = d.b1;
  EmitVertex();
  gl_Position = d.b;
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
  case kStyleNgonPolystar:
    if (v_in[0].data.params.x % 2 != 0) {
      emit_odd_polystar(position, v_in[0].data);
    } else {
      emit_even_polystar(position, v_in[0].data);
    }
    break;
  case kStyleNgonPolygram:
    emit_polygram(position, v_in[0].data);
    break;
  case kStyleBox:
    emit_box(position, v_in[0].data);
    break;
  case kStyleLine:
    emit_line(position, v_in[0].data);
    break;
  }
}