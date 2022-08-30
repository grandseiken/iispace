uniform vec2 render_scale;
uniform uvec2 render_dimensions;

float render_depth() {
  // Vertex z-range is [128, 128].
  return (128. + clamp(gl_in[0].gl_Position.z, -128., 128.)) / 256.;
}

vec4 render_position(vec2 position) {
  vec2 v = render_scale * (2. * position / vec2(render_dimensions) - 1.);
  return vec4(v.x, -v.y, render_depth(), 1.);
}

float game_distance(vec4 render_a, vec4 render_b) {
  vec2 a = (render_a.xy * vec2(render_dimensions) / 2. + 1.) / render_scale;
  vec2 b = (render_b.xy * vec2(render_dimensions) / 2. + 1.) / render_scale;
  return distance(a, b);
}

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

// Lines.
struct line_data {
  vec4 a;
  vec4 a0;
  vec4 a1;
  vec4 b0;
  vec4 b1;
  vec4 b;
};

line_data convert_line(vec2 position, shape_data d) {
  uint sides = d.params.x;
  float radius = d.dimensions.x;
  float angle = kPi * (.25 - 1. / (2. * float(sides)));
  vec2 n = from_polar(d.rotation, 1.);
  vec2 a = position - radius * n;
  vec2 b = position + radius * n;
  vec2 p = vec2(n.y, -n.x);
  float pd = d.line_width / 2.;
  float nd = pd / tan(angle);

  line_data r;
  r.a = render_position(a);
  r.a0 = render_position(a + nd * n + pd * p);
  r.a1 = render_position(a + nd * n - pd * p);
  r.b0 = render_position(b - nd * n + pd * p);
  r.b1 = render_position(b - nd * n - pd * p);
  r.b = render_position(b);
  return r;
}

// Boxes.
struct box_data {
  vec4 a_outer;
  vec4 b_outer;
  vec4 c_outer;
  vec4 d_outer;
  vec4 a_inner;
  vec4 b_inner;
  vec4 c_inner;
  vec4 d_inner;
};

box_data convert_box(vec2 position, shape_data d) {
  vec2 d0 = d.dimensions;
  vec2 d1 = d0 - vec2(d.line_width);

  box_data r;
  r.a_outer = render_position(position + rotate(d0, d.rotation));
  r.a_inner = render_position(position + rotate(d1, d.rotation));
  r.b_outer = render_position(position + rotate(vec2(-d0.x, d0.y), d.rotation));
  r.b_inner = render_position(position + rotate(vec2(-d1.x, d1.y), d.rotation));
  r.c_outer = render_position(position + rotate(-d0, d.rotation));
  r.c_inner = render_position(position + rotate(-d1, d.rotation));
  r.d_outer = render_position(position + rotate(vec2(d0.x, -d0.y), d.rotation));
  r.d_inner = render_position(position + rotate(vec2(d1.x, -d1.y), d.rotation));
  return r;
}

// Polygons.
struct polygon_data {
  vec2 position;
  uint sides;
  uint segments;
  float radius;
  float rotation;
  float rd;
};

polygon_data convert_polygon(vec2 position, shape_data d) {
  polygon_data r;
  r.position = position;
  r.sides = d.params.x;
  r.segments = d.params.y;
  r.radius = d.dimensions.x;
  r.rotation = d.rotation;
  r.rd = d.line_width * ngon_ulw_radius_retract(r.sides);
  return r;
}

vec4 polygon_outer_v(polygon_data d, uint i) {
  return render_position(d.position + ngon_vertex(i % d.sides, d.sides, d.rotation, d.radius));
}

vec4 polygon_inner_v(polygon_data d, uint i) {
  return render_position(d.position +
                         ngon_vertex(i % d.sides, d.sides, d.rotation, d.radius - d.rd));
}

// Polystars.
struct polystar_data {
  vec2 position;
  uint sides;
  float radius;
  float rotation;
  float vt;
  float cd;
};

struct polystar_outer {
  vec4 v;
  vec4 v0;
  vec4 v1;
};

struct polystar_inner {
  vec4 v0;
  vec4 v1;
};

polystar_data convert_polystar(vec2 position, shape_data d) {
  polystar_data r;
  r.position = position;
  r.sides = d.params.x;
  r.radius = d.dimensions.x;
  r.rotation = d.rotation;
  r.vt = clamp(d.line_width * ngon_ulw_vertex_lerp(r.sides, r.radius), 0., 1.);
  r.cd = clamp(d.line_width / 2., 0., r.radius);
  return r;
}

polystar_outer polystar_outer_v(polystar_data d, uint i) {
  polystar_outer r;
  vec2 v = ngon_vertex(i, d.sides, d.rotation, d.radius);
  vec2 v0 = ngon_vertex((i + d.sides - 1) % d.sides, d.sides, d.rotation, d.radius);
  vec2 v1 = ngon_vertex((i + 1) % d.sides, d.sides, d.rotation, d.radius);
  r.v = render_position(d.position + v);
  r.v0 = render_position(d.position + mix(v, v0, d.vt));
  r.v1 = render_position(d.position + mix(v, v1, d.vt));
  return r;
}

polystar_inner polystar_inner_v(polystar_data d, uint i) {
  polystar_inner r;
  r.v0 = render_position(d.position + ngon_vertex(i, d.sides, d.rotation - kPi / 2, d.cd));
  r.v1 = render_position(d.position + ngon_vertex(i, d.sides, d.rotation + kPi / 2, d.cd));
  return r;
}