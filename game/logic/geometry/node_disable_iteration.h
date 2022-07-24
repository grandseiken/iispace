#ifndef II_GAME_LOGIC_GEOMETRY_NODE_DISABLE_ITERATION_H
#define II_GAME_LOGIC_GEOMETRY_NODE_DISABLE_ITERATION_H
#include "game/common/math.h"
#include "game/logic/geometry/concepts.h"
#include "game/logic/geometry/iteration.h"
#include <type_traits>

namespace ii::geom {

template <IterTag DisableI, ShapeNode Node>
struct disable_iteration {};

template <IterTag DisableI, IterTag I, typename Parameters,
          ShapeNodeWithSubstitution<Parameters> Node>
constexpr void iterate(disable_iteration<DisableI, Node>, I tag, const Parameters& params,
                       const Transform auto& t, const IterateFunction<I> auto& f) {
  if constexpr (!std::is_same_v<DisableI, I>) {
    iterate(Node{}, tag, params, t, f);
  }
}

}  // namespace ii::geom

#endif