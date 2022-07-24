#ifndef II_GAME_LOGIC_GEOMETRY_EXPRESSIONS_H
#define II_GAME_LOGIC_GEOMETRY_EXPRESSIONS_H
#include "game/common/math.h"
#include "game/logic/geometry/concepts.h"
#include <glm/glm.hpp>
#include <cstddef>
#include <cstdint>

namespace ii::geom {

template <auto V>
struct constant {};
// Special vector constants only needed for MSVC.
template <fixed X, fixed Y>
struct constant_vec2 {};
template <std::uint32_t X, std::uint32_t Y>
struct constant_uvec2 {};
template <std::int32_t X, std::int32_t Y>
struct constant_ivec2 {};
template <std::size_t N>
struct parameter {};
template <std::size_t N, std::size_t I>
struct parameter_i {};
template <std::size_t N, std::size_t I, std::size_t J>
struct parameter_ij {};

template <Expression<bool> Condition, typename ETrue, typename EFalse>
struct ternary {};
template <typename E>
struct negate {};
template <typename E0, typename E1>
struct multiply {};
template <typename E0, typename E1>
struct equal {};

template <auto V>
constexpr auto evaluate(constant<V>, const auto&) {
  return V;
}

template <fixed X, fixed Y>
constexpr auto evaluate(constant_vec2<X, Y>, const auto&) {
  return vec2{X, Y};
}

template <std::uint32_t X, std::uint32_t Y>
constexpr auto evaluate(constant_uvec2<X, Y>, const auto&) {
  return glm::uvec2{X, Y};
}

template <std::int32_t X, std::int32_t Y>
constexpr auto evaluate(constant_ivec2<X, Y>, const auto&) {
  return glm::ivec2{X, Y};
}

template <std::size_t N>
constexpr auto evaluate(parameter<N>, const auto& params) {
  return get<N>(params);
}

template <std::size_t N, std::size_t I>
constexpr auto evaluate(parameter_i<N, I>, const auto& params) {
  return get<N>(params)[I];
}

template <std::size_t N, std::size_t I, std::size_t J>
constexpr auto evaluate(parameter_ij<N, I, J>, const auto& params) {
  return get<N>(params)[I][J];
}

template <Expression<bool> Condition, typename ETrue, typename EFalse>
constexpr auto evaluate(ternary<Condition, ETrue, EFalse>, const auto& params) {
  return evaluate(Condition{}, params) ? evaluate(ETrue{}, params) : evaluate(EFalse{}, params);
}

template <typename E>
constexpr auto evaluate(negate<E>, const auto& params) {
  return -evaluate(E{}, params);
}

template <typename E0, typename E1>
constexpr auto evaluate(multiply<E0, E1>, const auto& params) {
  return evaluate(E0{}, params) * evaluate(E1{}, params);
}

template <typename E0, typename E1>
constexpr auto evaluate(equal<E0, E1>, const auto& params) {
  return evaluate(E0{}, params) == evaluate(E1{}, params);
}

//////////////////////////////////////////////////////////////////////////////////
// Helper combinations.
//////////////////////////////////////////////////////////////////////////////////
template <std::size_t ParameterIndex, typename ETrue, typename EFalse>
using ternary_p = ternary<parameter<ParameterIndex>, ETrue, EFalse>;
template <std::size_t ParameterIndex>
using negate_p = negate<parameter<ParameterIndex>>;
template <auto C, std::size_t ParameterIndex>
using multiply_p = multiply<constant<C>, parameter<ParameterIndex>>;

}  // namespace ii::geom

#endif