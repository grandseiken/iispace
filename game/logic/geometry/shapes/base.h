#ifndef II_GAME_LOGIC_GEOMETRY_SHAPES_BASE_H
#define II_GAME_LOGIC_GEOMETRY_SHAPES_BASE_H
#include "game/common/math.h"
#include "game/logic/geometry/enums.h"
#include "game/logic/geometry/iteration.h"
#include <functional>

namespace ii::geom {

struct shape_data_base {
  constexpr shape_flag check_point_legacy(const vec2&, shape_flag) const {
    return shape_flag::kNone;
  }
  template <IterTag I>
  constexpr void iterate(I, const transform&, const IterateFunction<I> auto&) const {}
};

}  // namespace ii::geom

#endif