#ifndef II_GAME_LOGIC_GEOMETRY_NODE_EXPAND_H
#define II_GAME_LOGIC_GEOMETRY_NODE_EXPAND_H
#include "game/common/type_list.h"
#include "game/logic/geometry/expressions.h"
#include "game/logic/geometry/node.h"
#include <type_traits>

namespace ii::geom {

template <auto I, auto End>
struct range_values
: std::type_identity<tl::prepend<constant<I>, typename range_values<I + 1, End>::type>> {};
template <auto End>
struct range_values<End, End> : std::type_identity<tl::list<>> {};
template <typename T, template <T> typename F, T... I>
consteval auto expand_range_impl(tl::list<constant<I>...>) -> geom::compound<F<I>...>;
template <typename T, T Begin, T End, template <T> typename F>
using expand_range = decltype(expand_range_impl<T, F>(typename range_values<Begin, End>::type{}));

}  // namespace ii::geom

#endif