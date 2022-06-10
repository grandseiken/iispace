#include "game/logic/enemy.h"
#include "game/logic/player.h"
#include <algorithm>

namespace {
const std::uint32_t kFollowTime = 90;
const fixed kFollowSpeed = 2;

const std::uint32_t kChaserTime = 60;
const fixed kChaserSpeed = 4;

const fixed kSquareSpeed = 2 + 1_fx / 4;

const std::uint32_t kWallTimer = 80;
const fixed kWallSpeed = 1 + 1_fx / 4;

const std::uint32_t kFollowHubTimer = 170;
const fixed kFollowHubSpeed = 1;

const std::uint32_t kShielderTimer = 80;
const fixed kShielderSpeed = 2;

const std::uint32_t kTractorTimer = 50;
const fixed kTractorSpeed = 6 * (1_fx / 10);
const fixed kTractorBeamSpeed = 2 + 1_fx / 2;

class Follow : public Enemy {
public:
  Follow(ii::SimInterface& sim, const vec2& position, fixed rotation, fixed radius = 10);
  void update() override;

private:
  std::uint32_t timer_ = 0;
  Ship* target_ = nullptr;
};

class BigFollow : public Follow {
public:
  BigFollow(ii::SimInterface& sim, const vec2& position, bool has_score);
  void on_destroy(bool bomb) override;

private:
  bool has_score_ = 0;
};

class Chaser : public Enemy {
public:
  Chaser(ii::SimInterface& sim, const vec2& position);
  void update() override;

private:
  bool move_ = false;
  std::uint32_t timer_ = 0;
  vec2 dir_{0};
};

class Square : public Enemy {
public:
  Square(ii::SimInterface& sim, const vec2& position, fixed rotation = fixed_c::pi / 2);
  void update() override;
  void render() const override;

private:
  vec2 dir_{0};
  std::uint32_t timer_ = 0;
};

class Wall : public Enemy {
public:
  Wall(ii::SimInterface& sim, const vec2& position, bool rdir);
  void update() override;
  void on_destroy(bool bomb) override;

private:
  vec2 dir_{0};
  std::uint32_t timer_ = 0;
  bool rotate_ = false;
  bool rdir_ = false;
};

class FollowHub : public Enemy {
public:
  FollowHub(ii::SimInterface& sim, const vec2& position, bool powera = false, bool powerb = false);
  void update() override;
  void on_destroy(bool bomb) override;

private:
  std::uint32_t timer_ = 0;
  vec2 dir_{0};
  std::uint32_t count_ = 0;
  bool powera_ = false;
  bool powerb_ = false;
};

class Shielder : public Enemy {
public:
  Shielder(ii::SimInterface& sim, const vec2& position, bool power = false);
  void update() override;

private:
  vec2 dir_{0};
  std::uint32_t timer_ = 0;
  bool rotate_ = false;
  bool rdir_ = false;
  bool power_ = false;
};

class Tractor : public Enemy {
public:
  Tractor(ii::SimInterface& sim, const vec2& position, bool power = false);
  void update() override;
  void render() const override;

private:
  std::uint32_t timer_ = 0;
  vec2 dir_{0};
  bool power_ = false;

  bool ready_ = false;
  bool spinning_ = false;
  ii::SimInterface::ship_list players_;
};

Follow::Follow(ii::SimInterface& sim, const vec2& position, fixed rotation, fixed radius)
: Enemy{sim, position, ii::ship_flag::kNone} {
  add_new_shape<ii::Polygon>(vec2{0}, radius, 4, colour_hue360(270, .6f), rotation,
                             ii::shape_flag::kDangerous | ii::shape_flag::kVulnerable);
}

void Follow::update() {
  shape().rotate(fixed_c::tenth);
  if (!sim().alive_players()) {
    return;
  }

  ++timer_;
  if (!target_ || timer_ > kFollowTime) {
    target_ = sim().nearest_player(shape().centre);
    timer_ = 0;
  }
  auto d = target_->shape().centre - shape().centre;
  move(normalise(d) * kFollowSpeed);
}

BigFollow::BigFollow(ii::SimInterface& sim, const vec2& position, bool has_score)
: Follow{sim, position, 0, 20}, has_score_{has_score} {}

void BigFollow::on_destroy(bool bomb) {
  if (bomb) {
    return;
  }

  vec2 d = rotate(vec2{10, 0}, shape().rotation());
  for (std::uint32_t i = 0; i < 3; ++i) {
    ii::spawn_follow(sim(), shape().centre + d, has_score_);
    d = rotate(d, 2 * fixed_c::pi / 3);
  }
}

Chaser::Chaser(ii::SimInterface& sim, const vec2& position)
: Enemy{sim, position, ii::ship_flag::kNone}, timer_{kChaserTime} {
  add_new_shape<ii::Polygon>(vec2{0}, 10, 4, colour_hue360(210, .6f), 0,
                             ii::shape_flag::kDangerous | ii::shape_flag::kVulnerable,
                             ii::Polygon::T::kPolygram);
}

void Chaser::update() {
  bool before = is_on_screen();

  if (timer_) {
    timer_--;
  }
  if (!timer_) {
    timer_ = kChaserTime * (move_ + 1);
    move_ = !move_;
    if (move_) {
      dir_ = kChaserSpeed * sim().nearest_player_direction(shape().centre);
    }
  }
  if (move_) {
    move(dir_);
    if (!before && is_on_screen()) {
      move_ = false;
    }
  } else {
    shape().rotate(fixed_c::tenth);
  }
}

Square::Square(ii::SimInterface& sim, const vec2& position, fixed rotation)
: Enemy{sim, position, ii::ship_flag::kWall}, timer_{sim.random(80) + 40} {
  add_new_shape<ii::Box>(vec2{0}, 10, 10, colour_hue360(120, .6f), 0,
                         ii::shape_flag::kDangerous | ii::shape_flag::kVulnerable);
  dir_ = from_polar(rotation, 1_fx);
}

void Square::update() {
  if (is_on_screen() && sim().get_non_wall_count() == 0) {
    if (timer_) {
      timer_--;
    }
  } else {
    timer_ = sim().random(80) + 40;
  }

  if (!timer_) {
    damage(4, false, 0);
  }

  const vec2& pos = shape().centre;
  if (pos.x < 0 && dir_.x <= 0) {
    dir_.x = -dir_.x;
    if (dir_.x <= 0) {
      dir_.x = 1;
    }
  }
  if (pos.y < 0 && dir_.y <= 0) {
    dir_.y = -dir_.y;
    if (dir_.y <= 0) {
      dir_.y = 1;
    }
  }

  if (pos.x > ii::kSimDimensions.x && dir_.x >= 0) {
    dir_.x = -dir_.x;
    if (dir_.x >= 0) {
      dir_.x = -1;
    }
  }
  if (pos.y > ii::kSimDimensions.y && dir_.y >= 0) {
    dir_.y = -dir_.y;
    if (dir_.y >= 0) {
      dir_.y = -1;
    }
  }
  dir_ = normalise(dir_);
  move(dir_ * kSquareSpeed);
  shape().set_rotation(angle(dir_));
}

void Square::render() const {
  if (sim().get_non_wall_count() == 0 && (timer_ % 4 == 1 || timer_ % 4 == 2)) {
    render_with_colour(colour_hue(0.f, .2f, 0.f));
  } else {
    Enemy::render();
  }
}

Wall::Wall(ii::SimInterface& sim, const vec2& position, bool rdir)
: Enemy{sim, position, ii::ship_flag::kWall}, dir_{0, 1}, rdir_{rdir} {
  add_new_shape<ii::Box>(vec2{0}, 10, 40, colour_hue360(120, .5f, .6f), 0,
                         ii::shape_flag::kDangerous | ii::shape_flag::kVulnerable);
}

void Wall::update() {
  auto count = sim().get_non_wall_count();
  if (!count && timer_ % 8 < 2) {
    auto hp = handle().get<ii::Health>()->hp;
    if (hp > 2) {
      sim().play_sound(ii::sound::kEnemySpawn, 1.f, 0.f);
    }
    damage(std::max(2u, hp) - 2, false, 0);
  }

  if (rotate_) {
    auto d =
        rotate(dir_, (rdir_ ? -1 : 1) * (kWallTimer - timer_) * fixed_c::pi / (4 * kWallTimer));

    shape().set_rotation(angle(d));
    if (!--timer_) {
      rotate_ = false;
      dir_ = rotate(dir_, rdir_ ? -fixed_c::pi / 4 : fixed_c::pi / 4);
    }
    return;
  } else {
    if (++timer_ > kWallTimer * 6) {
      if (is_on_screen()) {
        timer_ = kWallTimer;
        rotate_ = true;
      } else {
        timer_ = 0;
      }
    }
  }

  vec2 pos = shape().centre;
  if ((pos.x < 0 && dir_.x < -fixed_c::hundredth) || (pos.y < 0 && dir_.y < -fixed_c::hundredth) ||
      (pos.x > ii::kSimDimensions.x && dir_.x > fixed_c::hundredth) ||
      (pos.y > ii::kSimDimensions.y && dir_.y > fixed_c::hundredth)) {
    dir_ = -normalise(dir_);
  }

  move(dir_ * kWallSpeed);
  shape().set_rotation(angle(dir_));
}

void Wall::on_destroy(bool bomb) {
  if (bomb) {
    return;
  }
  auto d = rotate(dir_, fixed_c::pi / 2);
  auto v = shape().centre + d * 10 * 3;
  if (v.x >= 0 && v.x <= ii::kSimDimensions.x && v.y >= 0 && v.y <= ii::kSimDimensions.y) {
    ii::spawn_square(sim(), v, shape().rotation());
  }

  v = shape().centre - d * 10 * 3;
  if (v.x >= 0 && v.x <= ii::kSimDimensions.x && v.y >= 0 && v.y <= ii::kSimDimensions.y) {
    ii::spawn_square(sim(), v, shape().rotation());
  }
}

FollowHub::FollowHub(ii::SimInterface& sim, const vec2& position, bool powera, bool powerb)
: Enemy{sim, position, ii::ship_flag::kNone}, powera_{powera}, powerb_{powerb} {
  auto c = colour_hue360(240, .7f);
  add_new_shape<ii::Polygon>(vec2{0}, 16, 4, c, fixed_c::pi / 4,
                             ii::shape_flag::kDangerous | ii::shape_flag::kVulnerable,
                             ii::Polygon::T::kPolygram);
  if (powerb_) {
    add_new_shape<ii::Polygon>(vec2{16, 0}, 8, 4, c, fixed_c::pi / 4, ii::shape_flag::kNone,
                               ii::Polygon::T::kPolystar);
    add_new_shape<ii::Polygon>(vec2{-16, 0}, 8, 4, c, fixed_c::pi / 4, ii::shape_flag::kNone,
                               ii::Polygon::T::kPolystar);
    add_new_shape<ii::Polygon>(vec2{0, 16}, 8, 4, c, fixed_c::pi / 4, ii::shape_flag::kNone,
                               ii::Polygon::T::kPolystar);
    add_new_shape<ii::Polygon>(vec2{0, -16}, 8, 4, c, fixed_c::pi / 4, ii::shape_flag::kNone,
                               ii::Polygon::T::kPolystar);
  }

  add_new_shape<ii::Polygon>(vec2{16, 0}, 8, 4, c, fixed_c::pi / 4);
  add_new_shape<ii::Polygon>(vec2{-16, 0}, 8, 4, c, fixed_c::pi / 4);
  add_new_shape<ii::Polygon>(vec2{0, 16}, 8, 4, c, fixed_c::pi / 4);
  add_new_shape<ii::Polygon>(vec2{0, -16}, 8, 4, c, fixed_c::pi / 4);
}

void FollowHub::update() {
  ++timer_;
  if (timer_ > (powera_ ? kFollowHubTimer / 2 : kFollowHubTimer)) {
    timer_ = 0;
    ++count_;
    if (is_on_screen()) {
      if (powerb_) {
        ii::spawn_chaser(sim(), shape().centre);
      } else {
        ii::spawn_follow(sim(), shape().centre);
      }
      play_sound_random(ii::sound::kEnemySpawn);
    }
  }

  dir_ = shape().centre.x < 0                   ? vec2{1, 0}
      : shape().centre.x > ii::kSimDimensions.x ? vec2{-1, 0}
      : shape().centre.y < 0                    ? vec2{0, 1}
      : shape().centre.y > ii::kSimDimensions.y ? vec2{0, -1}
      : count_ > 3                              ? (count_ = 0, rotate(dir_, -fixed_c::pi / 2))
                                                : dir_;

  fixed s = powera_ ? fixed_c::hundredth * 5 + fixed_c::tenth : fixed_c::hundredth * 5;
  shape().rotate(s);
  shapes()[0]->rotate(-s);

  move(dir_ * kFollowHubSpeed);
}

void FollowHub::on_destroy(bool bomb) {
  if (bomb) {
    return;
  }
  if (powerb_) {
    ii::spawn_big_follow(sim(), shape().centre, true);
  }
  ii::spawn_chaser(sim(), shape().centre);
}

Shielder::Shielder(ii::SimInterface& sim, const vec2& position, bool power)
: Enemy{sim, position, ii::ship_flag::kNone}, dir_{0, 1}, power_{power} {
  auto c0 = colour_hue360(150, .2f);
  auto c1 = colour_hue360(160, .5f, .6f);
  add_new_shape<ii::Polygon>(vec2{24, 0}, 8, 6, c0, 0, ii::shape_flag::kVulnShield,
                             ii::Polygon::T::kPolystar);
  add_new_shape<ii::Polygon>(vec2{-24, 0}, 8, 6, c0, 0, ii::shape_flag::kVulnShield,
                             ii::Polygon::T::kPolystar);
  add_new_shape<ii::Polygon>(vec2{0, 24}, 8, 6, c0, fixed_c::pi / 2, ii::shape_flag::kVulnShield,
                             ii::Polygon::T::kPolystar);
  add_new_shape<ii::Polygon>(vec2{0, -24}, 8, 6, c0, fixed_c::pi / 2, ii::shape_flag::kVulnShield,
                             ii::Polygon::T::kPolystar);
  add_new_shape<ii::Polygon>(vec2{24, 0}, 8, 6, c1, 0);
  add_new_shape<ii::Polygon>(vec2{-24, 0}, 8, 6, c1, 0);
  add_new_shape<ii::Polygon>(vec2{0, 24}, 8, 6, c1, fixed_c::pi / 2);
  add_new_shape<ii::Polygon>(vec2{0, -24}, 8, 6, c1, fixed_c::pi / 2);

  add_new_shape<ii::Polygon>(vec2{0, 0}, 24, 4, c0, 0, ii::shape_flag::kNone,
                             ii::Polygon::T::kPolystar);
  add_new_shape<ii::Polygon>(vec2{0, 0}, 14, 8, power ? c1 : c0, 0,
                             ii::shape_flag::kDangerous | ii::shape_flag::kVulnerable);
}

void Shielder::update() {
  fixed s = power_ ? fixed_c::hundredth * 12 : fixed_c::hundredth * 4;
  shape().rotate(s);
  shapes()[9]->rotate(-2 * s);
  for (std::uint32_t i = 0; i < 8; ++i) {
    shapes()[i]->rotate(-s);
  }

  bool on_screen = false;
  dir_ = shape().centre.x < 0                   ? vec2{1, 0}
      : shape().centre.x > ii::kSimDimensions.x ? vec2{-1, 0}
      : shape().centre.y < 0                    ? vec2{0, 1}
      : shape().centre.y > ii::kSimDimensions.y ? vec2{0, -1}
                                                : (on_screen = true, dir_);

  if (!on_screen && rotate_) {
    timer_ = 0;
    rotate_ = false;
  }

  fixed speed = kShielderSpeed +
      (power_ ? fixed_c::tenth * 3 : fixed_c::tenth * 2) * (16 - handle().get<ii::Health>()->hp);
  if (rotate_) {
    auto d = rotate(
        dir_, (rdir_ ? 1 : -1) * (kShielderTimer - timer_) * fixed_c::pi / (2 * kShielderTimer));
    if (!--timer_) {
      rotate_ = false;
      dir_ = rotate(dir_, (rdir_ ? 1 : -1) * fixed_c::pi / 2);
    }
    move(d * speed);
  } else {
    ++timer_;
    if (timer_ > kShielderTimer * 2) {
      timer_ = kShielderTimer;
      rotate_ = true;
      rdir_ = sim().random(2) != 0;
    }
    if (is_on_screen() && power_ && timer_ % kShielderTimer == kShielderTimer / 2) {
      ii::spawn_boss_shot(sim(), shape().centre, 3 * sim().nearest_player_direction(shape().centre),
                          colour_hue360(160, .5f, .6f));
      play_sound_random(ii::sound::kBossFire);
    }
    move(dir_ * speed);
  }
  dir_ = normalise(dir_);
}

Tractor::Tractor(ii::SimInterface& sim, const vec2& position, bool power)
: Enemy{sim, position, ii::ship_flag::kNone}, timer_{kTractorTimer * 4}, power_{power} {
  auto c = colour_hue360(300, .5f, .6f);
  add_new_shape<ii::Polygon>(vec2{24, 0}, 12, 6, c, 0,
                             ii::shape_flag::kDangerous | ii::shape_flag::kVulnerable,
                             ii::Polygon::T::kPolygram);
  add_new_shape<ii::Polygon>(vec2{-24, 0}, 12, 6, c, 0,
                             ii::shape_flag::kDangerous | ii::shape_flag::kVulnerable,
                             ii::Polygon::T::kPolygram);
  add_new_shape<ii::Line>(vec2{0, 0}, vec2{24, 0}, vec2{-24, 0}, c);
  if (power) {
    add_new_shape<ii::Polygon>(vec2{24, 0}, 16, 6, c, 0, ii::shape_flag::kNone,
                               ii::Polygon::T::kPolystar);
    add_new_shape<ii::Polygon>(vec2{-24, 0}, 16, 6, c, 0, ii::shape_flag::kNone,
                               ii::Polygon::T::kPolystar);
  }
}

void Tractor::update() {
  shapes()[0]->rotate(fixed_c::hundredth * 5);
  shapes()[1]->rotate(-fixed_c::hundredth * 5);
  if (power_) {
    shapes()[3]->rotate(-fixed_c::hundredth * 8);
    shapes()[4]->rotate(fixed_c::hundredth * 8);
  }

  if (shape().centre.x < 0) {
    dir_ = vec2{1, 0};
  } else if (shape().centre.x > ii::kSimDimensions.x) {
    dir_ = vec2{-1, 0};
  } else if (shape().centre.y < 0) {
    dir_ = vec2{0, 1};
  } else if (shape().centre.y > ii::kSimDimensions.y) {
    dir_ = vec2{0, -1};
  } else {
    ++timer_;
  }

  if (!ready_ && !spinning_) {
    move(dir_ * kTractorSpeed * (is_on_screen() ? 1 : 2 + fixed_c::half));

    if (timer_ > kTractorTimer * 8) {
      ready_ = true;
      timer_ = 0;
    }
  } else if (ready_) {
    if (timer_ > kTractorTimer) {
      ready_ = false;
      spinning_ = true;
      timer_ = 0;
      players_ = sim().players();
      play_sound(ii::sound::kBossFire);
    }
  } else if (spinning_) {
    shape().rotate(fixed_c::tenth * 3);
    for (const auto& ship : players_) {
      if (!((Player*)ship)->is_killed()) {
        auto d = normalise(shape().centre - ship->position());
        ship->position() += d * kTractorBeamSpeed;
      }
    }

    if (timer_ % (kTractorTimer / 2) == 0 && is_on_screen() && power_) {
      ii::spawn_boss_shot(sim(), shape().centre, 4 * sim().nearest_player_direction(shape().centre),
                          colour_hue360(300, .5f, .6f));
      play_sound_random(ii::sound::kBossFire);
    }

    if (timer_ > kTractorTimer * 5) {
      spinning_ = false;
      timer_ = 0;
    }
  }
}

void Tractor::render() const {
  Enemy::render();
  if (spinning_) {
    for (std::size_t i = 0; i < players_.size(); ++i) {
      if (((timer_ + i * 4) / 4) % 2 && !((Player*)players_[i])->is_killed()) {
        sim().render_line(to_float(shape().centre), to_float(players_[i]->position()),
                          colour_hue360(300, .5f, .6f));
      }
    }
  }
}

}  // namespace

namespace ii {

std::function<void(damage_type)> make_legacy_enemy_on_destroy(ecs::handle h) {
  auto enemy = static_cast<::Enemy*>(h.get<LegacyShip>()->ship.get());
  return [enemy](damage_type type) {
    enemy->explosion();
    enemy->on_destroy(type == damage_type::kBomb);
  };
}

void spawn_follow(SimInterface& sim, const vec2& position, bool has_score, fixed rotation) {
  auto h = sim.create_legacy(std::make_unique<Follow>(sim, position, rotation));
  h.add(legacy_collision(/* bounding width */ 10, h));
  h.add(Enemy{.threat_value = 1, .score_reward = has_score ? 15u : 0});
  h.add(Health{.hp = 1,
               .destroy_sound = ii::sound::kEnemyShatter,
               .on_destroy = make_legacy_enemy_on_destroy(h)});
}

void spawn_big_follow(SimInterface& sim, const vec2& position, bool has_score) {
  auto h = sim.create_legacy(std::make_unique<BigFollow>(sim, position, has_score));
  h.add(legacy_collision(/* bounding width */ 10, h));
  h.add(Enemy{.threat_value = 3, .score_reward = has_score ? 20u : 0});
  h.add(Health{.hp = 3,
               .destroy_sound = ii::sound::kPlayerDestroy,
               .on_destroy = make_legacy_enemy_on_destroy(h)});
}

void spawn_chaser(SimInterface& sim, const vec2& position) {
  auto h = sim.create_legacy(std::make_unique<Chaser>(sim, position));
  h.add(legacy_collision(/* bounding width */ 10, h));
  h.add(Enemy{.threat_value = 2, .score_reward = 30});
  h.add(Health{.hp = 2,
               .destroy_sound = ii::sound::kEnemyShatter,
               .on_destroy = make_legacy_enemy_on_destroy(h)});
}

void spawn_square(SimInterface& sim, const vec2& position, fixed rotation) {
  auto h = sim.create_legacy(std::make_unique<Square>(sim, position, rotation));
  h.add(legacy_collision(/* bounding width */ 15, h));
  h.add(Enemy{.threat_value = 2, .score_reward = 25});
  h.add(Health{.hp = 4, .on_destroy = make_legacy_enemy_on_destroy(h)});
}

void spawn_wall(SimInterface& sim, const vec2& position, bool rdir) {
  auto h = sim.create_legacy(std::make_unique<Wall>(sim, position, rdir));
  h.add(legacy_collision(/* bounding width */ 50, h));
  h.add(Enemy{.threat_value = 4, .score_reward = 20});
  h.add(Health{.hp = 10, .on_destroy = make_legacy_enemy_on_destroy(h)});
}

void spawn_follow_hub(SimInterface& sim, const vec2& position, bool power_a, bool power_b) {
  auto h = sim.create_legacy(std::make_unique<FollowHub>(sim, position, power_a, power_b));
  h.add(legacy_collision(/* bounding width */ 16, h));
  h.add(Enemy{.threat_value = 6u + 2 * power_a + 2 * power_b,
              .score_reward = 50u + power_a * 10 + power_b * 10});
  h.add(Health{.hp = 14,
               .destroy_sound = ii::sound::kPlayerDestroy,
               .on_destroy = make_legacy_enemy_on_destroy(h)});
}

void spawn_shielder(SimInterface& sim, const vec2& position, bool power) {
  auto h = sim.create_legacy(std::make_unique<Shielder>(sim, position, power));
  h.add(legacy_collision(/* bounding width */ 36, h));
  h.add(Enemy{.threat_value = 8u + 2 * power, .score_reward = 60u + power * 40});
  h.add(Health{.hp = 16,
               .destroy_sound = ii::sound::kPlayerDestroy,
               .on_destroy = make_legacy_enemy_on_destroy(h)});
}

void spawn_tractor(SimInterface& sim, const vec2& position, bool power) {
  auto h = sim.create_legacy(std::make_unique<Tractor>(sim, position, power));
  h.add(legacy_collision(/* bounding width */ 36, h));
  h.add(Enemy{.threat_value = 10u + 2 * power, .score_reward = 85u + 40 * power});
  h.add(Health{.hp = 50,
               .destroy_sound = ii::sound::kPlayerDestroy,
               .on_destroy = make_legacy_enemy_on_destroy(h)});
}

void spawn_boss_shot(SimInterface& sim, const vec2& position, const vec2& velocity,
                     const glm::vec4& c) {
  auto h = sim.create_legacy(std::make_unique<BossShot>(sim, position, velocity, c));
  h.add(legacy_collision(/* bounding width */ 12, h));
  h.add(Enemy{.threat_value = 1});
  h.add(Health{.hp = 0, .on_destroy = make_legacy_enemy_on_destroy(h)});
}

}  // namespace ii

Enemy::Enemy(ii::SimInterface& sim, const vec2& position, ii::ship_flag type)
: ii::Ship{sim, position, type | ii::ship_flag::kEnemy} {}

void Enemy::render() const {
  if (auto c = handle().get<ii::Health>(); c && c->hit_timer) {
    for (std::size_t i = 0; i < shapes().size(); ++i) {
      bool hit_flash = !c->hit_flash_ignore_index || i < *c->hit_flash_ignore_index;
      shapes()[i]->render(sim(), to_float(shape().centre), shape().rotation().to_float(),
                          hit_flash ? std::make_optional(glm::vec4{1.f}) : std::nullopt);
    }
    return;
  }
  Ship::render();
}

BossShot::BossShot(ii::SimInterface& sim, const vec2& position, const vec2& velocity,
                   const glm::vec4& c)
: Enemy{sim, position, ii::ship_flag::kWall}, dir_{velocity} {
  add_new_shape<ii::Polygon>(vec2{0}, 16, 8, c, 0, ii::shape_flag::kNone,
                             ii::Polygon::T::kPolystar);
  add_new_shape<ii::Polygon>(vec2{0}, 10, 8, c, 0);
  add_new_shape<ii::Polygon>(vec2{0}, 12, 8, glm::vec4{0.f}, 0, ii::shape_flag::kDangerous);
}

void BossShot::update() {
  move(dir_);
  vec2 p = shape().centre;
  if ((p.x < -10 && dir_.x < 0) || (p.x > ii::kSimDimensions.x + 10 && dir_.x > 0) ||
      (p.y < -10 && dir_.y < 0) || (p.y > ii::kSimDimensions.y + 10 && dir_.y > 0)) {
    destroy();
  }
  shape().set_rotation(shape().rotation() + fixed_c::hundredth * 2);
}