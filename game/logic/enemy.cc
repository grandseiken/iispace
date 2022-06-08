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
  Follow(ii::SimInterface& sim, const vec2& position, bool has_score, fixed rotation,
         fixed radius = 10, std::uint32_t hp = 1);
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

Follow::Follow(ii::SimInterface& sim, const vec2& position, bool has_score, fixed rotation,
               fixed radius, std::uint32_t hp)
: Enemy{sim, position, kShipNone, hp} {
  add_new_shape<ii::Polygon>(vec2{0}, radius, 4, colour_hue360(270, .6f), rotation,
                             kDangerous | kVulnerable);
  set_score(has_score ? 15 : 0);
  set_destroy_sound(ii::sound::kEnemyShatter);
  set_enemy_value(1);
}

void Follow::update() {
  shape().rotate(fixed_c::tenth);
  if (!sim().alive_players()) {
    return;
  }

  ++timer_;
  if (!target_ || timer_ > kFollowTime) {
    target_ = nearest_player();
    timer_ = 0;
  }
  auto d = target_->shape().centre - shape().centre;
  move(normalise(d) * kFollowSpeed);
}

BigFollow::BigFollow(ii::SimInterface& sim, const vec2& position, bool has_score)
: Follow{sim, position, has_score, 0, 20, 3}, has_score_{has_score} {
  set_score(has_score ? 20 : 0);
  set_destroy_sound(ii::sound::kPlayerDestroy);
  set_enemy_value(3);
}

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
: Enemy{sim, position, kShipNone, 2}, timer_{kChaserTime} {
  add_new_shape<ii::Polygon>(vec2{0}, 10, 4, colour_hue360(210, .6f), 0, kDangerous | kVulnerable,
                             ii::Polygon::T::kPolygram);
  set_score(30);
  set_destroy_sound(ii::sound::kEnemyShatter);
  set_enemy_value(2);
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
      dir_ = kChaserSpeed * normalise(nearest_player()->shape().centre - shape().centre);
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
: Enemy{sim, position, kShipWall, 4}, timer_{sim.random(80) + 40} {
  add_new_shape<ii::Box>(vec2{0}, 10, 10, colour_hue360(120, .6f), 0, kDangerous | kVulnerable);
  dir_ = from_polar(rotation, 1_fx);
  set_score(25);
  set_enemy_value(2);
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
: Enemy{sim, position, kShipWall, 10}, dir_{0, 1}, rdir_{rdir} {
  add_new_shape<ii::Box>(vec2{0}, 10, 40, colour_hue360(120, .5f, .6f), 0,
                         kDangerous | kVulnerable);
  set_score(20);
  set_enemy_value(4);
}

void Wall::update() {
  auto count = sim().get_non_wall_count();
  if (!count && timer_ % 8 < 2) {
    if (get_hp() > 2) {
      sim().play_sound(ii::sound::kEnemySpawn, 1.f, 0.f);
    }
    damage(std::max(2u, get_hp()) - 2, false, 0);
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
: Enemy{sim, position, kShipNone, 14}, powera_{powera}, powerb_{powerb} {
  auto c = colour_hue360(240, .7f);
  add_new_shape<ii::Polygon>(vec2{0}, 16, 4, c, fixed_c::pi / 4, kDangerous | kVulnerable,
                             ii::Polygon::T::kPolygram);
  if (powerb_) {
    add_new_shape<ii::Polygon>(vec2{16, 0}, 8, 4, c, fixed_c::pi / 4, 0, ii::Polygon::T::kPolystar);
    add_new_shape<ii::Polygon>(vec2{-16, 0}, 8, 4, c, fixed_c::pi / 4, 0,
                               ii::Polygon::T::kPolystar);
    add_new_shape<ii::Polygon>(vec2{0, 16}, 8, 4, c, fixed_c::pi / 4, 0, ii::Polygon::T::kPolystar);
    add_new_shape<ii::Polygon>(vec2{0, -16}, 8, 4, c, fixed_c::pi / 4, 0,
                               ii::Polygon::T::kPolystar);
  }

  add_new_shape<ii::Polygon>(vec2{16, 0}, 8, 4, c, fixed_c::pi / 4);
  add_new_shape<ii::Polygon>(vec2{-16, 0}, 8, 4, c, fixed_c::pi / 4);
  add_new_shape<ii::Polygon>(vec2{0, 16}, 8, 4, c, fixed_c::pi / 4);
  add_new_shape<ii::Polygon>(vec2{0, -16}, 8, 4, c, fixed_c::pi / 4);
  set_score(50 + powera_ * 10 + powerb_ * 10);
  set_destroy_sound(ii::sound::kPlayerDestroy);
  set_enemy_value(6 + (powera ? 2 : 0) + (powerb ? 2 : 0));
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
: Enemy{sim, position, kShipNone, 16}, dir_{0, 1}, power_{power} {
  auto c0 = colour_hue360(150, .2f);
  auto c1 = colour_hue360(160, .5f, .6f);
  add_new_shape<ii::Polygon>(vec2{24, 0}, 8, 6, c0, 0, kVulnShield, ii::Polygon::T::kPolystar);
  add_new_shape<ii::Polygon>(vec2{-24, 0}, 8, 6, c0, 0, kVulnShield, ii::Polygon::T::kPolystar);
  add_new_shape<ii::Polygon>(vec2{0, 24}, 8, 6, c0, fixed_c::pi / 2, kVulnShield,
                             ii::Polygon::T::kPolystar);
  add_new_shape<ii::Polygon>(vec2{0, -24}, 8, 6, c0, fixed_c::pi / 2, kVulnShield,
                             ii::Polygon::T::kPolystar);
  add_new_shape<ii::Polygon>(vec2{24, 0}, 8, 6, c1, 0, 0);
  add_new_shape<ii::Polygon>(vec2{-24, 0}, 8, 6, c1, 0, 0);
  add_new_shape<ii::Polygon>(vec2{0, 24}, 8, 6, c1, fixed_c::pi / 2, 0);
  add_new_shape<ii::Polygon>(vec2{0, -24}, 8, 6, c1, fixed_c::pi / 2, 0);

  add_new_shape<ii::Polygon>(vec2{0, 0}, 24, 4, c0, 0, 0, ii::Polygon::T::kPolystar);
  add_new_shape<ii::Polygon>(vec2{0, 0}, 14, 8, power ? c1 : c0, 0, kDangerous | kVulnerable);
  set_score(60 + power_ * 40);
  set_destroy_sound(ii::sound::kPlayerDestroy);
  set_enemy_value(8 + (power ? 2 : 0));
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

  fixed speed =
      kShielderSpeed + (power_ ? fixed_c::tenth * 3 : fixed_c::tenth * 2) * (16 - get_hp());
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
      Player* p = nearest_player();
      auto v = shape().centre;

      auto d = normalise(p->shape().centre - v);
      ii::spawn_boss_shot(sim(), v, d * 3, colour_hue360(160, .5f, .6f));
      play_sound_random(ii::sound::kBossFire);
    }
    move(dir_ * speed);
  }
  dir_ = normalise(dir_);
}

Tractor::Tractor(ii::SimInterface& sim, const vec2& position, bool power)
: Enemy{sim, position, kShipNone, 50}, timer_{kTractorTimer * 4}, power_{power} {
  auto c = colour_hue360(300, .5f, .6f);
  add_new_shape<ii::Polygon>(vec2{24, 0}, 12, 6, c, 0, kDangerous | kVulnerable,
                             ii::Polygon::T::kPolygram);
  add_new_shape<ii::Polygon>(vec2{-24, 0}, 12, 6, c, 0, kDangerous | kVulnerable,
                             ii::Polygon::T::kPolygram);
  add_new_shape<ii::Line>(vec2{0, 0}, vec2{24, 0}, vec2{-24, 0}, c);
  if (power) {
    add_new_shape<ii::Polygon>(vec2{24, 0}, 16, 6, c, 0, 0, ii::Polygon::T::kPolystar);
    add_new_shape<ii::Polygon>(vec2{-24, 0}, 16, 6, c, 0, 0, ii::Polygon::T::kPolystar);
  }
  set_score(85 + power * 40);
  set_destroy_sound(ii::sound::kPlayerDestroy);
  set_enemy_value(10 + (power ? 2 : 0));
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
        auto d = normalise(shape().centre - ship->shape().centre);
        ship->move(d * kTractorBeamSpeed);
      }
    }

    if (timer_ % (kTractorTimer / 2) == 0 && is_on_screen() && power_) {
      Player* p = nearest_player();
      auto d = normalise(p->shape().centre - shape().centre);
      ii::spawn_boss_shot(sim(), shape().centre, d * 4, colour_hue360(300, .5f, .6f));
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
        sim().render_line(to_float(shape().centre), to_float(players_[i]->shape().centre),
                          colour_hue360(300, .5f, .6f));
      }
    }
  }
}

}  // namespace

namespace ii {

void spawn_follow(SimInterface& sim, const vec2& position, bool has_score, fixed rotation) {
  auto h = sim.create_legacy(std::make_unique<Follow>(sim, position, has_score, rotation));
  h.emplace<Collision>(/* bounding width */ 10);
}

void spawn_big_follow(SimInterface& sim, const vec2& position, bool has_score) {
  auto h = sim.create_legacy(std::make_unique<BigFollow>(sim, position, has_score));
  h.emplace<Collision>(/* bounding width */ 10);
}

void spawn_chaser(SimInterface& sim, const vec2& position) {
  auto h = sim.create_legacy(std::make_unique<Chaser>(sim, position));
  h.emplace<Collision>(/* bounding width */ 10);
}

void spawn_square(SimInterface& sim, const vec2& position, fixed rotation) {
  auto h = sim.create_legacy(std::make_unique<Square>(sim, position, rotation));
  h.emplace<Collision>(/* bounding width */ 15);
}

void spawn_wall(SimInterface& sim, const vec2& position, bool rdir) {
  auto h = sim.create_legacy(std::make_unique<Wall>(sim, position, rdir));
  h.emplace<Collision>(/* bounding width */ 50);
}

void spawn_follow_hub(SimInterface& sim, const vec2& position, bool power_a, bool power_b) {
  auto h = sim.create_legacy(std::make_unique<FollowHub>(sim, position, power_a, power_b));
  h.emplace<Collision>(/* bounding width */ 16);
}

void spawn_shielder(SimInterface& sim, const vec2& position, bool power) {
  auto h = sim.create_legacy(std::make_unique<Shielder>(sim, position, power));
  h.emplace<Collision>(/* bounding width */ 36);
}

void spawn_tractor(SimInterface& sim, const vec2& position, bool power) {
  auto h = sim.create_legacy(std::make_unique<Tractor>(sim, position, power));
  h.emplace<Collision>(/* bounding width */ 36);
}

void spawn_boss_shot(SimInterface& sim, const vec2& position, const vec2& velocity,
                     const glm::vec4& c) {
  auto h = sim.create_legacy(std::make_unique<BossShot>(sim, position, velocity, c));
  h.emplace<Collision>(/* bounding width */ 12);
}

}  // namespace ii

Enemy::Enemy(ii::SimInterface& sim, const vec2& position, Ship::ship_category type,
             std::uint32_t hp)
: ii::Ship{sim, position, static_cast<Ship::ship_category>(type | kShipEnemy)}, hp_{hp} {}

void Enemy::damage(std::uint32_t damage, bool magic, Player* source) {
  hp_ = hp_ < damage ? 0 : hp_ - damage;
  if (damage > 0) {
    play_sound_random(ii::sound::kEnemyHit);
  }

  if (!hp_ && !is_destroyed()) {
    play_sound_random(destroy_sound_);
    if (source && get_score() > 0) {
      source->add_score(get_score());
    }
    explosion();
    on_destroy(damage >= Player::kBombDamage);
    destroy();
  } else if (!is_destroyed()) {
    if (damage > 0) {
      play_sound_random(ii::sound::kEnemyHit);
    }
    damaged_ = damage >= Player::kBombDamage ? 25 : 1;
  }
}

void Enemy::render() const {
  if (!damaged_) {
    Ship::render();
    return;
  }
  render_with_colour(glm::vec4{1.f});
  --damaged_;
}

BossShot::BossShot(ii::SimInterface& sim, const vec2& position, const vec2& velocity,
                   const glm::vec4& c)
: Enemy{sim, position, kShipWall, 0}, dir_{velocity} {
  add_new_shape<ii::Polygon>(vec2{0}, 16, 8, c, 0, 0, ii::Polygon::T::kPolystar);
  add_new_shape<ii::Polygon>(vec2{0}, 10, 8, c, 0, 0);
  add_new_shape<ii::Polygon>(vec2{0}, 12, 8, glm::vec4{0.f}, 0, kDangerous);
  set_score(0);
  set_enemy_value(1);
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