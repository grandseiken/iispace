#ifndef II_GAME_LOGIC_GEOMETRY_NODE_H
#define II_GAME_LOGIC_GEOMETRY_NODE_H
#include "game/common/math.h"
#include "game/logic/geometry/concepts.h"
#include "game/logic/geometry/iteration.h"

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
using null_shape = compound<>;

//////////////////////////////////////////////////////////////////////////////////
// Iteration functions.
//////////////////////////////////////////////////////////////////////////////////
template <IterTag I, typename Parameters, ShapeExpressionWithSubstitution<Parameters> S>
constexpr void iterate(S, I tag, const Parameters& params, const Transform auto& t,
                       const IterateFunction<I> auto& f) {
  evaluate(S{}, params).iterate(tag, t, f);
  t.increment_index();
}

template <IterTag I, typename Parameters, ShapeNodeWithSubstitution<Parameters>... Nodes>
constexpr void iterate(compound<Nodes...>, I tag, const Parameters& params, const Transform auto& t,
                       const IterateFunction<I> auto& f) {
  (void)tag;
  (void)params;
  (void)t;
  (void)f;
  (iterate(Nodes{}, tag, params, t, f), ...);
}

}  // namespace ii::geom

#endif
