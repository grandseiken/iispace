#ifndef II_GAME_LOGIC_GEOMETRY_NODE_FOR_EACH_H
#define II_GAME_LOGIC_GEOMETRY_NODE_FOR_EACH_H
#include "game/logic/geometry/concepts.h"
#include "game/logic/geometry/iteration.h"

namespace ii::geom {

template <typename T, T Begin, T End, template <T> typename NodeMetaFunction>
struct for_each {};

template <IterTag I, typename T, T Begin, T End, template <T> typename NodeMetaFunction>
constexpr void iterate(for_each<T, Begin, End, NodeMetaFunction>, I tag, const auto& params,
                       const Transform auto& t, const IterateFunction<I> auto& f) {
  if constexpr (Begin != End) {
    iterate(NodeMetaFunction<Begin>{}, tag, params, t, f);
    iterate(for_each<T, Begin + 1, End, NodeMetaFunction>{}, tag, params, t, f);
  }
}

}  // namespace ii::geom

#endif
