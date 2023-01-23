#ifndef II_GAME_GEOMETRY_NODE_DISABLE_ITERATION_H
#define II_GAME_GEOMETRY_NODE_DISABLE_ITERATION_H
#include "game/common/math.h"
#include "game/geometry/concepts.h"
#include "game/geometry/iteration.h"
#include <type_traits>

namespace ii::geom {

template <IterTag DisableI, ShapeNode Node>
struct disable_iteration : shape_node {};

template <IterTag DisableI, IterTag I, typename Parameters, ShapeNode Node>
constexpr void iterate(disable_iteration<DisableI, Node>, I tag, const Parameters& params,
                       const Transform auto& t, IterateFunction<I> auto&& f) {
  if constexpr (!std::is_same_v<DisableI, I>) {
    iterate(Node{}, tag, params, t, f);
  }
}

}  // namespace ii::geom

#endif