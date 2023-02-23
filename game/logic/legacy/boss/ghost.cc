#include "game/common/colour.h"
#include "game/logic/legacy/boss/boss_internal.h"
#include "game/logic/legacy/enemy/enemy.h"
#include "game/logic/legacy/player/powerup.h"
#include "game/logic/legacy/ship_template.h"
#include "game/logic/sim/io/events.h"
#include <array>

namespace ii::legacy {
namespace {
constexpr cvec4 c0 = colour::hue360(280, .7f);
constexpr cvec4 c1 = colour::hue360(280, .5f, .6f);
constexpr cvec4 c2 = colour::hue360(270, .2f);
using namespace geom;

struct GhostWall : ecs::component {
  static constexpr sound kDestroySound = sound::kEnemyDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kNone;
  static constexpr fixed kSpeed = 3 + 1_fx / 2;
  static constexpr fixed kOffsetH = 260;
  static constexpr float kZIndex = 0.f;
  static constexpr fixed kBoundingWidth = 640;
  static constexpr auto kFlags = shape_flag::kDangerous | shape_flag::kShield;

  static void construct_shape(node& root) {
    auto& n = root.add(translate{key{'v'}});
    auto& v = n.add(enable{key{'V'}});
    auto& h = n.add(enable{compare(root, false, key{'V'})});

    auto& vb = root.create(compound{});
    vb.add(box_collider{.dimensions = vec2{240, 10}, .flags = kFlags});
    vb.add(box{.dimensions = vec2{240, 10}, .line = {.colour0 = c0}});
    vb.add(box{.dimensions = vec2{240, 7}, .line = {.colour0 = c0}});
    vb.add(box{.dimensions = vec2{240, 4}, .line = {.colour0 = c0}});
    v.add(enable{compare(root, false, key{'G'})}).add(vb);
    auto& vg = v.add(enable{key{'G'}});
    vg.add(box_collider{.dimensions = vec2{640, 10}, .flags = kFlags});
    vg.add(box{.dimensions = vec2{640, 10}, .line = {.colour0 = c0}});
    vg.add(box{.dimensions = vec2{640, 7}, .line = {.colour0 = c0}});
    vg.add(box{.dimensions = vec2{640, 4}, .line = {.colour0 = c0}});

    auto& hb = root.create(rotate{pi<fixed> / 2});
    hb.add(vb);
    auto add_horizontal = [&hb](fixed d, node& t) {
      t.add(translate{vec2{0, d - kOffsetH}}).add(hb);
      t.add(translate{vec2{0, d + kOffsetH}}).add(hb);
    };
    add_horizontal(100, h.add(enable{compare(root, false, key{'G'})}));
    add_horizontal(-100, h.add(enable{key{'G'}}));
  }

  void set_parameters(const Transform& transform, parameter_set& parameters) const {
    parameters.add(key{'v'}, transform.centre).add(key{'V'}, vertical).add(key{'G'}, gap_swap);
  }

  GhostWall(const vec2& dir, bool vertical, bool gap_swap)
  : dir{dir}, vertical{vertical}, gap_swap{gap_swap} {}
  vec2 dir{0};
  bool vertical = false;
  bool gap_swap = false;

  void update(ecs::handle h, Transform& transform, SimInterface& sim) {
    auto d = sim.dimensions();
    if ((dir.x > 0 && transform.centre.x < 32) || (dir.y > 0 && transform.centre.y < 16) ||
        (dir.x < 0 && transform.centre.x >= d.x - 32) ||
        (dir.y < 0 && transform.centre.y >= d.y - 16)) {
      transform.move(dir * kSpeed / 2);
    } else {
      transform.move(dir * kSpeed);
    }

    if ((dir.x > 0 && transform.centre.x > d.x + 10) ||
        (dir.y > 0 && transform.centre.y > d.y + 10) || (dir.x < 0 && transform.centre.x < -10) ||
        (dir.y < 0 && transform.centre.y < -10)) {
      h.emplace<Destroy>();
    }
  }
};
DEBUG_STRUCT_TUPLE(GhostWall, dir, vertical, gap_swap);

void spawn_ghost_wall_vertical(SimInterface& sim, bool swap, bool no_gap) {
  vec2 position{sim.dimensions().x / 2, swap ? -10 : 10 + sim.dimensions().y};
  vec2 dir{0, swap ? 1 : -1};
  auto h = create_ship<GhostWall>(sim, position);
  add_enemy_health<GhostWall>(h, 0);
  h.add(GhostWall{dir, true, no_gap});
  h.add(Enemy{.threat_value = 1});
}

void spawn_ghost_wall_horizontal(SimInterface& sim, bool swap, bool swap_gap) {
  vec2 position{swap ? -10 : 10 + sim.dimensions().x, sim.dimensions().y / 2};
  vec2 dir{swap ? 1 : -1, 0};
  auto h = create_ship<GhostWall>(sim, position);
  add_enemy_health<GhostWall>(h, 0);
  h.add(GhostWall{dir, false, swap_gap});
  h.add(Enemy{.threat_value = 1});
}

struct GhostMine : ecs::component {
  static constexpr sound kDestroySound = sound::kEnemyDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kNone;
  static constexpr float kZIndex = 0.f;
  static constexpr fixed kBoundingWidth = 24;
  static constexpr auto kFlags =
      shape_flag::kDangerous | shape_flag::kShield | shape_flag::kWeakShield;

  static void construct_shape(node& root) {
    auto& n = root.add(translate_rotate{key{'v'}, key{'r'}});
    n.add(ngon{.dimensions = nd(24, 8), .line = {.colour0 = key{'c'}}});
    n.add(enable{key{'C'}}).add(ball_collider{.dimensions = bd(24), .flags = kFlags});
    n.add(ngon{.dimensions = nd(20, 8), .line = {.colour0 = key{'c'}}});
  }

  void set_parameters(const Transform& transform, parameter_set& parameters) const {
    parameters.add(key{'v'}, transform.centre)
        .add(key{'r'}, transform.rotation)
        .add(key{'c'}, (timer / 4) % 2 ? colour::kZero : c1)
        .add(key{'C'}, !timer);
  }

  GhostMine(ecs::entity_id ghost_boss) : ghost_boss{ghost_boss} {}
  std::uint32_t timer = 80;
  ecs::entity_id ghost_boss{0};

  void update(ecs::handle h, Transform& transform, SimInterface& sim) {
    if (timer == 80) {
      auto e = sim.emit(resolve_key::reconcile(h.id(), resolve_tag::kRespawn));
      auto& r = resolve_entity_shape<default_shape_definition<GhostMine>>(h, sim);
      explode_shapes(e, r);
      transform.set_rotation(sim.random_fixed() * 2 * pi<fixed>);
    }
    timer && --timer;
    if (sim.collide_any(check_point(shape_flag::kEnemyInteraction, transform.centre))) {
      if (sim.random(6) == 0 ||
          (sim.index().contains(ghost_boss) &&
           sim.index().get(ghost_boss)->get<Health>()->is_hp_low() && sim.random(5) == 0)) {
        spawn_big_follow(sim, transform.centre, /* score */ false);
      } else {
        spawn_follow(sim, transform.centre, /* score */ false);
      }
      ecs::call<&Health::damage>(h, sim, 1, damage_type::kNormal, h.id(), std::nullopt);
    }
  }
};
DEBUG_STRUCT_TUPLE(GhostMine, timer, ghost_boss);

void spawn_ghost_mine(SimInterface& sim, const vec2& position, ecs::const_handle ghost) {
  auto h = create_ship<GhostMine>(sim, position);
  add_enemy_health<GhostMine>(h, 0);
  h.add(GhostMine{ghost.id()});
  h.add(Enemy{.threat_value = 1});
}

struct GhostBoss : ecs::component {
  static constexpr std::uint32_t kBaseHp = 700;
  static constexpr std::uint32_t kTimer = 250;
  static constexpr std::uint32_t kAttackTime = 100;

  static constexpr fixed kBoundingWidth = 640;
  static constexpr auto kFlags = shape_flag::kDangerous | shape_flag::kVulnerable |
      shape_flag::kEnemyInteraction | shape_flag::kShield;

  static vec2 outer_shape_d(std::uint32_t i, std::uint32_t j) {
    return from_polar_legacy(j * 2 * pi<fixed> / (16 + i * 6), 100_fx + i * 60);
  }

  static void construct_shape(node& root) {
    auto& n = root.add(translate_rotate{key{'v'}, key{'r'}});

    // Outer stars.
    key k{128};
    for (unsigned char i = 0; i < 5; ++i) {
      auto& r = n.add(rotate{key{'0'} + key{i}});
      for (std::uint32_t j = 0; j < 16 + i * 6; ++j) {
        auto& t = r.add(translate_rotate{outer_shape_d(i, j), key{'o'}});
        line_style line{.colour0 = c1};
        if (i) {
          line.colour0 = compare(root, true, k, c0, c2);
          ++k;
        }
        t.add(ngon{.dimensions = nd(16, 8),
                   .style = ngon_style::kPolystar,
                   .line = line,
                   .flags = render::flag::kLegacy_NoExplode | render::flag::kNoFlash});
        t.add(ngon{.dimensions = nd(12, 8),
                   .line = line,
                   .flags = render::flag::kLegacy_NoExplode | render::flag::kNoFlash});
      }
    }

    // Main centre.
    n.add(ngon{.dimensions = nd(32, 8), .line = {.colour0 = c1}});
    n.add(ngon{.dimensions = nd(48, 8), .line = {.colour0 = c0}});
    auto& c = n.add(enable{key{'C'}});
    c.add(ball_collider{.dimensions = bd(32), .flags = shape_flag::kShield});
    c.add(ball_collider{
        .dimensions = bd(48),
        .flags = shape_flag::kDangerous | shape_flag::kEnemyInteraction | shape_flag::kVulnerable});

    n.add(rotate{multiply(root, 3_fx, key{'R'})})
        .add(
            ngon{.dimensions = nd(24, 8), .style = ngon_style::kPolygram, .line = {.colour0 = c0}});
    for (std::uint32_t i = 0; i < 10; ++i) {
      n.add(/* dummy */ ball{.line = {.colour0 = c0}});
    }

    // Sparks.
    auto& spark = root.create(compound{});
    for (std::uint32_t i = 0; i < 8; ++i) {
      n.add(translate_rotate{from_polar_legacy(i * pi<fixed> / 4, 48_fx),
                             multiply(root, 3_fx, key{'R'})})
          .add(spark);
      spark.add(line{.a = from_polar_legacy(i * pi<fixed> / 4, 10_fx),
                     .b = from_polar_legacy(i * pi<fixed> / 4, 20_fx),
                     .style = {.colour0 = c1},
                     .flags = i ? render::flag::kLegacy_NoExplode : render::flag::kNone});
    }

    // Box attack.
    auto& box_b = root.create(compound{});
    auto& b = n.add(enable{key{'B'}});
    b.add(translate{vec2{320 + 32, 0}}).add(box_b);
    b.add(translate{vec2{-320 - 32, 0}}).add(box_b);
    box_b.add(enable{key{'C'}})
        .add(box_collider{.dimensions = vec2{320, 10},
                          .flags = shape_flag::kDangerous | shape_flag::kEnemyInteraction});
    box_b.add(
        box{.dimensions = vec2{320, 10}, .line = {.colour0 = c0}, .flags = render::flag::kNoFlash});
    box_b.add(
        box{.dimensions = vec2{320, 7}, .line = {.colour0 = c0}, .flags = render::flag::kNoFlash});
    box_b.add(
        box{.dimensions = vec2{320, 4}, .line = {.colour0 = c0}, .flags = render::flag::kNoFlash});
  }

  void set_parameters(const Transform& transform, parameter_set& parameters) const {
    parameters.add(key{'v'}, transform.centre)
        .add(key{'r'}, transform.rotation)
        .add(key{'R'}, inner_ring_rotation)
        .add(key{'o'}, outer_ball_rotation)
        .add(key{'C'}, collision_enabled)
        .add(key{'B'}, box_attack_shape_enabled);
    for (unsigned char i = 0; i < 5; ++i) {
      parameters.add(key{'0'} + key{i}, outer_rotation[i]);
    }
    auto k = key{128};
    for (std::uint32_t i = 1; i < 5; ++i) {
      for (std::uint32_t j = 0; j < 16 + i * 6; ++j) {
        parameters.add(k, outer_dangerous[i][j]);
        ++k;
      }
    }
  }

  std::tuple<vec2, fixed, fixed> shape_parameters(const Transform& transform) const {
    return {transform.centre, transform.rotation, inner_ring_rotation};
  }

  bool visible = false;
  bool shot_type = false;
  bool rdir = false;
  bool danger_enable = false;
  bool is_legacy = false;
  std::uint32_t vtime = 0;
  std::uint32_t timer = 0;
  std::uint32_t attack = 0;
  std::uint32_t attack_time = kAttackTime;
  std::uint32_t start_time = 120;
  std::uint32_t danger_circle = 0;
  std::uint32_t danger_offset1 = 0;
  std::uint32_t danger_offset2 = 0;
  std::uint32_t danger_offset3 = 0;
  std::uint32_t danger_offset4 = 0;
  // Shape modifiers.
  std::array<std::array<bool, 16 + 4 * 6>, 5> outer_dangerous;
  fixed inner_ring_rotation = 0;
  fixed outer_ball_rotation = 0;
  std::array<fixed, 5> outer_rotation{0_fx, 0_fx, 0_fx, 0_fx, 0_fx};
  bool box_attack_shape_enabled = false;
  bool collision_enabled = false;

  void
  update(ecs::handle h, Transform& transform, Boss& boss, const Health& health, SimInterface& sim) {
    auto e = sim.emit(resolve_key::predicted());
    outer_dangerous[0].fill(false);
    for (std::uint32_t n = 1; n < 5; ++n) {
      auto t_n = geom::transform{}
                     .translate(transform.centre)
                     .rotate(transform.rotation + outer_rotation[n]);
      for (std::uint32_t i = 0; i < 16 + n * 6; ++i) {
        auto t = n == 1 ? (i + danger_offset1) % 11
            : n == 2    ? (i + danger_offset2) % 7
            : n == 3    ? (i + danger_offset3) % 17
                        : (i + danger_offset4) % 10;

        bool b = n == 1 ? (t % 11 == 0 || t % 11 == 5)
            : n == 2    ? (t % 7 == 0)
            : n == 3    ? (t % 17 == 0 || t % 17 == 8)
                        : (t % 10 == 0);

        outer_dangerous[n][i] =
            ((n == 4 && attack != 2) || (n + (n == 3)) & danger_circle) && visible && b;
        if (outer_dangerous[n][i]) {
          auto t_i = t_n.translate(outer_shape_d(n, i)).rotate(outer_ball_rotation);
          sim.global_entity().get<GlobalData>()->extra_enemy_warnings.emplace_back(t_i.v);
        }
      }
    }
    danger_enable = !vtime;
    if (attack != 2 || attack_time >= kAttackTime * 4 || !attack_time) {
      transform.rotate(-fixed_c::hundredth * 4);
    }

    inner_ring_rotation = normalise_angle(inner_ring_rotation + 2 * fixed_c::hundredth);
    outer_ball_rotation = normalise_angle(outer_ball_rotation - fixed_c::tenth);
    for (std::uint32_t n = 0; n < 5; ++n) {
      std::uint32_t m = 0;
      // Compatibility with legacy bug that incorrectly positioned collision shapes.
      // Now looks correct (but less cool than supposed to) in legacy mode.
      // Fixed to work as intended otherwise.
      if (sim.is_legacy()) {
        m = !n ? 5 : n % 2 ? 3 : 4;
      } else {
        m = n % 2 ? 3 : 5;
      }
      outer_rotation[n] = normalise_angle(outer_rotation[n] + m * fixed_c::hundredth +
                                          ((n % 2 ? 3_fx : -3_fx) / 2000) * n);
    }

    if (start_time) {
      boss.show_hp_bar = false;
      --start_time;
      return;
    }

    if (!attack_time) {
      ++timer;
      if (timer == 9 * kTimer / 10 ||
          (timer >= kTimer / 10 && timer < 9 * kTimer / 10 - 16 &&
           ((!(timer % 16) && attack == 2) || (!(timer % 32) && shot_type)))) {
        for (std::uint32_t i = 0; i < 8; ++i) {
          auto d = from_polar_legacy(i * pi<fixed> / 4 + transform.rotation, 5_fx);
          spawn_boss_shot(sim, transform.centre, d, c0);
        }
        e.play_random(sound::kBossFire, transform.centre);
      }
      if (timer != 9 * kTimer / 10 && timer >= kTimer / 10 && timer < 9 * kTimer / 10 - 16 &&
          !(timer % 16) && (!shot_type || attack == 2)) {
        auto d = sim.nearest_player_direction(transform.centre);
        if (d != vec2{}) {
          spawn_boss_shot(sim, transform.centre, d * 5, c0);
          spawn_boss_shot(sim, transform.centre, d * -5, c0);
          e.play_random(sound::kBossFire, transform.centre);
        }
      }
    } else {
      --attack_time;

      if (attack == 2) {
        if (attack_time >= kAttackTime * 4 && !(attack_time % 8)) {
          auto y = sim.random(sim.dimensions().y.to_int() + 1);
          auto x = sim.random(sim.dimensions().x.to_int() + 1);
          vec2 pos{x, y};
          spawn_ghost_mine(sim, pos, h);
          e.play_random(sound::kEnemySpawn, pos);
          danger_circle = 0;
        } else if (attack_time == kAttackTime * 4 - 1) {
          visible = true;
          vtime = 60;
          box_attack_shape_enabled = true;
          e.play(sound::kBossFire, transform.centre);
        } else if (attack_time < kAttackTime * 3) {
          if (attack_time == kAttackTime * 3 - 1) {
            e.play(sound::kBossAttack, transform.centre);
          }
          transform.rotate((rdir ? 1 : -1) * 2 * pi<fixed> / (kAttackTime * 6));
          if (!attack_time) {
            box_attack_shape_enabled = false;
          }
          danger_offset1 = sim.random(11);
          danger_offset2 = sim.random(7);
          danger_offset3 = sim.random(17);
          danger_offset3 = sim.random(10);  // Bug compatibility: probably should be danger_offset4.
        }
      }

      if (!attack && attack_time == kAttackTime * 2) {
        bool r = sim.random_bool();
        spawn_ghost_wall_horizontal(sim, r, true);
        spawn_ghost_wall_horizontal(sim, !r, false);
      }
      if (attack == 1 && attack_time == kAttackTime * 2) {
        rdir = sim.random_bool();
        spawn_ghost_wall_vertical(sim, !rdir, false);
      }
      if (attack == 1 && attack_time == kAttackTime * 1) {
        spawn_ghost_wall_vertical(sim, rdir, true);
      }

      if (attack <= 1 && health.is_hp_low() && attack_time == kAttackTime) {
        auto r = sim.random(4);
        auto d = sim.dimensions();
        vec2 v = r == 0 ? vec2{-d.x / 4, d.y / 2}
            : r == 1    ? vec2{d.x + d.x / 4, d.y / 2}
            : r == 2    ? vec2{d.x / 2, -d.y / 4}
                        : vec2{d.x / 2, d.y + d.y / 4};
        spawn_big_follow(sim, v, false);
      }
      if (!attack_time && !visible) {
        visible = true;
        vtime = 60;
      }
      if (attack_time == 0 && attack != 2) {
        auto r = sim.random(3);
        danger_circle |= 1 + (r == 2 ? 3 : r);
        if (sim.random_bool() || health.is_hp_low()) {
          r = sim.random(3);
          danger_circle |= 1 + (r == 2 ? 3 : r);
        }
        if (sim.random_bool() || health.is_hp_low()) {
          r = sim.random(3);
          danger_circle |= 1 + (r == 2 ? 3 : r);
        }
      } else {
        danger_circle = 0;
      }
    }

    if (timer >= kTimer) {
      timer = 0;
      visible = false;
      vtime = 60;
      attack = sim.random(3);
      shot_type = !sim.random_bool();
      collision_enabled = false;
      e.play(sound::kBossAttack, transform.centre);

      if (!attack) {
        attack_time = kAttackTime * 2 + 50;
      }
      if (attack == 1) {
        attack_time = kAttackTime * 3;
        for (std::uint32_t i = 0; i < sim.player_count(); ++i) {
          spawn_powerup(sim, transform.centre, powerup_type::kShield);
        }
      }
      if (attack == 2) {
        attack_time = kAttackTime * 6;
        rdir = sim.random_bool();
      }
    }

    if (vtime) {
      --vtime;
      if (!vtime && visible) {
        collision_enabled = true;
      }
    }
  }

  static void construct_danger_shape(node& root) {
    root.add(translate_rotate{key{'v'}, key{'r'}})
        .add(rotate{key{'R'}})
        .add(translate{key{'V'}})
        .add(ball_collider{.dimensions = bd(9), .flags = shape_flag::kDangerous});
  }

  static void construct_inner_shape(node& root) {
    root.add(translate_rotate{key{'v'}, key{'r'}})
        .add(rotate{key{'R'}})
        .add(translate{key{'V'}})
        .add(ball_collider{.dimensions = bd(16),
                           .flags = shape_flag::kDangerous | shape_flag::kEnemyInteraction});
  }

  geom::hit_result check_collision(ecs::const_handle h, const Transform& transform,
                                   const geom::check_t& check, const SimInterface& sim) const {
    auto c = check;
    c.legacy_algorithm = sim.is_legacy();
    auto result = sim.is_legacy()
        ? ship_check_collision_legacy<default_shape_definition<GhostBoss>>(h, c, sim)
        : ship_check_collision<default_shape_definition<GhostBoss>>(h, c, sim);
    if (collision_enabled) {
      auto& bank = sim.shape_bank();
      auto& parameters = bank.parameters([&](parameter_set& parameters) {
        parameters.add(key{'v'}, transform.centre).add(key{'r'}, transform.rotation);
      });
      for (std::uint32_t i = 0; i < (danger_enable ? 5 : 1); ++i) {
        const auto& node = bank[i ? &construct_danger_shape : construct_inner_shape];
        parameters.add(key{'R'}, outer_rotation[i]);
        for (std::uint32_t j = 0; j < 16 + 6 * i; ++j) {
          if (!i || outer_dangerous[i][j]) {
            parameters.add(key{'V'}, outer_shape_d(i, j)).add(key{'o'}, outer_ball_rotation);
            geom::check_collision(result, c, node, parameters);
          }
        }
      }
    }
    return result;
  }

  void render_override(ecs::const_handle h, const Health& health,
                       std::vector<render::shape>& output, const SimInterface& sim) const {
    std::optional<cvec4> colour_override;
    if ((start_time / 4) % 2 == 1) {
      colour_override = {0.f, 0.f, 0.f, 1.f / 255};
    } else if ((visible && ((vtime / 4) % 2 == 0)) || (!visible && ((vtime / 4) % 2 == 1))) {
      colour_override.reset();
    } else {
      colour_override = c2;
    }
    render_entity_shape_override<default_shape_definition<GhostBoss>>(output, h, &health, sim,
                                                                      -16.f, colour_override);
  }
};
DEBUG_STRUCT_TUPLE(GhostBoss, visible, shot_type, rdir, danger_enable, vtime, timer, attack,
                   attack_time, start_time, danger_circle, danger_offset1, danger_offset2,
                   danger_offset3, danger_offset4);

}  // namespace

void spawn_ghost_boss(SimInterface& sim, std::uint32_t cycle) {
  auto h = sim.index().create();
  h.add(Update{.update = ecs::call<&GhostBoss::update>});
  h.add(Transform{.centre = sim.dimensions() / 2});
  h.add(Collision{.flags = GhostBoss::kFlags,
                  .bounding_width = GhostBoss::kBoundingWidth,
                  .check_collision = ecs::call<&GhostBoss::check_collision>});
  h.add(Render{.render = sfn::cast<Render::render_t, ecs::call<&GhostBoss::render_override>>});

  h.add(Enemy{.threat_value = 100,
              .boss_score_reward =
                  calculate_boss_score(boss_flag::kBoss2B, sim.player_count(), cycle)});
  h.add(Health{
      .hp = calculate_boss_hp(GhostBoss::kBaseHp, sim.player_count(), cycle),
      .hit_sound0 = std::nullopt,
      .hit_sound1 = sound::kEnemyShatter,
      .destroy_sound = std::nullopt,
      .damage_transform = &scale_boss_damage,
      .on_hit = &boss_on_hit<true, GhostBoss>,
      .on_destroy = ecs::call<&boss_on_destroy<GhostBoss>>,
  });
  h.add(Boss{.boss = boss_flag::kBoss2B});
  h.add(GhostBoss{.is_legacy = sim.is_legacy()});
}

}  // namespace ii::legacy
