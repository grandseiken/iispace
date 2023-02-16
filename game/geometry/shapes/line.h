#ifndef II_GAME_GEOMETRY_SHAPES_LINE_H
#define II_GAME_GEOMETRY_SHAPES_LINE_H
#include "game/geometry/expressions.h"
#include "game/geometry/node.h"
#include "game/geometry/shapes/data.h"
#include "game/render/data/shapes.h"

namespace ii::geom {

//////////////////////////////////////////////////////////////////////////////////
// Render shape.
//////////////////////////////////////////////////////////////////////////////////
struct line_data : shape_data_base {
  using shape_data_base::iterate;
  vec2 a{0};
  vec2 b{0};
  line_style style;
  render::flag flags = render::flag::kNone;

  void iterate(iterate_resolve_t, const transform& t, resolve_result& r) const;
};

constexpr line_data make_line(const vec2& a, const vec2& b, line_style style,
                              render::flag flags = render::flag::kNone) {
  return {{}, a, b, style, flags};
}

template <Expression<vec2>, Expression<vec2>, Expression<line_style>,
          Expression<render::flag> = constant<render::flag::kNone>>
struct line_eval : shape_node {};

template <Expression<vec2> A, Expression<vec2> B, Expression<line_style> Style,
          Expression<render::flag> RFlags>
constexpr auto evaluate(line_eval<A, B, Style, RFlags>, const auto& params) {
  return make_line(vec2{evaluate(A{}, params)}, vec2{evaluate(B{}, params)},
                   evaluate(Style{}, params), render::flag{evaluate(RFlags{}, params)});
}

//////////////////////////////////////////////////////////////////////////////////
// Helper combinations.
//////////////////////////////////////////////////////////////////////////////////
template <vec2 A, vec2 B, line_style Style, render::flag RFlags = render::flag::kNone>
using line = constant<make_line(A, B, Style, RFlags)>;
template <vec2 A, vec2 B, std::size_t N, line_style Style = sline(),
          render::flag RFlags = render::flag::kNone>
using line_colour_p = line_eval<constant<A>, constant<B>, set_colour_p<Style, N>, constant<RFlags>>;

}  // namespace ii::geom

#endif