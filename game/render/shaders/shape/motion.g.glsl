#include "game/render/shaders/shape/data.glsl"
#include "game/render/shaders/shape/geometry.glsl"

uniform float trail_alpha;

layout(lines) in;
layout(triangle_strip, max_vertices = 85) out;

in v_out_t {
  shape_data data;
}
v_in[];

out vec4 g_colour;

void emit2c(vec4 v0, vec4 c0, vec4 v1, vec4 c1) {
  set_vertex_data(v0);
  g_colour = vec4(c0.rgb, c0.a * trail_alpha * clamp(game_distance(v0, v1) / 12., .25, .75));
  EmitVertex();
  set_vertex_data(v1);
  g_colour = vec4(c1.rgb, 0.);
  EmitVertex();
}

void emit_polygon(vec2 a, vec2 b, shape_data data_a, shape_data data_b) {
  polygon_data da = convert_polygon(a, data_a);
  polygon_data db = convert_polygon(b, data_b);
  for (uint i = 0; i <= da.segments; ++i) {
    emit2c(polygon_outer_v(da, i), data_a.colour, polygon_outer_v(db, i), data_b.colour);
  }
  EndPrimitive();
}

void emit_odd_polystar(vec2 a, vec2 b, shape_data data_a, shape_data data_b) {
  polystar_data da = convert_polystar(a, data_a);
  polystar_data db = convert_polystar(b, data_b);
  for (uint i = 0; i < da.sides; ++i) {
    emit2c(polystar_outer_v(da, i).v, data_a.colour, polystar_outer_v(db, i).v, data_b.colour);
    emit2c(render_position(a), data_a.colour, render_position(b), data_b.colour);
    EndPrimitive();
  }
}

void emit_even_polystar(vec2 a, vec2 b, shape_data data_a, shape_data data_b) {
  polystar_data da = convert_polystar(a, data_a);
  polystar_data db = convert_polystar(b, data_b);
  for (uint i = 0; i < da.sides / 2; ++i) {
    emit2c(polystar_outer_v(da, i).v, data_a.colour, polystar_outer_v(db, i).v, data_b.colour);
    emit2c(polystar_outer_v(da, i + da.sides / 2).v, data_a.colour,
           polystar_outer_v(db, i + db.sides / 2).v, data_b.colour);
    EndPrimitive();
  }
}

void emit_polygram(vec2 a, vec2 b, shape_data data_a, shape_data data_b) {
  uint start = data_a.params.y;
  polystar_data da = convert_polystar(a, data_a);
  polystar_data db = convert_polystar(b, data_b);
  polystar_outer va = polystar_outer_v(da, start);
  polystar_outer vb = polystar_outer_v(db, start);
  for (uint i = start + 2; i < da.sides - (start == 0 ? 1 : 0); ++i) {
    polystar_outer ua = polystar_outer_v(da, i);
    polystar_outer ub = polystar_outer_v(db, i);
    emit2c(va.v, data_a.colour, vb.v, data_b.colour);
    emit2c(ua.v, data_a.colour, ub.v, data_b.colour);
    EndPrimitive();
  }
}

void emit_box(vec2 a, vec2 b, shape_data data_a, shape_data data_b) {
  box_data da = convert_box(a, data_a);
  box_data db = convert_box(b, data_b);
  emit2c(da.a_outer, data_a.colour, db.a_outer, data_b.colour);
  emit2c(da.b_outer, data_a.colour, db.b_outer, data_b.colour);
  emit2c(da.c_outer, data_a.colour, db.c_outer, data_b.colour);
  emit2c(da.d_outer, data_a.colour, db.d_outer, data_b.colour);
  emit2c(da.a_outer, data_a.colour, db.a_outer, data_b.colour);
  EndPrimitive();
}

void emit_line(vec2 a, vec2 b, shape_data data_a, shape_data data_b) {
  line_data da = convert_line(a, data_a);
  line_data db = convert_line(b, data_b);
  emit2c(da.a, data_a.colour, db.a, data_b.colour);
  emit2c(da.b, data_a.colour, db.b, data_b.colour);
  EndPrimitive();
}

void main() {
  vec2 position_a = gl_in[0].gl_Position.xy;
  vec2 position_b = gl_in[1].gl_Position.xy;

  switch (v_in[0].data.style) {
  case kStyleNgonPolygon:
    emit_polygon(position_a, position_b, v_in[0].data, v_in[1].data);
    break;
  case kStyleNgonPolystar:
    if (v_in[0].data.params.x % 2 != 0) {
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
  }
}