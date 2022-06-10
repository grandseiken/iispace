#include "game/logic/boss/boss_internal.h"
#include "game/logic/enemy.h"
#include "game/logic/player.h"

namespace {
const std::uint32_t kSbBaseHp = 520;
const std::uint32_t kSbArcHp = 75;

class SnakeTail : public Enemy {
  friend class Snake;

public:
  SnakeTail(ii::SimInterface& sim, const vec2& position, const glm::vec4& colour);
  void update() override;
  void on_destroy(bool bomb) override;

private:
  SnakeTail* tail_ = nullptr;
  SnakeTail* head_ = nullptr;
  std::uint32_t timer_ = 150;
  std::uint32_t dtimer_ = 0;
};

class Snake : public Enemy {
public:
  Snake(ii::SimInterface& sim, const vec2& position, const glm::vec4& colour,
        const vec2& dir = vec2{0}, fixed rot = 0);
  void update() override;
  void on_destroy(bool bomb) override;

private:
  SnakeTail* tail_ = nullptr;
  std::uint32_t timer_ = 0;
  vec2 dir_{0};
  std::uint32_t count_ = 0;
  glm::vec4 colour_{0.f};
  bool shot_snake_ = false;
  fixed shot_rot_ = 0;
};

SnakeTail* spawn_snake_tail(ii::SimInterface& sim, const vec2& position, const glm::vec4& colour) {
  auto u = std::make_unique<SnakeTail>(sim, position, colour);
  auto p = u.get();
  auto h = sim.create_legacy(std::move(u));
  h.add(ii::legacy_collision(/* bounding width */ 22));
  h.add(ii::Enemy{.threat_value = 1});
  h.add(ii::Health{.hp = 1, .on_destroy = &ii::legacy_enemy_on_destroy});
  return p;
}

void spawn_snake(ii::SimInterface& sim, const vec2& position, const glm::vec4& colour,
                 const vec2& dir = vec2{0}, fixed rot = 0) {
  auto h = sim.create_legacy(std::make_unique<Snake>(sim, position, colour, dir, rot));
  h.add(ii::legacy_collision(/* bounding width */ 32));
  h.add(ii::Enemy{.threat_value = 5});
  h.add(ii::Health{.hp = 5,
                   .destroy_sound = ii::sound::kPlayerDestroy,
                   .on_destroy = &ii::legacy_enemy_on_destroy});
}

SnakeTail::SnakeTail(ii::SimInterface& sim, const vec2& position, const glm::vec4& colour)
: Enemy{sim, position, ii::ship_flag::kNone} {
  add_new_shape<ii::Polygon>(
      vec2{0}, 10, 4, colour, 0,
      ii::shape_flag::kDangerous | ii::shape_flag::kShield | ii::shape_flag::kVulnShield);
}

void SnakeTail::update() {
  static const fixed z15 = fixed_c::hundredth * 15;
  shape().rotate(z15);
  if (!--timer_) {
    on_destroy(false);
    destroy();
    explosion();
  }
  if (dtimer_ && !--dtimer_) {
    if (tail_) {
      tail_->dtimer_ = 4;
    }
    on_destroy(false);
    destroy();
    explosion();
    play_sound_random(ii::sound::kEnemyDestroy);
  }
}

void SnakeTail::on_destroy(bool bomb) {
  if (head_) {
    head_->tail_ = 0;
  }
  if (tail_) {
    tail_->head_ = 0;
  }
  head_ = 0;
  tail_ = 0;
}

Snake::Snake(ii::SimInterface& sim, const vec2& position, const glm::vec4& colour, const vec2& dir,
             fixed rot)
: Enemy{sim, position, ii::ship_flag::kNone}, colour_{colour}, shot_rot_{rot} {
  add_new_shape<ii::Polygon>(vec2{0}, 14, 3, colour, 0, ii::shape_flag::kVulnerable);
  add_new_shape<ii::Polygon>(vec2{0}, 10, 3, glm::vec4{0.f}, 0, ii::shape_flag::kDangerous);
  if (dir == vec2{0}) {
    auto r = sim.random(4);
    dir_ = r == 0 ? vec2{1, 0} : r == 1 ? vec2{-1, 0} : r == 2 ? vec2{0, 1} : vec2{0, -1};
  } else {
    dir_ = normalise(dir);
    shot_snake_ = true;
  }
  shape().set_rotation(angle(dir_));
}

void Snake::update() {
  if (!(shape().centre.x >= -8 && shape().centre.x <= ii::kSimDimensions.x + 8 &&
        shape().centre.y >= -8 && shape().centre.y <= ii::kSimDimensions.y + 8)) {
    tail_ = 0;
    destroy();
    return;
  }

  auto c = colour_;
  c.x += (timer_ % 256) / 256.f;
  shapes()[0]->colour = c;
  ++timer_;
  if (timer_ % (shot_snake_ ? 4 : 8) == 0) {
    auto c_dark = c;
    c_dark.a = .6f;
    auto* t = spawn_snake_tail(sim(), shape().centre, c_dark);
    if (tail_ != 0) {
      tail_->head_ = t;
      t->tail_ = tail_;
    }
    play_sound_random(ii::sound::kBossFire, 0, .5f);
    tail_ = t;
  }
  if (!shot_snake_ && timer_ % 48 == 0 && !sim().random(3)) {
    dir_ = rotate(dir_, (sim().random(2) ? 1 : -1) * fixed_c::pi / 2);
    shape().set_rotation(angle(dir_));
  }
  move(dir_ * (shot_snake_ ? 4 : 2));
  if (timer_ % 8 == 0) {
    dir_ = rotate(dir_, 8 * shot_rot_);
    shape().set_rotation(angle(dir_));
  }
}

void Snake::on_destroy(bool bomb) {
  if (tail_) {
    tail_->dtimer_ = 4;
  }
}

class RainbowShot : public BossShot {
public:
  RainbowShot(ii::SimInterface& sim, const vec2& position, const vec2& velocity, ii::Ship* boss);
  void update() override;

private:
  ii::Ship* boss_ = nullptr;
  std::uint32_t timer_ = 0;
};

class SuperBossArc : public Boss {
public:
  SuperBossArc(ii::SimInterface& sim, const vec2& position, std::uint32_t i, ii::Ship* boss,
               std::uint32_t timer = 0);

  void update() override;
  void on_destroy(bool bomb) override;
  void render() const override;

  std::uint32_t GetTimer() const {
    return timer_;
  }

private:
  ii::Ship* boss_ = nullptr;
  std::uint32_t i_ = 0;
  std::uint32_t timer_ = 0;
  std::uint32_t stimer_ = 0;
};

void spawn_rainbow_shot(ii::SimInterface& sim, const vec2& position, const vec2& velocity,
                        ii::Ship* boss) {
  auto h = sim.create_legacy(std::make_unique<RainbowShot>(sim, position, velocity, boss));
  h.add(ii::legacy_collision(/* bounding width */ 12));
  h.add(ii::Enemy{.threat_value = 1});
  h.add(ii::Health{.hp = 0, .on_destroy = &ii::legacy_enemy_on_destroy});
}

SuperBossArc* spawn_super_boss_arc(ii::SimInterface& sim, const vec2& position, std::uint32_t cycle,
                                   std::uint32_t i, ii::Ship* boss, std::uint32_t timer = 0) {
  auto u = std::make_unique<SuperBossArc>(sim, position, i, boss, timer);
  auto p = u.get();
  auto h = sim.create_legacy(std::move(u));
  h.add(ii::legacy_collision(/* bounding width */ 640));
  h.add(ii::Enemy{.threat_value = 10});
  h.add(ii::Health{
      .hp = ii::calculate_boss_hp(kSbArcHp, sim.player_count(), cycle),
      .hit_sound0 = std::nullopt,
      .hit_sound1 = ii::sound::kEnemyShatter,
      .destroy_sound = std::nullopt,
      .damage_transform =
          +[](ii::SimInterface& sim, ii::ecs::handle h, ii::damage_type type,
              std::uint32_t damage) {
            return ii::scale_boss_damage(sim, h, type,
                                         type == ii::damage_type::kBomb ? damage / 2 : damage);
          },
      .on_hit = ii::make_legacy_boss_on_hit(true),
      .on_destroy = &ii::legacy_boss_on_destroy,
  });
  return p;
}

class SuperBoss : public Boss {
public:
  enum class state { kArrive, kIdle, kAttack };

  SuperBoss(ii::SimInterface& sim, std::uint32_t cycle);

  void update() override;
  void on_destroy(bool bomb) override;

private:
  friend class SuperBossArc;
  friend class RainbowShot;

  std::uint32_t cycle_ = 0;
  std::uint32_t ctimer_ = 0;
  std::uint32_t timer_ = 0;
  std::vector<bool> destroyed_;
  std::vector<SuperBossArc*> arcs_;
  state state_ = state::kArrive;
  std::uint32_t snakes_ = 0;
};

RainbowShot::RainbowShot(ii::SimInterface& sim, const vec2& position, const vec2& velocity,
                         ii::Ship* boss)
: BossShot{sim, position, velocity}, boss_{boss} {}

void RainbowShot::update() {
  BossShot::update();
  static const vec2 center = {ii::kSimDimensions.x / 2, ii::kSimDimensions.y / 2};

  if (length(shape().centre - center) > 100 && timer_ % 2 == 0) {
    const auto& list = sim().collision_list(shape().centre, ii::shape_flag::kShield);
    SuperBoss* s = (SuperBoss*)boss_;
    for (std::size_t i = 0; i < list.size(); ++i) {
      bool boss = false;
      for (std::size_t j = 0; j < s->arcs_.size(); ++j) {
        if (list[i] == s->arcs_[j]) {
          boss = true;
          break;
        }
      }
      if (!boss) {
        continue;
      }

      explosion(std::nullopt, 4, true, to_float(shape().centre - dir_));
      destroy();
      return;
    }
  }

  ++timer_;
  static const fixed r = 6 * (8 * fixed_c::hundredth / 10);
  if (timer_ % 8 == 0) {
    dir_ = rotate(dir_, r);
  }
}

SuperBossArc::SuperBossArc(ii::SimInterface& sim, const vec2& position, std::uint32_t i,
                           ii::Ship* boss, std::uint32_t timer)
: Boss{sim, position}, boss_{boss}, i_{i}, timer_{timer} {
  glm::vec4 c{0.f};
  add_new_shape<ii::PolyArc>(vec2{0}, 140, 32, 2, c, i * 2 * fixed_c::pi / 16);
  add_new_shape<ii::PolyArc>(vec2{0}, 135, 32, 2, c, i * 2 * fixed_c::pi / 16);
  add_new_shape<ii::PolyArc>(vec2{0}, 130, 32, 2, c, i * 2 * fixed_c::pi / 16);
  add_new_shape<ii::PolyArc>(vec2{0}, 125, 32, 2, c, i * 2 * fixed_c::pi / 16,
                             ii::shape_flag::kShield);
  add_new_shape<ii::PolyArc>(vec2{0}, 120, 32, 2, c, i * 2 * fixed_c::pi / 16);
  add_new_shape<ii::PolyArc>(vec2{0}, 115, 32, 2, c, i * 2 * fixed_c::pi / 16);
  add_new_shape<ii::PolyArc>(vec2{0}, 110, 32, 2, c, i * 2 * fixed_c::pi / 16);
  add_new_shape<ii::PolyArc>(vec2{0}, 105, 32, 2, c, i * 2 * fixed_c::pi / 16,
                             ii::shape_flag::kShield);
}

void SuperBossArc::update() {
  shape().rotate(6_fx / 1000);
  for (std::uint32_t i = 0; i < 8; ++i) {
    shapes()[7 - i]->colour = {((i * 32 + 2 * timer_) % 256) / 256.f, 1.f, .5f,
                               handle().get<ii::Health>()->is_hp_low() ? .6f : 1.f};
  }
  ++timer_;
  ++stimer_;
  if (stimer_ == 64) {
    shapes()[0]->category = ii::shape_flag::kDangerous | ii::shape_flag::kVulnerable;
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
  play_sound_random(ii::sound::kExplosion);
  ((SuperBoss*)boss_)->destroyed_[i_] = true;
}

SuperBoss::SuperBoss(ii::SimInterface& sim, std::uint32_t cycle)
: Boss{sim, {ii::kSimDimensions.x / 2, -ii::kSimDimensions.y / (2 + fixed_c::half)}}
, cycle_{cycle} {
  add_new_shape<ii::Polygon>(vec2{0}, 40, 32, glm::vec4{0.f}, 0,
                             ii::shape_flag::kDangerous | ii::shape_flag::kVulnerable);
  add_new_shape<ii::Polygon>(vec2{0}, 35, 32, glm::vec4{0.f}, 0);
  add_new_shape<ii::Polygon>(vec2{0}, 30, 32, glm::vec4{0.f}, 0, ii::shape_flag::kShield);
  add_new_shape<ii::Polygon>(vec2{0}, 25, 32, glm::vec4{0.f}, 0);
  add_new_shape<ii::Polygon>(vec2{0}, 20, 32, glm::vec4{0.f}, 0);
  add_new_shape<ii::Polygon>(vec2{0}, 15, 32, glm::vec4{0.f}, 0);
  add_new_shape<ii::Polygon>(vec2{0}, 10, 32, glm::vec4{0.f}, 0);
  add_new_shape<ii::Polygon>(vec2{0}, 5, 32, glm::vec4{0.f}, 0);
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
  if (shape().centre.y < ii::kSimDimensions.y / 2) {
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
        play_sound_random(ii::sound::kBossAttack);
      }
    } else if (r == 5) {
      state_ = state::kAttack;
      timer_ = 0;
      fixed f = sim().random_fixed() * (2 * fixed_c::pi);
      for (std::uint32_t i = 0; i < 64; ++i) {
        vec2 d = from_polar(f + i * pi2d64, 1_fx);
        spawn_snake(sim(), shape().centre + d * 16, c, d);
        play_sound_random(ii::sound::kBossAttack);
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
      play_sound(ii::sound::kEnemySpawn);
    }
  }
  static const fixed pi2d128 = 2 * fixed_c::pi / 128;
  if (state_ == state::kIdle && timer_ % 72 == 0) {
    for (std::uint32_t i = 0; i < 128; ++i) {
      vec2 d = from_polar(i * pi2d128, 1_fx);
      spawn_rainbow_shot(sim(), shape().centre + d * 42, move_vec + d * 3, this);
      play_sound_random(ii::sound::kBossFire);
    }
  }

  if (snakes_) {
    --snakes_;
    vec2 d = from_polar(sim().random_fixed() * (2 * fixed_c::pi),
                        fixed{sim().random(32) + sim().random(16)});
    spawn_snake(sim(), d + shape().centre, c);
    play_sound_random(ii::sound::kEnemySpawn);
  }
  move(move_vec);
}

void SuperBoss::on_destroy(bool) {
  for (const auto& ship : sim().all_ships(ii::ship_flag::kEnemy)) {
    if (ship != this) {
      ship->damage(Player::kBombDamage * 100, false, 0);
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
  play_sound(ii::sound::kExplosion);

  for (std::uint32_t i = 0; i < 8; ++i) {
    ii::spawn_powerup(sim(), shape().centre, ii::powerup_type::kBomb);
  }
}

}  // namespace

namespace ii {
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
      .hit_sound1 = ii::sound::kEnemyShatter,
      .destroy_sound = std::nullopt,
      .damage_transform = &scale_boss_damage,
      .on_hit = make_legacy_boss_on_hit(true),
      .on_destroy = &legacy_boss_on_destroy,
  });
  h.add(Boss{.boss = boss_flag::kBoss3A});
}
}  // namespace ii