#include "game/common/colour.h"
#include "game/geometry/legacy/attachment_point.h"
#include "game/geometry/legacy/line.h"
#include "game/geometry/legacy/ngon.h"
#include "game/geometry/node_disable_iteration.h"
#include "game/geometry/node_for_each.h"
#include "game/logic/legacy/boss/boss_internal.h"
#include "game/logic/legacy/enemy/enemy.h"
#include "game/logic/legacy/ship_template.h"
#include "game/logic/sim/io/events.h"

namespace ii::legacy {
namespace {

struct TractorBoss : ecs::component {
  static constexpr std::uint32_t kBaseHp = 900;
  static constexpr std::uint32_t kTimer = 100;
  static constexpr shape_flag kDangerousVulnerable =
      shape_flag::kDangerous | shape_flag::kVulnerable;
  static constexpr fixed kSpeed = 2;
  static constexpr fixed kTractorBeamSpeed = 2 + 1_fx / 2;
  static constexpr float kZIndex = -4.f;

  static constexpr cvec4 c0 = colour::hue360(300, .5f, .6f);
  static constexpr cvec4 c1 = colour::hue360(300, 1.f / 3, .6f);
  static constexpr cvec4 c2 = colour::hue360(300, .4f, .5f);

  using attack_shape = standard_transform<geom::translate_p<2, geom::polygon<8, 6, c0>>>;
  template <std::size_t BI>
  struct ball_inner {
    template <fixed I>
    using ball =
        geom::translate_eval<geom::constant<rotate_legacy(vec2{24, 0}, I* pi<fixed> / 4)>,
                             geom::rotate_eval<geom::multiply_p<BI ? 1 : -1, 2>,
                                               geom::polygram<12, 6, c0, 0, kDangerousVulnerable>>>;
    using shape = geom::for_each<fixed, 0, 8, ball>;
  };

  template <std::size_t I>
  using ball_shape = geom::translate<
      0, I ? 96 : -96,
      geom::rotate_eval<
          geom::multiply_p<fixed{I ? -1 : 1} / 2, 2>,
          geom::compound<geom::attachment_point<I, 0, 0>, geom::polygram<12, 6, c1>,
                         geom::polygon<30, 12, cvec4{0.f}, 0, shape_flag::kShield>,
                         geom::disable_iteration<
                             geom::iterate_volumes_t,
                             geom::compound<geom::polygon<12, 12, c1>, geom::polygon<2, 6, c1>,
                                            geom::polygon<36, 12, c0, 0, kDangerousVulnerable>,
                                            geom::polygon<34, 12, c0>, geom::polygon<32, 12, c0>,
                                            typename ball_inner<I>::shape>>>>>;
  using shape = standard_transform<
      geom::for_each<std::size_t, 0, 2, ball_shape>,
      geom::ngon_eval<geom::parameter<3>, geom::constant<16u>, geom::constant<c2>>,
      geom::line<-2, -96, -2, 96, c0>, geom::line<0, -96, 0, 96, c1>,
      geom::line<2, -96, 2, 96, c0>>;
  std::tuple<vec2, fixed, fixed, fixed> shape_parameters(const Transform& transform) const {
    return {transform.centre, transform.rotation, sub_rotation,
            static_cast<std::uint32_t>(attack_shapes.size()) / (1 + fixed_c::half)};
  }

  static std::uint32_t bounding_width(const SimInterface& sim) {
    return sim.is_legacy() ? 640 : 140;
  }

  TractorBoss(SimInterface& sim) : shoot_type{sim.random_bool()} {}
  bool will_attack = false;
  bool stopped = false;
  bool generating = false;
  bool attacking = false;
  bool move_away = false;
  bool gen_dir = false;
  bool sound = true;
  std::uint32_t shoot_type = 0;
  std::uint32_t timer = 0;
  std::vector<vec2> attack_shapes;
  std::vector<vec2> targets;
  fixed sub_rotation = 0;

  void update(ecs::handle h, Transform& transform, Render& render, const Health& health,
              SimInterface& sim) {
    auto e = sim.emit(resolve_key::predicted());
    auto dim_x = sim.dimensions().x;
    if (transform.centre.x <= dim_x / 2 && will_attack && !stopped && !move_away) {
      stopped = true;
      generating = true;
      gen_dir = !sim.random_bool();
      timer = 0;
    }

    if (transform.centre.x < -150) {
      transform.centre.x = dim_x + 150;
      will_attack = !will_attack;
      shoot_type = sim.random_bool();
      if (will_attack) {
        shoot_type = 0;
      }
      move_away = false;
      sound = !will_attack;
      transform.rotate(sim.random_fixed() * 2 * pi<fixed>);
    }

    ++timer;
    if (!stopped) {
      transform.move(kSpeed * vec2{-1, 0});
      if (!will_attack && sim.is_on_screen(transform.centre) &&
          !(timer % (16 - sim.alive_players() * 2))) {
        if (!shoot_type || (health.is_hp_low() && shoot_type == 1)) {
          auto p = sim.nearest_player_position(transform.centre);
          auto f = [&](std::size_t, const vec2& v, const vec2&) {
            auto d = normalise(p - v);
            spawn_boss_shot(sim, v, d * 5, c0);
            spawn_boss_shot(sim, v, d * -5, c0);
          };
          iterate_entity_attachment_points<TractorBoss>(h, sim.is_legacy(), f);
          e.play_random(sound::kBossFire, transform.centre);
        }
        if (shoot_type == 1 || health.is_hp_low()) {
          auto d = sim.nearest_player_direction(transform.centre);
          spawn_boss_shot(sim, transform.centre, d * 5, c0);
          spawn_boss_shot(sim, transform.centre, d * -5, c0);
          e.play_random(sound::kBossFire, transform.centre);
        }
      }
      if ((!will_attack || move_away) && sim.is_on_screen(transform.centre)) {
        if (sound) {
          e.play(sound::kBossAttack, transform.centre);
          sound = false;
        }
        targets.clear();
        sim.index().iterate_dispatch<Player>([&](const Player& p, Transform& p_transform) {
          if (!p.is_killed && length(p_transform.centre - transform.centre) <= dim_x) {
            targets.push_back(p_transform.centre);
            p_transform.centre +=
                normalise(transform.centre - p_transform.centre) * kTractorBeamSpeed;
          }
        });
      }
    } else {
      if (generating) {
        if (timer >= kTimer * 5) {
          timer = 0;
          generating = false;
          attacking = false;
          attack_shapes.clear();
          e.play(sound::kBossAttack, transform.centre);
        }

        if (timer < kTimer * 4 && !(timer % (10 - 2 * sim.alive_players()))) {
          auto f = [&](std::size_t index, const vec2& v, const vec2&) {
            spawn_bounce(sim, v,
                         !index == gen_dir ? transform.rotation + pi<fixed> : transform.rotation);
          };
          iterate_entity_attachment_points<TractorBoss>(h, sim.is_legacy(), f);
          e.play_random(sound::kEnemySpawn, transform.centre);
        }

        if (health.is_hp_low() && !(timer % (20 - sim.alive_players() * 2))) {
          auto d = sim.nearest_player_direction(transform.centre);
          spawn_boss_shot(sim, transform.centre, d * 5, c0);
          spawn_boss_shot(sim, transform.centre, d * -5, c0);
          e.play_random(sound::kBossFire, transform.centre);
        }
      } else {
        if (!attacking) {
          if (timer >= kTimer * 4) {
            timer = 0;
            attacking = true;
          }
          if (timer % (kTimer / (1 + fixed_c::half)).to_int() == kTimer / 8) {
            auto f = [&](std::size_t, const vec2& v, const vec2&) {
              auto d = from_polar_legacy(sim.random_fixed() * (2 * pi<fixed>), 5_fx);
              spawn_boss_shot(sim, v, d, c0);
              d = sim.rotate_compatibility(d, pi<fixed> / 2);
              spawn_boss_shot(sim, v, d, c0);
              d = sim.rotate_compatibility(d, pi<fixed> / 2);
              spawn_boss_shot(sim, v, d, c0);
              d = sim.rotate_compatibility(d, pi<fixed> / 2);
              spawn_boss_shot(sim, v, d, c0);
            };
            iterate_entity_attachment_points<TractorBoss>(h, sim.is_legacy(), f);
            e.play_random(sound::kBossFire, transform.centre);
          }
          targets.clear();

          sim.index().iterate_dispatch<Player>([&](const Player& p, Transform& p_transform) {
            if (!p.is_killed && length(p_transform.centre - transform.centre) <= dim_x) {
              targets.push_back(p_transform.centre);
              p_transform.centre +=
                  normalise(transform.centre - p_transform.centre) * kTractorBeamSpeed;
            }
          });

          sim.index().iterate_dispatch_if<Enemy>([&](ecs::handle eh, Transform& e_transform) {
            if (eh.id() == h.id() || length(e_transform.centre - transform.centre) > dim_x) {
              return;
            }
            e.play_random(sound::kBossAttack, transform.centre, /* volume */ .3f);
            targets.push_back(e_transform.centre);
            e_transform.centre +=
                normalise(transform.centre - e_transform.centre) * (4 + fixed_c::half);
            if (!(eh.has<WallTag>()) && length(e_transform.centre - transform.centre) <= 40) {
              eh.emplace<Destroy>();
              attack_shapes.emplace_back(0);
            }
          });
        } else {
          timer = 0;
          stopped = false;
          move_away = true;
          for (std::uint32_t i = 0; i < attack_shapes.size(); ++i) {
            vec2 d = from_polar_legacy(
                i * (2 * pi<fixed>) / static_cast<std::uint32_t>(attack_shapes.size()), 5_fx);
            spawn_boss_shot(sim, transform.centre, d, c0);
          }
          e.play(sound::kBossFire, transform.centre);
          e.play_random(sound::kExplosion, transform.centre);
          attack_shapes.clear();
        }
      }
    }

    for (auto& v : attack_shapes) {
      v = from_polar_legacy(sim.random_fixed() * (2 * pi<fixed>),
                            2 * (sim.random_fixed() - fixed_c::half) *
                                fixed{static_cast<std::uint32_t>(attack_shapes.size())} /
                                (1 + fixed_c::half));
    }

    fixed r = 0;
    if (!will_attack || (stopped && !generating && !attacking)) {
      r = fixed_c::hundredth;
    } else if (stopped && attacking && !generating) {
      r = 0;
    } else {
      r = fixed_c::hundredth / 2;
    }
    transform.rotate(r);
    sub_rotation += fixed_c::tenth;
  }

  void on_hit(ecs::handle h, const Transform& transform, SimInterface& sim, EmitHandle& e,
              damage_type type, const vec2& source_position) const {
    boss_on_hit<true, TractorBoss>(h, sim, e, type, source_position);
    // Compatiblity with old attack shapes actually being part of the shape.
    if (type == damage_type::kBomb) {
      for (const auto& v : attack_shapes) {
        std::tuple parameters{transform.centre, transform.rotation, v};
        explode_shapes<attack_shape>(e, parameters);
        explode_shapes<attack_shape>(e, parameters, cvec4{1.f}, 12);
        explode_shapes<attack_shape>(e, parameters, std::nullopt, 24);
      }
    }
  }

  void render(const Transform& transform, std::vector<render::shape>& output,
              const SimInterface& sim) const {
    if ((stopped && !generating && !attacking) ||
        (!stopped && (move_away || !will_attack) && sim.is_on_screen(transform.centre))) {
      for (std::size_t i = 0; i < targets.size(); ++i) {
        if (((timer + i * 4) / 4) % 2) {
          output.emplace_back(render::shape::line(to_float(transform.centre), to_float(targets[i]),
                                                  c0, 0.f, 1.f,
                                                  static_cast<unsigned char>(1 + timer % 8)));
        }
      }
    }
    for (const auto& v : attack_shapes) {
      std::tuple parameters{transform.centre, transform.rotation, v};
      render_shape<attack_shape>(output, parameters);
    }
  }
};
DEBUG_STRUCT_TUPLE(TractorBoss, will_attack, stopped, generating, attacking, move_away, gen_dir,
                   sound, shoot_type, timer, attack_shapes, targets, sub_rotation);

}  // namespace

void spawn_tractor_boss(SimInterface& sim, std::uint32_t cycle) {
  auto h = create_ship<TractorBoss>(
      sim, {sim.dimensions().x * (1 + fixed_c::half), sim.dimensions().y / 2});
  h.add(Enemy{.threat_value = 100,
              .boss_score_reward =
                  calculate_boss_score(boss_flag::kBoss2A, sim.player_count(), cycle)});
  h.add(Health{
      .hp = calculate_boss_hp(TractorBoss::kBaseHp, sim.player_count(), cycle),
      .hit_sound0 = std::nullopt,
      .hit_sound1 = sound::kEnemyShatter,
      .destroy_sound = std::nullopt,
      .damage_transform = &scale_boss_damage,
      .on_hit = ecs::call<&TractorBoss::on_hit>,
      .on_destroy = ecs::call<&boss_on_destroy<TractorBoss>>,
  });
  h.add(Boss{.boss = boss_flag::kBoss2A});
  h.add(TractorBoss{sim});
}

}  // namespace ii::legacy