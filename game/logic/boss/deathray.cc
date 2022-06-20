#include "game/logic/boss/boss_internal.h"
#include "game/logic/enemy/enemy.h"
#include "game/logic/player/player.h"
#include "game/logic/ship/geometry.h"
#include "game/logic/ship/ship_template.h"

namespace ii {
namespace {
constexpr std::uint32_t kDrbBaseHp = 600;
constexpr std::uint32_t kDrbArmHp = 100;
constexpr std::uint32_t kDrbRayTimer = 100;
constexpr std::uint32_t kDrbTimer = 100;
constexpr std::uint32_t kDrbArmRTimer = 400;

constexpr fixed kDrbSpeed = 5;

constexpr glm::vec4 c0 = colour_hue360(150, 1.f / 3, .6f);
constexpr glm::vec4 c1 = colour_hue360(150, .6f);
constexpr glm::vec4 c2 = colour_hue(0.f, .8f, 0.f);
constexpr glm::vec4 c3 = colour_hue(0.f, .6f, 0.f);

struct DeathRay : ecs::component {
  static constexpr ship_flag kShipFlags = ship_flag::kEnemy;
  static constexpr fixed kBoundingWidth = 48;
  static constexpr sound kDestroySound = sound::kEnemyDestroy;
  static constexpr fixed kSpeed = 10;

  using shape = standard_transform<geom::box_shape<10, 48, glm::vec4{0.f}, shape_flag::kDangerous>,
                                   geom::line_shape<48, fixed_c::pi / 2, glm::vec4{1.f}>>;

  void update(ecs::handle h, Transform& transform, SimInterface&) {
    transform.move(vec2{1, 0} * kSpeed);
    if (transform.centre.x > kSimDimensions.x + 20) {
      h.emplace<Destroy>();
    }
  }
};

void spawn_death_ray(SimInterface& sim, const vec2& position) {
  auto h = create_ship<DeathRay>(sim, position);
  add_enemy_health<DeathRay>(h, 0);
  h.add(DeathRay{});
  h.add(Enemy{.threat_value = 1});
}

struct DeathArm : ecs::component {
  static constexpr ship_flag kShipFlags = ship_flag::kEnemy;
  static constexpr fixed kBoundingWidth = 60;
  static constexpr sound kDestroySound = sound::kPlayerDestroy;

  static constexpr std::uint32_t kTimer = 300;
  static constexpr fixed kSpeed = 4;

  using shape = standard_transform<
      geom::ngon_shape<60, 4, c1>,
      geom::conditional_p<
          2, geom::ngon_shape<50, 4, c0, geom::ngon_style::kPolygram, shape_flag::kVulnerable>,
          geom::ngon_shape<50, 4, c0, geom::ngon_style::kPolygram,
                           shape_flag::kDangerous | shape_flag::kVulnerable>>,
      geom::ball_collider_shape<40, shape_flag::kShield>, geom::ngon_shape<20, 4, c1>,
      geom::ngon_shape<18, 4, c0>>;

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
    if (timer % (kTimer / 2) == kTimer / 4) {
      sim.play_sound(sound::kBossFire, transform.centre, true);
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
        vec2 d = sim.index().get(death_boss)->get<LegacyShip>()->ship->position() +
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
      sim.play_sound(sound::kBossAttack);
    }
    transform.centre = sim.index().get(death_boss)->get<LegacyShip>()->ship->position() +
        vec2{80, is_top ? 80 : -80};

    if (start) {
      if (start == 30) {
        explode_entity_shapes<DeathArm>(h, sim);
        explode_entity_shapes_with_colour<DeathArm>(h, sim, glm::vec4{1.f});
      }
      start--;
    }
  }

  void on_destroy(ecs::const_handle h, const Transform& transform, SimInterface& sim) const {
    explode_entity_shapes<DeathArm>(h, sim);
    explode_entity_shapes_with_colour<DeathArm>(h, sim, glm::vec4{1.f}, 12);
    explode_entity_shapes_with_colour<DeathArm>(h, sim, c1, 24);
  }
};

ecs::handle spawn_death_arm(SimInterface& sim, IShip* boss, bool is_top, std::uint32_t hp) {
  auto h = create_ship<DeathArm>(sim, vec2{0});
  add_enemy_health<DeathArm>(h, hp);
  h.add(DeathArm{boss->handle().id(), is_top});
  h.add(Enemy{.threat_value = 10});
  return h;
}

class DeathRayBoss : public ::Boss {
public:
  DeathRayBoss(SimInterface& sim);

  void update() override;
  void render() const override;

  std::uint32_t get_damage(std::uint32_t damage) {
    return !arms_.empty() ? 0 : damage;
  }

private:
  std::uint32_t timer_ = 0;
  bool laser_ = false;
  bool dir_ = true;
  std::uint32_t pos_ = 1;
  std::vector<ecs::entity_id> arms_;
  std::uint32_t arm_timer_ = 0;
  std::uint32_t shot_timer_ = 0;

  std::uint32_t ray_attack_timer_ = 0;
  vec2 ray_src1_{0};
  vec2 ray_src2_{0};
  vec2 ray_dest_{0};

  std::vector<std::pair<std::uint32_t, std::uint32_t>> shot_queue_;
};

DeathRayBoss::DeathRayBoss(SimInterface& sim)
: Boss{sim, {kSimDimensions.x * (3_fx / 20), -kSimDimensions.y}}, timer_{kDrbTimer * 2} {
  add_new_shape<Polygon>(vec2{0}, 110, 12, c0, fixed_c::pi / 12, shape_flag::kNone,
                         Polygon::T::kPolystar);
  add_new_shape<Polygon>(vec2{0}, 70, 12, c1, fixed_c::pi / 12, shape_flag::kNone,
                         Polygon::T::kPolygram);
  add_new_shape<Polygon>(vec2{0}, 120, 12, c1, fixed_c::pi / 12,
                         shape_flag::kDangerous | shape_flag::kVulnerable);
  add_new_shape<Polygon>(vec2{0}, 115, 12, c1, fixed_c::pi / 12);
  add_new_shape<Polygon>(vec2{0}, 110, 12, c1, fixed_c::pi / 12, shape_flag::kShield);

  auto* s1 = add_new_shape<CompoundShape>(vec2{0}, 0, shape_flag::kDangerous);
  for (std::uint32_t i = 1; i < 12; ++i) {
    auto* s2 = s1->add_new_shape<CompoundShape>(vec2{0}, i * fixed_c::pi / 6);
    s2->add_new_shape<Box>(vec2{130, 0}, 10, 24, c1, 0);
    s2->add_new_shape<Box>(vec2{130, 0}, 8, 22, c0, 0);
  }
  shape().rotate(2 * fixed_c::pi * sim.random_fixed());
}

void DeathRayBoss::update() {
  std::erase_if(arms_, [&](ecs::entity_id id) {
    return !sim().index().contains(id) || sim().index().get(id)->has<Destroy>();
  });
  bool positioned = true;
  fixed d = pos_ == 0 ? 1 * kSimDimensions.y / 4 - shape().centre.y
      : pos_ == 1     ? 2 * kSimDimensions.y / 4 - shape().centre.y
      : pos_ == 2     ? 3 * kSimDimensions.y / 4 - shape().centre.y
      : pos_ == 3     ? 1 * kSimDimensions.y / 8 - shape().centre.y
      : pos_ == 4     ? 3 * kSimDimensions.y / 8 - shape().centre.y
      : pos_ == 5     ? 5 * kSimDimensions.y / 8 - shape().centre.y
                      : 7 * kSimDimensions.y / 8 - shape().centre.y;

  if (abs(d) > 3) {
    move(vec2{0, d / abs(d)} * kDrbSpeed);
    positioned = false;
  }

  if (ray_attack_timer_) {
    ray_attack_timer_--;
    if (ray_attack_timer_ == 40) {
      ray_dest_ = sim().nearest_player_position(shape().centre);
    }
    if (ray_attack_timer_ < 40) {
      auto d = normalise(ray_dest_ - shape().centre);
      spawn_boss_shot(sim(), shape().centre, d * 10, c2);
      play_sound_random(sound::kBossAttack);
      explosion();
    }
  }

  bool going_fast = false;
  if (laser_) {
    if (timer_) {
      if (positioned) {
        timer_--;
      }

      if (timer_ < kDrbTimer * 2 && !(timer_ % 3)) {
        spawn_death_ray(sim(), shape().centre + vec2{100, 0});
        play_sound_random(sound::kBossFire);
      }
      if (!timer_) {
        laser_ = false;
        timer_ = kDrbTimer * (2 + sim().random(3));
      }
    } else {
      fixed r = shape().rotation();
      if (r == 0) {
        timer_ = kDrbTimer * 2;
      }

      if (r < fixed_c::tenth || r > 2 * fixed_c::pi - fixed_c::tenth) {
        shape().set_rotation(0);
      } else {
        going_fast = true;
        shape().rotate(dir_ ? fixed_c::tenth : -fixed_c::tenth);
      }
    }
  } else {
    shape().rotate(dir_ ? fixed_c::hundredth * 2 : -fixed_c::hundredth * 2);
    if (is_on_screen()) {
      timer_--;
      if (timer_ % kDrbTimer == 0 && timer_ && !sim().random(4)) {
        dir_ = sim().random(2) != 0;
        pos_ = sim().random(7);
      }
      if (timer_ == kDrbTimer * 2 + 50 && arms_.size() == 2) {
        ray_attack_timer_ = kDrbRayTimer;
        ray_src1_ = sim().index().get(arms_[0])->get<Transform>()->centre;
        ray_src2_ = sim().index().get(arms_[1])->get<Transform>()->centre;
        play_sound(sound::kEnemySpawn);
      }
    }
    if (!timer_) {
      laser_ = true;
      pos_ = sim().random(7);
    }
  }

  ++shot_timer_;
  bool is_hp_low = handle().get<Health>()->is_hp_low();
  if (arms_.empty() && !is_hp_low) {
    if (shot_timer_ % 8 == 0) {
      shot_queue_.emplace_back((shot_timer_ / 8) % 12, 16);
      play_sound_random(sound::kBossFire);
    }
  }
  if (arms_.empty() && is_hp_low) {
    if (shot_timer_ % 48 == 0) {
      for (std::uint32_t i = 1; i < 16; i += 3) {
        shot_queue_.emplace_back(i, 16);
      }
      play_sound_random(sound::kBossFire);
    }
    if (shot_timer_ % 48 == 16) {
      for (std::uint32_t i = 2; i < 12; i += 3) {
        shot_queue_.emplace_back(i, 16);
      }
      play_sound_random(sound::kBossFire);
    }
    if (shot_timer_ % 48 == 32) {
      for (std::uint32_t i = 3; i < 12; i += 3) {
        shot_queue_.emplace_back(i, 16);
      }
      play_sound_random(sound::kBossFire);
    }
    if (shot_timer_ % 128 == 0) {
      ray_attack_timer_ = kDrbRayTimer;
      vec2 d1 = from_polar(sim().random_fixed() * 2 * fixed_c::pi, 110_fx);
      vec2 d2 = from_polar(sim().random_fixed() * 2 * fixed_c::pi, 110_fx);
      ray_src1_ = shape().centre + d1;
      ray_src2_ = shape().centre + d2;
      play_sound(sound::kEnemySpawn);
    }
  }
  if (arms_.empty()) {
    ++arm_timer_;
    if (arm_timer_ >= kDrbArmRTimer) {
      arm_timer_ = 0;
      if (!is_hp_low) {
        auto players = sim().get_lives() ? sim().player_count() : sim().alive_players();
        std::uint32_t hp =
            (kDrbArmHp * (7 * fixed_c::tenth + 3 * fixed_c::tenth * players)).to_int();
        arms_.push_back(spawn_death_arm(sim(), this, true, hp).id());
        arms_.push_back(spawn_death_arm(sim(), this, false, hp).id());
        play_sound(sound::kEnemySpawn);
      }
    }
  }

  for (std::size_t i = 0; i < shot_queue_.size(); ++i) {
    if (!going_fast || shot_timer_ % 2) {
      auto n = shot_queue_[i].first;
      vec2 d = rotate(vec2{1, 0}, shape().rotation() + n * fixed_c::pi / 6);
      spawn_boss_shot(sim(), shape().centre + d * 120, d * 5, c1);
    }
    shot_queue_[i].second--;
    if (!shot_queue_[i].second) {
      shot_queue_.erase(shot_queue_.begin() + i);
      --i;
    }
  }
}

void DeathRayBoss::render() const {
  Boss::render();
  for (std::uint32_t i = ray_attack_timer_; i <= ray_attack_timer_ + 16; ++i) {
    auto k = i < 8 ? 0 : i - 8;
    if (k < 40 || k > kDrbRayTimer) {
      continue;
    }

    auto pos = to_float(shape().centre);
    auto d = to_float(ray_src1_) - pos;
    d *= static_cast<float>(k - 40) / (kDrbRayTimer - 40);
    Polygon s{vec2{0}, 10, 6, c3, 0, shape_flag::kNone, Polygon::T::kPolystar};
    s.render(sim(), d + pos, 0);

    d = to_float(ray_src2_) - pos;
    d *= static_cast<float>(k - 40) / (kDrbRayTimer - 40);
    Polygon s2{vec2{0}, 10, 6, c3, 0, shape_flag::kNone, Polygon::T::kPolystar};
    s2.render(sim(), d + pos, 0);
  }
}

std::uint32_t transform_death_ray_boss_damage(ecs::handle h, SimInterface& sim, damage_type type,
                                              std::uint32_t damage) {
  // TODO.
  auto d = static_cast<DeathRayBoss*>(h.get<LegacyShip>()->ship.get())->get_damage(damage);
  return scale_boss_damage(h, sim, type, d);
}

}  // namespace

void spawn_death_ray_boss(SimInterface& sim, std::uint32_t cycle) {
  auto h = sim.create_legacy(std::make_unique<DeathRayBoss>(sim));
  h.add(legacy_collision(/* bounding width */ 640));
  h.add(ShipFlags{.flags = ship_flag::kEnemy | ship_flag::kBoss});
  h.add(Enemy{.threat_value = 100,
              .boss_score_reward =
                  calculate_boss_score(boss_flag::kBoss2C, sim.player_count(), cycle)});
  h.add(Health{
      .hp = calculate_boss_hp(kDrbBaseHp, sim.player_count(), cycle),
      .hit_flash_ignore_index = 5,
      .hit_sound0 = std::nullopt,
      .hit_sound1 = sound::kEnemyShatter,
      .destroy_sound = std::nullopt,
      .damage_transform = &transform_death_ray_boss_damage,
      .on_hit = &legacy_boss_on_hit<true>,
      .on_destroy = &legacy_boss_on_destroy,
  });
  h.add(Boss{.boss = boss_flag::kBoss2C});
}
}  // namespace ii