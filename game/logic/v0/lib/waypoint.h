#ifndef GAME_LOGIC_V0_LIB_WAYPOINT_H
#define GAME_LOGIC_V0_LIB_WAYPOINT_H
#include "game/common/collision.h"
#include "game/common/math.h"
#include "game/common/struct_tuple.h"
#include "game/logic/sim/components.h"
#include "game/render/data/shapes.h"
#include <cstdint>
#include <optional>
#include <vector>

namespace ii::v0 {

template <typename Tag = std::monostate>
struct waypoint_t {
  vec2 v{0};
  Tag tag = {};
};
TEMPLATE_DEBUG_STRUCT_TUPLE(waypoint_t, v, tag);

template <typename Tag = std::monostate>
struct WaypointFollower {
  WaypointFollower(fixed turn_radius) : turn_radius{turn_radius} {}
  fixed turn_radius = 0_fx;
  std::optional<vec2> direction;
  std::vector<waypoint_t<Tag>> waypoints;

  bool empty() const { return waypoints.empty(); }
  std::size_t size() const { return waypoints.size(); }
  bool needs_add() const { return size() < 3u; }
  const waypoint_t<Tag>& front() const { return waypoints.front(); }
  const waypoint_t<Tag>& back() const { return waypoints.back(); }

  void add(const vec2& wp, const Tag& tag = {}) {
    waypoints.emplace_back(waypoint_t<Tag>{wp, tag});
  }
  void reverse() {
    if (waypoints.size() >= 2) {
      std::swap(waypoints[0], waypoints[1]);
      waypoints.erase(waypoints.begin() + 2, waypoints.end());
    } else if (waypoints.size() == 1) {
      direction.reset();
    }
    if (direction) {
      *direction *= -1;
    }
  }

  void update(vec2& position, fixed speed) {
    if (waypoints.empty()) {
      return;
    }
    if (!direction) {
      direction = normalise(waypoints.front().v - position);
    }
    while ((waypoints.size() >= 2u && completed(position, waypoints[0].v, waypoints[1].v)) ||
           (waypoints.size() >= 3u && in_range(position, speed, waypoints[1].v, waypoints[2].v))) {
      waypoints.erase(waypoints.begin());
    }
    if (waypoints.size() == 1u) {
      do_move(position, speed,
              waypoints.front().v + 2 * turn_radius * normalise(waypoints.front().v - position));
    } else if (in_range(position, speed, waypoints[0].v, waypoints[1].v)) {
      do_move(position, speed,
              2 * turn_radius * normalise(waypoints[1].v - waypoints[0].v) + waypoints[1].v);
    } else {
      do_move(position, speed,
              2 * turn_radius * normalise(waypoints[1].v - waypoints[0].v) +
                  line_point_closest_point(waypoints[0].v, waypoints[1].v, position));
    }
  }

  // Whether we are past the line ab.
  bool completed(const vec2& position, const vec2& a, const vec2& b) const {
    return dot(position - a, b - a) >= length_squared(b - a);
  }

  // Whether we need to start turning now in order to get to directed line ab.
  bool in_range(const vec2& position, fixed speed, const vec2& a, const vec2& b) const {
    auto d_sq = line_point_distance_sq(a, b, position);
    auto angle_cos = dot(b - a, *direction);
    if (a != b) {
      angle_cos /= length(b - a);
    }
    return d_sq <= square(std::max(speed, 1_fx) + turn_radius * (1 - angle_cos));
  }

  void do_move(vec2& position, fixed speed, const vec2& target) {
    std::optional<vec2> new_position;
    fixed best_distance = 0;
    vec2 new_direction{0};

    auto r = turn_radius;
    auto try_case = [&](bool b) {
      // Centre of circle of rotation.
      auto pn = (b ? -1_fx : 1_fx) * perpendicular(*direction);
      auto c = position - r * pn;
      auto u = target - c;
      auto u_sq = length_squared(u);
      auto r_sq = square(r);
      if (u_sq < r_sq) {
        return;  // Point inside the circle. TODO: could just rotate around it maybe?
      }
      auto d_sq = u_sq - r_sq;
      auto d = sqrt(d_sq);  // Distance from tangent point to target.
      // Normal from centre to tangent point in correct direction.
      auto a = (r * u + (b ? -d : d) * perpendicular(u)) / (r_sq + d_sq);
      auto t = c + r * a;  // Tangent point on circle of rotation.
      // Signed angle of rotation to tangent point.
      auto angle = atan2(pn.x * a.y - pn.y * a.x, dot(pn, a));
      // We are rotating in positive direction if b is true.
      if ((angle >= 0) != b && r * abs(angle) < 1_fx) {
        // Escape hatch to avoid full rotations in case of inaccuracy.
        auto dv = target - position;
        best_distance = length(dv);
        new_direction = dv / best_distance;
        new_position = position + new_direction * speed;
        return;
      }
      auto abs_angle = (angle >= 0) == b ? abs(angle) : 2 * pi<fixed> - abs(angle);
      auto arc_distance = r * abs_angle;
      auto total_distance = d + arc_distance;
      if (new_position && total_distance >= best_distance) {
        return;
      }
      best_distance = total_distance;
      if (speed < arc_distance) {
        auto pn0 = ::rotate(pn, (b ? speed : -speed) / r);
        new_direction = normalise((b ? 1_fx : -1_fx) * perpendicular(pn0));
        new_position = c + r * pn0;
      } else {
        new_direction = (b ? 1_fx : -1_fx) * perpendicular(a);
        new_position = t + (speed - arc_distance) * new_direction;
      }
    };

    try_case(true);
    try_case(false);
    if (new_position) {
      position = *new_position;
      direction = new_direction;
    } else {
      position += *direction * speed;
    }
  }

  void debug_render(std::vector<render::shape>& output, const vec2& position, fixed speed) const {
    for (std::uint32_t i = 0; i < waypoints.size(); ++i) {
      output.emplace_back(render::shape{
          .origin = to_float(waypoints[i].v),
          .colour0 = colour::kWhite0,
          .z_index = colour::z::kBackgroundEffect0,
          .data = render::ngon{.radius = 8, .sides = 8},
      });
      if (i + 1 < waypoints.size()) {
        output.emplace_back(render::shape::line(to_float(waypoints[i].v),
                                                to_float(waypoints[i + 1].v), colour::kWhite0,
                                                colour::z::kBackgroundEffect0));
      }
    }
    if (direction) {
      auto c = waypoints.size() >= 2u && in_range(position, speed, waypoints[0].v, waypoints[1].v)
          ? colour::misc::kNewGreen0
          : colour::solarized::kDarkYellow;
      output.emplace_back(render::shape::line(
          to_float(position), to_float(position + 16 * *direction), c, colour::z::kPlayer, 2.f));
    }
  }
};
TEMPLATE_DEBUG_STRUCT_TUPLE(WaypointFollower, turn_radius, direction, waypoints);

}  // namespace ii::v0

#endif