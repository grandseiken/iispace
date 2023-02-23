#ifndef II_GAME_GEOMETRY_TYPES_H
#define II_GAME_GEOMETRY_TYPES_H
#include "game/common/enum.h"
#include "game/common/math.h"
#include "game/render/data/shapes.h"
#include <cstdint>
#include <span>
#include <vector>

namespace ii {

enum class shape_flag : std::uint32_t {
  kNone = 0b00000000,
  kVulnerable = 0b00000001,        // Can be hit by player (and consumes non-penetrating shots).
  kWeakVulnerable = 0b00000010,    // Can be hit by player, and shots automatically penetrate.
  kBombVulnerable = 0b00000100,    // Can be hit by bomb.
  kDangerous = 0b00001000,         // Kills player.
  kShield = 0b00010000,            // Blocks all player projectiles.
  kWeakShield = 0b00100000,        // Blocks normal player projectiles, magic shots can penetrate.
  kSafeShield = 0b01000000,        // Blocks enemy projectiles.
  kEnemyInteraction = 0b10000000,  // Interactions between enemies.
  kEverything = 0b11111111,
};

template <>
struct bitmask_enum<shape_flag> : std::true_type {};

}  // namespace ii

namespace ii::geom {
using render::ngon_style;
using render::tag_t;
using render_flag = render::flag;

struct check_point_t {
  vec2 v{0};
};

struct check_line_t {
  vec2 a{0};
  vec2 b{0};
};

struct check_ball_t {
  vec2 c{0};
  fixed r = 0;
};

struct check_convex_t {
  std::span<const vec2> vs;
};

using check_extent_t = std::variant<check_point_t, check_line_t, check_ball_t, check_convex_t>;

struct check_t {
  shape_flag mask = shape_flag::kNone;
  check_extent_t extent;
  bool legacy_algorithm = false;
};

inline constexpr check_t check_point(shape_flag mask, const vec2& v) {
  return {mask, check_point_t{v}};
}
inline constexpr check_t check_line(shape_flag mask, const vec2& a, const vec2& b) {
  return {mask, check_line_t{a, b}};
}
inline constexpr check_t check_ball(shape_flag mask, const vec2& c, fixed r) {
  return {mask, check_ball_t{c, r}};
}
inline constexpr check_t check_convex(shape_flag mask, std::span<const vec2> vs) {
  return {mask, check_convex_t{vs}};
}

struct hit_result {
  shape_flag mask = shape_flag::kNone;
  std::vector<vec2> shape_centres;

  void add(shape_flag hit_mask) { mask |= hit_mask; }

  void add(shape_flag hit_mask, const vec2& v) {
    mask |= hit_mask;
    shape_centres.emplace_back(v);
  }
};

struct transform {
  constexpr transform(const vec2& v = vec2{0}, fixed r = 0) : v{v}, r{r} {}
  vec2 v;
  fixed r;

  constexpr const vec2& operator*() const { return v; }
  constexpr fixed rotation() const { return r; }

  constexpr transform translate(const vec2& t) const { return {v + ::rotate(t, r), r}; }
  constexpr transform rotate(fixed a) const { return {v, r + a}; }
};

}  // namespace ii::geom

#endif
