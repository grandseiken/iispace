#ifndef II_GAME_LOGIC_GEOMETRY_SHAPES_LINE_H
#define II_GAME_LOGIC_GEOMETRY_SHAPES_LINE_H
#include "game/geometry/expressions.h"
#include "game/geometry/shapes/data.h"
#include "game/logic/sim/io/render.h"

namespace ii::geom {
inline namespace shapes {

//////////////////////////////////////////////////////////////////////////////////
// Render shape.
//////////////////////////////////////////////////////////////////////////////////
struct line_data : shape_data_base {
  using shape_data_base::iterate;
  vec2 a{0};
  vec2 b{0};
  line_style style;

  constexpr void
  iterate(iterate_lines_t, const Transform auto& t, const LineFunction auto& f) const {
    if (style.colour.a) {
      std::invoke(f, *t.translate(a), *t.translate(b), style.colour, style.width, style.z);
    }
  }

  constexpr void
  iterate(iterate_shapes_t, const Transform auto& t, const ShapeFunction auto& f) const {
    std::invoke(f,
                render::shape::line(to_float(*t.translate(a)), to_float(*t.translate(b)),
                                    style.colour, style.z, style.width, style.index));
  }

  constexpr void
  iterate(iterate_centres_t, const Transform auto& t, const PointFunction auto& f) const {
    std::invoke(f, *t.translate((a + b) / 2), style.colour);
  }
};

constexpr line_data make_line(const vec2& a, const vec2& b, line_style style) {
  return {{}, a, b, style};
}

template <Expression<vec2> A, Expression<vec2> B, Expression<line_style> Style>
struct line_eval {};

template <Expression<vec2> A, Expression<vec2> B, Expression<line_style> Style>
constexpr auto evaluate(line_eval<A, B, Style>, const auto& params) {
  return make_line(vec2{evaluate(A{}, params)}, vec2{evaluate(B{}, params)},
                   evaluate(Style{}, params));
}

//////////////////////////////////////////////////////////////////////////////////
// Helper combinations.
//////////////////////////////////////////////////////////////////////////////////
template <vec2 A, vec2 B, line_style Style>
using line = constant<make_line(A, B, Style)>;
template <vec2 A, vec2 B, std::size_t N, line_style Style = sline()>
using line_colour_p = line_eval<constant<A>, constant<B>, set_colour_p<Style, N>>;

}  // namespace shapes
}  // namespace ii::geom

#endif