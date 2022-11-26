#include "game/render/shaders/shape/data.glsl"
#include "game/render/shaders/shape/geometry.glsl"

layout(lines) in;
layout(triangle_strip, max_vertices = 102) out;

in v_out_t {
  shape_vertex_data data;
}
v_in[];

flat out uint g_buffer_index;
centroid out float g_interpolate;

void emit2c(vec4 v0, vec4 v1) {
  set_vertex_data(v0);
  g_interpolate = 0.;
  EmitVertex();
  set_vertex_data(v1);
  g_interpolate = 1.;
  EmitVertex();
}

void emit_polygon(vec2 a, vec2 b, shape_vertex_data data_a, shape_vertex_data data_b) {
  polygon_data da = convert_polygon(a, data_a);
  polygon_data db = convert_polygon(b, data_b);
  for (uint i = 0; i <= da.segments; ++i) {
    emit2c(polygon_outer_v(da, i), polygon_outer_v(db, i));
  }
  EndPrimitive();
  if (da.inner_radius != 0.) {
    for (uint i = 0; i <= da.segments; ++i) {
      emit2c(polygon_inner_outer_v(da, i), polygon_inner_outer_v(db, i));
    }
    EndPrimitive();
  }
}

void emit_odd_polystar(vec2 a, vec2 b, shape_vertex_data data_a, shape_vertex_data data_b) {
  polystar_data da = convert_polystar(a, data_a);
  polystar_data db = convert_polystar(b, data_b);
  for (uint i = 0; i < da.segments; ++i) {
    emit2c(polystar_outer_v(da, i).v, polystar_outer_v(db, i).v);
    emit2c(polystar_inner_outer_v(da, i).v, polystar_inner_outer_v(db, i).v);
    EndPrimitive();
  }
}

void emit_even_polystar(vec2 a, vec2 b, shape_vertex_data data_a, shape_vertex_data data_b) {
  polystar_data da = convert_polystar(a, data_a);
  polystar_data db = convert_polystar(b, data_b);
  for (uint i = 0; i < da.sides / 2; ++i) {
    emit2c(polystar_outer_v(da, i).v, polystar_outer_v(db, i).v);
    emit2c(polystar_outer_v(da, i + da.sides / 2).v, polystar_outer_v(db, i + db.sides / 2).v);
    EndPrimitive();
  }
}

void emit_polygram(vec2 a, vec2 b, shape_vertex_data data_a, shape_vertex_data data_b) {
  uint start = data_a.params.y;
  polystar_data da = convert_polystar(a, data_a);
  polystar_data db = convert_polystar(b, data_b);
  polystar_outer va = polystar_outer_v(da, start);
  polystar_outer vb = polystar_outer_v(db, start);
  for (uint i = start + 2; i < da.sides - (start == 0 ? 1 : 0); ++i) {
    polystar_outer ua = polystar_outer_v(da, i);
    polystar_outer ub = polystar_outer_v(db, i);
    emit2c(va.v, vb.v);
    emit2c(ua.v, ub.v);
    EndPrimitive();
  }
}

void emit_box(vec2 a, vec2 b, shape_vertex_data data_a, shape_vertex_data data_b) {
  box_data da = convert_box(a, data_a);
  box_data db = convert_box(b, data_b);
  emit2c(da.a_outer, db.a_outer);
  emit2c(da.b_outer, db.b_outer);
  emit2c(da.c_outer, db.c_outer);
  emit2c(da.d_outer, db.d_outer);
  emit2c(da.a_outer, db.a_outer);
  EndPrimitive();
}

void emit_line(vec2 a, vec2 b, shape_vertex_data data_a, shape_vertex_data data_b) {
  line_data da = convert_line(a, data_a);
  line_data db = convert_line(b, data_b);
  emit2c(da.a, db.a);
  emit2c(da.b, db.b);
  EndPrimitive();
}

void emit_ball(vec2 a, vec2 b, shape_vertex_data data_a, shape_vertex_data data_b) {
  ball_data da = convert_ball(a, data_a);
  ball_data db = convert_ball(b, data_b);
  vec2 dv = vec2(greaterThan(a, b));

  set_vertex_data(mix(da.v_min, da.v_max, vec4(dv.xy, 1., 1.)));
  EmitVertex();
  set_vertex_data(mix(da.v_min, da.v_max, vec4(dv.x, 1. - dv.y, 1., 0.)));
  EmitVertex();
  set_vertex_data(mix(da.v_min, da.v_max, vec4(1. - dv.x, dv.y, 0., 1.)));
  EmitVertex();
  set_vertex_data(mix(db.v_min, db.v_max, vec4(dv.x, 1. - dv.y, 1., 0.)));
  EmitVertex();
  set_vertex_data(mix(db.v_min, db.v_max, vec4(1. - dv.x, dv.y, 0., 1.)));
  EmitVertex();
  set_vertex_data(mix(db.v_min, db.v_max, vec4(1. - dv.x, 1. - dv.y, 0., 0.)));
  EmitVertex();
  EndPrimitive();
}

void main() {
  vec2 position_a = gl_in[0].gl_Position.xy;
  vec2 position_b = gl_in[1].gl_Position.xy;

  g_buffer_index = v_in[0].data.buffer_index;
  switch (v_in[0].data.style) {
  case kStyleNgonPolygon:
    emit_polygon(position_a, position_b, v_in[0].data, v_in[1].data);
    break;
  case kStyleNgonPolystar:
    if (v_in[0].data.params.x % 2 != 0 || v_in[0].data.dimensions.y != 0. ||
        v_in[0].data.params.x != v_in[0].data.params.y) {
      emit_odd_polystar(position_a, position_b, v_in[0].data, v_in[1].data);
    } else {
      emit_even_polystar(position_a, position_b, v_in[0].data, v_in[1].data);
    }
    break;
  case kStyleNgonPolygram:
    emit_polygram(position_a, position_b, v_in[0].data, v_in[1].data);
    break;
  case kStyleBox:
    emit_box(position_a, position_b, v_in[0].data, v_in[1].data);
    break;
  case kStyleLine:
    emit_line(position_a, position_b, v_in[0].data, v_in[1].data);
    break;
  case kStyleBall:
    emit_ball(position_a, position_b, v_in[0].data, v_in[1].data);
    break;
  }
}