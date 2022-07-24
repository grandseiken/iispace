#ifndef II_GAME_LOGIC_GEOMETRY_NODE_CONDITIONAL_H
#define II_GAME_LOGIC_GEOMETRY_NODE_CONDITIONAL_H
#include "game/common/math.h"
#include "game/logic/geometry/concepts.h"
#include "game/logic/geometry/expressions.h"
#include "game/logic/geometry/iteration.h"
#include "game/logic/geometry/node.h"
#include <cstddef>
#include <type_traits>
#include <utility>

namespace ii::geom {

//////////////////////////////////////////////////////////////////////////////////
// Definitions.
//////////////////////////////////////////////////////////////////////////////////
struct null_shape {};
template <Expression<bool> Condition, ShapeNode TrueNode, ShapeNode FalseNode>
struct conditional_eval {};

//////////////////////////////////////////////////////////////////////////////////
// Iteration functions.
//////////////////////////////////////////////////////////////////////////////////
constexpr shape_flag check_point_legacy(null_shape, const auto&, const vec2&, shape_flag) {
  return shape_flag::kNone;
}
template <IterTag I>
constexpr void
iterate(null_shape, I tag, const auto&, const transform&, const IterateFunction<I> auto&) {}

template <typename Parameters, ExpressionWithSubstitution<bool, Parameters> Condition,
          ShapeNodeWithSubstitution<Parameters> TrueNode,
          ShapeNodeWithSubstitution<Parameters> FalseNode>
constexpr shape_flag check_point_legacy(conditional_eval<Condition, TrueNode, FalseNode>,
                                        const Parameters& params, const vec2& v, shape_flag mask) {
  return bool{evaluate(Condition{}, params)} ? check_point_legacy(TrueNode{}, params, v, mask)
                                             : check_point_legacy(FalseNode{}, params, v, mask);
}

template <IterTag I, typename Parameters, ExpressionWithSubstitution<bool, Parameters> Condition,
          ShapeNodeWithSubstitution<Parameters> TrueNode,
          ShapeNodeWithSubstitution<Parameters> FalseNode>
constexpr void
iterate(conditional_eval<Condition, TrueNode, FalseNode>, I tag, const Parameters& params,
        const transform& t, const IterateFunction<I> auto& f) {
  if constexpr (std::is_same_v<std::remove_cvref_t<Parameters>, arbitrary_parameters>) {
    iterate(TrueNode{}, tag, params, t, f);
    iterate(FalseNode{}, tag, params, t, f);
  } else {
    if (bool{evaluate(Condition{}, params)}) {
      iterate(TrueNode{}, tag, params, t, f);
    } else {
      iterate(FalseNode{}, tag, params, t, f);
    }
  }
}

//////////////////////////////////////////////////////////////////////////////////
// Helper combinations.
//////////////////////////////////////////////////////////////////////////////////
template <Expression<bool> Condition, ShapeNode... Nodes>
using if_eval = conditional_eval<Condition, pack<Nodes...>, null_shape>;

template <typename A, typename B>
struct pair {
  using a = A;
  using b = B;
};
template <typename Pair>
using pair_a = typename Pair::a;
template <typename Pair>
using pair_b = typename Pair::b;
template <auto Value, ShapeNode Node>
using switch_entry = pair<constant<Value>, Node>;
template <typename Value, typename... SwitchEntries>
using switch_eval =
    compound<if_eval<equal<Value, pair_a<SwitchEntries>>, pair_b<SwitchEntries>>...>;

template <std::size_t ParameterIndex, ShapeNode TrueNode, ShapeNode FalseNode>
using conditional_p = conditional_eval<parameter<ParameterIndex>, TrueNode, FalseNode>;
template <std::size_t ParameterIndex, ShapeNode... Nodes>
using if_p = if_eval<parameter<ParameterIndex>, Nodes...>;
template <std::size_t ParameterIndex, typename... SwitchEntries>
using switch_p = switch_eval<parameter<ParameterIndex>, SwitchEntries...>;

}  // namespace ii::geom

#endif