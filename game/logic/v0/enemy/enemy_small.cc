#include "game/common/colour.h"
#include "game/logic/geometry/shapes/ngon.h"
#include "game/logic/v0/enemy/enemy.h"
#include "game/logic/v0/enemy/enemy_template.h"
#include <algorithm>

namespace ii::v0 {
namespace {

void spawn_follow(SimInterface& sim, std::uint32_t size, const vec2& position,
                  std::optional<vec2> direction, bool drop, fixed rotation = 0,
                  const vec2& initial_velocity = vec2{0});
void spawn_chaser(SimInterface& sim, std::uint32_t size, const vec2& position, bool drop,
                  fixed rotation = 0, std::uint32_t stagger = 0);

struct Follow : ecs::component {
  static constexpr sound kDestroySound = sound::kEnemyShatter;
  static constexpr rumble_type kDestroyRumble = rumble_type::kSmall;

  static constexpr fixed kSpeed = 7_fx / 4_fx;
  static constexpr std::uint32_t kTime = 60;
  static constexpr std::uint32_t kSmallWidth = 11;
  static constexpr std::uint32_t kBigWidth = 22;
  static constexpr std::uint32_t kHugeWidth = 33;

  static constexpr auto z = colour::kZEnemySmall;
  static constexpr auto c = colour::hue360(270, .6f, .75f);
  static constexpr auto cf = colour::alpha(c, .25f);
  static constexpr auto outline = geom::nline(colour::kOutline, colour::kZOutline, 2.f);
  using small_shape =
      standard_transform<geom::ngon<geom::nd(kSmallWidth + 2, 4), outline>,
                         geom::ngon_with_collider<
                             geom::nd(kSmallWidth, 4), geom::nline(c, z, 1.5f), geom::sfill(cf, z),
                             shape_flag::kDangerous | shape_flag::kVulnerable>>;
  using big_shape = standard_transform<
      geom::ngon<geom::nd(kBigWidth + 2, 4), outline>,
      geom::ngon_with_collider<geom::nd(kBigWidth, 4), geom::nline(c, z, 1.5f), geom::sfill(cf, z),
                               shape_flag::kDangerous | shape_flag::kVulnerable>>;
  using huge_shape = standard_transform<
      geom::ngon<geom::nd(kHugeWidth + 2, 4), outline>,
      geom::ngon_with_collider<geom::nd(kHugeWidth, 4), geom::nline(c, z, 1.5f), geom::sfill(cf, z),
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
  std::vector<ecs::entity_id> closest;

  void update(ecs::handle h, Transform& transform, SimInterface& sim) {
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

    fixed move_scale = 1_fx;
    vec2 target_spread{0};
    if (extra_velocity == vec2{0}) {
      if ((+h.id() + sim.tick_count()) % 16 == 0) {
        thread_local std::vector<SimInterface::range_info> range_output;
        range_output.clear();
        closest.clear();
        sim.in_range(transform.centre, 24, ecs::id<Follow>(), /* max */ 6, range_output);
        for (const auto& e : range_output) {
          closest.emplace_back(e.h.id());
        }
      }
      for (auto id : closest) {
        if (auto eh = sim.index().get(id); eh && !eh->has<Destroy>()) {
          auto d = eh->get<Transform>()->centre - transform.centre;
          auto d_sq = length_squared(d);
          if (d != vec2{0}) {
            target_spread -=
                (1 + eh->get<Follow>()->size) * d / (sqrt(d_sq) * std::max(4_fx, d_sq));
          }
        }
      }
    } else {
      // TODO: make this some sort of new generic apply-force component?
      transform.move(extra_velocity);
      extra_velocity *= 31_fx / 32;
      auto t = length(extra_velocity);
      move_scale = std::max(0_fx, 1_fx - t / 2);
      if (t < 1_fx / 128_fx) {
        extra_velocity = vec2{0};
      }
    }
    spread_velocity = rc_smooth(spread_velocity, 6 * kSpeed * target_spread, 15_fx / 16_fx);
    transform.move(spread_velocity);
    if (!sim.alive_players()) {
      return;
    }

    // TODO: acquire new target if current is dead.
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
    if (!ph->get<Player>()->is_killed) {
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
      spawn_follow(sim, size - 1, transform.centre + size * d, std::nullopt, /* drop */ false,
                   transform.rotation, d / 6);
      d = rotate(d, 2 * fixed_c::pi / 5);
    }
  }
};
DEBUG_STRUCT_TUPLE(Follow, timer, size, in_formation, extra_velocity, spread_velocity, direction,
                   target, next_target, closest);

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
                  std::optional<vec2> direction, bool drop, fixed rotation,
                  const vec2& initial_velocity) {
  if (size == 2) {
    auto h = create_follow_ship<Follow::huge_shape>(sim, 2, 40, Follow::kHugeWidth, position,
                                                    direction, rotation, initial_velocity);
    if (drop) {
      h.add(DropTable{.shield_drop_chance = 15, .bomb_drop_chance = 30});
    }
  } else if (size == 1) {
    auto h = create_follow_ship<Follow::big_shape>(sim, 1, 24, Follow::kBigWidth, position,
                                                   direction, rotation, initial_velocity);
    if (drop) {
      h.add(DropTable{.shield_drop_chance = 15, .bomb_drop_chance = 10});
    }
  } else {
    auto h = create_follow_ship<Follow::small_shape>(sim, 0, 8, Follow::kSmallWidth, position,
                                                     direction, rotation, initial_velocity);
    if (drop) {
      h.add(DropTable{.shield_drop_chance = 1, .bomb_drop_chance = 1});
    }
  }
}

struct Chaser : ecs::component {
  static constexpr sound kDestroySound = sound::kEnemyShatter;
  static constexpr rumble_type kDestroyRumble = rumble_type::kSmall;

  static constexpr fixed kSpeed = 15_fx / 4_fx;
  static constexpr std::uint32_t kTime = 80;
  static constexpr std::uint32_t kSmallWidth = 11;
  static constexpr std::uint32_t kBigWidth = 18;

  static constexpr auto z = colour::kZEnemySmall;
  static constexpr auto c = colour::kSolarizedDarkCyan;
  static constexpr auto cf = colour::alpha(c, .25f);
  static constexpr auto outline = geom::nline(colour::kOutline, colour::kZOutline, 2.f);
  using small_shape = standard_transform<
      geom::ngon<geom::nd(kSmallWidth + 2, 4), outline>,
      geom::ngon_with_collider<geom::nd(kSmallWidth, 4),
                               geom::nline(geom::ngon_style::kPolygram, c, z), geom::sfill(cf, z),
                               shape_flag::kDangerous | shape_flag::kVulnerable>>;
  using big_shape =
      standard_transform<geom::ngon<geom::nd(kBigWidth + 2, 4), outline>,
                         geom::ngon_with_collider<
                             geom::nd(kBigWidth, 4), geom::nline(geom::ngon_style::kPolygram, c, z),
                             geom::sfill(cf, z), shape_flag::kDangerous | shape_flag::kVulnerable>>;

  Chaser(std::uint32_t size, std::uint32_t stagger) : timer{kTime - stagger}, size{size} {}
  std::uint32_t timer = 0;
  std::uint32_t size = 0;
  ecs::entity_id next_target_id{0};
  bool on_screen = false;
  bool is_moving = false;
  vec2 direction{0};
  vec2 spread_velocity{0};
  std::vector<ecs::entity_id> closest;

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
        direction =
            normalise(sim.index().get(next_target_id)->get<Transform>()->centre - transform.centre);
        next_target_id = ecs::entity_id{0};
      }
    }
    if (is_moving) {
      transform.move(kSpeed * (1_fx + (size && sim.is_on_screen(transform.centre)) * 1_fx / 4_fx) *
                     direction);

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

    if ((+h.id() + sim.tick_count()) % 16 == 0) {
      thread_local std::vector<SimInterface::range_info> range_output;
      range_output.clear();
      closest.clear();
      sim.in_range(transform.centre, 24, ecs::id<Chaser>(), /* max */ 6, range_output);
      for (const auto& e : range_output) {
        closest.emplace_back(e.h.id());
      }
    }
    for (auto id : closest) {
      if (auto eh = sim.index().get(id); id != h.id() && eh && !eh->has<Destroy>()) {
        if (is_moving != eh->get<Chaser>()->is_moving) {
          continue;
        }
        auto d = eh->get<Transform>()->centre - transform.centre;
        auto d_sq = length_squared(d);
        if (d != vec2{0}) {
          target_spread -= (1 + eh->get<Chaser>()->size) * d / (sqrt(d_sq) * std::max(1_fx, d_sq));
        }
      }
    }
    target_spread *= (3_fx / 2_fx) * kSpeed * (is_moving ? fixed{kTime - timer} / kTime : 1_fx);
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
      spawn_chaser(sim, size - 1, transform.centre + size * d, /* drop */ false, transform.rotation,
                   is_moving ? 10 * i : kTime - 10 * (1 + i));
      d = rotate(d, 2 * fixed_c::pi / 3);
    }
  }
};
DEBUG_STRUCT_TUPLE(Chaser, timer, size, next_target_id, on_screen, is_moving, direction,
                   spread_velocity, closest);

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

void spawn_chaser(SimInterface& sim, std::uint32_t size, const vec2& position, bool drop,
                  fixed rotation, std::uint32_t stagger) {
  if (size == 1) {
    auto h = create_chaser_ship<Chaser::big_shape>(sim, 1, 32, Chaser::kBigWidth, position,
                                                   rotation, stagger);
    if (drop) {
      h.add(DropTable{.shield_drop_chance = 10, .bomb_drop_chance = 20});
    }
  } else {
    auto h = create_chaser_ship<Chaser::small_shape>(sim, 0, 16, Chaser::kSmallWidth, position,
                                                     rotation, stagger);
    if (drop) {
      h.add(DropTable{.shield_drop_chance = 3, .bomb_drop_chance = 4});
    }
  }
}

}  // namespace

void spawn_follow(SimInterface& sim, const vec2& position, std::optional<vec2> direction,
                  bool drop) {
  spawn_follow(sim, /* size */ 0, position, direction, drop);
}

void spawn_big_follow(SimInterface& sim, const vec2& position, std::optional<vec2> direction,
                      bool drop) {
  spawn_follow(sim, /* size */ 1, position, direction, drop);
}

void spawn_huge_follow(SimInterface& sim, const vec2& position, std::optional<vec2> direction,
                       bool drop) {
  spawn_follow(sim, /* size */ 2, position, direction, drop);
}

void spawn_chaser(SimInterface& sim, const vec2& position, bool drop) {
  spawn_chaser(sim, /* size */ 0, position, drop);
}

void spawn_big_chaser(SimInterface& sim, const vec2& position, bool drop) {
  spawn_chaser(sim, /* size */ 1, position, drop);
}

}  // namespace ii::v0
