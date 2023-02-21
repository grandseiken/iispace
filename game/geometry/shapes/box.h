#ifndef II_GAME_GEOMETRY_SHAPES_BOX_H
#define II_GAME_GEOMETRY_SHAPES_BOX_H
#include "game/common/collision.h"
#include "game/geometry/expressions.h"
#include "game/geometry/node.h"
#include "game/geometry/shapes/data.h"
#include "game/render/data/shapes.h"
#include <cstddef>
#include <cstdint>

namespace ii::geom {

//////////////////////////////////////////////////////////////////////////////////
// Render shape.
//////////////////////////////////////////////////////////////////////////////////
struct box_data : shape_data_base {
  using shape_data_base::iterate;
  vec2 dimensions{0};
  line_style line;
  fill_style fill;
  render::flag flags = render::flag::kNone;

  void iterate(iterate_resolve_t, const transform& t, resolve_result& r) const;
};

constexpr box_data make_box(const vec2& dimensions, line_style line, fill_style fill = sfill(),
                            render::flag flags = render::flag::kNone) {
  return {{}, dimensions, line, fill, flags};
}

template <Expression<vec2>, Expression<line_style>, Expression<fill_style> = constant<sfill()>,
          Expression<render::flag> = constant<render::flag::kNone>>
struct box_eval : shape_node {};

template <Expression<vec2> Dimensions, Expression<line_style> Line, Expression<fill_style> Fill,
          Expression<render::flag> RFlags>
constexpr auto evaluate(box_eval<Dimensions, Line, Fill, RFlags>, const auto& params) {
  return make_box(vec2{evaluate(Dimensions{}, params)}, evaluate(Line{}, params),
                  evaluate(Fill{}, params), render::flag{evaluate(RFlags{}, params)});
}

//////////////////////////////////////////////////////////////////////////////////
// Helper combinations.
//////////////////////////////////////////////////////////////////////////////////
template <vec2 Dimensions, line_style Line, fill_style Fill = sfill(),
          render::flag RFlags = render::flag::kNone>
using box = constant<make_box(Dimensions, Line, Fill, RFlags)>;

template <vec2 Dimensions, std::size_t N, line_style Line = sline(),
          render::flag RFlags = render::flag::kNone>
using box_colour_p =
    box_eval<constant<Dimensions>, set_colour_p<Line, N>, constant<sfill()>, constant<RFlags>>;
template <vec2 Dimensions, std::size_t N0, std::size_t N1, line_style Line = sline(),
          fill_style Fill = sfill(), render::flag RFlags = render::flag::kNone>
using box_colour_p2 = box_eval<constant<Dimensions>, set_colour_p<Line, N0>, set_colour_p<Fill, N1>,
                               constant<RFlags>>;

}  // namespace ii::geom

#endif