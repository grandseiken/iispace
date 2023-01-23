#include "game/geometry/shapes/ball.h"
#include "game/geometry/shapes/box.h"
#include "game/geometry/shapes/ngon.h"

namespace ii::geom {
namespace {

std::span<const vec2>
ngon_convex_segment(const ngon_dimensions& dimensions, std::array<vec2, 4>& va, std::uint32_t i) {
  if (dimensions.inner_radius) {
    vec2 a = ::rotate(vec2{1, 0}, 2 * i * pi<fixed> / dimensions.sides);
    vec2 b = ::rotate(vec2{1, 0}, 2 * (i + 1) * pi<fixed> / dimensions.sides);
    va[0] = dimensions.inner_radius * a;
    va[1] = dimensions.inner_radius * b;
    va[2] = dimensions.radius * b;
    va[3] = dimensions.radius * a;
    return {va.begin(), va.begin() + 4};
  }
  va[0] = vec2{0};
  va[1] = ::rotate(vec2{dimensions.radius, 0}, 2 * i * pi<fixed> / dimensions.sides);
  va[2] = ::rotate(vec2{dimensions.radius, 0}, 2 * (i + 1) * pi<fixed> / dimensions.sides);
  return {va.begin(), va.begin() + 3};
}

}  // namespace

void ball_collider_data::iterate(iterate_check_collision_t it, const convert_local_transform& t,
                                 hit_result& hit) const {
  if (!(flags & it.mask)) {
    return;
  }
  if (const auto* c = std::get_if<check_point>(&it.check)) {
    auto d_sq = length_squared(t.transform_ignore_rotation(c->v));
    if (d_sq <= square(dimensions.radius) && d_sq >= square(dimensions.inner_radius)) {
      hit.add(flags & it.mask, t.inverse_transform(vec2{0}));
    }
  } else if (const auto* c = std::get_if<check_line>(&it.check)) {
    auto a = t.transform(c->a);
    auto b = t.transform(c->b);
    auto ir_sq = square(dimensions.inner_radius);
    if (intersect_line_ball(a, b, vec2{0}, dimensions.radius) &&
        (length_squared(a) >= ir_sq || length_squared(b) >= ir_sq)) {
      hit.add(flags & it.mask, t.inverse_transform(vec2{0}));
    }
  } else if (const auto* c = std::get_if<check_ball>(&it.check)) {
    auto d_sq = length_squared(t.transform_ignore_rotation(c->c));
    auto r = dimensions.radius + c->r;
    if (d_sq <= r * r &&
        (!dimensions.inner_radius || sqrt(d_sq) + c->r >= dimensions.inner_radius)) {
      hit.add(flags & it.mask, t.inverse_transform(vec2{0}));
    }
  } else if (const auto* c = std::get_if<check_convex>(&it.check)) {
    auto va = t.transform(c->vs);
    auto ir_sq = square(dimensions.inner_radius);
    if (intersect_convex_ball(va, vec2{0}, dimensions.radius) &&
        (!dimensions.inner_radius || std::any_of(va.begin(), va.end(), [&](const vec2& v) {
          return length_squared(v) >= ir_sq;
        }))) {
      hit.add(flags & it.mask, t.inverse_transform(vec2{0}));
    }
  }
}

void box_collider_data::iterate(iterate_check_collision_t it, const convert_local_transform& t,
                                hit_result& hit) const {
  if (!(flags & it.mask)) {
    return;
  }
  if (const auto* c = std::get_if<check_point>(&it.check)) {
    auto v = t.transform(c->v);
    if (abs(v.x) <= dimensions.x && abs(v.y) <= dimensions.y) {
      hit.add(flags & it.mask, t.inverse_transform(vec2{0}));
    }
  } else if (const auto* c = std::get_if<check_line>(&it.check)) {
    if (intersect_aabb_line(-dimensions, dimensions, t.transform(c->a), t.transform(c->b))) {
      hit.add(flags & it.mask, t.inverse_transform(vec2{0}));
    }
  } else if (const auto* c = std::get_if<check_ball>(&it.check)) {
    if (intersect_aabb_ball(-dimensions, dimensions, t.transform(c->c), c->r)) {
      hit.add(flags & it.mask, t.inverse_transform(vec2{0}));
    }
  } else if (const auto* c = std::get_if<check_convex>(&it.check)) {
    auto va = t.transform(c->vs);
    if (intersect_aabb_convex(-dimensions, dimensions, va)) {
      hit.add(flags & it.mask, t.inverse_transform(vec2{0}));
    }
  }
}

void ngon_collider_data::iterate(iterate_check_collision_t it, const convert_local_transform& t,
                                 hit_result& hit) const {
  if (!(flags & it.mask)) {
    return;
  }
  std::array<vec2, 4> va{};
  if (const auto* c = std::get_if<check_point>(&it.check)) {
    auto v = t.transform(c->v);
    auto theta = angle(v);
    if (theta < 0) {
      theta += 2 * pi<fixed>;
    }
    auto a = 2 * pi<fixed> / dimensions.sides;
    auto at = (theta % a) / a;
    auto dt = (1 + 2 * at * (at - 1) * (1 - cos(a)));
    auto d_sq = dt * square(dimensions.radius);
    auto i_sq = dt * square(dimensions.inner_radius);
    if (theta <= a * dimensions.segments && length_squared(v) <= d_sq &&
        length_squared(v) >= i_sq) {
      hit.add(flags & it.mask, t.inverse_transform(vec2{0}));
    }
  } else if (const auto* c = std::get_if<check_line>(&it.check)) {
    auto line_a = t.transform(c->a);
    auto line_b = t.transform(c->b);
    for (std::uint32_t i = 0; i < dimensions.segments; ++i) {
      auto convex = ngon_convex_segment(dimensions, va, i);
      if (intersect_convex_line(convex, line_a, line_b)) {
        hit.add(flags & it.mask, t.inverse_transform(vec2{0}));
        break;
      }
    }
  } else if (const auto* c = std::get_if<check_ball>(&it.check)) {
    auto v = t.transform(c->c);
    for (std::uint32_t i = 0; i < dimensions.segments; ++i) {
      auto convex = ngon_convex_segment(dimensions, va, i);
      if (intersect_convex_ball(convex, v, c->r)) {
        hit.add(flags & it.mask, t.inverse_transform(vec2{0}));
        break;
      }
    }
  } else if (const auto* c = std::get_if<check_convex>(&it.check)) {
    auto va0 = t.transform(c->vs);
    for (std::uint32_t i = 0; i < dimensions.segments; ++i) {
      auto convex = ngon_convex_segment(dimensions, va, i);
      if (intersect_convex_convex(convex, va0)) {
        hit.add(flags & it.mask, t.inverse_transform(vec2{0}));
        break;
      }
    }
  }
}

}  // namespace ii::geom