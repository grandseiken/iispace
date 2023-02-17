#ifndef II_GAME_GEOM2_TRANSFORM_H
#define II_GAME_GEOM2_TRANSFORM_H
#include "game/common/math.h"
#include <cstdint>

namespace ii::geom2 {

struct transform {
  constexpr transform(const vec2& v = vec2{0}, fixed r = 0)
  : v{v}, r{r} {}
  vec2 v;
  fixed r;

  constexpr const vec2& operator*() const { return v; }
  constexpr fixed rotation() const { return r; }

  constexpr transform translate(const vec2& t) const { return {v + ::rotate(t, r), r}; }
  constexpr transform rotate(fixed a) const { return {v, r + a}; }
};

}  // namespace ii::geom2

#endif
