#ifndef II_GAME_GEOMETRY_SHAPES_BALL_H
#define II_GAME_GEOMETRY_SHAPES_BALL_H
#include "game/common/collision.h"
#include "game/geometry/expressions.h"
#include "game/geometry/node.h"
#include "game/geometry/shapes/data.h"
#include "game/render/data/shapes.h"

namespace ii::geom {

using ball_dimensions = geom2::resolve_result::ball_dimensions;

constexpr ball_dimensions bd(fixed radius = 0, fixed inner_radius = 0) {
  return {radius, inner_radius};
}

//////////////////////////////////////////////////////////////////////////////////
// Render shape.
//////////////////////////////////////////////////////////////////////////////////
struct ball_data : shape_data_base {
  using shape_data_base::iterate;
  ball_dimensions dimensions;
  line_style line;
  fill_style fill;
  render::flag flags = render::flag::kNone;

  void iterate(iterate_resolve_t, const transform& t, resolve_result& r) const;
};

constexpr ball_data make_ball(ball_dimensions dimensions, line_style line,
                              fill_style fill = sfill(), render::flag flags = render::flag::kNone) {
  return {{}, dimensions, line, fill, flags};
}

template <Expression<ball_dimensions>, Expression<line_style>,
          Expression<fill_style> = constant<sfill()>,
          Expression<render::flag> = constant<render::flag::kNone>>
struct ball_eval : shape_node {};

template <Expression<ball_dimensions> Dimensions, Expression<line_style> Line,
          Expression<fill_style> Fill, Expression<render::flag> RFlags>
constexpr auto evaluate(ball_eval<Dimensions, Line, Fill, RFlags>, const auto& params) {
  return make_ball(evaluate(Dimensions{}, params), evaluate(Line{}, params),
                   evaluate(Fill{}, params), render::flag{evaluate(RFlags{}, params)});
}

//////////////////////////////////////////////////////////////////////////////////
// Helper combinations.
//////////////////////////////////////////////////////////////////////////////////
template <ball_dimensions Dimensions, line_style Line, fill_style Fill = sfill(),
          render::flag RFlags = render::flag::kNone>
using ball = constant<make_ball(Dimensions, Line, Fill, RFlags)>;

template <ball_dimensions Dimensions, std::size_t N, line_style Line = sline(),
          render::flag RFlags = render::flag::kNone>
using ball_colour_p =
    ball_eval<constant<Dimensions>, set_colour_p<Line, N>, constant<sfill()>, constant<RFlags>>;
template <ball_dimensions Dimensions, std::size_t N0, std::size_t N1, line_style Line = sline(),
          fill_style Fill = sfill(), render::flag RFlags = render::flag::kNone>
using ball_colour_p2 = ball_eval<constant<Dimensions>, set_colour_p<Line, N0>,
                                 set_colour_p<Fill, N1>, constant<RFlags>>;

}  // namespace ii::geom

#endif