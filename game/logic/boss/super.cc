#include "game/logic/boss/boss_internal.h"
#include "game/logic/enemy/enemy.h"
#include "game/logic/player.h"

namespace ii {
namespace {
constexpr std::uint32_t kSbBaseHp = 520;
constexpr std::uint32_t kSbArcHp = 75;

class SuperBossArc : public ::Boss {
public:
  SuperBossArc(SimInterface& sim, const vec2& position, std::uint32_t i, Ship* boss,
               std::uint32_t timer = 0);

  void update() override;
  void on_destroy(bool bomb) override;
  void render() const override;

  std::uint32_t GetTimer() const {
    return timer_;
  }

private:
  Ship* boss_ = nullptr;
  std::uint32_t i_ = 0;
  std::uint32_t timer_ = 0;
  std::uint32_t stimer_ = 0;
};

SuperBossArc* spawn_super_boss_arc(SimInterface& sim, const vec2& position, std::uint32_t cycle,
                                   std::uint32_t i, Ship* boss, std::uint32_t timer = 0) {
  auto u = std::make_unique<SuperBossArc>(sim, position, i, boss, timer);
  auto p = u.get();
  auto h = sim.create_legacy(std::move(u));
  h.add(legacy_collision(/* bounding width */ 640));
  h.add(Enemy{.threat_value = 10});
  h.add(Health{
      .hp = calculate_boss_hp(kSbArcHp, sim.player_count(), cycle),
      .hit_sound0 = std::nullopt,
      .hit_sound1 = sound::kEnemyShatter,
      .destroy_sound = std::nullopt,
      .damage_transform =
          +[](ecs::handle h, SimInterface& sim, damage_type type, std::uint32_t damage) {
            return scale_boss_damage(h, sim, type,
                                     type == damage_type::kBomb ? damage / 2 : damage);
          },
      .on_hit = &legacy_boss_on_hit<true>,
      .on_destroy = &legacy_boss_on_destroy,
  });
  return p;
}

class SuperBoss : public ::Boss {
public:
  enum class state { kArrive, kIdle, kAttack };

  SuperBoss(SimInterface& sim, std::uint32_t cycle);

  void update() override;
  void on_destroy(bool bomb) override;

private:
  friend class SuperBossArc;

  std::uint32_t cycle_ = 0;
  std::uint32_t ctimer_ = 0;
  std::uint32_t timer_ = 0;
  std::vector<bool> destroyed_;
  std::vector<SuperBossArc*> arcs_;
  state state_ = state::kArrive;
  std::uint32_t snakes_ = 0;
};

SuperBossArc::SuperBossArc(SimInterface& sim, const vec2& position, std::uint32_t i, Ship* boss,
                           std::uint32_t timer)
: Boss{sim, position}, boss_{boss}, i_{i}, timer_{timer} {
  glm::vec4 c{0.f};
  add_new_shape<PolyArc>(vec2{0}, 140, 32, 2, c, i * 2 * fixed_c::pi / 16);
  add_new_shape<PolyArc>(vec2{0}, 135, 32, 2, c, i * 2 * fixed_c::pi / 16);
  add_new_shape<PolyArc>(vec2{0}, 130, 32, 2, c, i * 2 * fixed_c::pi / 16);
  add_new_shape<PolyArc>(vec2{0}, 125, 32, 2, c, i * 2 * fixed_c::pi / 16,
                         shape_flag::kShield | shape_flag::kSafeShield);
  add_new_shape<PolyArc>(vec2{0}, 120, 32, 2, c, i * 2 * fixed_c::pi / 16);
  add_new_shape<PolyArc>(vec2{0}, 115, 32, 2, c, i * 2 * fixed_c::pi / 16);
  add_new_shape<PolyArc>(vec2{0}, 110, 32, 2, c, i * 2 * fixed_c::pi / 16);
  add_new_shape<PolyArc>(vec2{0}, 105, 32, 2, c, i * 2 * fixed_c::pi / 16,
                         shape_flag::kShield | shape_flag::kSafeShield);
}

void SuperBossArc::update() {
  shape().rotate(6_fx / 1000);
  for (std::uint32_t i = 0; i < 8; ++i) {
    shapes()[7 - i]->colour = {((i * 32 + 2 * timer_) % 256) / 256.f, 1.f, .5f,
                               handle().get<Health>()->is_hp_low() ? .6f : 1.f};
  }
  ++timer_;
  ++stimer_;
  if (stimer_ == 64) {
    shapes()[0]->category = shape_flag::kDangerous | shape_flag::kVulnerable;
  }
}

void SuperBossArc::render() const {
  if (stimer_ >= 64 || stimer_ % 4 < 2) {
    Boss::render();
  }
}

void SuperBossArc::on_destroy(bool) {
  vec2 d = from_polar(i_ * 2 * fixed_c::pi / 16 + shape().rotation(), 120_fx);
  move(d);
  explosion();
  explosion(glm::vec4{1.f}, 12);
  explosion(shapes()[0]->colour, 24);
  explosion(glm::vec4{1.f}, 36);
  explosion(shapes()[0]->colour, 48);
  play_sound_random(sound::kExplosion);
  ((SuperBoss*)boss_)->destroyed_[i_] = true;
}

SuperBoss::SuperBoss(SimInterface& sim, std::uint32_t cycle)
: Boss{sim, {kSimDimensions.x / 2, -kSimDimensions.y / (2 + fixed_c::half)}}, cycle_{cycle} {
  add_new_shape<Polygon>(vec2{0}, 40, 32, glm::vec4{0.f}, 0,
                         shape_flag::kDangerous | shape_flag::kVulnerable);
  add_new_shape<Polygon>(vec2{0}, 35, 32, glm::vec4{0.f}, 0);
  add_new_shape<Polygon>(vec2{0}, 30, 32, glm::vec4{0.f}, 0, shape_flag::kShield);
  add_new_shape<Polygon>(vec2{0}, 25, 32, glm::vec4{0.f}, 0);
  add_new_shape<Polygon>(vec2{0}, 20, 32, glm::vec4{0.f}, 0);
  add_new_shape<Polygon>(vec2{0}, 15, 32, glm::vec4{0.f}, 0);
  add_new_shape<Polygon>(vec2{0}, 10, 32, glm::vec4{0.f}, 0);
  add_new_shape<Polygon>(vec2{0}, 5, 32, glm::vec4{0.f}, 0);
  for (std::uint32_t i = 0; i < 16; ++i) {
    destroyed_.push_back(false);
  }
}

void SuperBoss::update() {
  if (arcs_.empty()) {
    for (std::uint32_t i = 0; i < 16; ++i) {
      arcs_.push_back(spawn_super_boss_arc(sim(), shape().centre, cycle_, i, this));
    }
  } else {
    for (std::uint32_t i = 0; i < 16; ++i) {
      if (destroyed_[i]) {
        continue;
      }
      arcs_[i]->shape().centre = shape().centre;
    }
  }
  vec2 move_vec{0};
  shape().rotate(6_fx / 1000);
  auto c = colour_hue((ctimer_ % 128) / 128.f);
  for (std::uint32_t i = 0; i < 8; ++i) {
    shapes()[7 - i]->colour = colour_hue(((i * 32 + 2 * ctimer_) % 256) / 256.f);
  }
  ++ctimer_;
  if (shape().centre.y < kSimDimensions.y / 2) {
    move_vec = {0, 1};
  } else if (state_ == state::kArrive) {
    state_ = state::kIdle;
  }

  ++timer_;
  if (state_ == state::kAttack && timer_ == 192) {
    timer_ = 100;
    state_ = state::kIdle;
  }

  static const fixed d5d1000 = 5_fx / 1000;
  static const fixed pi2d64 = 2 * fixed_c::pi / 64;
  static const fixed pi2d32 = 2 * fixed_c::pi / 32;

  if (state_ == state::kIdle && is_on_screen() && timer_ && timer_ % 300 == 0) {
    auto r = sim().random(6);
    if (r == 0 || r == 1) {
      snakes_ = 16;
    } else if (r == 2 || r == 3) {
      state_ = state::kAttack;
      timer_ = 0;
      fixed f = sim().random_fixed() * (2 * fixed_c::pi);
      fixed rf = d5d1000 * (1 + sim().random(2));
      for (std::uint32_t i = 0; i < 32; ++i) {
        vec2 d = from_polar(f + i * pi2d32, 1_fx);
        if (r == 2) {
          rf = d5d1000 * (1 + sim().random(4));
        }
        spawn_snake(sim(), shape().centre + d * 16, c, d, rf);
        play_sound_random(sound::kBossAttack);
      }
    } else if (r == 5) {
      state_ = state::kAttack;
      timer_ = 0;
      fixed f = sim().random_fixed() * (2 * fixed_c::pi);
      for (std::uint32_t i = 0; i < 64; ++i) {
        vec2 d = from_polar(f + i * pi2d64, 1_fx);
        spawn_snake(sim(), shape().centre + d * 16, c, d);
        play_sound_random(sound::kBossAttack);
      }
    } else {
      state_ = state::kAttack;
      timer_ = 0;
      snakes_ = 32;
    }
  }

  if (state_ == state::kIdle && is_on_screen() && timer_ % 300 == 200 && !is_destroyed()) {
    std::vector<std::uint32_t> wide3;
    std::uint32_t timer = 0;
    for (std::uint32_t i = 0; i < 16; ++i) {
      if (destroyed_[(i + 15) % 16] && destroyed_[i] && destroyed_[(i + 1) % 16]) {
        wide3.push_back(i);
      }
      if (!destroyed_[i]) {
        timer = arcs_[i]->GetTimer();
      }
    }
    if (!wide3.empty()) {
      auto r = sim().random(wide3.size());
      auto* s = spawn_super_boss_arc(sim(), shape().centre, cycle_, wide3[r], this, timer);
      s->shape().set_rotation(shape().rotation() - (6_fx / 1000));
      arcs_[wide3[r]] = s;
      destroyed_[wide3[r]] = false;
      play_sound(sound::kEnemySpawn);
    }
  }
  static const fixed pi2d128 = 2 * fixed_c::pi / 128;
  if (state_ == state::kIdle && timer_ % 72 == 0) {
    for (std::uint32_t i = 0; i < 128; ++i) {
      vec2 d = from_polar(i * pi2d128, 1_fx);
      spawn_boss_shot(sim(), shape().centre + d * 42, move_vec + d * 3,
                      glm::vec4{0.f, 0.f, .6f, 1.f}, /* rotate speed */ 6_fx / 1000);
      play_sound_random(sound::kBossFire);
    }
  }

  if (snakes_) {
    --snakes_;
    vec2 d = from_polar(sim().random_fixed() * (2 * fixed_c::pi),
                        fixed{sim().random(32) + sim().random(16)});
    spawn_snake(sim(), d + shape().centre, c);
    play_sound_random(sound::kEnemySpawn);
  }
  move(move_vec);
}

void SuperBoss::on_destroy(bool) {
  for (const auto& ship : sim().all_ships(ship_flag::kEnemy)) {
    if (ship != this) {
      ship->damage(::Player::kBombDamage * 100, false, 0);
    }
  }
  explosion();
  explosion(glm::vec4{1.f}, 12);
  explosion(shapes()[0]->colour, 24);
  explosion(glm::vec4{1.f}, 36);
  explosion(shapes()[0]->colour, 48);

  std::uint32_t n = 1;
  for (std::uint32_t i = 0; i < 16; ++i) {
    vec2 v = from_polar(sim().random_fixed() * (2 * fixed_c::pi),
                        fixed{8 + sim().random(64) + sim().random(64)});
    auto c = shapes()[0]->colour;
    c.x += n / 128.f;
    fireworks_.push_back(std::make_pair(n, std::make_pair(shape().centre + v, c)));
    n += i;
  }
  sim().rumble_all(25);
  play_sound(sound::kExplosion);

  for (std::uint32_t i = 0; i < 8; ++i) {
    spawn_powerup(sim(), shape().centre, powerup_type::kBomb);
  }
}

}  // namespace

void spawn_super_boss(SimInterface& sim, std::uint32_t cycle) {
  auto h = sim.create_legacy(std::make_unique<SuperBoss>(sim, cycle));
  h.add(legacy_collision(/* bounding width */ 640));
  h.add(Enemy{.threat_value = 100,
              .boss_score_reward =
                  calculate_boss_score(boss_flag::kBoss3A, sim.player_count(), cycle)});
  h.add(Health{
      .hp = calculate_boss_hp(kSbBaseHp, sim.player_count(), cycle),
      .hit_flash_ignore_index = 8,
      .hit_sound0 = std::nullopt,
      .hit_sound1 = sound::kEnemyShatter,
      .destroy_sound = std::nullopt,
      .damage_transform = &scale_boss_damage,
      .on_hit = &legacy_boss_on_hit<true>,
      .on_destroy = &legacy_boss_on_destroy,
  });
  h.add(Boss{.boss = boss_flag::kBoss3A});
}
}  // namespace ii