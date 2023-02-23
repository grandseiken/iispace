#include "game/common/colour.h"
#include "game/logic/legacy/enemy/enemy.h"
#include "game/logic/legacy/ship_template.h"

namespace ii::legacy {
namespace {
using namespace geom;

struct Bounce : ecs::component {
  static constexpr float kZIndex = 8.f;
  static constexpr sound kDestroySound = sound::kEnemyShatter;
  static constexpr rumble_type kDestroyRumble = rumble_type::kSmall;
  static constexpr fixed kBoundingWidth = 8;
  static constexpr auto kFlags = shape_flag::kDangerous | shape_flag::kVulnerable;

  static void construct_shape(node& root) {
    auto& n = root.add(translate_rotate{.v = key{'v'}, .r = key{'r'}});
    n.add(ngon{.dimensions = nd(8, 6), .line = {.colour0 = colour::hue360(300, .5f, .6f)}});
    n.add(ball_collider{.dimensions = bd(8), .flags = kFlags});
  }

  void set_parameters(const Transform& transform, parameter_set& parameters) const {
    parameters.add(key{'v'}, transform.centre).add(key{'r'}, transform.rotation);
  }

  Bounce(fixed angle) : dir{from_polar_legacy(angle, 3_fx)} {}
  vec2 dir{0};

  void update(Transform& transform, SimInterface& sim) {
    auto d = sim.dimensions();
    if ((transform.centre.x > d.x && dir.x > 0) || (transform.centre.x < 0 && dir.x < 0)) {
      dir.x = -dir.x;
    }
    if ((transform.centre.y > d.y && dir.y > 0) || (transform.centre.y < 0 && dir.y < 0)) {
      dir.y = -dir.y;
    }
    transform.move(dir);
  }
};
DEBUG_STRUCT_TUPLE(Bounce, dir);

struct Follow : ecs::component {
  static constexpr float kZIndex = 8.f;
  static constexpr sound kDestroySound = sound::kEnemyShatter;
  static constexpr rumble_type kDestroyRumble = rumble_type::kSmall;

  static constexpr std::uint32_t kTime = 90;
  static constexpr fixed kSpeed = 2;
  static constexpr fixed kBoundingWidth = 10;
  static constexpr auto kFlags = shape_flag::kDangerous | shape_flag::kVulnerable;

  static void construct_shape(node& root) {
    auto& n = root.add(translate_rotate{.v = key{'v'}, .r = key{'r'}});
    auto& s = n.add(enable{compare(root, false, key{'b'})});
    auto& b = n.add(enable{key{'b'}});
    s.add(ngon{.dimensions = nd(10, 4), .line = {.colour0 = colour::hue360(270, .6f)}});
    s.add(ball_collider{.dimensions = bd(10), .flags = kFlags});
    b.add(ngon{.dimensions = nd(20, 4), .line = {.colour0 = colour::hue360(270, .6f)}});
    b.add(ball_collider{.dimensions = bd(20), .flags = kFlags});
  }

  void set_parameters(const Transform& transform, parameter_set& parameters) const {
    parameters.add(key{'v'}, transform.centre)
        .add(key{'r'}, transform.rotation)
        .add(key{'b'}, is_big_follow);
  }

  Follow(bool is_big_follow) : is_big_follow{is_big_follow} {}
  std::uint32_t timer = 0;
  std::optional<ecs::entity_id> target;
  bool is_big_follow = false;

  void update(Transform& transform, SimInterface& sim) {
    transform.rotate(fixed_c::tenth);
    if (!sim.alive_players()) {
      return;
    }

    ++timer;
    if (!target || timer > kTime) {
      target = sim.nearest_player(transform.centre).id();
      timer = 0;
    }
    auto d = sim.index().get(*target)->get<Transform>()->centre - transform.centre;
    transform.move(normalise(d) * kSpeed);
  }

  void on_destroy(const Enemy& enemy, const Transform& transform, SimInterface& sim, EmitHandle&,
                  damage_type type) const {
    if (!is_big_follow || type == damage_type::kBomb) {
      return;
    }
    vec2 d = sim.rotate_compatibility(vec2{10, 0}, transform.rotation);
    for (std::uint32_t i = 0; i < 3; ++i) {
      spawn_follow(sim, transform.centre + d, enemy.score_reward != 0);
      d = sim.rotate_compatibility(d, 2 * pi<fixed> / 3);
    }
  }
};
DEBUG_STRUCT_TUPLE(Follow, timer, target, is_big_follow);

struct Chaser : ecs::component {
  static constexpr float kZIndex = 8.f;
  static constexpr sound kDestroySound = sound::kEnemyShatter;
  static constexpr rumble_type kDestroyRumble = rumble_type::kSmall;

  static constexpr std::uint32_t kTime = 60;
  static constexpr fixed kSpeed = 4;
  static constexpr fixed kBoundingWidth = 10;
  static constexpr auto kFlags = shape_flag::kDangerous | shape_flag::kVulnerable;

  static void construct_shape(node& root) {
    auto& n = root.add(translate_rotate{.v = key{'v'}, .r = key{'r'}});
    n.add(ngon{.dimensions = nd(10, 4),
               .style = ngon_style::kPolygram,
               .line = {.colour0 = colour::hue360(210, .6f)}});
    n.add(ball_collider{.dimensions = bd(10), .flags = kFlags});
  }

  void set_parameters(const Transform& transform, parameter_set& parameters) const {
    parameters.add(key{'v'}, transform.centre).add(key{'r'}, transform.rotation);
  }

  bool move = false;
  std::uint32_t timer = kTime;
  vec2 dir{0};

  void update(Transform& transform, SimInterface& sim) {
    bool before = sim.is_on_screen(transform.centre);

    if (timer) {
      --timer;
    }
    if (!timer) {
      timer = kTime * (move + 1);
      move = !move;
      if (move) {
        dir = kSpeed * sim.nearest_player_direction(transform.centre);
      }
    }
    if (move) {
      transform.move(dir);
      if (!before && sim.is_on_screen(transform.centre)) {
        move = false;
      }
    } else {
      transform.rotate(fixed_c::tenth);
    }
  }
};
DEBUG_STRUCT_TUPLE(Chaser, move, timer, dir);
}  // namespace

void spawn_bounce(SimInterface& sim, const vec2& position, fixed angle) {
  auto h = create_ship<Bounce>(sim, position);
  add_enemy_health<Bounce>(h, 1);
  h.add(Bounce{angle});
  h.add(Enemy{.threat_value = 1});
}

void spawn_follow(SimInterface& sim, const vec2& position, bool has_score, fixed rotation) {
  auto h = create_ship<Follow>(sim, position, rotation);
  add_enemy_health<Follow>(h, 1);
  h.add(Follow{false});
  h.add(Enemy{.threat_value = 1, .score_reward = has_score ? 15u : 0});
}

void spawn_big_follow(SimInterface& sim, const vec2& position, bool has_score) {
  auto h = create_ship<Follow>(sim, position);
  add_enemy_health<Follow>(h, 3, sound::kPlayerDestroy, rumble_type::kMedium);
  h.add(Follow{true});
  h.add(Enemy{.threat_value = 3, .score_reward = has_score ? 20u : 0});
}

void spawn_chaser(SimInterface& sim, const vec2& position) {
  auto h = create_ship<Chaser>(sim, position);
  add_enemy_health<Chaser>(h, 2);
  h.add(Chaser{});
  h.add(Enemy{.threat_value = 2, .score_reward = 30});
}

}  // namespace ii::legacy
