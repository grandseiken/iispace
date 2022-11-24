#include "game/render/shaders/shape/data.glsl"
#include "game/render/shaders/shape/geometry.glsl"

layout(points) in;
layout(triangle_strip, max_vertices = 113) out;

in v_out_t {
  shape_vertex_data data;
}
v_in[];

flat out uint g_buffer_index;

void emit2(vec4 v0, vec4 v1) {
  set_vertex_data(v0);
  EmitVertex();
  set_vertex_data(v1);
  EmitVertex();
}

void emit3(vec4 v0, vec4 v1, vec4 v2) {
  set_vertex_data(v0);
  EmitVertex();
  set_vertex_data(v1);
  EmitVertex();
  set_vertex_data(v2);
  EmitVertex();
}

void emit_polygon(vec2 position, shape_vertex_data data) {
  polygon_data d = convert_polygon(position, data);
  for (uint i = 0; i <= d.segments; ++i) {
    emit2(polygon_inner_v(d, i), polygon_outer_v(d, i));
  }
  EndPrimitive();
  if (d.inner_radius != 0.) {
    for (uint i = 0; i <= d.segments; ++i) {
      emit2(polygon_inner_inner_v(d, i), polygon_inner_outer_v(d, i));
    }
    EndPrimitive();
  }
}

void emit_odd_polystar(vec2 position, shape_vertex_data data) {
  polystar_data d = convert_polystar(position, data);
  for (uint i = 0; d.inner_radius == 0. && i < d.segments; ++i) {
    polystar_outer a = polystar_outer_v(d, i);
    polystar_inner b = polystar_inner_v(d, i);

    emit2(b.v0, b.v1);
    emit3(a.v0, a.v1, a.v);
    EndPrimitive();
  }
  for (uint i = 0; d.inner_radius != 0. && i < d.segments; ++i) {
    polystar_outer a = polystar_outer_v(d, i);
    polystar_outer b = polystar_inner_outer_v(d, i);

    emit3(a.v0, a.v, b.v0);
    emit3(a.v1, b.v, b.v1);
    EndPrimitive();
  }
}

void emit_even_polystar(vec2 position, shape_vertex_data data) {
  polystar_data d = convert_polystar(position, data);
  for (uint i = 0; i < d.sides / 2; ++i) {
    polystar_outer a = polystar_outer_v(d, i);
    polystar_outer b = polystar_outer_v(d, i + d.sides / 2);

    emit3(a.v, a.v0, a.v1);
    emit3(b.v1, b.v0, b.v);
    EndPrimitive();
  }
}

void emit_polygram(vec2 position, shape_vertex_data data) {
  uint start = data.params.y;
  // TODO: reusing polystar data doesn't guarantee exact line width, probably fine for now.
  polystar_data d = convert_polystar(position, data);
  polystar_outer v = polystar_outer_v(d, start);
  for (uint i = start + 2; i < d.sides - (start == 0 ? 1 : 0); ++i) {
    polystar_outer u = polystar_outer_v(d, i);

    emit3(u.v, u.v0, u.v1);
    emit3(v.v1, v.v0, v.v);
    EndPrimitive();
  }
}

void emit_box(vec2 position, shape_vertex_data data) {
  box_data d = convert_box(position, data);
  emit2(d.a_outer, d.a_inner);
  emit2(d.b_outer, d.b_inner);
  emit2(d.c_outer, d.c_inner);
  emit2(d.d_outer, d.d_inner);
  emit2(d.a_outer, d.a_inner);
  EndPrimitive();
}

void emit_line(vec2 position, shape_vertex_data data) {
  line_data d = convert_line(position, data);
  emit3(d.a, d.a0, d.a1);
  emit3(d.b0, d.b1, d.b);
  EndPrimitive();
}

void emit_ball(vec2 position, shape_vertex_data data) {
  // TODO: could do it like the box and avoid rendering most of the inside.
  ball_data d = convert_ball(position, data);
  emit2(d.v_min, vec4(d.v_max.x, d.v_min.yzw));
  emit2(vec4(d.v_min.x, d.v_max.yzw), d.v_max);
  EndPrimitive();
}

void main() {
  vec2 position = gl_in[0].gl_Position.xy;

  g_buffer_index = v_in[0].data.buffer_index;
  switch (v_in[0].data.style) {
  case kStyleNgonPolygon:
    emit_polygon(position, v_in[0].data);
    break;
  case kStyleNgonPolystar:
    if (v_in[0].data.params.x % 2 != 0 || v_in[0].data.dimensions.y != 0. ||
        v_in[0].data.params.x != v_in[0].data.params.y) {
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
  case kStyleBall:
    emit_ball(position, v_in[0].data);
    break;
  }
}