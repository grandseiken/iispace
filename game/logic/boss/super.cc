#include "game/logic/boss/boss_internal.h"
#include "game/logic/enemy/enemy.h"
#include "game/logic/geometry/geometry.h"
#include "game/logic/player/player.h"
#include "game/logic/ship/ship_template.h"
#include <array>

namespace ii {
namespace {

struct SuperBossArc : public ecs::component {
  static constexpr std::uint32_t kBaseHp = 75;

  template <fixed Radius, std::size_t I, shape_flag Flags = shape_flag::kNone>
  using polyarc =
      geom::polyarc_eval<geom::constant<Radius>, geom::constant<32u>, geom::constant<2u>,
                         geom::parameter_i<3, I>, geom::constant<Flags>>;
  using shape = standard_transform<
      geom::rotate_p<2, polyarc<140, 0>, polyarc<135, 1>, polyarc<130, 2>,
                     polyarc<125, 3,
                             shape_flag::kShield | shape_flag::kSafeShield |
                                 shape_flag::kDangerous | shape_flag::kVulnerable>,
                     polyarc<120, 4>, polyarc<115, 5>, polyarc<110, 6>,
                     polyarc<105, 7, shape_flag::kShield | shape_flag::kSafeShield>>>;

  std::tuple<vec2, fixed, fixed, std::array<glm::vec4, 8>>
  shape_parameters(const Transform& transform) const {
    return {transform.centre, transform.rotation, i * 2 * fixed_c::pi / 16, colours};
  }

  static std::uint32_t bounding_width(const SimInterface& sim) {
    return sim.conditions().compatibility == compatibility_level::kLegacy ? 640 : 130;
  }

  SuperBossArc(ecs::entity_id boss, std::uint32_t i, std::uint32_t timer)
  : boss{boss}, i{i}, timer{timer} {}
  ecs::entity_id boss{0};
  std::uint32_t i = 0;
  std::uint32_t timer = 0;
  std::uint32_t s_timer = 0;
  std::array<glm::vec4, 8> colours{glm::vec4{0.f}};

  void update(Transform& transform, const Health& health, SimInterface&) {
    transform.rotate(6_fx / 1000);
    for (std::uint32_t i = 0; i < 8; ++i) {
      colours[7 - i] = {((i * 32 + 2 * timer) % 256) / 256.f, 1.f, .5f,
                        health.is_hp_low() ? .6f : 1.f};
    }
    ++timer;
  }

  void on_destroy(const Transform& transform, SimInterface& sim, damage_type) const {
    auto parameters = shape_parameters(transform);
    std::get<0>(parameters) += from_polar(i * 2 * fixed_c::pi / 16 + transform.rotation, 120_fx);
    explode_shapes<shape>(sim, parameters);
    explode_shapes<shape>(sim, parameters, glm::vec4{1.f}, 12);
    explode_shapes<shape>(sim, parameters, std::nullopt, 24);
    explode_shapes<shape>(sim, parameters, glm::vec4{1.f}, 36);
    explode_shapes<shape>(sim, parameters, std::nullopt, 48);
    sim.play_sound(sound::kExplosion, std::get<0>(parameters), /* random */ true);
  }
};

ecs::handle spawn_super_boss_arc(SimInterface& sim, const vec2& position, std::uint32_t cycle,
                                 std::uint32_t i, ecs::handle boss, std::uint32_t timer = 0) {
  auto h = create_ship<SuperBossArc>(sim, position);
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
      .on_hit = &boss_on_hit<true, SuperBossArc>,
      .on_destroy = ecs::call<&SuperBossArc::on_destroy>,
  });
  h.add(SuperBossArc{boss.id(), i, timer});
  return h;
}

struct SuperBoss : ecs::component {
  enum class state { kArrive, kIdle, kAttack };
  static constexpr std::uint32_t kBaseHp = 520;

  template <fixed Radius, std::size_t I, shape_flag Flags = shape_flag::kNone>
  using polygon =
      geom::ngon_eval<geom::constant<Radius>, geom::constant<32u>, geom::parameter_i<2, I>,
                      geom::constant<geom::ngon_style::kPolygon>, geom::constant<Flags>>;
  using shape =
      standard_transform<polygon<40, 0, shape_flag::kDangerous | shape_flag::kVulnerable>,
                         polygon<35, 1>, polygon<30, 2, shape_flag::kShield>, polygon<25, 3>,
                         polygon<20, 4>, polygon<15, 5>, polygon<10, 6>, polygon<5, 7>>;

  std::tuple<vec2, fixed, std::array<glm::vec4, 8>>
  shape_parameters(const Transform& transform) const {
    return {transform.centre, transform.rotation, colours};
  }

  static std::uint32_t bounding_width(const SimInterface& sim) {
    return sim.conditions().compatibility == compatibility_level::kLegacy ? 640 : 50;
  }

  SuperBoss(std::uint32_t cycle) : cycle{cycle} {}
  state state = state::kArrive;
  std::uint32_t cycle = 0;
  std::uint32_t ctimer = 0;
  std::uint32_t timer = 0;
  std::uint32_t snakes = 0;
  std::vector<ecs::entity_id> arcs;
  std::array<glm::vec4, 8> colours{glm::vec4{0.f}};

  void update(ecs::handle h, Transform& transform, SimInterface& sim) {
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
    auto c = colour_hue((ctimer % 128) / 128.f);
    for (std::uint32_t i = 0; i < 8; ++i) {
      colours[7 - i] = colour_hue(((i * 32 + 2 * ctimer) % 256) / 256.f);
    }
    ++ctimer;
    if (transform.centre.y < kSimDimensions.y / 2) {
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
    static constexpr fixed pi2d64 = 2 * fixed_c::pi / 64;
    static constexpr fixed pi2d32 = 2 * fixed_c::pi / 32;

    if (state == state::kIdle && sim.is_on_screen(transform.centre) && timer && timer % 300 == 0) {
      auto r = sim.random(6);
      if (r == 0 || r == 1) {
        snakes = 16;
      } else if (r == 2 || r == 3) {
        state = state::kAttack;
        timer = 0;
        fixed f = sim.random_fixed() * (2 * fixed_c::pi);
        fixed rf = d5d1000 * (1 + sim.random(2));
        for (std::uint32_t i = 0; i < 32; ++i) {
          vec2 d = from_polar(f + i * pi2d32, 1_fx);
          if (r == 2) {
            rf = d5d1000 * (1 + sim.random(4));
          }
          spawn_snake(sim, transform.centre + d * 16, c, d, rf);
          sim.play_sound(sound::kBossAttack, transform.centre, /* random */ true);
        }
      } else if (r == 5) {
        state = state::kAttack;
        timer = 0;
        fixed f = sim.random_fixed() * (2 * fixed_c::pi);
        for (std::uint32_t i = 0; i < 64; ++i) {
          vec2 d = from_polar(f + i * pi2d64, 1_fx);
          spawn_snake(sim, transform.centre + d * 16, c, d);
          sim.play_sound(sound::kBossAttack, transform.centre, /* random */ true);
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
        sim.play_sound(sound::kEnemySpawn, transform.centre);
      }
    }
    static const fixed pi2d128 = 2 * fixed_c::pi / 128;
    if (state == state::kIdle && timer % 72 == 0) {
      for (std::uint32_t i = 0; i < 128; ++i) {
        vec2 d = from_polar(i * pi2d128, 1_fx);
        spawn_boss_shot(sim, transform.centre + d * 42, move_vec + d * 3,
                        glm::vec4{0.f, 0.f, .6f, 1.f}, /* rotate speed */ 6_fx / 1000);
        sim.play_sound(sound::kBossFire, transform.centre, /* random */ true);
      }
    }

    if (snakes) {
      --snakes;
      vec2 d = from_polar(sim.random_fixed() * (2 * fixed_c::pi),
                          fixed{sim.random(32) + sim.random(16)});
      spawn_snake(sim, d + transform.centre, c);
      sim.play_sound(sound::kEnemySpawn, transform.centre, /* random */ true);
    }
    transform.move(move_vec);
  }

  void on_destroy(ecs::const_handle h, const Transform& transform, SimInterface& sim,
                  damage_type) const {
    sim.index().iterate_dispatch_if<Enemy>([&](ecs::handle eh, Health& health) {
      if (eh.id() != h.id()) {
        health.damage(eh, sim, 100 * Player::kBombDamage, damage_type::kBomb, std::nullopt);
      }
    });
    auto parameters = shape_parameters(transform);
    explode_shapes<shape>(sim, parameters);
    explode_shapes<shape>(sim, parameters, glm::vec4{1.f}, 12);
    explode_shapes<shape>(sim, parameters, std::nullopt, 24);
    explode_shapes<shape>(sim, parameters, glm::vec4{1.f}, 36);
    explode_shapes<shape>(sim, parameters, std::nullopt, 48);

    std::uint32_t n = 1;
    for (std::uint32_t i = 0; i < 16; ++i) {
      vec2 v = from_polar(sim.random_fixed() * (2 * fixed_c::pi),
                          fixed{8 + sim.random(64) + sim.random(64)});
      sim.global_entity().get<GlobalData>()->fireworks.push_back(GlobalData::fireworks_entry{
          .time = n, .position = transform.centre + v, .colour = colours[i]});
      n += i;
    }
    sim.rumble_all(25);
    sim.play_sound(sound::kExplosion, transform.centre);

    for (std::uint32_t i = 0; i < 8; ++i) {
      spawn_powerup(sim, transform.centre, powerup_type::kBomb);
    }
  }
};

}  // namespace

void spawn_super_boss(SimInterface& sim, std::uint32_t cycle) {
  auto h =
      create_ship<SuperBoss>(sim, {kSimDimensions.x / 2, -kSimDimensions.y / (2 + fixed_c::half)});
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
      .on_hit = &boss_on_hit<true, SuperBoss>,
      .on_destroy = ecs::call<&SuperBoss::on_destroy>,
  });
  h.add(Boss{.boss = boss_flag::kBoss3A});
  h.add(SuperBoss{cycle});
}
}  // namespace ii
