#ifndef II_GAME_LOGIC_GEOMETRY_SHAPES_ATTACHMENT_POINT_H
#define II_GAME_LOGIC_GEOMETRY_SHAPES_ATTACHMENT_POINT_H
#include "game/logic/geometry/expressions.h"
#include "game/logic/geometry/shapes/base.h"
#include <cstddef>

namespace ii::geom {

struct attachment_point_data : shape_data_base {
  using shape_data_base::check_point_legacy;
  using shape_data_base::iterate;
  std::size_t index{0};
  vec2 d{0};

  constexpr void iterate(iterate_attachment_points_t, const transform& t,
                         const AttachmentPointFunction auto& f) const {
    std::invoke(f, index, t.v, t.translate(d).v - t.v);
  }
};

constexpr attachment_point_data make_attachment_point(std::size_t index, const vec2& d) {
  return {{}, index, d};
}

template <Expression<std::size_t> Index, Expression<vec2> Direction = constant<vec2{0}>>
struct attachment_point_eval {};

template <Expression<std::size_t> Index, Expression<vec2> Direction>
constexpr auto evaluate(attachment_point_eval<Index, Direction>, const auto& params) {
  return make_attachment_point(std::size_t{evaluate(Index{}, params)},
                               vec2{evaluate(Direction{}, params)});
}

//////////////////////////////////////////////////////////////////////////////////
// Helper combinations.
//////////////////////////////////////////////////////////////////////////////////
template <std::size_t Index, fixed DX, fixed DY>
using attachment_point = constant<make_attachment_point(Index, vec2{DX, DY})>;

}  // namespace ii::geom

#endif