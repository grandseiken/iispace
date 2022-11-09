#ifndef II_GAME_LOGIC_GEOMETRY_SHAPES_BALL_H
#define II_GAME_LOGIC_GEOMETRY_SHAPES_BALL_H
#include "game/logic/geometry/expressions.h"

namespace ii::geom {
inline namespace shapes {

struct ball_collider_data : shape_data_base {
  using shape_data_base::iterate;
  fixed radius = 0;
  shape_flag flags = shape_flag::kNone;

  constexpr void
  iterate(iterate_collision_t it, const Transform auto& t, const FlagFunction auto& f) const {
    if (+(flags & it.mask) && length_squared(t.deref_ignore_rotation()) < radius * radius) {
      std::invoke(f, flags & it.mask);
    }
  }

  constexpr void iterate(iterate_flags_t, const Transform auto&, const FlagFunction auto& f) const {
    std::invoke(f, flags);
  }
};

constexpr ball_collider_data
make_ball_collider(fixed radius, shape_flag flags = shape_flag::kNone) {
  return {{}, radius, flags};
}

template <Expression<fixed> Radius, Expression<shape_flag> Flags>
struct ball_collider_eval {};

template <Expression<fixed> Radius, Expression<shape_flag> Flags>
constexpr auto evaluate(ball_collider_eval<Radius, Flags>, const auto& params) {
  return make_ball_collider(fixed{evaluate(Radius{}, params)},
                            shape_flag{evaluate(Flags{}, params)});
}

//////////////////////////////////////////////////////////////////////////////////
// Helper combinations.
//////////////////////////////////////////////////////////////////////////////////
template <fixed Radius, shape_flag Flags>
using ball_collider = constant<make_ball_collider(Radius, Flags)>;

}  // namespace shapes
}  // namespace ii::geom

#endif