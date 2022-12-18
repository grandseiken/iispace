#ifndef II_GAME_GEOMETRY_NODE_TRANSFORM_H
#define II_GAME_GEOMETRY_NODE_TRANSFORM_H
#include "game/common/math.h"
#include "game/geometry/concepts.h"
#include "game/geometry/expressions.h"
#include "game/geometry/iteration.h"
#include "game/geometry/node.h"
#include <cstddef>

namespace ii::geom {

//////////////////////////////////////////////////////////////////////////////////
// Definitions.
//////////////////////////////////////////////////////////////////////////////////
template <Expression<vec2> V, ShapeNode Node>
struct translate_eval {};
template <Expression<fixed> Angle, ShapeNode Node>
struct rotate_eval {};

//////////////////////////////////////////////////////////////////////////////////
// Iteration functions.
//////////////////////////////////////////////////////////////////////////////////
template <IterTag I, typename Parameters, ExpressionWithSubstitution<vec2, Parameters> V,
          ShapeNodeWithSubstitution<Parameters> Node>
constexpr void iterate(translate_eval<V, Node>, I tag, const Parameters& params,
                       const Transform auto& t, const IterateFunction<I> auto& f) {
  iterate(Node{}, tag, params, t.translate(vec2{evaluate(V{}, params)}), f);
}

template <IterTag I, typename Parameters, ExpressionWithSubstitution<fixed, Parameters> Angle,
          ShapeNodeWithSubstitution<Parameters> Node>
constexpr void iterate(rotate_eval<Angle, Node>, I tag, const Parameters& params,
                       const Transform auto& t, const IterateFunction<I> auto& f) {
  iterate(Node{}, tag, params, t.rotate(fixed{evaluate(Angle{}, params)}), f);
}

//////////////////////////////////////////////////////////////////////////////////
// Helper combinations.
//////////////////////////////////////////////////////////////////////////////////
template <fixed X, fixed Y, ShapeNode... Nodes>
using translate = translate_eval<constant_vec2<X, Y>, pack<Nodes...>>;
template <fixed Angle, ShapeNode... Nodes>
using rotate = rotate_eval<constant<Angle>, pack<Nodes...>>;

template <std::size_t ParameterIndex, ShapeNode... Nodes>
using translate_p = translate_eval<parameter<ParameterIndex>, pack<Nodes...>>;
template <std::size_t ParameterIndex, ShapeNode... Nodes>
using rotate_p = rotate_eval<parameter<ParameterIndex>, pack<Nodes...>>;

}  // namespace ii::geom

#endif