#ifndef II_GAME_LOGIC_GEOMETRY_NODE_H
#define II_GAME_LOGIC_GEOMETRY_NODE_H
#include "game/common/math.h"
#include "game/logic/geometry/concepts.h"
#include "game/logic/geometry/iteration.h"

// TODO: massive (incompatible) speedups to be had by:
// - Rewriting check_point() to do forward-transformations (i.e. as an iterate function) instead of
//   backwards-transformations. This allows combining/eliding many unnecessary rotations.
// - Rewriting the rotate() function to use standard rotation matrix.
// - Rewriting collision detector to fix bugs and use a proper spatial hash.
// Needs to be possible to switch between old and new behaviours.
namespace ii::geom {

//////////////////////////////////////////////////////////////////////////////////
// Definitions.
//////////////////////////////////////////////////////////////////////////////////
template <ShapeNode... Nodes>
struct compound {};

namespace detail {
template <ShapeNode Node, ShapeNode... Rest>
struct pack {
  using type = compound<Node, Rest...>;
};
template <ShapeNode Node>
struct pack<Node> {
  using type = Node;
};
}  // namespace detail

template <ShapeNode... Nodes>
using pack = typename detail::pack<Nodes...>::type;

//////////////////////////////////////////////////////////////////////////////////
// Iteration functions.
//////////////////////////////////////////////////////////////////////////////////
template <typename Parameters, ShapeExpressionWithSubstitution<Parameters> S>
constexpr shape_flag
check_point_legacy(S, const Parameters& params, const vec2& v, shape_flag mask) {
  return evaluate(S{}, params).check_point_legacy(v, mask);
}

template <IterTag I, typename Parameters, ShapeExpressionWithSubstitution<Parameters> S>
constexpr void
iterate(S, I tag, const Parameters& params, const transform& t, const IterateFunction<I> auto& f) {
  evaluate(S{}, params).iterate(tag, t, f);
  if (t.shape_index_out) {
    ++*t.shape_index_out;
  }
}

template <typename Parameters, ShapeNodeWithSubstitution<Parameters>... Nodes>
constexpr shape_flag
check_point_legacy(compound<Nodes...>, const Parameters& params, const vec2& v, shape_flag mask) {
  return (check_point_legacy(Nodes{}, params, v, mask) | ...);
}

template <IterTag I, typename Parameters, ShapeNodeWithSubstitution<Parameters>... Nodes>
constexpr void iterate(compound<Nodes...>, I tag, const Parameters& params, const transform& t,
                       const IterateFunction<I> auto& f) {
  (iterate(Nodes{}, tag, params, t, f), ...);
}

}  // namespace ii::geom

#endif