#version 460
#include "game/render/shaders/shape/shape.glsl"

uniform vec2 render_scale;
uniform uvec2 render_dimensions;

layout(points) in;
layout(triangle_strip, max_vertices = 128) out;

in v_out_t {
  shape_data data;
}
v_in[];

out vec4 g_colour;

vec2 from_polar(float a, float r) {
  return vec2(r * cos(a), r * sin(a));
}

vec2 rotate(vec2 v, float a) {
  float c = cos(a);
  float s = sin(a);
  return vec2(v.x * c - v.y * s, v.x * s + v.y * c);
}

vec2 ngon_vertex(uint i, uint n, float rotation, float radius) {
  return from_polar(rotation + float(i) * 2. * kPi / float(n), radius);
};

float render_depth() {
  // Vertex z-range is [128, 128].
  return (128. + clamp(gl_in[0].gl_Position.z, -128., 128.)) / 256.;
}

vec4 render_position(vec2 position) {
  vec2 v = render_scale * (2. * position / vec2(render_dimensions) - 1.);
  return vec4(v.x, -v.y, render_depth(), 1.);
}

// How far the vertices of an ngon must be retracted so that its _edges_ have
// been retracted by a distance of 1. That is, if we want to draw its boundary with
// unit line width, how much should the radius of the inner vertices be reduced by?
float ngon_ulw_radius_retract(uint n) {
  return sqrt(2. / (1. + cos(2. * kPi / float(n))));
}

// How far from the vertex of an ngon we need to move along each side so that two points
// are separated by a distance of 1, as a proportion of side length. That is, if we want
// to draw a line of unit width from its centre to a vertex, how much should we lerp
// towards neighbouring vertices?
float ngon_ulw_vertex_lerp(uint n, float radius) {
  return 1. / (2. * radius * sin(2. * kPi / float(n)));
}

void emit_polygon(vec2 position, shape_data data) {
  uint sides = data.params.x;
  uint segments = data.params.y;
  float radius = data.dimensions.x;
  float d = data.line_width * ngon_ulw_radius_retract(sides);
  for (uint i = 0; i <= segments; ++i) {
    gl_Position =
        render_position(position + ngon_vertex(i % sides, sides, data.rotation, radius - d));
    EmitVertex();
    gl_Position = render_position(position + ngon_vertex(i % sides, sides, data.rotation, radius));
    EmitVertex();
  }
  EndPrimitive();
}

void emit_odd_polystar(vec2 position, shape_data data) {
  uint sides = data.params.x;
  uint segments = data.params.y;
  float radius = data.dimensions.x;

  float vt = clamp(data.line_width * ngon_ulw_vertex_lerp(sides, radius), 0., 1.);
  float cd = clamp(data.line_width / 2., 0., radius);

  for (uint i = 0; i < sides; ++i) {
    vec2 v = ngon_vertex(i, sides, data.rotation, radius);
    vec2 vl = ngon_vertex((i + sides - 1) % sides, sides, data.rotation, radius);
    vec2 vr = ngon_vertex((i + 1) % sides, sides, data.rotation, radius);
    vec2 cl = ngon_vertex(i, sides, data.rotation - kPi / 2, cd);
    vec2 cr = ngon_vertex(i, sides, data.rotation + kPi / 2, cd);
    vec2 l = mix(v, vl, vt);
    vec2 r = mix(v, vr, vt);

    gl_Position = render_position(position + cr);
    EmitVertex();
    gl_Position = render_position(position + cl);
    EmitVertex();
    gl_Position = render_position(position + r);
    EmitVertex();
    gl_Position = render_position(position + l);
    EmitVertex();
    gl_Position = render_position(position + v);
    EmitVertex();
    EndPrimitive();
  }
}

void emit_even_polystar(vec2 position, shape_data data) {
  uint sides = data.params.x;
  uint segments = data.params.y;
  float radius = data.dimensions.x;

  float vt = clamp(data.line_width * ngon_ulw_vertex_lerp(sides, radius), 0., 1.);
  for (uint i = 0; i < sides / 2; ++i) {
    vec2 v = ngon_vertex(i, sides, data.rotation, radius);
    vec2 vl = ngon_vertex((i + sides - 1) % sides, sides, data.rotation, radius);
    vec2 vr = ngon_vertex((i + 1) % sides, sides, data.rotation, radius);
    vec2 vo = ngon_vertex(i + sides / 2, sides, data.rotation, radius);
    vec2 vol = ngon_vertex((i + sides / 2 + sides - 1) % sides, sides, data.rotation, radius);
    vec2 vor = ngon_vertex((i + sides / 2 + 1) % sides, sides, data.rotation, radius);
    vec2 l = mix(v, vl, vt);
    vec2 r = mix(v, vr, vt);
    vec2 ol = mix(vo, vol, vt);
    vec2 or = mix(vo, vor, vt);

    gl_Position = render_position(position + vo);
    EmitVertex();
    gl_Position = render_position(position + or);
    EmitVertex();
    gl_Position = render_position(position + ol);
    EmitVertex();
    gl_Position = render_position(position + l);
    EmitVertex();
    gl_Position = render_position(position + r);
    EmitVertex();
    gl_Position = render_position(position + v);
    EmitVertex();
    EndPrimitive();
  }
}

void emit_polygram(vec2 position, shape_data data) {
  uint sides = data.params.x;
  uint start = data.params.y;
  float radius = data.dimensions.x;

  // TODO: reusing endpoints from emit_even_polystar(), doesn't guarantee exact line width,
  // probably fine for now.
  float vt = clamp(data.line_width * ngon_ulw_vertex_lerp(sides, radius), 0., 1.);
  vec2 v = ngon_vertex(start, sides, data.rotation, radius);
  vec2 vl = ngon_vertex((start + sides - 1) % sides, sides, data.rotation, radius);
  vec2 vr = ngon_vertex((start + 1) % sides, sides, data.rotation, radius);
  for (uint i = start + 2; i < sides && i < start + sides - 1; ++i) {
    vec2 vo = ngon_vertex(i, sides, data.rotation, radius);
    vec2 vol = ngon_vertex((i + sides - 1) % sides, sides, data.rotation, radius);
    vec2 vor = ngon_vertex((i + 1) % sides, sides, data.rotation, radius);
    vec2 l = mix(v, vl, vt);
    vec2 r = mix(v, vr, vt);
    vec2 ol = mix(vo, vol, vt);
    vec2 or = mix(vo, vor, vt);

    gl_Position = render_position(position + vo);
    EmitVertex();
    gl_Position = render_position(position + or);
    EmitVertex();
    gl_Position = render_position(position + ol);
    EmitVertex();
    gl_Position = render_position(position + l);
    EmitVertex();
    gl_Position = render_position(position + r);
    EmitVertex();
    gl_Position = render_position(position + v);
    EmitVertex();
    EndPrimitive();
  }
}

void emit_box(vec2 position, shape_data data) {
  vec2 d0 = data.dimensions;
  vec2 d1 = d0 - vec2(data.line_width);

  gl_Position = render_position(position + rotate(d0, data.rotation));
  EmitVertex();
  gl_Position = render_position(position + rotate(d1, data.rotation));
  EmitVertex();
  gl_Position = render_position(position + rotate(vec2(-d0.x, d0.y), data.rotation));
  EmitVertex();
  gl_Position = render_position(position + rotate(vec2(-d1.x, d1.y), data.rotation));
  EmitVertex();
  gl_Position = render_position(position + rotate(-d0, data.rotation));
  EmitVertex();
  gl_Position = render_position(position + rotate(-d1, data.rotation));
  EmitVertex();
  gl_Position = render_position(position + rotate(vec2(d0.x, -d0.y), data.rotation));
  EmitVertex();
  gl_Position = render_position(position + rotate(vec2(d1.x, -d1.y), data.rotation));
  EmitVertex();
  gl_Position = render_position(position + rotate(d0, data.rotation));
  EmitVertex();
  gl_Position = render_position(position + rotate(d1, data.rotation));
  EmitVertex();
  EndPrimitive();
}

void emit_line(vec2 position, shape_data data) {
  uint sides = data.params.x;
  float radius = data.dimensions.x;
  float angle = kPi * (.25 - 1. / (2. * float(sides)));
  vec2 n = from_polar(data.rotation, 1.);
  vec2 a = position - radius * n;
  vec2 b = position + radius * n;
  vec2 p = vec2(n.y, -n.x);
  float pd = data.line_width / 2.;
  float nd = pd / tan(angle);

  gl_Position = render_position(a);
  EmitVertex();
  gl_Position = render_position(a + nd * n + pd * p);
  EmitVertex();
  gl_Position = render_position(a + nd * n - pd * p);
  EmitVertex();
  gl_Position = render_position(b - nd * n + pd * p);
  EmitVertex();
  gl_Position = render_position(b - nd * n - pd * p);
  EmitVertex();
  gl_Position = render_position(b);
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