#ifndef II_GAME_GEOMETRY_LEGACY_BALL_COLLIDER_H
#define II_GAME_GEOMETRY_LEGACY_BALL_COLLIDER_H
#include "game/geometry/expressions.h"

namespace ii::geom {
inline namespace legacy {

struct ball_collider_data : shape_data_base {
  using shape_data_base::iterate;
  fixed radius = 0;
  shape_flag flags = shape_flag::kNone;

  constexpr void iterate(iterate_flags_t, const null_transform&, FlagFunction auto&& f) const {
    std::invoke(f, flags);
  }

  constexpr void
  iterate(iterate_check_collision_t it, const Transform auto& t, hit_result& hit) const {
    if (!(flags & it.mask)) {
      return;
    }
    if (const auto* c = std::get_if<check_point>(&it.check)) {
      if (length_squared(t.transform_ignore_rotation(c->v)) < square(radius)) {
        hit.add(flags & it.mask);
      }
    }
  }

  constexpr void
  iterate(iterate_volumes_t, const Transform auto& t, VolumeFunction auto&& f) const {
    std::invoke(f, *t, 0_fx, cvec4{0.f}, cvec4{0.f});
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

}  // namespace legacy
}  // namespace ii::geom

#endif