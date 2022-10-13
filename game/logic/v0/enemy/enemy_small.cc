#include "game/logic/geometry/shapes/ngon.h"
#include "game/logic/v0/enemy/enemy.h"
#include "game/logic/v0/enemy/enemy_template.h"
#include <algorithm>

namespace ii::v0 {
namespace {

void spawn_follow(SimInterface& sim, const vec2& position, std::uint32_t size, fixed rotation = 0,
                  const vec2& initial_velocity = vec2{0});

struct Follow : ecs::component {
  static constexpr float kZIndex = 8.f;
  static constexpr sound kDestroySound = sound::kEnemyShatter;
  static constexpr rumble_type kDestroyRumble = rumble_type::kSmall;

  static constexpr std::uint32_t kTime = 90;
  static constexpr fixed kSpeed = 2;

  static constexpr std::uint32_t kSmallWidth = 11;
  static constexpr std::uint32_t kBigWidth = 22;
  static constexpr std::uint32_t kHugeWidth = 33;

  using small_shape =
      standard_transform<geom::polygon<11, 4, colour_hue360(270, .6f),
                                       shape_flag::kDangerous | shape_flag::kVulnerable>>;
  using big_shape =
      standard_transform<geom::polygon<22, 4, colour_hue360(270, .6f),
                                       shape_flag::kDangerous | shape_flag::kVulnerable>>;
  using huge_shape =
      standard_transform<geom::polygon<33, 4, colour_hue360(270, .6f),
                                       shape_flag::kDangerous | shape_flag::kVulnerable>>;

  Follow(std::uint32_t size, const vec2& extra_velocity)
  : size{size}, extra_velocity{extra_velocity} {}
  std::uint32_t timer = 0;
  std::uint32_t size = 0;
  vec2 extra_velocity{0};
  std::optional<ecs::entity_id> target;
  std::optional<ecs::entity_id> next_target;

  void update(Transform& transform, SimInterface& sim) {
    transform.rotate(fixed_c::tenth / (1_fx + size * 1_fx / 2_fx));
    fixed move_scale = 1_fx;
    if (extra_velocity != vec2{0}) {
      transform.move(extra_velocity);
      extra_velocity *= 31_fx / 32;
      auto t = length(extra_velocity);
      move_scale = std::max(0_fx, 1_fx - t / 2);
      if (t < 1_fx / 128_fx) {
        extra_velocity = vec2{0};
      }
    }
    if (!sim.alive_players()) {
      return;
    }

    ++timer;
    if (!target || timer >= kTime) {
      (target ? next_target : target) = sim.nearest_player(transform.centre).id();
      timer = 0;
    }
    if (timer >= kTime / 3 && next_target) {
      target = next_target;
      next_target.reset();
    }

    auto ph = sim.index().get(*target);
    if (!ph->get<Player>()->is_killed()) {
      auto d = ph->get<Transform>()->centre - transform.centre;
      transform.move(normalise(d) * move_scale * kSpeed);
    }
  }

  void
  on_destroy(const Transform& transform, SimInterface& sim, EmitHandle&, damage_type type) const {
    if (!size || type == damage_type::kBomb) {
      return;
    }
    vec2 d = rotate(vec2{10, 0}, transform.rotation);
    for (std::uint32_t i = 0; i < 5; ++i) {
      spawn_follow(sim, transform.centre + size * d, size - 1, transform.rotation, d / 6);
      d = rotate(d, 2 * fixed_c::pi / 5);
    }
  }
};
DEBUG_STRUCT_TUPLE(Follow, timer, size, extra_velocity, target, next_target);

template <geom::ShapeNode S>
ecs::handle
create_follow_ship(SimInterface& sim, const vec2& position, std::uint32_t health,
                   std::uint32_t size, fixed width, fixed rotation, const vec2& initial_velocity) {
  auto h = create_ship<Follow>(sim, position, rotation);
  add_render<Follow, S>(h);
  h.add(Collision{.flags = get_shape_flags<Follow, S>(),
                  .bounding_width = width,
                  .check = &ship_check_point<Follow, S>});
  if (size) {
    add_enemy_health<Follow, S>(h, health, sound::kPlayerDestroy, rumble_type::kMedium);
  } else {
    add_enemy_health<Follow, S>(h, health);
  }
  h.add(Enemy{.threat_value = health});
  h.add(Follow{size, initial_velocity});
  return h;
}

void spawn_follow(SimInterface& sim, const vec2& position, std::uint32_t size, fixed rotation,
                  const vec2& initial_velocity) {
  if (size == 2) {
    create_follow_ship<Follow::huge_shape>(sim, position, 5, 2, Follow::kHugeWidth, rotation,
                                           initial_velocity);
  } else if (size == 1) {
    create_follow_ship<Follow::big_shape>(sim, position, 3, 1, Follow::kBigWidth, rotation,
                                          initial_velocity);
  } else {
    create_follow_ship<Follow::small_shape>(sim, position, 1, 0, Follow::kSmallWidth, rotation,
                                            initial_velocity);
  }
}

}  // namespace

void spawn_follow(SimInterface& sim, const vec2& position) {
  spawn_follow(sim, position, /* size */ 0);
}

void spawn_big_follow(SimInterface& sim, const vec2& position) {
  spawn_follow(sim, position, /* size */ 1);
}

void spawn_huge_follow(SimInterface& sim, const vec2& position) {
  spawn_follow(sim, position, /* size */ 2);
}

}  // namespace ii::v0
