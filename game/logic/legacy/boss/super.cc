#include "game/common/colour.h"
#include "game/logic/legacy/boss/boss_internal.h"
#include "game/logic/legacy/enemy/enemy.h"
#include "game/logic/legacy/player/powerup.h"
#include "game/logic/legacy/ship_template.h"
#include "game/logic/sim/io/events.h"
#include <array>

namespace ii::legacy {
namespace {
using namespace geom2;

struct SuperBossArc : public ecs::component {
  static constexpr std::uint32_t kBaseHp = 75;
  static constexpr float kZIndex = -4.f;
  static constexpr auto kFlags = shape_flag::kDangerous | shape_flag::kVulnerable |
      shape_flag::kShield | shape_flag::kSafeShield;

  static void construct_shape(node& root) {
    auto& n = root.add(translate_rotate{key{'v'}, key{'r'}}).add(rotate{key{'R'}});
    for (unsigned char j = 0; j < 8; ++j) {
      auto r = 140 - 5_fx * j;
      n.add(ngon{.dimensions = nd(r, 32, 2), .line = {.colour0 = key{'0'} + key{j}}});
      auto flags = j == 3 ? kFlags
          : j == 7        ? shape_flag::kShield | shape_flag::kSafeShield
                          : shape_flag::kNone;
      if (+flags) {
        n.add(
            arc_collider{.dimensions = bd(r, r - 10), .arc_angle = pi<fixed> / 8, .flags = flags});
      }
    }
  }

  void set_parameters(const Transform& transform, parameter_set& parameters) const {
    parameters.add(key{'v'}, transform.centre)
        .add(key{'r'}, transform.rotation)
        .add(key{'R'}, i * 2 * pi<fixed> / 16);
    for (unsigned char j = 0; j < 8; ++j) {
      parameters.add(key{'0'} + key{j}, colours[j]);
    }
  }

  static constexpr fixed bounding_width(bool is_legacy) { return is_legacy ? 640 : 130; }

  SuperBossArc(ecs::entity_id boss, std::uint32_t i, std::uint32_t timer)
  : boss{boss}, i{i}, timer{timer} {}
  ecs::entity_id boss{0};
  std::uint32_t i = 0;
  std::uint32_t timer = 0;
  std::uint32_t s_timer = 0;
  std::array<cvec4, 8> colours{colour::kZero};

  void update(Transform& transform, const Health& health, SimInterface&) {
    transform.rotate(6_fx / 1000);
    for (std::uint32_t i = 0; i < 8; ++i) {
      colours[7 - i] = {((i * 32 + 2 * timer) % 256) / 256.f, 1.f, .5f,
                        health.is_hp_low() ? .6f : 1.f};
    }
    ++timer;
  }

  void on_destroy(ecs::const_handle h, const Transform& transform, SimInterface& sim, EmitHandle& e,
                  damage_type, const vec2& source) const {
    {
      auto& r = resolve_entity_shape2<shape_definition_with_width<SuperBossArc, 0>>(h, sim);
      destruct_lines(e, r, to_float(source), 64);
    }
    auto v =
        transform.centre + from_polar_legacy(i * 2 * pi<fixed> / 16 + transform.rotation, 120_fx);
    auto& r = resolve_shape2<&construct_shape>(sim, [&](parameter_set& parameters) {
      set_parameters(transform, parameters);
      parameters.add(key{'v'}, v);
    });
    explode_shapes(e, r);
    explode_shapes(e, r, cvec4{1.f}, 12);
    explode_shapes(e, r, std::nullopt, 24);
    explode_shapes(e, r, cvec4{1.f}, 36);
    explode_shapes(e, r, std::nullopt, 48);
    e.play_random(sound::kExplosion, v);
  }
};
DEBUG_STRUCT_TUPLE(SuperBossArc, boss, i, timer, s_timer);

ecs::handle spawn_super_boss_arc(SimInterface& sim, const vec2& position, std::uint32_t cycle,
                                 std::uint32_t i, ecs::handle boss, std::uint32_t timer = 0) {
  using legacy_shape =
      shape_definition_with_width<SuperBossArc, SuperBossArc::bounding_width(true)>;
  using shape = shape_definition_with_width<SuperBossArc, SuperBossArc::bounding_width(false)>;

  auto h = sim.is_legacy() ? create_ship2<SuperBossArc, legacy_shape>(sim, position)
                           : create_ship2<SuperBossArc, shape>(sim, position);
  h.add(Enemy{.threat_value = 10});
  h.add(Health{
      .hp = calculate_boss_hp(SuperBossArc::kBaseHp, sim.player_count(), cycle),
      .hit_sound0 = std::nullopt,
      .hit_sound1 = sound::kEnemyShatter,
      .destroy_sound = std::nullopt,
      .damage_transform =
          +[](ecs::handle h, SimInterface& sim, damage_type type, std::uint32_t damage) {
            return scale_boss_damage(h, sim, type,
                                     type == damage_type::kBomb ? damage / 2 : damage);
          },
      .on_hit = &boss_on_hit2<true, SuperBossArc, shape>,
      .on_destroy = ecs::call<&SuperBossArc::on_destroy>,
  });
  h.add(SuperBossArc{boss.id(), i, timer});
  return h;
}

struct SuperBoss : ecs::component {
  enum class state { kArrive, kIdle, kAttack };
  static constexpr std::uint32_t kBaseHp = 520;
  static constexpr float kZIndex = -4.f;
  static constexpr auto kFlags =
      shape_flag::kDangerous | shape_flag::kVulnerable | shape_flag::kShield;

  static void construct_shape(node& root) {
    auto& n = root.add(translate_rotate{key{'v'}, key{'r'}});
    for (unsigned char j = 0; j < 8; ++j) {
      auto r = 40 - 5_fx * j;
      n.add(ngon{.dimensions = nd(r, 32), .line = {.colour0 = key{'0'} + key{j}}});
      auto flags = j == 0 ? shape_flag::kDangerous | shape_flag::kVulnerable
          : j == 2        ? shape_flag::kShield
                          : shape_flag::kNone;
      if (+flags) {
        n.add(ball_collider{.dimensions = bd(r), .flags = flags});
      }
    }
  }

  void set_parameters(const Transform& transform, parameter_set& parameters) const {
    parameters.add(key{'v'}, transform.centre).add(key{'r'}, transform.rotation);
    for (unsigned char j = 0; j < 8; ++j) {
      parameters.add(key{'0'} + key{j}, colours[j]);
    }
  }

  static constexpr fixed bounding_width(bool is_legacy) { return is_legacy ? 640 : 50; }

  SuperBoss(std::uint32_t cycle) : cycle{cycle} {}
  state state = state::kArrive;
  std::uint32_t cycle = 0;
  std::uint32_t ctimer = 0;
  std::uint32_t timer = 0;
  std::uint32_t snakes = 0;
  std::vector<ecs::entity_id> arcs;
  std::array<cvec4, 8> colours{colour::kZero};

  void update(ecs::handle h, Transform& transform, SimInterface& sim) {
    auto e = sim.emit(resolve_key::predicted());
    std::vector<bool> destroyed_arcs;
    if (arcs.empty()) {
      for (std::uint32_t i = 0; i < 16; ++i) {
        arcs.push_back(spawn_super_boss_arc(sim, transform.centre, cycle, i, h).id());
        destroyed_arcs.push_back(false);
      }
    } else {
      for (auto id : arcs) {
        auto arc_h = sim.index().get(id);
        destroyed_arcs.push_back(!arc_h || arc_h->has<Destroy>());
      }
      for (std::uint32_t i = 0; i < 16; ++i) {
        if (destroyed_arcs[i]) {
          continue;
        }
        sim.index().get(arcs[i])->get<Transform>()->centre = transform.centre;
      }
    }
    vec2 move_vec{0};
    transform.rotate(6_fx / 1000);
    auto c = colour::hue((ctimer % 128) / 128.f);
    for (std::uint32_t i = 0; i < 8; ++i) {
      colours[7 - i] = colour::hue(((i * 32 + 2 * ctimer) % 256) / 256.f);
    }
    ++ctimer;
    if (transform.centre.y < sim.dimensions().y / 2) {
      move_vec = {0, 1};
    } else if (state == state::kArrive) {
      state = state::kIdle;
    }

    ++timer;
    if (state == state::kAttack && timer == 192) {
      timer = 100;
      state = state::kIdle;
    }

    static constexpr fixed d5d1000 = 5_fx / 1000;
    static constexpr fixed pi2d64 = 2 * pi<fixed> / 64;
    static constexpr fixed pi2d32 = 2 * pi<fixed> / 32;

    if (state == state::kIdle && sim.is_on_screen(transform.centre) && timer && timer % 300 == 0) {
      auto r = sim.random(6);
      if (r == 0 || r == 1) {
        snakes = 16;
      } else if (r == 2 || r == 3) {
        state = state::kAttack;
        timer = 0;
        fixed f = sim.random_fixed() * (2 * pi<fixed>);
        fixed rf = d5d1000 * (1 + sim.random_bool());
        for (std::uint32_t i = 0; i < 32; ++i) {
          vec2 d = from_polar_legacy(f + i * pi2d32, 1_fx);
          if (r == 2) {
            rf = d5d1000 * (1 + sim.random(4));
          }
          spawn_snake(sim, transform.centre + d * 16, c, d, rf);
          e.play_random(sound::kBossAttack, transform.centre);
        }
      } else if (r == 5) {
        state = state::kAttack;
        timer = 0;
        fixed f = sim.random_fixed() * (2 * pi<fixed>);
        for (std::uint32_t i = 0; i < 64; ++i) {
          vec2 d = from_polar_legacy(f + i * pi2d64, 1_fx);
          spawn_snake(sim, transform.centre + d * 16, c, d);
          e.play_random(sound::kBossAttack, transform.centre);
        }
      } else {
        state = state::kAttack;
        timer = 0;
        snakes = 32;
      }
    }

    if (state == state::kIdle && sim.is_on_screen(transform.centre) && timer % 300 == 200 &&
        !h.has<Destroy>()) {
      std::vector<std::uint32_t> wide3;
      std::uint32_t timer = 0;
      for (std::uint32_t i = 0; i < 16; ++i) {
        if (destroyed_arcs[(i + 15) % 16] && destroyed_arcs[i] && destroyed_arcs[(i + 1) % 16]) {
          wide3.push_back(i);
        }
        if (!destroyed_arcs[i]) {
          timer = sim.index().get(arcs[i])->get<SuperBossArc>()->timer;
        }
      }
      if (!wide3.empty()) {
        auto r = sim.random(wide3.size());
        auto arc_h = spawn_super_boss_arc(sim, transform.centre, cycle, wide3[r], h, timer);
        arc_h.get<Transform>()->set_rotation(transform.rotation - (6_fx / 1000));
        arcs[wide3[r]] = arc_h.id();
        destroyed_arcs[wide3[r]] = false;
        e.play(sound::kEnemySpawn, transform.centre);
      }
    }
    static const fixed pi2d128 = 2 * pi<fixed> / 128;
    if (state == state::kIdle && timer % 72 == 0) {
      for (std::uint32_t i = 0; i < 128; ++i) {
        vec2 d = from_polar_legacy(i * pi2d128, 1_fx);
        spawn_boss_shot(sim, transform.centre + d * 42, move_vec + d * 3, cvec4{0.f, 0.f, .6f, 1.f},
                        /* rotate speed */ 6_fx / 1000);
        e.play_random(sound::kBossFire, transform.centre);
      }
    }

    if (snakes) {
      --snakes;
      auto r0 = sim.random(32);
      auto r1 = sim.random(16);
      fixed length{r0 + r1};
      vec2 d = from_polar_legacy(sim.random_fixed() * (2 * pi<fixed>), length);
      spawn_snake(sim, d + transform.centre, c);
      e.play_random(sound::kEnemySpawn, transform.centre);
    }
    transform.move(move_vec);
  }

  void on_destroy(ecs::const_handle h, const Transform& transform, SimInterface& sim, EmitHandle& e,
                  damage_type, const vec2& source) const {
    sim.index().iterate_dispatch_if<Enemy>([&](ecs::handle eh, Health& health) {
      if (eh.id() != h.id()) {
        health.damage(eh, sim, 100 * GlobalData::kBombDamage, damage_type::kBomb, h.id());
      }
    });

    auto& r = resolve_entity_shape2<shape_definition_with_width<SuperBoss, 0>>(h, sim);
    explode_shapes(e, r);
    explode_shapes(e, r, cvec4{1.f}, 12);
    explode_shapes(e, r, std::nullopt, 24);
    explode_shapes(e, r, cvec4{1.f}, 36);
    explode_shapes(e, r, std::nullopt, 48);
    destruct_lines(e, r, to_float(source), 128);

    std::uint32_t n = 1;
    for (std::uint32_t i = 0; i < 16; ++i) {
      vec2 v = from_polar_legacy(sim.random_fixed() * (2 * pi<fixed>),
                                 fixed{8 + sim.random(64) + sim.random(64)});
      sim.global_entity().get<GlobalData>()->fireworks.push_back(GlobalData::fireworks_entry{
          .time = n, .position = transform.centre + v, .colour = colours[i % 8]});
      n += i;
    }
    e.rumble_all(30, 1.f, 1.f).play(sound::kExplosion, transform.centre);

    for (std::uint32_t i = 0; i < 8; ++i) {
      spawn_powerup(sim, transform.centre, powerup_type::kBomb);
    }
  }
};
DEBUG_STRUCT_TUPLE(SuperBoss, state, cycle, ctimer, timer, snakes, arcs);

}  // namespace

void spawn_super_boss(SimInterface& sim, std::uint32_t cycle) {
  using legacy_shape = shape_definition_with_width<SuperBoss, SuperBoss::bounding_width(true)>;
  using shape = shape_definition_with_width<SuperBoss, SuperBoss::bounding_width(false)>;

  vec2 position{sim.dimensions().x / 2, -sim.dimensions().y / (2 + fixed_c::half)};
  auto h = sim.is_legacy() ? create_ship2<SuperBoss, legacy_shape>(sim, position)
                           : create_ship2<SuperBoss, shape>(sim, position);

  h.add(Enemy{.threat_value = 100,
              .boss_score_reward =
                  calculate_boss_score(boss_flag::kBoss3A, sim.player_count(), cycle)});
  h.add(Health{
      .hp = calculate_boss_hp(SuperBoss::kBaseHp, sim.player_count(), cycle),
      .hit_flash_ignore_index = 8,
      .hit_sound0 = std::nullopt,
      .hit_sound1 = sound::kEnemyShatter,
      .destroy_sound = std::nullopt,
      .damage_transform = &scale_boss_damage,
      .on_hit = &boss_on_hit2<true, SuperBoss, shape>,
      .on_destroy = ecs::call<&SuperBoss::on_destroy>,
  });
  h.add(Boss{.boss = boss_flag::kBoss3A});
  h.add(SuperBoss{cycle});
}

}  // namespace ii::legacy
