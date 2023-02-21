#ifndef II_GAME_GEOM2_SHAPE_DATA_H
#define II_GAME_GEOM2_SHAPE_DATA_H
#include "game/common/math.h"
#include "game/geom2/types.h"
#include "game/geom2/value_parameters.h"
#include <cstdint>

namespace ii::geom2 {

//////////////////////////////////////////////////////////////////////////////////
// Basic definitions.
//////////////////////////////////////////////////////////////////////////////////
struct line_style {
  value<cvec4> colour0 = cvec4{0.f};
  value<cvec4> colour1 = colour0;
  value<float> z = 0.f;
  value<float> width = 1.f;
};

struct fill_style {
  value<cvec4> colour0 = cvec4{0.f};
  value<cvec4> colour1 = colour0;
  value<float> z = 0.f;
};

struct ball_dimensions {
  value<fixed> radius = 0_fx;
  value<fixed> inner_radius = 0_fx;
};

struct ngon_dimensions {
  value<fixed> radius = 0_fx;
  value<fixed> inner_radius = 0_fx;
  value<std::uint32_t> sides = 0u;
  value<std::uint32_t> segments = sides;
};

inline constexpr line_style sline(const cvec4& colour, float z, float width = 1.f) {
  return {.colour0 = colour, .z = z, .width = width};
}

inline constexpr fill_style sfill(const cvec4& colour, float z) {
  return {.colour0 = colour, .z = z};
}

inline constexpr ball_dimensions bd(fixed radius, fixed inner_radius = 0) {
  return {.radius = radius, .inner_radius = inner_radius};
}

inline constexpr ngon_dimensions nd(fixed radius, std::uint32_t sides, std::uint32_t segments = 0) {
  return {.radius = radius, .sides = sides, .segments = segments ? segments : sides};
}

inline constexpr ngon_dimensions
nd2(fixed radius, fixed inner_radius, std::uint32_t sides, std::uint32_t segments = 0) {
  return {.radius = radius,
          .inner_radius = inner_radius,
          .sides = sides,
          .segments = segments ? segments : sides};
}

//////////////////////////////////////////////////////////////////////////////////
// Shape definitions.
//////////////////////////////////////////////////////////////////////////////////
struct ball_collider {
  ball_dimensions dimensions;
  value<shape_flag> flags = shape_flag::kNone;
};

struct box_collider {
  value<vec2> dimensions = vec2{0};
  value<shape_flag> flags = shape_flag::kNone;
};

struct ngon_collider {
  ngon_dimensions dimensions;
  value<shape_flag> flags = shape_flag::kNone;
};

struct arc_collider {
  ball_dimensions dimensions;
  value<fixed> arc_angle = 0_fx;
  value<shape_flag> flags = shape_flag::kNone;
};

struct ball {
  ball_dimensions dimensions;
  line_style line;
  fill_style fill;
  value<tag_t> tag = tag_t{0};
  value<render_flag> flags = render_flag::kNone;
};

struct box {
  value<vec2> dimensions = vec2{0};
  line_style line;
  fill_style fill;
  value<tag_t> tag = tag_t{0};
  value<render_flag> flags = render_flag::kNone;
};

struct line {
  value<vec2> a = vec2{0};
  value<vec2> b = vec2{0};
  line_style style;
  value<tag_t> tag = tag_t{0};
  value<render_flag> flags = render_flag::kNone;
};

struct ngon {
  ngon_dimensions dimensions;
  value<ngon_style> style = ngon_style::kPolygon;
  line_style line;
  fill_style fill;
  value<tag_t> tag = tag_t{0};
  value<render_flag> flags = render_flag::kNone;
};

//////////////////////////////////////////////////////////////////////////////////
// Compound nodes.
//////////////////////////////////////////////////////////////////////////////////
struct compound {};

struct translate {
  value<vec2> x = vec2{0};
};

struct rotate {
  value<fixed> x = 0_fx;
};

struct translate_rotate {
  value<vec2> v = vec2{0};
  value<fixed> r = 0_fx;
};

struct enable {
  value<bool> x = true;
};

}  // namespace ii::geom2

#endif
