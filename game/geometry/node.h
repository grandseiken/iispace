#ifndef II_GAME_GEOMETRY_NODE_H
#define II_GAME_GEOMETRY_NODE_H
#include "game/common/math.h"
#include "game/geometry/concepts.h"
#include "game/geometry/iteration.h"

namespace ii::geom {

//////////////////////////////////////////////////////////////////////////////////
// Definitions.
//////////////////////////////////////////////////////////////////////////////////
template <ShapeNode... Nodes>
struct compound : shape_node {};

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
constexpr void iterate(S, I tag, const Parameters& params, const Transformer auto& t,
                       IterateFunction<I> auto&& f) {
  evaluate(S{}, params).iterate(tag, t, f);
}

template <IterTag I, typename Parameters, ShapeNode... Nodes>
constexpr void iterate(compound<Nodes...>, I tag, const Parameters& params,
                       const Transformer auto& t, IterateFunction<I> auto&& f) {
  (void)tag;
  (void)params;
  (void)t;
  (void)f;
  (iterate(Nodes{}, tag, params, t, f), ...);
}

}  // namespace ii::geom

#endif
