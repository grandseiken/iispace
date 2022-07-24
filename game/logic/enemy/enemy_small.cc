#include "game/logic/enemy/enemy.h"
#include "game/logic/geometry/geometry.h"
#include "game/logic/ship/ship_template.h"

namespace ii {
namespace {

struct Bounce : ecs::component {
  static constexpr std::uint32_t kBoundingWidth = 8;
  static constexpr sound kDestroySound = sound::kEnemyShatter;

  using shape = standard_transform<geom::polygon<8, 6, colour_hue360(300, .5f, .6f),
                                                 shape_flag::kDangerous | shape_flag::kVulnerable>>;

  Bounce(fixed angle) : dir{from_polar(angle, 3_fx)} {}
  vec2 dir{0};

  void update(Transform& transform, SimInterface&) {
    if ((transform.centre.x > kSimDimensions.x && dir.x > 0) ||
        (transform.centre.x < 0 && dir.x < 0)) {
      dir.x = -dir.x;
    }
    if ((transform.centre.y > kSimDimensions.y && dir.y > 0) ||
        (transform.centre.y < 0 && dir.y < 0)) {
      dir.y = -dir.y;
    }
    transform.move(dir);
  }
};

struct Follow : ecs::component {
  static constexpr std::uint32_t kBoundingWidth = 10;
  static constexpr sound kDestroySound = sound::kEnemyShatter;

  static constexpr std::uint32_t kTime = 90;
  static constexpr fixed kSpeed = 2;

  using small_shape = geom::polygon<10, 4, colour_hue360(270, .6f),
                                    shape_flag::kDangerous | shape_flag::kVulnerable>;
  using big_shape = geom::polygon<20, 4, colour_hue360(270, .6f),
                                  shape_flag::kDangerous | shape_flag::kVulnerable>;
  using shape = standard_transform<geom::conditional_p<2, big_shape, small_shape>>;

  std::tuple<vec2, fixed, bool> shape_parameters(const Transform& transform) const {
    return {transform.centre, transform.rotation, is_big_follow};
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

  void on_destroy(const Enemy& enemy, const Transform& transform, SimInterface& sim,
                  damage_type type) const {
    if (!is_big_follow || type == damage_type::kBomb) {
      return;
    }
    vec2 d = sim.rotate_compatibility(vec2{10, 0}, transform.rotation);
    for (std::uint32_t i = 0; i < 3; ++i) {
      spawn_follow(sim, transform.centre + d, enemy.score_reward != 0);
      d = sim.rotate_compatibility(d, 2 * fixed_c::pi / 3);
    }
  }
};

struct Chaser : ecs::component {
  static constexpr std::uint32_t kBoundingWidth = 10;
  static constexpr sound kDestroySound = sound::kEnemyShatter;

  static constexpr std::uint32_t kTime = 60;
  static constexpr fixed kSpeed = 4;
  using shape =
      standard_transform<geom::polygram<10, 4, colour_hue360(210, .6f),
                                        shape_flag::kDangerous | shape_flag::kVulnerable>>;

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
  add_enemy_health<Follow>(h, 3, sound::kPlayerDestroy);
  h.add(Follow{true});
  h.add(Enemy{.threat_value = 3, .score_reward = has_score ? 20u : 0});
}

void spawn_chaser(SimInterface& sim, const vec2& position) {
  auto h = create_ship<Chaser>(sim, position);
  add_enemy_health<Chaser>(h, 2);
  h.add(Chaser{});
  h.add(Enemy{.threat_value = 2, .score_reward = 30});
}

}  // namespace ii
