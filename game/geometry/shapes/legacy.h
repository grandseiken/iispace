#ifndef II_GAME_GEOMETRY_SHAPES_LEGACY_H
#define II_GAME_GEOMETRY_SHAPES_LEGACY_H
#include "game/geometry/expressions.h"
#include "game/geometry/node.h"
#include "game/geometry/shapes/ngon.h"

namespace ii::geom {

//////////////////////////////////////////////////////////////////////////////////
// Dummy exploder.
//////////////////////////////////////////////////////////////////////////////////
struct legacy_dummy_data : shape_data_base {
  using shape_data_base::iterate;
  void iterate(iterate_resolve_t, const transform& t, resolve_result& r) const;
};

//////////////////////////////////////////////////////////////////////////////////
// Ball collider.
//////////////////////////////////////////////////////////////////////////////////
struct legacy_ball_collider_data : shape_data_base {
  using shape_data_base::iterate;
  fixed radius = 0;
  shape_flag flags = shape_flag::kNone;

  constexpr void iterate(iterate_flags_t, const null_transform&, shape_flag& mask) const {
    mask |= flags;
  }

  void iterate(iterate_check_collision_t it, const Transformer auto& t, hit_result& hit) const {
    if (!(flags & it.mask)) {
      return;
    }
    if (const auto* c = std::get_if<check_point_t>(&it.extent)) {
      if (length_squared(t.transform_ignore_rotation(c->v)) < square(radius)) {
        hit.add(flags & it.mask);
      }
    }
  }
};

constexpr legacy_ball_collider_data
make_legacy_ball_collider(fixed radius, shape_flag flags = shape_flag::kNone) {
  return {{}, radius, flags};
}

template <Expression<fixed> Radius, Expression<shape_flag> Flags>
struct legacy_ball_collider_eval {};

template <Expression<fixed> Radius, Expression<shape_flag> Flags>
constexpr auto evaluate(legacy_ball_collider_eval<Radius, Flags>, const auto& params) {
  return make_legacy_ball_collider(fixed{evaluate(Radius{}, params)},
                                   shape_flag{evaluate(Flags{}, params)});
}

//////////////////////////////////////////////////////////////////////////////////
// Box collider.
//////////////////////////////////////////////////////////////////////////////////
struct legacy_box_collider_data : shape_data_base {
  using shape_data_base::iterate;
  vec2 dimensions{0};
  shape_flag flags = shape_flag::kNone;

  constexpr void iterate(iterate_flags_t, const null_transform&, shape_flag& mask) const {
    mask |= flags;
  }

  constexpr void
  iterate(iterate_check_collision_t it, const Transformer auto& t, hit_result& hit) const {
    if (!(flags & it.mask)) {
      return;
    }
    if (const auto* c = std::get_if<check_point_t>(&it.extent)) {
      auto v = t.transform(c->v);
      if (abs(v.x) < dimensions.x && abs(v.y) < dimensions.y) {
        hit.add(flags & it.mask);
      }
    }
  }
};

constexpr legacy_box_collider_data
make_legacy_box_collider(const vec2& dimensions, shape_flag flags = shape_flag::kNone) {
  return {{}, dimensions, flags};
}

template <Expression<vec2> Dimensions, Expression<shape_flag> Flags = constant<shape_flag::kNone>>
struct legacy_box_collider_eval {};

template <Expression<vec2> Dimensions, Expression<shape_flag> Flags>
constexpr auto evaluate(legacy_box_collider_eval<Dimensions, Flags>, const auto& params) {
  return make_legacy_box_collider(vec2{evaluate(Dimensions{}, params)},
                                  shape_flag{evaluate(Flags{}, params)});
}

//////////////////////////////////////////////////////////////////////////////////
// Arc collider.
//////////////////////////////////////////////////////////////////////////////////
struct legacy_arc_collider_data : shape_data_base {
  using shape_data_base::iterate;
  ngon_dimensions dimensions;
  shape_flag flags = shape_flag::kNone;

  constexpr void iterate(iterate_flags_t, const null_transform&, shape_flag& mask) const {
    mask |= flags;
  }

  constexpr void
  iterate(iterate_check_collision_t it, const Transformer auto& t, hit_result& hit) const {
    if (!(flags & it.mask)) {
      return;
    }
    if (const auto* c = std::get_if<check_point_t>(&it.extent)) {
      auto v = t.transform(c->v);
      auto theta = angle(v);
      auto r = length(v);
      if (0 <= theta && theta <= (2 * pi<fixed> * dimensions.segments) / dimensions.sides &&
          r >= dimensions.radius - 10 && r < dimensions.radius) {
        hit.add(flags & it.mask);
      }
    }
  }
};

constexpr legacy_arc_collider_data
make_legacy_arc_collider(const ngon_dimensions& dimensions, shape_flag flags = shape_flag::kNone) {
  return {{}, dimensions, flags};
}

template <Expression<ngon_dimensions> Dimensions,
          Expression<shape_flag> Flags = constant<shape_flag::kNone>>
struct legacy_arc_collider_eval {};

template <Expression<ngon_dimensions> Dimensions, Expression<shape_flag> Flags>
constexpr auto evaluate(legacy_arc_collider_eval<Dimensions, Flags>, const auto& params) {
  return make_legacy_arc_collider(evaluate(Dimensions{}, params),
                                  shape_flag{evaluate(Flags{}, params)});
}

//////////////////////////////////////////////////////////////////////////////////
// Helper combinations.
//////////////////////////////////////////////////////////////////////////////////
using legacy_dummy = constant<legacy_dummy_data{}>;
template <fixed Radius, shape_flag Flags>
using legacy_ball_collider = constant<make_legacy_ball_collider(Radius, Flags)>;
template <fixed Radius, shape_flag Flags>
using legacy_ball_collider_dummy = compound<legacy_ball_collider<Radius, Flags>, legacy_dummy>;
template <vec2 Dimensions, shape_flag Flags>
using legacy_box_collider = constant<make_legacy_box_collider(Dimensions, Flags)>;
template <ngon_dimensions Dimensions, shape_flag Flags>
using legacy_arc_collider = constant<make_legacy_arc_collider(Dimensions, Flags)>;
template <ngon_dimensions Dimensions, ngon_line_style Line, shape_flag Flags>
using legacy_ngon =
    compound<ngon<Dimensions, Line>, legacy_ball_collider<Dimensions.radius, Flags>>;

}  // namespace ii::geom

#endif