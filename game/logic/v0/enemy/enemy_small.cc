#include "game/logic/geometry/shapes/ngon.h"
#include "game/logic/v0/enemy/enemy.h"
#include "game/logic/v0/enemy/enemy_template.h"
#include <algorithm>

namespace ii::v0 {
namespace {

void spawn_follow(SimInterface& sim, std::uint32_t size, const vec2& position,
                  std::optional<vec2> direction, fixed rotation = 0,
                  const vec2& initial_velocity = vec2{0});
void spawn_chaser(SimInterface& sim, std::uint32_t size, const vec2& position, fixed rotation = 0,
                  std::uint32_t stagger = 0);

struct Follow : ecs::component {
  static constexpr float kZIndex = 8.f;
  static constexpr sound kDestroySound = sound::kEnemyShatter;
  static constexpr rumble_type kDestroyRumble = rumble_type::kSmall;

  static constexpr fixed kSpeed = 7_fx / 4_fx;
  static constexpr std::uint32_t kTime = 90;
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

  Follow(std::uint32_t size, std::optional<vec2> direction, const vec2& extra_velocity)
  : size{size}
  , in_formation{extra_velocity == vec2{0, 0}}
  , extra_velocity{extra_velocity}
  , direction{direction} {}
  std::uint32_t timer = 0;
  std::uint32_t size = 0;
  bool in_formation = false;
  vec2 extra_velocity{0};
  vec2 spread_velocity{0};
  std::optional<vec2> direction;
  std::optional<ecs::entity_id> target;
  std::optional<ecs::entity_id> next_target;

  void update(Transform& transform, SimInterface& sim) {
    transform.rotate(fixed_c::tenth / (1_fx + size * 1_fx / 2_fx));

    if (sim.is_on_screen(transform.centre)) {
      in_formation = false;
    } else if (in_formation) {
      if (!direction || !check_direction_to_screen(sim, transform.centre, *direction)) {
        direction = normalise(direction_to_screen(sim, transform.centre));
      }
      transform.move(*direction * kSpeed);
      return;
    }

    // TODO: make this some sort of new generic apply-force component?
    fixed move_scale = 1_fx;
    vec2 target_spread{0};
    if (extra_velocity == vec2{0}) {
      for (const auto& eh : sim.in_range(transform.centre, 32, ecs::id<Follow>())) {
        auto d = transform.centre - eh.get<Transform>()->centre;
        if (d != vec2{0}) {
          target_spread +=
              (1 + eh.get<Follow>()->size) * normalise(d) / std::max(4_fx, length_squared(d));
        }
      }
    } else {
      transform.move(extra_velocity);
      extra_velocity *= 31_fx / 32;
      auto t = length(extra_velocity);
      move_scale = std::max(0_fx, 1_fx - t / 2);
      if (t < 1_fx / 128_fx) {
        extra_velocity = vec2{0};
      }
    }
    spread_velocity = rc_smooth(spread_velocity, 5 * kSpeed * target_spread, 15_fx / 16_fx);
    transform.move(spread_velocity);
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
      spawn_follow(sim, size - 1, transform.centre + size * d, std::nullopt, transform.rotation,
                   d / 6);
      d = rotate(d, 2 * fixed_c::pi / 5);
    }
  }
};
DEBUG_STRUCT_TUPLE(Follow, timer, size, in_formation, extra_velocity, spread_velocity, direction,
                   target, next_target);

template <geom::ShapeNode S>
ecs::handle create_follow_ship(SimInterface& sim, std::uint32_t size, std::uint32_t health,
                               fixed width, const vec2& position, std::optional<vec2> direction,
                               fixed rotation, const vec2& initial_velocity) {
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
  h.add(Follow{size, direction, initial_velocity});
  return h;
}

void spawn_follow(SimInterface& sim, std::uint32_t size, const vec2& position,
                  std::optional<vec2> direction, fixed rotation, const vec2& initial_velocity) {
  if (size == 2) {
    create_follow_ship<Follow::huge_shape>(sim, 2, 5, Follow::kHugeWidth, position, direction,
                                           rotation, initial_velocity);
  } else if (size == 1) {
    create_follow_ship<Follow::big_shape>(sim, 1, 3, Follow::kBigWidth, position, direction,
                                          rotation, initial_velocity);
  } else {
    create_follow_ship<Follow::small_shape>(sim, 0, 1, Follow::kSmallWidth, position, direction,
                                            rotation, initial_velocity);
  }
}

struct Chaser : ecs::component {
  static constexpr float kZIndex = 8.f;
  static constexpr sound kDestroySound = sound::kEnemyShatter;
  static constexpr rumble_type kDestroyRumble = rumble_type::kSmall;

  static constexpr fixed kSpeed = 15_fx / 4_fx;
  static constexpr std::uint32_t kTime = 80;
  static constexpr std::uint32_t kSmallWidth = 11;
  static constexpr std::uint32_t kBigWidth = 18;

  using small_shape =
      standard_transform<geom::polygram<11, 4, colour_hue360(210, .6f),
                                        shape_flag::kDangerous | shape_flag::kVulnerable>>;
  using big_shape =
      standard_transform<geom::polygram<18, 4, colour_hue360(210, .6f),
                                        shape_flag::kDangerous | shape_flag::kVulnerable>>;

  Chaser(std::uint32_t size, std::uint32_t stagger) : size{size}, timer{kTime - stagger} {}
  std::uint32_t timer = 0;
  std::uint32_t size = 0;
  ecs::entity_id next_target_id{0};
  bool on_screen = false;
  bool is_moving = false;
  vec2 direction{0};
  vec2 spread_velocity{0};

  void update(ecs::handle h, Transform& transform, SimInterface& sim) {
    bool was_on_screen = sim.is_on_screen(transform.centre);
    timer && --timer;
    if (!is_moving && !next_target_id && timer <= kTime / 4) {
      next_target_id = sim.nearest_player(transform.centre).id();
    }
    if (!timer) {
      is_moving = !is_moving;
      timer = kTime * (is_moving ? 1 : 2);
      if (is_moving) {
        direction = kSpeed * (1_fx + size * 1_fx / 4_fx) *
            normalise(sim.index().get(next_target_id)->get<Transform>()->centre - transform.centre);
        next_target_id = ecs::entity_id{0};
      }
    }
    if (is_moving) {
      transform.move(direction);

      if (was_on_screen && !sim.is_on_screen(transform.centre)) {
        on_screen = false;
      } else if (!on_screen && sim.is_on_screen(transform.centre)) {
        on_screen = true;
        is_moving = false;
      }
    } else {
      if (sim.is_on_screen(transform.centre)) {
        on_screen = true;
      }
      transform.rotate(fixed_c::tenth / (1_fx + size * 1_fx / 2_fx));
    }

    vec2 target_spread{0};
    for (const auto& eh : sim.in_range(transform.centre, 32, ecs::id<Chaser>())) {
      if (eh.id() == h.id() || (is_moving != eh.get<Chaser>()->is_moving)) {
        continue;
      }
      auto d = transform.centre - eh.get<Transform>()->centre;
      if (d != vec2{0}) {
        target_spread +=
            (1 + eh.get<Chaser>()->size) * normalise(d) / std::max(1_fx, length_squared(d));
      }
    }
    target_spread *= kSpeed * (is_moving ? fixed{kTime - timer} / kTime : 1_fx);
    spread_velocity = rc_smooth(spread_velocity, target_spread, 15_fx / 16_fx);
    transform.move(spread_velocity);
  }

  void
  on_destroy(const Transform& transform, SimInterface& sim, EmitHandle&, damage_type type) const {
    if (!size || type == damage_type::kBomb) {
      return;
    }
    vec2 d = rotate(vec2{12, 0}, transform.rotation);
    for (std::uint32_t i = 0; i < 3; ++i) {
      spawn_chaser(sim, size - 1, transform.centre + size * d, transform.rotation,
                   is_moving ? 10 * i : kTime - 10 * (1 + i));
      d = rotate(d, 2 * fixed_c::pi / 3);
    }
  }
};
DEBUG_STRUCT_TUPLE(Chaser, timer, size, next_target_id, on_screen, is_moving, direction,
                   spread_velocity);

template <geom::ShapeNode S>
ecs::handle
create_chaser_ship(SimInterface& sim, std::uint32_t size, std::uint32_t health, fixed width,
                   const vec2& position, fixed rotation, std::uint32_t stagger) {
  auto h = create_ship<Chaser>(sim, position, rotation);
  add_render<Chaser, S>(h);
  h.add(Collision{.flags = get_shape_flags<Chaser, S>(),
                  .bounding_width = width,
                  .check = &ship_check_point<Chaser, S>});
  if (size) {
    add_enemy_health<Chaser, S>(h, health, sound::kPlayerDestroy, rumble_type::kMedium);
  } else {
    add_enemy_health<Chaser, S>(h, health);
  }
  h.add(Enemy{.threat_value = health});
  h.add(Chaser{size, stagger});
  return h;
}

void spawn_chaser(SimInterface& sim, std::uint32_t size, const vec2& position, fixed rotation,
                  std::uint32_t stagger) {
  if (size == 1) {
    create_chaser_ship<Chaser::big_shape>(sim, 1, 4, Chaser::kBigWidth, position, rotation,
                                          stagger);
  } else {
    create_chaser_ship<Chaser::small_shape>(sim, 0, 2, Chaser::kSmallWidth, position, rotation,
                                            stagger);
  }
}

}  // namespace

void spawn_follow(SimInterface& sim, const vec2& position, std::optional<vec2> direction) {
  spawn_follow(sim, /* size */ 0, position, direction);
}

void spawn_big_follow(SimInterface& sim, const vec2& position, std::optional<vec2> direction) {
  spawn_follow(sim, /* size */ 1, position, direction);
}

void spawn_huge_follow(SimInterface& sim, const vec2& position, std::optional<vec2> direction) {
  spawn_follow(sim, /* size */ 2, position, direction);
}

void spawn_chaser(SimInterface& sim, const vec2& position) {
  spawn_chaser(sim, /* size */ 0, position);
}

void spawn_big_chaser(SimInterface& sim, const vec2& position) {
  spawn_chaser(sim, /* size */ 1, position);
}

}  // namespace ii::v0
