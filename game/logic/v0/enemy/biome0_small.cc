#include "game/common/colour.h"
#include "game/logic/v0/enemy/enemy.h"
#include "game/logic/v0/enemy/enemy_template.h"
#include <algorithm>

namespace ii::v0 {
namespace {
using namespace geom2;

ecs::handle spawn_follow(SimInterface& sim, std::uint32_t size, const vec2& position,
                         std::optional<vec2> direction, bool drop, fixed rotation = 0,
                         const vec2& initial_velocity = vec2{0});
ecs::handle spawn_chaser(SimInterface& sim, std::uint32_t size, const vec2& position, bool drop,
                         fixed rotation = 0, std::uint32_t stagger = 0);

struct Follow : ecs::component {
  static constexpr sound kDestroySound = sound::kEnemyShatter;
  static constexpr rumble_type kDestroyRumble = rumble_type::kSmall;

  static constexpr fixed kSpeed = 7_fx / 4_fx;
  static constexpr std::uint32_t kTime = 60;
  static constexpr std::uint32_t kSmallWidth = 11;
  static constexpr std::uint32_t kBigWidth = 22;
  static constexpr std::uint32_t kHugeWidth = 33;
  static constexpr auto kFlags = shape_flag::kDangerous | shape_flag::kVulnerable;

  static constexpr auto z = colour::kZEnemySmall;
  static constexpr auto c = colour::kNewPurple;
  static constexpr auto cf = colour::alpha(c, colour::kFillAlpha0);

  template <fixed Width>
  static void construct_shape(node& root) {
    auto& n = root.add(translate_rotate{.v = key{'v'}, .r = key{'r'}});
    n.add(ngon{.dimensions = nd(Width + 2, 4),
               .line = sline(colour::kOutline, colour::kZOutline, 2.f)});
    n.add(ngon{.dimensions = nd(Width, 4),
               .line = {.colour0 = key{'c'}, .z = z, .width = 1.5f},
               .fill = {.colour0 = key{'f'}, .z = z}});
    n.add(ngon_collider{.dimensions = nd(Width, 4), .flags = kFlags});
  }

  void set_parameters(const Transform& transform, const ColourOverride* colour,
                      parameter_set& parameters) const {
    parameters.add(key{'v'}, transform.centre)
        .add(key{'r'}, transform.rotation)
        .add(key{'c'}, colour ? colour->colour : c)
        .add(key{'f'}, colour ? colour::alpha(colour->colour, colour::kFillAlpha0) : cf);
  }

  Follow(std::uint32_t size, std::optional<vec2> direction, bool in_formation)
  : size{size}
  , in_formation{in_formation}
  , direction{direction}
  , spreader{.max_distance = 24_fx, .max_n = 6u} {}
  std::uint32_t timer = 0;
  std::uint32_t size = 0;
  bool in_formation = false;
  std::optional<vec2> direction;
  std::optional<ecs::entity_id> target;
  std::optional<ecs::entity_id> next_target;
  Spreader spreader;

  void update(ecs::handle h, Transform& transform, const Physics& physics, SimInterface& sim) {
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
    fixed spread_speed = 6 * kSpeed;
    if (physics.velocity != vec2{0}) {
      spread_speed = 0;
      move_scale = std::max(0_fx, 1_fx - length(physics.velocity) / 2_fx);
    }
    spreader.spread_closest<Follow>(
        h, transform, sim, spread_speed,
        Spreader::default_coefficient(2_fx, [](const Follow& f) { return 1_fx + f.size; }));
    if (!sim.alive_players()) {
      return;
    }

    if (!target) {
      target = sim.nearest_player(transform.centre).id();
    }
    if (++timer >= kTime) {
      if (next_target) {
        target = next_target;
      }
      next_target = sim.nearest_player(transform.centre).id();
      if (next_target == target) {
        next_target.reset();
      }
      timer = 0;
    }

    auto ph0 = sim.index().get(*target);
    auto d0 = ph0->get<Transform>()->centre - transform.centre;
    if (length_squared(d0) > square(kSpeed)) {
      d0 = kSpeed * normalise(d0);
    }
    if (next_target && timer >= kTime / 2) {
      auto ph1 = sim.index().get(*next_target);
      auto d1 = ph1->get<Transform>()->centre - transform.centre;
      if (length_squared(d1) > square(kSpeed)) {
        d1 = kSpeed * normalise(d1);
      }
      d0 = glm::mix(d0, d1, (2 * fixed{timer} - kTime) / kTime);
    }
    transform.move(d0 * move_scale);
  }

  void on_destroy(ecs::const_handle h, const Transform& transform, const EnemyStatus& status,
                  SimInterface& sim, EmitHandle&, damage_type type) const {
    if (!size || type == damage_type::kBomb || status.destroy_timer) {
      return;
    }
    vec2 d = ::rotate(vec2{10, 0}, transform.rotation);
    for (std::uint32_t i = 0; i < 5; ++i) {
      auto nh = spawn_follow(sim, size - 1, transform.centre + size * d, std::nullopt,
                             /* drop */ false, transform.rotation, d / 6);
      if (const auto* c = h.get<ColourOverride>(); c) {
        nh.add(*c);
      }
      d = ::rotate(d, 2 * pi<fixed> / 5);
    }
  }
};
DEBUG_STRUCT_TUPLE(Follow, timer, size, in_formation, direction, target, next_target, spreader);

template <fixed Width>
ecs::handle create_follow_ship(SimInterface& sim, std::uint32_t size, std::uint32_t health,
                               const vec2& position, std::optional<vec2> direction, fixed rotation,
                               const vec2& initial_velocity) {
  using shape = shape_definition<&Follow::construct_shape<Width>,
                                 ecs::call<&Follow::set_parameters>, Width, Follow::kFlags>;

  auto h = create_ship<Follow>(sim, position, rotation);
  add_render<Follow, shape>(h);
  add_collision<Follow, shape>(h);
  if (size) {
    add_enemy_health<Follow, shape>(h, health, sound::kPlayerDestroy, rumble_type::kMedium);
  } else {
    add_enemy_health<Follow, shape>(h, health);
  }
  h.add(Enemy{.threat_value = health / 8});
  h.add(Follow{size, direction, /* in formation */ initial_velocity == vec2{0, 0}});

  auto mass = 1_fx + size / 2_fx;
  auto& physics = h.add(Physics{.mass = mass});
  physics.apply_impulse(physics.mass * initial_velocity);
  return h;
}

ecs::handle spawn_follow(SimInterface& sim, std::uint32_t size, const vec2& position,
                         std::optional<vec2> direction, bool drop, fixed rotation,
                         const vec2& initial_velocity) {
  if (size == 2) {
    auto h = create_follow_ship<Follow::kHugeWidth>(sim, 2, 40, position, direction, rotation,
                                                    initial_velocity);
    if (drop) {
      h.add(DropTable{.shield_drop_chance = 15, .bomb_drop_chance = 30});
    }
    return h;
  }
  if (size == 1) {
    auto h = create_follow_ship<Follow::kBigWidth>(sim, 1, 24, position, direction, rotation,
                                                   initial_velocity);
    if (drop) {
      h.add(DropTable{.shield_drop_chance = 15, .bomb_drop_chance = 10});
    }
    return h;
  }
  auto h = create_follow_ship<Follow::kSmallWidth>(sim, 0, 8, position, direction, rotation,
                                                   initial_velocity);
  if (drop) {
    h.add(DropTable{.shield_drop_chance = 1, .bomb_drop_chance = 1});
  }
  return h;
}

struct Chaser : ecs::component {
  static constexpr sound kDestroySound = sound::kEnemyShatter;
  static constexpr rumble_type kDestroyRumble = rumble_type::kSmall;

  static constexpr fixed kSpeed = 15_fx / 4_fx;
  static constexpr std::uint32_t kTime = 80;
  static constexpr std::uint32_t kSmallWidth = 11;
  static constexpr std::uint32_t kBigWidth = 18;
  static constexpr auto kFlags = shape_flag::kDangerous | shape_flag::kVulnerable;

  static constexpr auto z = colour::kZEnemySmall;
  static constexpr auto c = colour::kSolarizedDarkCyan;
  static constexpr auto cf = colour::alpha(c, colour::kFillAlpha0);

  template <fixed Width>
  static void construct_shape(node& root) {
    auto& n = root.add(translate_rotate{.v = key{'v'}, .r = key{'r'}});
    n.add(ngon{.dimensions = nd(Width + 2, 4),
               .line = sline(colour::kOutline, colour::kZOutline, 2.f)});
    n.add(ngon{.dimensions = nd(Width, 4),
               .style = ngon_style::kPolygram,
               .line = sline(c, z),
               .fill = sfill(cf, z)});
    n.add(ngon_collider{.dimensions = nd(Width, 4), .flags = kFlags});
  }

  void set_parameters(const Transform& transform, parameter_set& parameters) const {
    parameters.add(key{'v'}, transform.centre).add(key{'r'}, transform.rotation);
  }

  Chaser(std::uint32_t size, std::uint32_t stagger)
  : timer{kTime - stagger}, size{size}, spreader{.max_distance = 24_fx, .max_n = 6u} {}
  std::uint32_t timer = 0;
  std::uint32_t size = 0;
  ecs::entity_id next_target_id{0};
  bool on_screen = false;
  bool is_moving = false;
  vec2 direction{0};
  Spreader spreader;

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

    auto spread_speed = (3_fx / 2_fx) * kSpeed * (is_moving ? fixed{kTime - timer} / kTime : 1_fx);
    spreader.spread_closest<Chaser>(h, transform, sim, spread_speed,
                                    Spreader::default_coefficient(1_fx, [&](const Chaser& c) {
                                      return is_moving == c.is_moving ? 1_fx + c.size : 0_fx;
                                    }));
  }

  void on_destroy(const Transform& transform, const EnemyStatus& status, SimInterface& sim,
                  EmitHandle&, damage_type type) const {
    if (!size || type == damage_type::kBomb || status.destroy_timer) {
      return;
    }
    vec2 d = ::rotate(vec2{12, 0}, transform.rotation);
    for (std::uint32_t i = 0; i < 3; ++i) {
      spawn_chaser(sim, size - 1, transform.centre + size * d, /* drop */ false, transform.rotation,
                   is_moving ? 10 * i : kTime - 10 * (1 + i));
      d = ::rotate(d, 2 * pi<fixed> / 3);
    }
  }
};
DEBUG_STRUCT_TUPLE(Chaser, timer, size, next_target_id, on_screen, is_moving, direction, spreader);

template <fixed Width>
ecs::handle create_chaser_ship(SimInterface& sim, std::uint32_t size, std::uint32_t health,
                               const vec2& position, fixed rotation, std::uint32_t stagger) {
  using shape = shape_definition<&Chaser::construct_shape<Width>,
                                 ecs::call<&Chaser::set_parameters>, Width, Chaser::kFlags>;

  auto h = create_ship<Chaser>(sim, position, rotation);
  add_render<Chaser, shape>(h);
  add_collision<Chaser, shape>(h);
  if (size) {
    add_enemy_health<Chaser, shape>(h, health, sound::kPlayerDestroy, rumble_type::kMedium);
  } else {
    add_enemy_health<Chaser, shape>(h, health);
  }
  h.add(Enemy{.threat_value = health / 8});
  h.add(Chaser{size, stagger});
  h.add(Physics{.mass = 1_fx + size / 2_fx});
  return h;
}

ecs::handle spawn_chaser(SimInterface& sim, std::uint32_t size, const vec2& position, bool drop,
                         fixed rotation, std::uint32_t stagger) {
  if (size == 1) {
    auto h = create_chaser_ship<Chaser::kBigWidth>(sim, 1, 32, position, rotation, stagger);
    if (drop) {
      h.add(DropTable{.shield_drop_chance = 10, .bomb_drop_chance = 20});
    }
    return h;
  }
  auto h = create_chaser_ship<Chaser::kSmallWidth>(sim, 0, 16, position, rotation, stagger);
  if (drop) {
    h.add(DropTable{.shield_drop_chance = 3, .bomb_drop_chance = 4});
  }
  return h;
}

struct FollowSponge : ecs::component {
  static constexpr sound kDestroySound = sound::kPlayerDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kLarge;

  static constexpr std::uint32_t kTime = 60;
  static constexpr fixed kSpeed = 7_fx / 8_fx;
  static constexpr fixed kBoundingWidth = 33;
  static constexpr auto kFlags = shape_flag::kDangerous | shape_flag::kVulnerable;

  static constexpr auto z = colour::kZEnemyMedium;
  static constexpr auto c = colour::kNewPurple;
  static constexpr auto cf = colour::alpha(c, colour::kFillAlpha0);

  static void construct_shape(node& root) {
    auto& n = root.add(translate_rotate{.v = key{'v'}, .r = key{'r'}});
    n.add(ngon{.dimensions = {.radius = key{'2'}, .inner_radius = 10_fx, .sides = 6},
               .line = sline(colour::kOutline, colour::kZOutline, 2.f)});
    n.add(ngon{.dimensions = {.radius = key{'0'}, .sides = 6}, .line = sline(c, z, 1.25f)});
    n.add(ngon{.dimensions = {.radius = key{'1'}, .inner_radius = 12_fx, .sides = 6},
               .line = sline(c, z, 2.f),
               .fill = sfill(cf, z)});
    n.add(ngon_collider{.dimensions = {.radius = key{'1'}, .sides = 6}, .flags = kFlags});
  }

  void set_parameters(const Transform& transform, parameter_set& parameters) const {
    auto anim_r = (14 + scale * kBoundingWidth) / 2 +
        (scale * kBoundingWidth - 22) * sin(fixed{anim} / 32) / 2;
    parameters.add(key{'v'}, transform.centre)
        .add(key{'r'}, transform.rotation)
        .add(key{'0'}, anim_r)
        .add(key{'1'}, scale * kBoundingWidth)
        .add(key{'2'}, scale * kBoundingWidth + 2);
  }

  FollowSponge() : spreader{.max_distance = 96_fx, .max_n = 4u} {}
  std::uint32_t timer = 0;
  std::uint32_t anim = 0;
  std::uint32_t spawns = 0;
  fixed scale = 0;
  std::optional<ecs::entity_id> target;
  std::optional<ecs::entity_id> next_target;
  Spreader spreader;

  void update(ecs::handle h, Transform& transform, const Health& health, SimInterface& sim) {
    ++anim;
    scale = (3_fx + fixed{health.hp} / health.max_hp) / 4;
    auto t = fixed{health.max_hp - health.hp} / health.max_hp;
    transform.rotate(t / 10 + fixed_c::tenth / 2);

    spreader.spread_closest<FollowSponge>(h, transform, sim, (1 + t) * 8,
                                          Spreader::linear_coefficient(2_fx));
    if (!sim.alive_players()) {
      return;
    }

    // TODO: acquire new target if current is dead. Stop if none.
    ++timer;
    if (!target || timer >= kTime) {
      (target ? next_target : target) = sim.nearest_player(transform.centre).id();
      timer = 0;
    }
    if (timer >= kTime / 5 && next_target) {
      target = next_target;
      next_target.reset();
    }

    auto ph = sim.index().get(*target);
    if (!ph->get<Player>()->is_killed) {
      auto d = ph->get<Transform>()->centre - transform.centre;
      bool on_screen = sim.is_on_screen(transform.centre + vec2{kBoundingWidth} / 2) ||
          sim.is_on_screen(transform.centre - vec2{kBoundingWidth} / 2);
      transform.move(normalise(d) * (on_screen ? 2 * t + kSpeed : (3_fx / 2) * kSpeed));
    }
  }

  void on_hit(const Transform& transform, const Health& health, SimInterface& sim, EmitHandle& eh,
              damage_type type, const vec2& source) {
    if (type == damage_type::kBomb || type == damage_type::kPredicted) {
      return;
    }
    auto target_spawns = (health.max_hp - health.hp) / 8;
    while (spawns < target_spawns) {
      eh.explosion(to_float(transform.centre), c, 6, std::nullopt, 2.f);
      auto a = angle(transform.centre - source) + (3_fx / 2) * sim.random_fixed() - 3_fx / 4;
      auto v = from_polar(a, 2_fx + 3_fx * sim.random_fixed());
      spawn_follow(sim, 0, transform.centre, std::nullopt, /* drop */ false, transform.rotation, v);
      ++spawns;
    }
    scale = (3_fx + fixed{health.hp} / health.max_hp) / 4;
  }
};
DEBUG_STRUCT_TUPLE(FollowSponge, timer, anim, spawns, scale, target, next_target, spreader);

}  // namespace

ecs::handle
spawn_follow(SimInterface& sim, const vec2& position, std::optional<vec2> direction, bool drop) {
  return spawn_follow(sim, /* size */ 0, position, direction, drop);
}

ecs::handle spawn_big_follow(SimInterface& sim, const vec2& position, std::optional<vec2> direction,
                             bool drop) {
  return spawn_follow(sim, /* size */ 1, position, direction, drop);
}

ecs::handle spawn_huge_follow(SimInterface& sim, const vec2& position,
                              std::optional<vec2> direction, bool drop) {
  return spawn_follow(sim, /* size */ 2, position, direction, drop);
}

ecs::handle spawn_chaser(SimInterface& sim, const vec2& position, bool drop) {
  return spawn_chaser(sim, /* size */ 0, position, drop);
}

ecs::handle spawn_big_chaser(SimInterface& sim, const vec2& position, bool drop) {
  return spawn_chaser(sim, /* size */ 1, position, drop);
}

ecs::handle spawn_follow_sponge(SimInterface& sim, const vec2& position) {
  auto h = create_ship_default<FollowSponge>(sim, position);
  add_enemy_health<FollowSponge>(h, 256, sound::kPlayerDestroy, rumble_type::kMedium);
  h.add(Enemy{.threat_value = 20u});
  h.add(FollowSponge{});
  h.add(Physics{.mass = 2_fx});
  h.add(DropTable{.shield_drop_chance = 30, .bomb_drop_chance = 40});
  return h;
}

}  // namespace ii::v0
