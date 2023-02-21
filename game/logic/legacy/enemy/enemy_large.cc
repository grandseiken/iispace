#include "game/common/colour.h"
#include "game/logic/legacy/enemy/enemy.h"
#include "game/logic/legacy/ship_template.h"

namespace ii::legacy {
namespace {
using namespace geom2;

struct FollowHub : ecs::component {
  static constexpr float kZIndex = 0.f;
  static constexpr sound kDestroySound = sound::kPlayerDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kLarge;

  static constexpr std::uint32_t kTimer = 170;
  static constexpr fixed kSpeed = 1;
  static constexpr fixed kBoundingWidth = 16;
  static constexpr auto kFlags = shape_flag::kDangerous | shape_flag::kVulnerable;
  static constexpr auto c = colour::hue360(240, .7f);

  static void construct_shape(node& root) {
    auto& n = root.add(translate{key{'v'}});
    auto& centre = n.add(rotate{pi<fixed> / 4});
    auto& r = n.add(rotate{key{'r'}});

    centre.add(
        ngon{.dimensions = nd(16, 4), .style = ngon_style::kPolygram, .line = {.colour0 = c}});
    centre.add(ball_collider{.dimensions = bd(16), .flags = kFlags});

    auto& spoke = root.create(compound{});
    r.add(translate{vec2{16, 0}}).add(spoke);
    r.add(translate{vec2{-16, 0}}).add(spoke);
    r.add(translate{vec2{0, 16}}).add(spoke);
    r.add(translate{vec2{0, -16}}).add(spoke);
    auto& spoke_r = spoke.add(rotate{pi<fixed> / 4});
    spoke_r.add(ngon{.dimensions = nd(8, 4), .line = {.colour0 = c}});
    spoke_r.add(enable{key{'p'}})
        .add(ngon{.dimensions = nd(8, 4), .style = ngon_style::kPolystar, .line = {.colour0 = c}});
  }

  void set_parameters(const Transform& transform, parameter_set& parameters) const {
    parameters.add(key{'v'}, transform.centre)
        .add(key{'r'}, transform.rotation)
        .add(key{'p'}, power_b);
  }

  FollowHub(bool power_a, bool power_b) : power_a{power_a}, power_b{power_b} {}
  std::uint32_t timer = 0;
  vec2 dir{0};
  std::uint32_t count = 0;
  bool power_a = false;
  bool power_b = false;

  void update(Transform& transform, SimInterface& sim) {
    ++timer;
    if (timer > (power_a ? kTimer / 2 : kTimer)) {
      timer = 0;
      ++count;
      if (sim.is_on_screen(transform.centre)) {
        if (power_b) {
          spawn_chaser(sim, transform.centre);
        } else {
          spawn_follow(sim, transform.centre);
        }
        sim.emit(resolve_key::predicted()).play_random(sound::kEnemySpawn, transform.centre);
      }
    }

    auto d = sim.dimensions();
    dir = transform.centre.x < 0   ? vec2{1, 0}
        : transform.centre.x > d.x ? vec2{-1, 0}
        : transform.centre.y < 0   ? vec2{0, 1}
        : transform.centre.y > d.y ? vec2{0, -1}
        : count > 3                ? (count = 0, sim.rotate_compatibility(dir, -pi<fixed> / 2))
                                   : dir;

    auto s = power_a ? fixed_c::hundredth * 5 + fixed_c::tenth : fixed_c::hundredth * 5;
    transform.rotate(s);
    transform.move(dir * kSpeed);
  }

  void
  on_destroy(const Transform& transform, SimInterface& sim, EmitHandle&, damage_type type) const {
    if (type == damage_type::kBomb) {
      return;
    }
    if (power_b) {
      spawn_big_follow(sim, transform.centre, true);
    }
    spawn_chaser(sim, transform.centre);
  }
};
DEBUG_STRUCT_TUPLE(FollowHub, timer, dir, count, power_a, power_b);

struct Shielder : ecs::component {
  static constexpr float kZIndex = 0.f;
  static constexpr sound kDestroySound = sound::kPlayerDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kLarge;

  static constexpr std::uint32_t kTimer = 80;
  static constexpr fixed kSpeed = 2;
  static constexpr fixed kBoundingWidth = 36;
  static constexpr auto kFlags =
      shape_flag::kDangerous | shape_flag::kVulnerable | shape_flag::kWeakShield;

  static constexpr auto c0 = colour::hue360(150, .2f);
  static constexpr auto c1 = colour::hue360(160, .5f, .6f);

  static void construct_shape(node& root) {
    auto& n = root.add(translate_rotate{key{'v'}, key{'r'}});
    n.add(ngon{.dimensions = nd(24, 4), .style = ngon_style::kPolystar, .line = {.colour0 = c0}});

    auto& centre = n.add(rotate{key{'R'}});
    centre.add(ngon{.dimensions = nd(14, 8), .line = {.colour0 = key{'c'}}});
    centre.add(ball_collider{.dimensions = bd(14),
                             .flags = shape_flag::kDangerous | shape_flag::kVulnerable});

    auto& spoke = root.create(rotate{key{'R'}});
    n.add(translate{vec2{24, 0}}).add(spoke);
    n.add(translate{vec2{-24, 0}}).add(spoke);
    n.add(translate_rotate{vec2{0, 24}, pi<fixed> / 2}).add(spoke);
    n.add(translate_rotate{vec2{0, -24}, pi<fixed> / 2}).add(spoke);

    spoke.add(
        ngon{.dimensions = nd(8, 6), .style = ngon_style::kPolystar, .line = {.colour0 = c0}});
    spoke.add(ngon{.dimensions = nd(8, 6), .line = {.colour0 = c1}});
    spoke.add(ball_collider{.dimensions = bd(8), .flags = shape_flag::kWeakShield});
  }

  void set_parameters(const Transform& transform, parameter_set& parameters) const {
    parameters.add(key{'v'}, transform.centre)
        .add(key{'r'}, transform.rotation)
        .add(key{'R'}, -transform.rotation)
        .add(key{'c'}, power ? c1 : c0);
  }

  Shielder(bool power) : power{power} {}
  vec2 dir{0, 1};
  std::uint32_t timer = 0;
  bool is_rotating = false;
  bool anti = false;
  bool power = false;

  void update(Transform& transform, const Health& health, SimInterface& sim) {
    fixed s = power ? fixed_c::hundredth * 12 : fixed_c::hundredth * 4;
    transform.rotate(s);

    bool on_screen = false;
    auto dim = sim.dimensions();
    dir = transform.centre.x < 0     ? vec2{1, 0}
        : transform.centre.x > dim.x ? vec2{-1, 0}
        : transform.centre.y < 0     ? vec2{0, 1}
        : transform.centre.y > dim.y ? vec2{0, -1}
                                     : (on_screen = true, dir);

    if (!on_screen && is_rotating) {
      timer = 0;
      is_rotating = false;
    }

    fixed speed = kSpeed + (power ? fixed_c::tenth * 3 : fixed_c::tenth * 2) * (16 - health.hp);
    if (is_rotating) {
      auto d = sim.rotate_compatibility(
          dir, (anti ? 1 : -1) * (kTimer - timer) * pi<fixed> / (2 * kTimer));
      if (!--timer) {
        is_rotating = false;
        dir = sim.rotate_compatibility(dir, (anti ? 1 : -1) * pi<fixed> / 2);
      }
      transform.move(d * speed);
    } else {
      ++timer;
      if (timer > kTimer * 2) {
        timer = kTimer;
        is_rotating = true;
        anti = sim.random_bool();
      }
      if (sim.is_on_screen(transform.centre) && power && timer % kTimer == kTimer / 2) {
        spawn_boss_shot(sim, transform.centre, 3 * sim.nearest_player_direction(transform.centre),
                        colour::hue360(160, .5f, .6f));
        sim.emit(resolve_key::predicted()).play_random(sound::kBossFire, transform.centre);
      }
      transform.move(dir * speed);
    }
    dir = normalise(dir);
  }
};
DEBUG_STRUCT_TUPLE(Shielder, dir, timer, is_rotating, anti, power);

struct Tractor : ecs::component {
  static constexpr float kZIndex = 0.f;
  static constexpr sound kDestroySound = sound::kPlayerDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kLarge;

  static constexpr std::uint32_t kTimer = 50;
  static constexpr fixed kSpeed = 6 * (1_fx / 10);
  static constexpr fixed kPullSpeed = 2 + 1_fx / 2;
  static constexpr fixed kBoundingWidth = 36;
  static constexpr auto kFlags = shape_flag::kDangerous | shape_flag::kVulnerable;
  static constexpr auto c = colour::hue360(300, .5f, .6f);

  static void construct_shape(node& root) {
    auto& n = root.add(translate_rotate{key{'v'}, key{'r'}});
    auto& orb = root.create(compound{});
    auto& star = root.create(compound{});

    n.add(translate_rotate{vec2{24, 0}, key{'S'}}).add(orb);
    n.add(translate_rotate{vec2{-24, 0}, key{'s'}}).add(orb);
    n.add(line{.a = vec2{-24, 0}, .b = vec2{24, 0}, .style = {.colour0 = c}});

    auto& star_if = n.add(enable{key{'p'}});
    star_if.add(translate_rotate{vec2{24, 0}, key{'T'}}).add(star);
    star_if.add(translate_rotate{vec2{-24, 0}, key{'t'}}).add(star);

    orb.add(ngon{.dimensions = nd(12, 6), .style = ngon_style::kPolygram, .line = {.colour0 = c}});
    orb.add(ball_collider{.dimensions = bd(12), .flags = kFlags});
    star.add(ngon{.dimensions = nd(16, 6), .style = ngon_style::kPolystar, .line = {.colour0 = c}});
  }

  void set_parameters(const Transform& transform, parameter_set& parameters) const {
    parameters.add(key{'v'}, transform.centre)
        .add(key{'r'}, transform.rotation)
        .add(key{'s'}, -5 * spoke_r)
        .add(key{'S'}, 5 * spoke_r)
        .add(key{'t'}, -8 * spoke_r)
        .add(key{'T'}, 8 * spoke_r)
        .add(key{'p'}, power);
  }

  std::uint32_t timer = kTimer * 4;
  vec2 dir{0};
  bool power = false;
  bool ready = false;
  bool spinning = false;
  fixed spoke_r = 0;

  Tractor(bool power) : power{power} {}

  void update(Transform& transform, SimInterface& sim) {
    spoke_r = normalise_angle(spoke_r + fixed_c::hundredth);

    auto dim = sim.dimensions();
    if (transform.centre.x < 0) {
      dir = vec2{1, 0};
    } else if (transform.centre.x > dim.x) {
      dir = vec2{-1, 0};
    } else if (transform.centre.y < 0) {
      dir = vec2{0, 1};
    } else if (transform.centre.y > dim.y) {
      dir = vec2{0, -1};
    } else {
      ++timer;
    }

    if (!ready && !spinning) {
      transform.move(dir * kSpeed * (sim.is_on_screen(transform.centre) ? 1 : 2 + fixed_c::half));

      if (timer > kTimer * 8) {
        ready = true;
        timer = 0;
      }
    } else if (ready) {
      if (timer > kTimer) {
        ready = false;
        spinning = true;
        timer = 0;
        sim.emit(resolve_key::predicted()).play(sound::kBossFire, transform.centre);
      }
    } else if (spinning) {
      transform.rotate(fixed_c::tenth * 3);
      sim.index().iterate_dispatch<Player>([&](const Player& p, Transform& p_transform) {
        if (!p.is_killed) {
          p_transform.centre += normalise(transform.centre - p_transform.centre) * kPullSpeed;
        }
      });
      if (timer % (kTimer / 2) == 0 && sim.is_on_screen(transform.centre) && power) {
        spawn_boss_shot(sim, transform.centre, 4 * sim.nearest_player_direction(transform.centre),
                        colour::hue360(300, .5f, .6f));
        sim.emit(resolve_key::predicted()).play_random(sound::kBossFire, transform.centre);
      }

      if (timer > kTimer * 5) {
        spinning = false;
        timer = 0;
      }
    }
  }

  void render(const Transform& transform, std::vector<render::shape>& output,
              const SimInterface& sim) const {
    if (spinning) {
      std::uint32_t i = 0;
      sim.index().iterate_dispatch<Player>([&](const Player& p, const Transform& p_transform) {
        if (((timer + i++ * 4) / 4) % 2 && !p.is_killed) {
          auto s = render::shape::line(to_float(transform.centre), to_float(p_transform.centre),
                                       colour::hue360(300, .5f, .6f), 0.f, 1.f,
                                       render::tag_t{'p' + p.player_number});
          output.emplace_back(s);
        }
      });
    }
  }
};
DEBUG_STRUCT_TUPLE(Tractor, timer, dir, power, ready, spinning, spoke_r);

}  // namespace

void spawn_follow_hub(SimInterface& sim, const vec2& position, bool power_a, bool power_b) {
  auto h = create_ship2<FollowHub>(sim, position);
  add_enemy_health2<FollowHub>(h, 14);
  h.add(FollowHub{power_a, power_b});
  h.add(Enemy{.threat_value = 6u + 2 * power_a + 2 * power_b,
              .score_reward = 50u + power_a * 10 + power_b * 10});
}

void spawn_shielder(SimInterface& sim, const vec2& position, bool power) {
  auto h = create_ship2<Shielder>(sim, position);
  add_enemy_health2<Shielder>(h, 16);
  h.add(Shielder{power});
  h.add(Enemy{.threat_value = 8u + 2 * power, .score_reward = 60u + power * 40});
}

void spawn_tractor(SimInterface& sim, const vec2& position, bool power) {
  auto h = create_ship2<Tractor>(sim, position);
  add_enemy_health2<Tractor>(h, 50);
  h.add(Tractor{power});
  h.add(Enemy{.threat_value = 10u + 2 * power, .score_reward = 85u + 40 * power});
}

}  // namespace ii::legacy
