#ifndef GAME_LOGIC_V0_LIB_WAYPOINT_H
#define GAME_LOGIC_V0_LIB_WAYPOINT_H
#include "game/common/collision.h"
#include "game/common/math.h"
#include "game/common/struct_tuple.h"
#include "game/logic/sim/components.h"
#include <optional>
#include <vector>

namespace ii::v0 {

struct WaypointFollower {
  WaypointFollower(fixed turn_radius) : turn_radius{turn_radius} {}
  fixed turn_radius = 0_fx;
  std::optional<vec2> direction;
  std::vector<vec2> waypoints;

  bool needs_waypoint() const { return waypoints.size() < 3u; }
  void add_waypoint(const vec2& wp) { waypoints.emplace_back(wp); }

  void update(vec2& position, fixed speed) {
    if (waypoints.empty()) {
      return;
    }
    if (!direction) {
      direction = normalise(waypoints.front() - position);
    }
    while ((waypoints.size() >= 2u && completed(position, waypoints[0], waypoints[1])) ||
           (waypoints.size() >= 3u && in_range(position, waypoints[1], waypoints[2]))) {
      waypoints.erase(waypoints.begin());
    }
    if (waypoints.size() == 1u) {
      do_move(position, speed, waypoints.front());
    } else if (in_range(position, waypoints[0], waypoints[1])) {
      do_move(position, speed, waypoints[1]);
    } else {
      do_move(position, speed, line_point_closest_point(waypoints[0], waypoints[1], position));
    }
  }

  // Whether we are past the line ab.
  bool completed(const vec2& position, const vec2& a, const vec2& b) const {
    return dot(position - a, b - a) >= length_squared(b - a);
  }

  // Whether we need to start turning now in order to get to directed line ab.
  bool in_range(const vec2& position, const vec2& a, const vec2& b) const {
    auto d_sq = line_point_distance_sq(a, b, position);
    auto angle_cos = dot(b - a, *direction);
    if (a != b) {
      angle_cos /= length(b - a);
    }
    return d_sq <= square(turn_radius * (1 - angle_cos));
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

      // Think un-needed: both tangent points and choose between them.
      // auto an = 1_fx / (r_sq + d_sq);
      // auto a0 = an * (r * u - d * perpendicular(u));
      // auto a1 = an * (r * u + d * perpendicular(u));
      // auto t0 = c + r * a0;
      // auto t1 = c + r * a1;
      // bool b0 = line_point_relation(t0, target, c) < 0 == b;
      // auto t = b0 ? t0 : t1;

      // Signed angle of rotation to tangent point.
      auto angle = atan2(pn.x * a.y - pn.y * a.x, dot(pn, a));
      // TODO: maybe need some leeway with abs(angle) < epsilon where we just move linearly.
      // We are rotating in positive direction if b is true.
      auto abs_angle = (angle >= 0) == b ? abs(angle) : 2 * pi<fixed> - abs(angle);
      auto arc_distance = r * abs_angle;
      auto total_distance = d + arc_distance;
      if (new_position && total_distance >= best_distance) {
        return;
      }
      best_distance = total_distance;
      if (speed < arc_distance) {
        auto pn0 = rotate(pn, (b ? speed : -speed) / r);
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
    }
  }
};
DEBUG_STRUCT_TUPLE(WaypointFollower, turn_radius, direction, waypoints);

}  // namespace ii::v0

#endif