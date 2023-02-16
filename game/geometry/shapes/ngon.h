#ifndef II_GAME_GEOMETRY_SHAPES_NGON_H
#define II_GAME_GEOMETRY_SHAPES_NGON_H
#include "game/common/collision.h"
#include "game/geometry/expressions.h"
#include "game/geometry/node.h"
#include "game/geometry/shapes/data.h"
#include "game/render/data/shapes.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace ii::geom {
using render::ngon_style;

struct ngon_dimensions {
  fixed radius = 0;
  fixed inner_radius = 0;
  std::uint32_t sides = 0;
  std::uint32_t segments = sides;
};

struct ngon_line_style : line_style {
  ngon_style style = ngon_style::kPolygon;
};

constexpr ngon_dimensions nd(fixed radius, std::uint32_t sides, std::uint32_t segments = 0) {
  return {radius, 0, sides, segments ? segments : sides};
}

constexpr ngon_dimensions
nd2(fixed radius, fixed inner_radius, std::uint32_t sides, std::uint32_t segments = 0) {
  return {radius, inner_radius, sides, segments ? segments : sides};
}

constexpr ngon_line_style
nline(const cvec4& colour = cvec4{0.f}, float z = 0.f, float width = 1.f, unsigned char index = 0) {
  return {{.colour0 = colour, .colour1 = colour, .z = z, .width = width, .index = index}};
}

constexpr ngon_line_style nline(const cvec4& colour0, const cvec4& colour1, float z = 0.f,
                                float width = 1.f, unsigned char index = 0) {
  return {{.colour0 = colour0, .colour1 = colour1, .z = z, .width = width, .index = index}};
}

constexpr ngon_line_style nline(ngon_style style, const cvec4& colour, float z = 0.f,
                                float width = 1.f, unsigned char index = 0) {
  return {{.colour0 = colour, .colour1 = colour, .z = z, .width = width, .index = index}, style};
}

constexpr ngon_line_style nline(ngon_style style, const cvec4& colour0, const cvec4& colour1,
                                float z = 0.f, float width = 1.f, unsigned char index = 0) {
  return {{.colour0 = colour0, .colour1 = colour1, .z = z, .width = width, .index = index}, style};
}

//////////////////////////////////////////////////////////////////////////////////
// Collider.
//////////////////////////////////////////////////////////////////////////////////
struct ngon_collider_data : shape_data_base {
  using shape_data_base::iterate;
  ngon_dimensions dimensions;
  shape_flag flags = shape_flag::kNone;

  constexpr void iterate(iterate_flags_t, const null_transform&, shape_flag& mask) const {
    mask |= flags;
  }

  void
  iterate(iterate_check_collision_t it, const convert_local_transform& t, hit_result& hit) const;
};

constexpr ngon_collider_data
make_ngon_collider(ngon_dimensions dimensions, shape_flag flags = shape_flag::kNone) {
  return {{}, dimensions, flags};
}

template <Expression<ngon_dimensions>, Expression<shape_flag> Flags = constant<shape_flag::kNone>>
struct ngon_collider_eval : shape_node {};

template <Expression<ngon_dimensions> Dimensions, Expression<shape_flag> Flags>
constexpr auto evaluate(ngon_collider_eval<Dimensions, Flags>, const auto& params) {
  return make_ngon_collider(evaluate(Dimensions{}, params), shape_flag{evaluate(Flags{}, params)});
}

//////////////////////////////////////////////////////////////////////////////////
// Render shape.
//////////////////////////////////////////////////////////////////////////////////
struct ngon_data : shape_data_base {
  using shape_data_base::iterate;
  ngon_dimensions dimensions;
  ngon_line_style line;
  fill_style fill;
  render::flag flags = render::flag::kNone;

  void iterate(iterate_resolve_t, const transform& t, resolve_result& r) const;
};

constexpr ngon_data make_ngon(ngon_dimensions dimensions, ngon_line_style line,
                              fill_style fill = sfill(), render::flag flags = render::flag::kNone) {
  return {{}, dimensions, line, fill, flags};
}

template <Expression<ngon_dimensions>, Expression<ngon_line_style>,
          Expression<fill_style> = constant<sfill()>,
          Expression<render::flag> = constant<render::flag::kNone>>
struct ngon_eval : shape_node {};

template <Expression<ngon_dimensions> Dimensions, Expression<ngon_line_style> Line,
          Expression<fill_style> Fill, Expression<render::flag> RFlags>
constexpr auto evaluate(ngon_eval<Dimensions, Line, Fill, RFlags>, const auto& params) {
  return make_ngon(evaluate(Dimensions{}, params), evaluate(Line{}, params),
                   evaluate(Fill{}, params), render::flag{evaluate(RFlags{}, params)});
}

//////////////////////////////////////////////////////////////////////////////////
// Helper combinations.
//////////////////////////////////////////////////////////////////////////////////
template <ngon_dimensions Dimensions, shape_flag Flags = shape_flag::kNone>
using ngon_collider = constant<make_ngon_collider(Dimensions, Flags)>;
template <ngon_dimensions Dimensions, ngon_line_style Line, fill_style Fill = sfill(),
          render::flag RFlags = render::flag::kNone>
using ngon = constant<make_ngon(Dimensions, Line, Fill, RFlags)>;
template <ngon_dimensions Dimensions, ngon_line_style Line, fill_style Fill,
          shape_flag Flags = shape_flag::kNone, render::flag RFlags = render::flag::kNone>
using ngon_with_collider =
    compound<ngon<Dimensions, Line, Fill, RFlags>, ngon_collider<Dimensions, Flags>>;

template <ngon_dimensions Dimensions, std::size_t N, ngon_line_style Line = nline(),
          render::flag RFlags = render::flag::kNone>
using ngon_colour_p =
    ngon_eval<constant<Dimensions>, set_colour_p<Line, N>, constant<sfill()>, constant<RFlags>>;
template <ngon_dimensions Dimensions, std::size_t N0, std::size_t N1,
          ngon_line_style Line = nline(), fill_style Fill = sfill(),
          render::flag RFlags = render::flag::kNone>
using ngon_colour_p2 = ngon_eval<constant<Dimensions>, set_colour_p<Line, N0>,
                                 set_colour_p<Fill, N1>, constant<RFlags>>;

}  // namespace ii::geom

#endif