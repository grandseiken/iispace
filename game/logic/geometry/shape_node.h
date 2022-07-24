#ifndef II_GAME_LOGIC_GEOMETRY_SHAPE_NODE_H
#define II_GAME_LOGIC_GEOMETRY_SHAPE_NODE_H
#include "game/common/math.h"
#include "game/common/type_list.h"
#include "game/logic/geometry/concepts.h"
#include "game/logic/geometry/expressions.h"
#include "game/logic/geometry/iteration.h"
#include <cstddef>
#include <type_traits>
#include <utility>

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

struct null_shape {};
template <IterTag DisableI, ShapeNode Node>
struct disable_iteration {};
template <Expression<vec2> V, ShapeNode Node>
struct translate_eval {};
template <Expression<fixed> Angle, ShapeNode Node>
struct rotate_eval {};
template <Expression<bool> Condition, ShapeNode TrueNode, ShapeNode FalseNode>
struct conditional_eval {};
template <Expression<bool> Condition, ShapeNode... Nodes>
using if_eval = conditional_eval<Condition, pack<Nodes...>, null_shape>;
template <typename Value, typename... SwitchEntries>
using switch_eval =
    compound<if_eval<equal<Value, pair_a<SwitchEntries>>, pair_b<SwitchEntries>>...>;

template <auto I, auto End>
struct range_values
: std::type_identity<tl::prepend<constant<I>, typename range_values<I + 1, End>::type>> {};
template <auto End>
struct range_values<End, End> : std::type_identity<tl::list<>> {};
template <typename T, template <T> typename F, T... I>
consteval auto expand_range_impl(tl::list<constant<I>...>) -> geom::compound<F<I>...>;
template <typename T, T Begin, T End, template <T> typename F>
using expand_range = decltype(expand_range_impl<T, F>(typename range_values<Begin, End>::type{}));

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

constexpr shape_flag check_point_legacy(null_shape, const auto&, const vec2&, shape_flag) {
  return shape_flag::kNone;
}
template <IterTag I>
constexpr void
iterate(null_shape, I tag, const auto&, const transform&, const IterateFunction<I> auto&) {}

template <IterTag DisableI, typename Parameters, ShapeNodeWithSubstitution<Parameters> Node>
constexpr shape_flag check_point_legacy(disable_iteration<DisableI, Node>, const Parameters& params,
                                        const vec2& v, shape_flag mask) {
  return check_point_legacy(Node{}, params, v, mask);
}

template <IterTag DisableI, IterTag I, typename Parameters,
          ShapeNodeWithSubstitution<Parameters> Node>
constexpr void iterate(disable_iteration<DisableI, Node>, I tag, const Parameters& params,
                       const transform& t, const IterateFunction<I> auto& f) {
  if constexpr (!std::is_same_v<DisableI, I>) {
    iterate(Node{}, tag, params, t, f);
  }
}

template <typename Parameters, ExpressionWithSubstitution<vec2, Parameters> V,
          ShapeNodeWithSubstitution<Parameters> Node>
constexpr shape_flag check_point_legacy(translate_eval<V, Node>, const Parameters& params,
                                        const vec2& v, shape_flag mask) {
  return check_point_legacy(Node{}, params, v - vec2{evaluate(V{}, params)}, mask);
}

template <IterTag I, typename Parameters, ExpressionWithSubstitution<vec2, Parameters> V,
          ShapeNodeWithSubstitution<Parameters> Node>
constexpr void iterate(translate_eval<V, Node>, I tag, const Parameters& params, const transform& t,
                       const IterateFunction<I> auto& f) {
  iterate(Node{}, tag, params, t.translate(vec2{evaluate(V{}, params)}), f);
}

template <typename Parameters, ExpressionWithSubstitution<fixed, Parameters> Angle,
          ShapeNodeWithSubstitution<Parameters> Node>
constexpr shape_flag check_point_legacy(rotate_eval<Angle, Node>, const Parameters& params,
                                        const vec2& v, shape_flag mask) {
  return check_point_legacy(Node{}, params, ::rotate_legacy(v, -fixed{evaluate(Angle{}, params)}),
                            mask);
}

template <IterTag I, typename Parameters, ExpressionWithSubstitution<fixed, Parameters> Angle,
          ShapeNodeWithSubstitution<Parameters> Node>
constexpr void iterate(rotate_eval<Angle, Node>, I tag, const Parameters& params,
                       const transform& t, const IterateFunction<I> auto& f) {
  iterate(Node{}, tag, params, t.rotate(fixed{evaluate(Angle{}, params)}), f);
}

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

template <std::size_t ParameterIndex, ShapeNode TrueNode, ShapeNode FalseNode>
using conditional_p = conditional_eval<parameter<ParameterIndex>, TrueNode, FalseNode>;
template <std::size_t ParameterIndex, ShapeNode... Nodes>
using if_p = if_eval<parameter<ParameterIndex>, Nodes...>;
template <std::size_t ParameterIndex, typename... SwitchEntries>
using switch_p = switch_eval<parameter<ParameterIndex>, SwitchEntries...>;

}  // namespace ii::geom

#endif