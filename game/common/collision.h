#ifndef II_GAME_COMMON_COLLISION_H
#define II_GAME_COMMON_COLLISION_H
#include "game/common/math.h"
#include <algorithm>
#include <optional>
#include <span>

namespace ii {
namespace detail {

inline constexpr vec2
convex_axis_projection_interval(std::span<const vec2> convex, const vec2& axis) {
  std::optional<vec2> r;
  for (const auto& v : convex) {
    auto p = cdot(v, axis);
    if (!r) {
      r = vec2{p};
      continue;
    }
    r->x = std::min(r->x, p);
    r->y = std::min(r->y, p);
  }
  return r.value_or(vec2{0});
}

inline constexpr vec2
line_axis_projection_interval(const vec2& a, const vec2& b, const vec2& axis) {
  auto pa = cdot(a, axis);
  auto pb = cdot(b, axis);
  return vec2{std::min(pa, pb), std::max(pa, pb)};
}

inline constexpr vec2 ball_normalised_axis_projection_interval(const vec2& ball_centre,
                                                               fixed ball_radius,
                                                               const vec2& axis) {
  auto c = cdot(ball_centre, axis);
  return vec2{c - ball_radius, c + ball_radius};
}

inline constexpr bool interval_overlap(const vec2& min_max_a, const vec2& min_max_b) {
  return min_max_a.y >= min_max_b.x && min_max_b.y >= min_max_a.x;
}

}  // namespace detail

inline constexpr bool
intersect_aabb_point(const vec2& aabb_min, const vec2& aabb_max, const vec2& v) {
  return v.x >= aabb_min.x && v.x <= aabb_max.x && v.y >= aabb_min.y && v.y <= aabb_max.y;
}

inline constexpr bool intersect_aabb_ball(const vec2& aabb_min, const vec2& aabb_max,
                                          const vec2& ball_centre, fixed ball_radius) {
  constexpr auto handle_dimension = [](fixed min, fixed max, fixed c) constexpr -> fixed {
    if (c < min) {
      return (c - min) * (c - min);
    }
    if (c > max) {
      return (c - max) * (c - max);
    }
    return 0;
  };
  fixed d = 0;
  d += handle_dimension(aabb_min.x, aabb_max.x, ball_centre.x);
  d += handle_dimension(aabb_min.y, aabb_max.y, ball_centre.y);
  return d <= ball_radius * ball_radius;
}

inline constexpr bool intersect_line_ball(const vec2& line_a, const vec2& line_b,
                                          const vec2& ball_centre, fixed ball_radius) {
  auto line_v = line_b - line_a;
  auto line_c = ball_centre - line_a;
  if (line_v == vec2{0}) {
    return length_squared(line_c) <= ball_radius * ball_radius;
  }

  auto k = cdot(line_c, line_v) / cdot(line_v, line_v);
  auto v = line_a + std::clamp(k, 0_fx, 1_fx) * line_v;
  auto cv = ball_centre - v;
  return length_squared(cv) <= ball_radius * ball_radius;
}

inline constexpr bool intersect_aabb_line(const vec2& aabb_min, const vec2& aabb_max,
                                          const vec2& line_a, const vec2& line_b) {
  constexpr auto line_f = [](const vec2& a, const vec2& b, const vec2& v) constexpr {
    return (b.y - a.y) * v.x + (a.x - b.x) * v.y + b.x * a.y - a.x * b.y;
  };
  auto f0 = line_f(line_a, line_b, aabb_min);
  auto f1 = line_f(line_a, line_b, aabb_max);
  auto f2 = line_f(line_a, line_b, {aabb_min.x, aabb_max.y});
  auto f3 = line_f(line_a, line_b, {aabb_max.x, aabb_min.y});
  return !((f0 < 0 && f1 < 0 && f2 < 0 && f3 < 0) || (f0 > 0 && f1 > 0 && f2 > 0 && f3 > 0) ||
           (line_a.x > aabb_max.x && line_b.x > aabb_max.x) ||
           (line_a.x < aabb_min.x && line_b.x < aabb_min.x) ||
           (line_a.y > aabb_max.y && line_b.y > aabb_max.y) ||
           (line_a.y < aabb_min.y && line_b.y < aabb_min.y));
}

inline constexpr bool
intersect_convex_line(std::span<const vec2> convex, const vec2& line_a, const vec2& line_b) {
  if (convex.empty()) {
    return false;
  }
  for (std::size_t i = 0; i < convex.size(); ++i) {
    auto axis = perpendicular(convex[(i + 1) % convex.size()] - convex[i]);
    if (!detail::interval_overlap(detail::convex_axis_projection_interval(convex, axis),
                                  detail::line_axis_projection_interval(line_a, line_b, axis))) {
      return false;
    }
  }
  auto axis = perpendicular(line_b - line_a);
  return detail::interval_overlap(detail::convex_axis_projection_interval(convex, axis),
                                  detail::line_axis_projection_interval(line_a, line_b, axis));
}

inline constexpr bool
intersect_convex_ball(std::span<const vec2> convex, const vec2& ball_centre, fixed ball_radius) {
  if (convex.empty()) {
    return false;
  }
  std::optional<vec2> closest_vertex;
  fixed closest_vertex_distance = 0;
  for (std::size_t i = 0; i < convex.size(); ++i) {
    auto axis = normalise(perpendicular(convex[(i + 1) % convex.size()] - convex[i]));
    if (!detail::interval_overlap(
            detail::convex_axis_projection_interval(convex, axis),
            detail::ball_normalised_axis_projection_interval(ball_centre, ball_radius, axis))) {
      return false;
    }
    auto d = length_squared(convex[i] - ball_centre);
    if (d <= ball_radius * ball_radius) {
      return true;
    }
    if (!closest_vertex || d < closest_vertex_distance) {
      closest_vertex = convex[i];
      closest_vertex_distance = d;
    }
  }
  auto axis = normalise(*closest_vertex - ball_centre);
  return detail::interval_overlap(
      detail::convex_axis_projection_interval(convex, axis),
      detail::ball_normalised_axis_projection_interval(ball_centre, ball_radius, axis));
}

inline constexpr bool
intersect_convex_convex(std::span<const vec2> convex_a, std::span<const vec2> convex_b) {
  if (convex_a.empty() || convex_b.empty()) {
    return false;
  }
  for (std::size_t i = 0; i < convex_a.size(); ++i) {
    auto axis = perpendicular(convex_a[(i + 1) % convex_a.size()] - convex_a[i]);
    if (!detail::interval_overlap(detail::convex_axis_projection_interval(convex_a, axis),
                                  detail::convex_axis_projection_interval(convex_b, axis))) {
      return false;
    }
  }
  for (std::size_t i = 0; i < convex_b.size(); ++i) {
    auto axis = perpendicular(convex_b[(i + 1) % convex_b.size()] - convex_b[i]);
    if (!detail::interval_overlap(detail::convex_axis_projection_interval(convex_a, axis),
                                  detail::convex_axis_projection_interval(convex_b, axis))) {
      return false;
    }
  }
  return true;
}

}  // namespace ii

#endif