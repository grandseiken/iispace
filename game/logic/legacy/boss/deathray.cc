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
#include "game/logic/legacy/ship_template.h"
#include "game/logic/sim/io/events.h"

namespace ii::legacy {
namespace {
constexpr cvec4 c0 = colour::hue360(150, 1.f / 3, .6f);
constexpr cvec4 c1 = colour::hue360(150, .6f);
constexpr cvec4 c2 = colour::hue(0.f, .8f, 0.f);
constexpr cvec4 c3 = colour::hue(0.f, .6f, 0.f);

struct DeathRay : ecs::component {
  static constexpr fixed kBoundingWidth = 48;
  static constexpr float kZIndex = 0.f;
  static constexpr sound kDestroySound = sound::kEnemyDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kNone;
  static constexpr fixed kSpeed = 10;

  using shape = standard_transform<geom::box<10, 48, cvec4{0.f}, 0, shape_flag::kDangerous>,
                                   geom::line<0, 48, 0, -48, cvec4{1.f}>>;

  void update(ecs::handle h, Transform& transform, SimInterface& sim) {
    transform.move(vec2{1, 0} * kSpeed);
    if (transform.centre.x > sim.dimensions().x + 20) {
      h.emplace<Destroy>();
    }
  }
};
DEBUG_STRUCT_TUPLE(DeathRay);

void spawn_death_ray(SimInterface& sim, const vec2& position) {
  auto h = create_ship<DeathRay>(sim, position);
  add_enemy_health<DeathRay>(h, 0);
  h.add(DeathRay{});
  h.add(Enemy{.threat_value = 1});
}

struct DeathArm : ecs::component {
  static constexpr fixed kBoundingWidth = 60;
  static constexpr float kZIndex = 4.f;
  static constexpr sound kDestroySound = sound::kPlayerDestroy;
  static constexpr rumble_type kDestroyRumble = rumble_type::kLarge;

  static constexpr std::uint32_t kTimer = 300;
  static constexpr fixed kSpeed = 4;

  using shape = standard_transform<
      geom::ngon<60, 4, c1>,
      geom::conditional_p<
          2, geom::polygram<50, 4, c0, 0, shape_flag::kVulnerable>,
          geom::polygram<50, 4, c0, 0, shape_flag::kDangerous | shape_flag::kVulnerable>>,
      geom::ball_collider<40, shape_flag::kShield>, geom::ngon<20, 4, c1>, geom::ngon<18, 4, c0>>;

  std::tuple<vec2, fixed, bool> shape_parameters(const Transform& transform) const {
    return {transform.centre, transform.rotation, start > 0};
  }

  ecs::entity_id death_boss{0};
  bool is_top = false;
  bool attacking = false;
  std::uint32_t start = 30;
  std::uint32_t timer = 0;
  std::uint32_t shots = 0;
  vec2 dir{0};
  vec2 target{0};

  DeathArm(ecs::entity_id death_boss, bool is_top)
  : death_boss{death_boss}, is_top{is_top}, timer{is_top ? 2 * kTimer / 3 : 0} {}

  void update(ecs::handle h, Transform& transform, SimInterface& sim) {
    auto e = sim.emit(resolve_key::predicted());
    if (timer % (kTimer / 2) == kTimer / 4) {
      e.play_random(sound::kBossFire, transform.centre);
      target = sim.nearest_player_position(transform.centre);
      shots = 16;
    }
    if (shots > 0) {
      auto d = normalise(target - transform.centre) * 5;
      spawn_boss_shot(sim, transform.centre, d, c1);
      --shots;
    }

    transform.rotate(fixed_c::hundredth * 5);
    if (attacking) {
      ++timer;
      if (timer < kTimer / 4) {
        auto d = sim.nearest_player_direction(transform.centre);
        if (d != vec2{}) {
          transform.move(kSpeed * (dir = d));
        }
      } else if (timer < kTimer / 2) {
        transform.move(dir * kSpeed);
      } else if (timer < kTimer) {
        vec2 d = sim.index().get(death_boss)->get<Transform>()->centre +
            vec2{80, is_top ? 80 : -80} - transform.centre;
        if (length(d) > kSpeed) {
          transform.move(normalise(d) * kSpeed);
        } else {
          attacking = false;
          timer = 0;
        }
      } else if (timer >= kTimer) {
        attacking = false;
        timer = 0;
      }
      return;
    }

    ++timer;
    if (timer >= kTimer) {
      timer = 0;
      attacking = true;
      dir = vec2{0};
      e.play(sound::kBossAttack, transform.centre);
    }
    transform.centre =
        sim.index().get(death_boss)->get<Transform>()->centre + vec2{80, is_top ? 80 : -80};

    if (start) {
      if (start == 30) {
        auto e = sim.emit(resolve_key::reconcile(h.id(), resolve_tag::kRespawn));
        explode_entity_shapes<DeathArm>(h, e);
        explode_entity_shapes<DeathArm>(h, e, cvec4{1.f});
      }
      start--;
    }
  }

  void on_destroy(ecs::const_handle h, SimInterface& sim, EmitHandle& e) const;
};
DEBUG_STRUCT_TUPLE(DeathArm, death_boss, is_top, attacking, start, timer, shots, dir, target);

ecs::handle spawn_death_arm(SimInterface& sim, ecs::handle boss, bool is_top, std::uint32_t hp) {
  auto h = create_ship<DeathArm>(sim, vec2{0});
  add_enemy_health<DeathArm>(h, hp);
  h.add(DeathArm{boss.id(), is_top});
  h.add(Enemy{.threat_value = 10});
  return h;
}

struct DeathRayBoss : public ecs::component {
  static constexpr std::uint32_t kBaseHp = 600;
  static constexpr std::uint32_t kArmHp = 100;
  static constexpr std::uint32_t kRayTimer = 100;
  static constexpr std::uint32_t kTimer = 100;
  static constexpr std::uint32_t kArmRTimer = 400;
  static constexpr fixed kSpeed = 5;
  static constexpr float kZIndex = -4.f;

  template <fixed I>
  using edge_shape =
      geom::rotate<I * pi<fixed> / 6,
                   geom::translate<130, 0, geom::box<10, 24, c1, 0, shape_flag::kDangerous>,
                                   geom::box<8, 22, c0, 0, shape_flag::kDangerous>>>;
  using shape = standard_transform<
      geom::rotate<pi<fixed> / 12, geom::polystar<110, 12, c0>, geom::polygram<70, 12, c1>,
                   geom::polygon<120, 12, c1, 0, shape_flag::kDangerous | shape_flag::kVulnerable>,
                   geom::ngon<115, 12, c1>, geom::polygon<110, 12, c1, 0, shape_flag::kShield>>,
      geom::box<0, 0, cvec4{0.f}>,
      geom::disable_iteration<geom::iterate_volumes_t, geom::for_each<fixed, 1, 12, edge_shape>>>;

  static std::uint32_t bounding_width(const SimInterface& sim) {
    return sim.is_legacy() ? 640 : 140;
  }

  std::vector<ecs::entity_id> arms;
  std::uint32_t timer = kTimer * 2;
  bool laser = false;
  bool dir = true;
  std::uint32_t pos = 1;
  std::uint32_t arm_timer = 0;
  std::uint32_t shot_timer = 0;

  std::uint32_t ray_attack_timer = 0;
  vec2 ray_src1{0};
  vec2 ray_src2{0};
  vec2 ray_dest{0};
  std::vector<std::pair<std::uint32_t, std::uint32_t>> shot_queue;

  void update(ecs::handle h, Transform& transform, const Health& health, SimInterface& sim) {
    auto e = sim.emit(resolve_key::predicted());
    bool in_position = true;
    auto sim_dim = sim.dimensions();
    fixed d = pos == 0 ? 1 * sim_dim.y / 4
        : pos == 1     ? 2 * sim_dim.y / 4
        : pos == 2     ? 3 * sim_dim.y / 4
        : pos == 3     ? 1 * sim_dim.y / 8
        : pos == 4     ? 3 * sim_dim.y / 8
        : pos == 5     ? 5 * sim_dim.y / 8
                       : 7 * sim_dim.y / 8;
    d -= transform.centre.y;

    if (abs(d) > 3) {
      transform.move(vec2{0, d / abs(d)} * kSpeed);
      in_position = false;
    }

    if (ray_attack_timer) {
      ray_attack_timer--;
      if (ray_attack_timer == 40) {
        ray_dest = sim.nearest_player_position(transform.centre);
      }
      if (ray_attack_timer < 40) {
        auto d = normalise(ray_dest - transform.centre);
        spawn_boss_shot(sim, transform.centre, d * 10, c2);
        e.play_random(sound::kBossAttack, transform.centre);
        auto e = sim.emit(resolve_key::predicted());
        explode_entity_shapes<DeathRayBoss>(h, e);
      }
    }

    bool going_fast = false;
    if (laser) {
      if (timer) {
        if (in_position) {
          --timer;
        }

        if (timer < kTimer * 2 && !(timer % 3)) {
          spawn_death_ray(sim, transform.centre + vec2{100, 0});
          e.play_random(sound::kBossFire, transform.centre);
        }
        if (!timer) {
          laser = false;
          timer = kTimer * (2 + sim.random(3));
        }
      } else {
        auto r = transform.rotation;
        if (!r) {
          timer = kTimer * 2;
        }

        if (r < fixed_c::tenth || r > 2 * pi<fixed> - fixed_c::tenth) {
          transform.rotation = 0;
        } else {
          going_fast = true;
          transform.rotate(dir ? fixed_c::tenth : -fixed_c::tenth);
        }
      }
    } else {
      transform.rotate(dir ? fixed_c::hundredth * 2 : -fixed_c::hundredth * 2);
      if (sim.is_on_screen(transform.centre)) {
        --timer;
        if (!(timer % kTimer) && timer && !sim.random(4)) {
          dir = sim.random_bool();
          pos = sim.random(7);
        }
        if (timer == kTimer * 2 + 50 && arms.size() == 2) {
          ray_attack_timer = kRayTimer;
          ray_src1 = sim.index().get(arms[0])->get<Transform>()->centre;
          ray_src2 = sim.index().get(arms[1])->get<Transform>()->centre;
          e.play(sound::kEnemySpawn, transform.centre);
        }
      }
      if (!timer) {
        laser = true;
        pos = sim.random(7);
      }
    }

    ++shot_timer;
    if (arms.empty() && !health.is_hp_low()) {
      if (!(shot_timer % 8)) {
        shot_queue.emplace_back((shot_timer / 8) % 12, 16);
        e.play_random(sound::kBossFire, transform.centre);
      }
    }
    if (arms.empty() && health.is_hp_low()) {
      if (shot_timer % 48 == 0) {
        for (std::uint32_t i = 1; i < 16; i += 3) {
          shot_queue.emplace_back(i, 16);
        }
        e.play_random(sound::kBossFire, transform.centre);
      }
      if (shot_timer % 48 == 16) {
        for (std::uint32_t i = 2; i < 12; i += 3) {
          shot_queue.emplace_back(i, 16);
        }
        e.play_random(sound::kBossFire, transform.centre);
      }
      if (shot_timer % 48 == 32) {
        for (std::uint32_t i = 3; i < 12; i += 3) {
          shot_queue.emplace_back(i, 16);
        }
        e.play_random(sound::kBossFire, transform.centre);
      }
      if (shot_timer % 128 == 0) {
        ray_attack_timer = kRayTimer;
        vec2 d1 = from_polar_legacy(sim.random_fixed() * 2 * pi<fixed>, 110_fx);
        vec2 d2 = from_polar_legacy(sim.random_fixed() * 2 * pi<fixed>, 110_fx);
        ray_src1 = transform.centre + d1;
        ray_src2 = transform.centre + d2;
        e.play(sound::kEnemySpawn, transform.centre);
      }
    }
    if (arms.empty()) {
      ++arm_timer;
      if (arm_timer >= kArmRTimer) {
        arm_timer = 0;
        if (!health.is_hp_low()) {
          auto players = sim.get_lives() ? sim.player_count() : sim.alive_players();
          std::uint32_t hp =
              (kArmHp * (7 * fixed_c::tenth + 3 * fixed_c::tenth * players)).to_int();
          arms.push_back(spawn_death_arm(sim, h, true, hp).id());
          arms.push_back(spawn_death_arm(sim, h, false, hp).id());
          e.play(sound::kEnemySpawn, transform.centre);
        }
      }
    }

    for (std::size_t i = 0; i < shot_queue.size(); ++i) {
      if (!going_fast || shot_timer % 2) {
        auto n = shot_queue[i].first;
        vec2 d = sim.rotate_compatibility(vec2{1, 0}, transform.rotation + n * pi<fixed> / 6);
        spawn_boss_shot(sim, transform.centre + d * 120, d * 5, c1);
      }
      shot_queue[i].second--;
      if (!shot_queue[i].second) {
        shot_queue.erase(shot_queue.begin() + i);
        --i;
      }
    }
  }

  void render(const Transform& transform, std::vector<render::shape>& output) const {
    using ray_shape = geom::translate_p<0, geom::polystar<10, 6, c3>>;
    for (std::uint32_t i = ray_attack_timer; i <= ray_attack_timer + 16; ++i) {
      auto k = i < 8 ? 0 : i - 8;
      if (k < 40 || k > kRayTimer) {
        continue;
      }

      auto v = ray_src1 - transform.centre;
      v *= static_cast<fixed>(k - 40) / (kRayTimer - 40);
      render_shape<ray_shape>(output, std::tuple{transform.centre + v});

      v = ray_src2 - transform.centre;
      v *= static_cast<fixed>(k - 40) / (kRayTimer - 40);
      render_shape<ray_shape>(output, std::tuple{transform.centre + v});
    }
  }

  std::uint32_t get_damage(std::uint32_t damage) const { return arms.empty() ? damage : 0u; }
};
DEBUG_STRUCT_TUPLE(DeathRayBoss, arms, timer, laser, dir, pos, arm_timer, shot_timer,
                   ray_attack_timer, ray_src1, ray_src2, ray_dest);

void DeathArm::on_destroy(ecs::const_handle h, SimInterface& sim, EmitHandle& e) const {
  explode_entity_shapes<DeathArm>(h, e);
  explode_entity_shapes<DeathArm>(h, e, cvec4{1.f}, 12);
  explode_entity_shapes<DeathArm>(h, e, c1, 24);
  if (auto boss_h = sim.index().get(death_boss); boss_h) {
    std::erase(boss_h->get<DeathRayBoss>()->arms, h.id());
  }
}

std::uint32_t transform_death_ray_boss_damage(ecs::handle h, SimInterface& sim, damage_type type,
                                              std::uint32_t damage) {
  // TODO: weird.
  auto d = ecs::call<&DeathRayBoss::get_damage>(h, damage);
  return scale_boss_damage(h, sim, type, d);
}

}  // namespace

void spawn_death_ray_boss(SimInterface& sim, std::uint32_t cycle) {
  auto h = create_ship<DeathRayBoss>(sim, {sim.dimensions().x * (3_fx / 20), -sim.dimensions().y});
  h.add(Enemy{.threat_value = 100,
              .boss_score_reward =
                  calculate_boss_score(boss_flag::kBoss2C, sim.player_count(), cycle)});
  h.add(Health{
      .hp = calculate_boss_hp(DeathRayBoss::kBaseHp, sim.player_count(), cycle),
      .hit_flash_ignore_index = 5,
      .hit_sound0 = std::nullopt,
      .hit_sound1 = sound::kEnemyShatter,
      .destroy_sound = std::nullopt,
      .damage_transform = &transform_death_ray_boss_damage,
      .on_hit = &boss_on_hit<true, DeathRayBoss>,
      .on_destroy = ecs::call<&boss_on_destroy<DeathRayBoss>>,
  });
  h.add(Boss{.boss = boss_flag::kBoss2C});
  h.add(DeathRayBoss{});
  h.get<Transform>()->rotate(2 * pi<fixed> * sim.random_fixed());
}
}  // namespace ii::legacy