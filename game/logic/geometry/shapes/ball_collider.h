#ifndef II_GAME_LOGIC_GEOMETRY_SHAPES_BALL_COLLIDER_H
#define II_GAME_LOGIC_GEOMETRY_SHAPES_BALL_COLLIDER_H
#include "game/logic/geometry/expressions.h"
#include "game/logic/geometry/shapes/base.h"

namespace ii::geom {

struct ball_collider_data : shape_data_base {
  using shape_data_base::iterate;
  fixed radius = 0;
  shape_flag flags = shape_flag::kNone;

  constexpr shape_flag check_point_legacy(const vec2& v, shape_flag mask) const {
    return +(flags & mask) && v.x * v.x + v.y * v.y < radius * radius ? flags & mask
                                                                      : shape_flag::kNone;
  }

  constexpr void iterate(iterate_flags_t, const transform&, const FlagFunction auto& f) const {
    std::invoke(f, flags);
  }

  constexpr void iterate(iterate_centres_t, const transform& t, const PointFunction auto& f) const {
    std::invoke(f, t.v, glm::vec4{0.f});
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

template <fixed Radius, shape_flag Flags>
using ball_collider = constant<make_ball_collider(Radius, Flags)>;

}  // namespace ii::geom

#endif