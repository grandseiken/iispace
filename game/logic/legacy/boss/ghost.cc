#include "game/common/colour.h"
#include "game/geometry/legacy/ball_collider.h"
#include "game/geometry/legacy/box.h"
#include "game/geometry/legacy/line.h"
#include "game/geometry/legacy/ngon.h"
#include "game/geometry/node_conditional.h"
#include "game/geometry/node_disable_iteration.h"
#include "game/geometry/node_for_each.h"
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

template <geom::ShapeNode S>
Collision::hit_result shape_check_point_compatibility(const auto& parameters, bool is_legacy,
                                                      const vec2& v, shape_flag mask) {
  return is_legacy ? shape_check_point_legacy<S>(parameters, v, mask)
                   : shape_check_point<S>(parameters, v, mask);
}

struct GhostWall : ecs::component {
  static constexpr fixed kBoundingWidth = 640;
  static constexpr sound kDestroySound = sound::kEnemyDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kNone;
  static constexpr fixed kSpeed = 3 + 1_fx / 2;
  static constexpr fixed kOffsetH = 260;
  static constexpr float kZIndex = 0.f;

  template <std::uint32_t length>
  using gw_box =
      geom::compound<geom::box<length, 10, c0, 0, shape_flag::kDangerous | shape_flag::kShield>,
                     geom::box<length, 7, c0>, geom::box<length, 4, c0>>;
  using gw_horizontal_base = geom::rotate<pi<fixed> / 2, gw_box<240>>;
  template <fixed Y0, fixed Y1>
  using gw_horizontal_align = geom::compound<geom::translate<0, Y0, gw_horizontal_base>,
                                             geom::translate<0, Y1, gw_horizontal_base>>;
  using shape = geom::translate_p<
      0,
      geom::conditional_p<
          1, geom::conditional_p<2, gw_box<640>, gw_box<240>>,
          geom::conditional_p<2, gw_horizontal_align<-100 - kOffsetH, -100 + kOffsetH>,
                              gw_horizontal_align<100 - kOffsetH, 100 + kOffsetH>>>>;

  std::tuple<vec2, bool, bool> shape_parameters(const Transform& transform) const {
    return {transform.centre, vertical, gap_swap};
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
  static constexpr fixed kBoundingWidth = 24;
  static constexpr sound kDestroySound = sound::kEnemyDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kNone;
  static constexpr float kZIndex = 0.f;

  using shape = standard_transform<
      geom::conditional_p<
          2, geom::ngon_colour_p<24, 8, 3>,
          geom::polygon_colour_p<
              24, 8, 3, 0, shape_flag::kDangerous | shape_flag::kShield | shape_flag::kWeakShield>>,
      geom::ngon_colour_p<20, 8, 3>>;

  std::tuple<vec2, fixed, bool, cvec4> shape_parameters(const Transform& transform) const {
    return {transform.centre, transform.rotation, timer > 0, (timer / 4) % 2 ? cvec4{0.f} : c1};
  }

  std::uint32_t timer = 80;
  ecs::entity_id ghost_boss{0};
  GhostMine(ecs::entity_id ghost_boss) : ghost_boss{ghost_boss} {}

  void update(ecs::handle h, Transform& transform, SimInterface& sim) {
    if (timer == 80) {
      auto e = sim.emit(resolve_key::reconcile(h.id(), resolve_tag::kRespawn));
      explode_entity_shapes<GhostMine>(h, e);
      transform.set_rotation(sim.random_fixed() * 2 * pi<fixed>);
    }
    timer && --timer;
    if (sim.collide_point_any(transform.centre, shape_flag::kEnemyInteraction)) {
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
  static constexpr std::uint32_t kBoundingWidth = 640;
  static constexpr std::uint32_t kBaseHp = 700;
  static constexpr std::uint32_t kTimer = 250;
  static constexpr std::uint32_t kAttackTime = 100;
  static constexpr shape_flag kShapeFlags = shape_flag::kEverything;

  using centre_shape =
      geom::compound<geom::polygon<32, 8, c1, 0, shape_flag::kShield>,
                     geom::polygon<48, 8, c0, 0,
                                   shape_flag::kDangerous | shape_flag::kEnemyInteraction |
                                       shape_flag::kVulnerable>>;

  using render_shape =
      standard_transform<centre_shape,
                         geom::rotate_eval<geom::multiply_p<3, 2>, geom::polygram<24, 8, c0>>>;

  using explode_shape =
      standard_transform<centre_shape,
                         geom::rotate_eval<geom::multiply_p<3, 2>, geom::polygram<24, 8, c0>>>;
  using shape = geom::ngon<0, 2, c1>;  // Just for fireworks.

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
    if (!(attack == 2 && attack_time < kAttackTime * 4 && attack_time)) {
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

  const vec2& outer_shape_d(std::uint32_t n, std::uint32_t i) const {
    static const std::vector<std::vector<vec2>> kLookup = [] {
      std::vector<std::vector<vec2>> v;
      for (std::uint32_t n = 0; n < 5; ++n) {
        v.emplace_back();
        for (std::uint32_t i = 0; i < 16 + n * 6; ++i) {
          v.back().push_back(from_polar_legacy(i * 2 * pi<fixed> / (16 + n * 6), 100_fx + n * 60));
        }
      }
      return v;
    }();
    return kLookup[n][i];
  }

  using box_attack_component = geom::compound<
      geom::box<320, 10, c0, 0, shape_flag::kDangerous | shape_flag::kEnemyInteraction>,
      geom::box<320, 7, c0>, geom::box<320, 4, c0>>;
  using box_attack_shape =
      standard_transform<geom::compound<geom::translate<320 + 32, 0, box_attack_component>,
                                        geom::translate<-320 - 32, 0, box_attack_component>>>;
  std::tuple<vec2, fixed> box_attack_parameters(const Transform& transform) const {
    return {transform.centre, transform.rotation};
  }

  Collision::hit_result
  check_point(const Transform& transform, const vec2& v, shape_flag mask) const {
    Collision::hit_result result;
    if (!collision_enabled) {
      return result;
    }
    result.mask |= shape_check_point_compatibility<standard_transform<centre_shape>>(
                       shape_parameters(transform), is_legacy, v, mask)
                       .mask;
    if (box_attack_shape_enabled) {
      result.mask |= shape_check_point_compatibility<box_attack_shape>(
                         box_attack_parameters(transform), is_legacy, v, mask)
                         .mask;
    }

    using ring0_ball =
        geom::ball_collider<16, shape_flag::kDangerous | shape_flag::kEnemyInteraction>;
    using ring0_ball_t =
        standard_transform<geom::rotate_p<2, geom::translate_p<3, geom::rotate_p<4, ring0_ball>>>>;
    if (+(mask & (shape_flag::kDangerous | shape_flag::kEnemyInteraction))) {
      for (std::uint32_t i = 0; i < 16; ++i) {
        std::tuple parameters{transform.centre, transform.rotation, outer_rotation[0],
                              outer_shape_d(0, i), outer_ball_rotation};
        result.mask |=
            shape_check_point_compatibility<ring0_ball_t>(parameters, is_legacy, v, mask).mask;
      }
    }

    using ringN_ball = geom::ball_collider<9, shape_flag::kDangerous>;
    using ringN_ball_t =
        standard_transform<geom::rotate_p<2, geom::translate_p<3, geom::rotate_p<4, ringN_ball>>>>;
    for (std::uint32_t n = 1; n < 5 && +(mask & shape_flag::kDangerous); ++n) {
      for (std::uint32_t i = 0; i < 16 + n * 6; ++i) {
        if (!outer_dangerous[n][i] || !danger_enable) {
          continue;
        }
        std::tuple parameters{transform.centre, transform.rotation, outer_rotation[n],
                              outer_shape_d(n, i), outer_ball_rotation};
        result.mask |=
            shape_check_point_compatibility<ringN_ball_t>(parameters, is_legacy, v, mask).mask;
      }
    }
    return result;
  }

  template <fixed I>
  using spark_line = geom::line_eval<geom::constant<10 * from_polar_legacy(I* pi<fixed> / 4, 1_fx)>,
                                     geom::constant<20 * from_polar_legacy(I* pi<fixed> / 4, 1_fx)>,
                                     geom::constant<c1>>;
  using spark_shape = standard_transform<geom::translate_p<
      2,
      geom::rotate_p<3, spark_line<0>,
                     geom::disable_iteration<geom::iterate_volumes_t,
                                             geom::for_each<fixed, 1, 8, spark_line>>>>>;
  std::tuple<vec2, fixed, vec2, fixed>
  spark_shape_parameters(const Transform& transform, std::uint32_t i) const {
    return {transform.centre, transform.rotation, from_polar_legacy(i * pi<fixed> / 4, 48_fx),
            2 * inner_ring_rotation};
  }

  void explode_shapes(const Transform& transform, EmitHandle& e,
                      const std::optional<cvec4> colour_override = std::nullopt,
                      std::uint32_t time = 8, const std::optional<vec2>& towards = std::nullopt,
                      std::optional<float> speed = std::nullopt) const {
    legacy::explode_shapes<explode_shape>(e, shape_parameters(transform), colour_override, time,
                                          towards, speed);
    for (std::uint32_t i = 0; i < 10; ++i) {
      e.explosion(to_float(transform.centre), cvec4{0.f});  // Compatibility.
    }
    for (std::uint32_t i = 0; i < 8; ++i) {
      legacy::explode_shapes<spark_shape>(e, spark_shape_parameters(transform, i), colour_override,
                                          time, towards, speed);
    }
    if (box_attack_shape_enabled) {
      legacy::explode_shapes<box_attack_shape>(e, box_attack_parameters(transform), colour_override,
                                               time, towards, speed);
    }
  }

  void destruct_lines(const Transform& transform, EmitHandle& e, const vec2& source,
                      std::uint32_t time) const {
    legacy::destruct_lines<explode_shape>(e, shape_parameters(transform), source);
    for (std::uint32_t i = 0; i < 8; ++i) {
      legacy::destruct_lines<spark_shape>(e, spark_shape_parameters(transform, i), source, time);
    }
    if (box_attack_shape_enabled) {
      legacy::destruct_lines<box_attack_shape>(e, box_attack_parameters(transform), source, time);
    }
  }

  void render_override(const Transform& transform, const Health& health,
                       std::vector<render::shape>& output) const {
    std::optional<cvec4> colour_override;
    if ((start_time / 4) % 2 == 1) {
      colour_override = {0.f, 0.f, 0.f, 1.f / 255};
    } else if ((visible && ((vtime / 4) % 2 == 0)) || (!visible && ((vtime / 4) % 2 == 1))) {
      colour_override.reset();
    } else {
      colour_override = c2;
    }
    render_entity_shape_override<render_shape>(output, &health, shape_parameters(transform), -4.f,
                                               {}, colour_override);
    for (std::uint32_t i = 0; i < 8; ++i) {
      render_entity_shape_override<spark_shape>(
          output, &health, spark_shape_parameters(transform, i), -2.f, {}, colour_override);
    }
    if (box_attack_shape_enabled) {
      render_entity_shape_override<box_attack_shape>(
          output, nullptr, box_attack_parameters(transform), 0.f, {}, colour_override);
    }

    using outer_star_shape = standard_transform<
        geom::translate_p<2,
                          geom::rotate_p<3,
                                         geom::compound<geom::polystar_colour_p<16, 8, 4>,
                                                        geom::polygon_colour_p<12, 8, 4>>>>>;
    for (std::uint32_t n = 0; n < 5; ++n) {
      for (std::uint32_t i = 0; i < 16 + n * 6; ++i) {
        auto colour = outer_dangerous[n][i] ? c0 : n ? c2 : c1;
        std::tuple parameters{transform.centre, transform.rotation + outer_rotation[n],
                              outer_shape_d(n, i), outer_ball_rotation, colour};
        render_entity_shape_override<outer_star_shape>(output, nullptr, parameters, -16.f, {},
                                                       colour_override);
      }
    }
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
  h.add(Collision{.flags = GhostBoss::kShapeFlags,
                  .bounding_width = GhostBoss::kBoundingWidth,
                  .check_point = ecs::call<&GhostBoss::check_point>});
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
