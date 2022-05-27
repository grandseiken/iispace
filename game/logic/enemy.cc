#include "game/logic/enemy.h"
#include "game/logic/player.h"
#include <algorithm>

namespace {
const std::int32_t kFollowTime = 90;
const fixed kFollowSpeed = 2;

const std::int32_t kChaserTime = 60;
const fixed kChaserSpeed = 4;

const fixed kSquareSpeed = 2 + fixed(1) / 4;

const std::int32_t kWallTimer = 80;
const fixed kWallSpeed = 1 + fixed(1) / 4;

const std::int32_t kFollowHubTimer = 170;
const fixed kFollowHubSpeed = 1;

const std::int32_t kShielderTimer = 80;
const fixed kShielderSpeed = 2;

const std::int32_t kTractorTimer = 50;
const fixed kTractorSpeed = 6 * (fixed(1) / 10);
}  // namespace
const fixed Tractor::kTractorBeamSpeed = 2 + fixed(1) / 2;

Enemy::Enemy(const vec2& position, Ship::ship_category type, std::int32_t hp)
: Ship{position, static_cast<Ship::ship_category>(type | kShipEnemy)}, hp_{hp} {}

void Enemy::damage(std::int32_t damage, bool magic, Player* source) {
  hp_ -= std::max(damage, 0);
  if (damage > 0) {
    play_sound_random(Lib::sound::kEnemyHit);
  }

  if (hp_ <= 0 && !is_destroyed()) {
    play_sound_random(destroy_sound_);
    if (source && get_score() > 0) {
      source->add_score(get_score());
    }
    explosion();
    on_destroy(damage >= Player::kBombDamage);
    destroy();
  } else if (!is_destroyed()) {
    if (damage > 0) {
      play_sound_random(Lib::sound::kEnemyHit);
    }
    damaged_ = damage >= Player::kBombDamage ? 25 : 1;
  }
}

void Enemy::render() const {
  if (!damaged_) {
    Ship::render();
    return;
  }
  render_with_colour(0xffffffff);
  --damaged_;
}

Follow::Follow(const vec2& position, fixed radius, std::int32_t hp)
: Enemy{position, kShipNone, hp} {
  add_new_shape<Polygon>(vec2{}, radius, 4, 0x9933ffff, 0, kDangerous | kVulnerable);
  set_score(15);
  set_bounding_width(10);
  set_destroy_sound(Lib::sound::kEnemyShatter);
  set_enemy_value(1);
}

void Follow::update() {
  shape().rotate(fixed_c::tenth);
  if (!game().alive_players()) {
    return;
  }

  ++timer_;
  if (!target_ || timer_ > kFollowTime) {
    target_ = nearest_player();
    timer_ = 0;
  }
  vec2 d = target_->shape().centre - shape().centre;
  move(d.normalised() * kFollowSpeed);
}

BigFollow::BigFollow(const vec2& position, bool has_score)
: Follow{position, 20, 3}, has_score_{has_score} {
  set_score(has_score ? 20 : 0);
  set_destroy_sound(Lib::sound::kPlayerDestroy);
  set_enemy_value(3);
}

void BigFollow::on_destroy(bool bomb) {
  if (bomb) {
    return;
  }

  vec2 d = vec2{10, 0}.rotated(shape().rotation());
  for (std::int32_t i = 0; i < 3; ++i) {
    auto s = std::make_unique<Follow>(shape().centre + d);
    if (!has_score_) {
      s->set_score(0);
    }
    spawn(std::move(s));
    d = d.rotated(2 * fixed_c::pi / 3);
  }
}

Chaser::Chaser(const vec2& position) : Enemy{position, kShipNone, 2}, timer_{kChaserTime} {
  add_new_shape<Polygon>(vec2{}, 10, 4, 0x3399ffff, 0, kDangerous | kVulnerable,
                         Polygon::T::kPolygram);
  set_score(30);
  set_bounding_width(10);
  set_destroy_sound(Lib::sound::kEnemyShatter);
  set_enemy_value(2);
}

void Chaser::update() {
  bool before = is_on_screen();

  if (timer_) {
    timer_--;
  }
  if (timer_ <= 0) {
    timer_ = kChaserTime * (move_ + 1);
    move_ = !move_;
    if (move_) {
      dir_ = kChaserSpeed * (nearest_player()->shape().centre - shape().centre).normalised();
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

Square::Square(const vec2& position, fixed rotation)
: Enemy{position, kShipWall, 4}, timer_{z::rand_int(80) + 40} {
  add_new_shape<Box>(vec2{}, 10, 10, 0x33ff33ff, 0, kDangerous | kVulnerable);
  dir_ = vec2::from_polar(rotation, 1);
  set_score(25);
  set_bounding_width(15);
  set_enemy_value(2);
}

void Square::update() {
  if (is_on_screen() && game().get_non_wall_count() == 0) {
    timer_--;
  } else {
    timer_ = z::rand_int(80) + 40;
  }

  if (timer_ == 0) {
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

  if (pos.x > Lib::kWidth && dir_.x >= 0) {
    dir_.x = -dir_.x;
    if (dir_.x >= 0) {
      dir_.x = -1;
    }
  }
  if (pos.y > Lib::kHeight && dir_.y >= 0) {
    dir_.y = -dir_.y;
    if (dir_.y >= 0) {
      dir_.y = -1;
    }
  }
  dir_ = dir_.normalised();
  move(dir_ * kSquareSpeed);
  shape().set_rotation(dir_.angle());
}

void Square::render() const {
  if (game().get_non_wall_count() == 0 && (timer_ % 4 == 1 || timer_ % 4 == 2)) {
    render_with_colour(0x33333300);
  } else {
    Enemy::render();
  }
}

Wall::Wall(const vec2& position, bool rdir)
: Enemy{position, kShipWall, 10}, dir_{0, 1}, rdir_{rdir} {
  add_new_shape<Box>(vec2{}, 10, 40, 0x33cc33ff, 0, kDangerous | kVulnerable);
  set_score(20);
  set_bounding_width(50);
  set_enemy_value(4);
}

void Wall::update() {
  auto count = game().get_non_wall_count();
  if (!count && timer_ % 8 < 2) {
    if (get_hp() > 2) {
      lib().play_sound(Lib::sound::kEnemySpawn, 1.f, 0.f);
    }
    damage(get_hp() - 2, false, 0);
  }

  if (rotate_) {
    vec2 d =
        dir_.rotated((rdir_ ? 1 : -1) * (timer_ - kWallTimer) * fixed_c::pi / (4 * kWallTimer));

    shape().set_rotation(d.angle());
    timer_--;
    if (timer_ <= 0) {
      timer_ = 0;
      rotate_ = false;
      dir_ = dir_.rotated(rdir_ ? -fixed_c::pi / 4 : fixed_c::pi / 4);
    }
    return;
  } else {
    ++timer_;
    if (timer_ > kWallTimer * 6) {
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
      (pos.x > Lib::kWidth && dir_.x > fixed_c::hundredth) ||
      (pos.y > Lib::kHeight && dir_.y > fixed_c::hundredth)) {
    dir_ = -dir_.normalised();
  }

  move(dir_ * kWallSpeed);
  shape().set_rotation(dir_.angle());
}

void Wall::on_destroy(bool bomb) {
  if (bomb) {
    return;
  }
  vec2 d = dir_.rotated(fixed_c::pi / 2);

  vec2 v = shape().centre + d * 10 * 3;
  if (v.x >= 0 && v.x <= Lib::kWidth && v.y >= 0 && v.y <= Lib::kHeight) {
    spawn_new<Square>(v, shape().rotation());
  }

  v = shape().centre - d * 10 * 3;
  if (v.x >= 0 && v.x <= Lib::kWidth && v.y >= 0 && v.y <= Lib::kHeight) {
    spawn_new<Square>(v, shape().rotation());
  }
}

FollowHub::FollowHub(const vec2& position, bool powera, bool powerb)
: Enemy{position, kShipNone, 14}, powera_{powera}, powerb_{powerb} {
  add_new_shape<Polygon>(vec2{}, 16, 4, 0x6666ffff, fixed_c::pi / 4, kDangerous | kVulnerable,
                         Polygon::T::kPolygram);
  if (powerb_) {
    add_new_shape<Polygon>(vec2{16, 0}, 8, 4, 0x6666ffff, fixed_c::pi / 4, 0,
                           Polygon::T::kPolystar);
    add_new_shape<Polygon>(vec2{-16, 0}, 8, 4, 0x6666ffff, fixed_c::pi / 4, 0,
                           Polygon::T::kPolystar);
    add_new_shape<Polygon>(vec2{0, 16}, 8, 4, 0x6666ffff, fixed_c::pi / 4, 0,
                           Polygon::T::kPolystar);
    add_new_shape<Polygon>(vec2{0, -16}, 8, 4, 0x6666ffff, fixed_c::pi / 4, 0,
                           Polygon::T::kPolystar);
  }

  add_new_shape<Polygon>(vec2{16, 0}, 8, 4, 0x6666ffff, fixed_c::pi / 4);
  add_new_shape<Polygon>(vec2{-16, 0}, 8, 4, 0x6666ffff, fixed_c::pi / 4);
  add_new_shape<Polygon>(vec2{0, 16}, 8, 4, 0x6666ffff, fixed_c::pi / 4);
  add_new_shape<Polygon>(vec2{0, -16}, 8, 4, 0x6666ffff, fixed_c::pi / 4);
  set_score(50 + powera_ * 10 + powerb_ * 10);
  set_bounding_width(16);
  set_destroy_sound(Lib::sound::kPlayerDestroy);
  set_enemy_value(6 + (powera ? 2 : 0) + (powerb ? 2 : 0));
}

void FollowHub::update() {
  ++timer_;
  if (timer_ > (powera_ ? kFollowHubTimer / 2 : kFollowHubTimer)) {
    timer_ = 0;
    ++count_;
    if (is_on_screen()) {
      if (powerb_) {
        spawn_new<Chaser>(shape().centre);
      } else {
        spawn_new<Follow>(shape().centre);
      }
      play_sound_random(Lib::sound::kEnemySpawn);
    }
  }

  dir_ = shape().centre.x < 0           ? vec2{1, 0}
      : shape().centre.x > Lib::kWidth  ? vec2{-1, 0}
      : shape().centre.y < 0            ? vec2{0, 1}
      : shape().centre.y > Lib::kHeight ? vec2{0, -1}
      : count_ > 3                      ? (count_ = 0, dir_.rotated(-fixed_c::pi / 2))
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
    spawn_new<BigFollow>(shape().centre, true);
  }
  spawn_new<Chaser>(shape().centre);
}

Shielder::Shielder(const vec2& position, bool power)
: Enemy{position, kShipNone, 16}, dir_{0, 1}, power_{power} {
  add_new_shape<Polygon>(vec2{24, 0}, 8, 6, 0x006633ff, 0, kVulnShield, Polygon::T::kPolystar);
  add_new_shape<Polygon>(vec2{-24, 0}, 8, 6, 0x006633ff, 0, kVulnShield, Polygon::T::kPolystar);
  add_new_shape<Polygon>(vec2{0, 24}, 8, 6, 0x006633ff, fixed_c::pi / 2, kVulnShield,
                         Polygon::T::kPolystar);
  add_new_shape<Polygon>(vec2{0, -24}, 8, 6, 0x006633ff, fixed_c::pi / 2, kVulnShield,
                         Polygon::T::kPolystar);
  add_new_shape<Polygon>(vec2{24, 0}, 8, 6, 0x33cc99ff, 0, 0);
  add_new_shape<Polygon>(vec2{-24, 0}, 8, 6, 0x33cc99ff, 0, 0);
  add_new_shape<Polygon>(vec2{0, 24}, 8, 6, 0x33cc99ff, fixed_c::pi / 2, 0);
  add_new_shape<Polygon>(vec2{0, -24}, 8, 6, 0x33cc99ff, fixed_c::pi / 2, 0);

  add_new_shape<Polygon>(vec2{0, 0}, 24, 4, 0x006633ff, 0, 0, Polygon::T::kPolystar);
  add_new_shape<Polygon>(vec2{0, 0}, 14, 8, power ? 0x33cc99ff : 0x006633ff, 0,
                         kDangerous | kVulnerable);
  set_score(60 + power_ * 40);
  set_bounding_width(36);
  set_destroy_sound(Lib::sound::kPlayerDestroy);
  set_enemy_value(8 + (power ? 2 : 0));
}

void Shielder::update() {
  fixed s = power_ ? fixed_c::hundredth * 12 : fixed_c::hundredth * 4;
  shape().rotate(s);
  shapes()[9]->rotate(-2 * s);
  for (std::int32_t i = 0; i < 8; ++i) {
    shapes()[i]->rotate(-s);
  }

  bool on_screen = false;
  dir_ = shape().centre.x < 0           ? vec2{1, 0}
      : shape().centre.x > Lib::kWidth  ? vec2{-1, 0}
      : shape().centre.y < 0            ? vec2{0, 1}
      : shape().centre.y > Lib::kHeight ? vec2{0, -1}
                                        : (on_screen = true, dir_);

  if (!on_screen && rotate_) {
    timer_ = 0;
    rotate_ = false;
  }

  fixed speed =
      kShielderSpeed + (power_ ? fixed_c::tenth * 3 : fixed_c::tenth * 2) * (16 - get_hp());
  if (rotate_) {
    vec2 d = dir_.rotated((rdir_ ? 1 : -1) * (kShielderTimer - timer_) * fixed_c::pi /
                          (2 * kShielderTimer));
    timer_--;
    if (timer_ <= 0) {
      timer_ = 0;
      rotate_ = false;
      dir_ = dir_.rotated((rdir_ ? 1 : -1) * fixed_c::pi / 2);
    }
    move(d * speed);
  } else {
    ++timer_;
    if (timer_ > kShielderTimer * 2) {
      timer_ = kShielderTimer;
      rotate_ = true;
      rdir_ = z::rand_int(2) != 0;
    }
    if (is_on_screen() && power_ && timer_ % kShielderTimer == kShielderTimer / 2) {
      Player* p = nearest_player();
      vec2 v = shape().centre;

      vec2 d = (p->shape().centre - v).normalised();
      spawn_new<BossShot>(v, d * 3, 0x33cc99ff);
      play_sound_random(Lib::sound::kBossFire);
    }
    move(dir_ * speed);
  }
  dir_ = dir_.normalised();
}

Tractor::Tractor(const vec2& position, bool power)
: Enemy{position, kShipNone, 50}, timer_{kTractorTimer * 4}, power_{power} {
  add_new_shape<Polygon>(vec2{24, 0}, 12, 6, 0xcc33ccff, 0, kDangerous | kVulnerable,
                         Polygon::T::kPolygram);
  add_new_shape<Polygon>(vec2{-24, 0}, 12, 6, 0xcc33ccff, 0, kDangerous | kVulnerable,
                         Polygon::T::kPolygram);
  add_new_shape<Line>(vec2{0, 0}, vec2{24, 0}, vec2{-24, 0}, 0xcc33ccff);
  if (power) {
    add_new_shape<Polygon>(vec2{24, 0}, 16, 6, 0xcc33ccff, 0, 0, Polygon::T::kPolystar);
    add_new_shape<Polygon>(vec2{-24, 0}, 16, 6, 0xcc33ccff, 0, 0, Polygon::T::kPolystar);
  }
  set_score(85 + power * 40);
  set_bounding_width(36);
  set_destroy_sound(Lib::sound::kPlayerDestroy);
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
  } else if (shape().centre.x > Lib::kWidth) {
    dir_ = vec2{-1, 0};
  } else if (shape().centre.y < 0) {
    dir_ = vec2{0, 1};
  } else if (shape().centre.y > Lib::kHeight) {
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
      players_ = game().players();
      play_sound(Lib::sound::kBossFire);
    }
  } else if (spinning_) {
    shape().rotate(fixed_c::tenth * 3);
    for (const auto& ship : players_) {
      if (!((Player*)ship)->is_killed()) {
        vec2 d = (shape().centre - ship->shape().centre).normalised();
        ship->move(d * kTractorBeamSpeed);
      }
    }

    if (timer_ % (kTractorTimer / 2) == 0 && is_on_screen() && power_) {
      Player* p = nearest_player();
      vec2 d = (p->shape().centre - shape().centre).normalised();
      spawn_new<BossShot>(shape().centre, d * 4, 0xcc33ccff);
      play_sound_random(Lib::sound::kBossFire);
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
        lib().render_line(to_float(shape().centre), to_float(players_[i]->shape().centre),
                          0xcc33ccff);
      }
    }
  }
}

BossShot::BossShot(const vec2& position, const vec2& velocity, colour_t c)
: Enemy{position, kShipWall, 0}, dir_{velocity} {
  add_new_shape<Polygon>(vec2{}, 16, 8, c, 0, 0, Polygon::T::kPolystar);
  add_new_shape<Polygon>(vec2{}, 10, 8, c, 0, 0);
  add_new_shape<Polygon>(vec2{}, 12, 8, 0, 0, kDangerous);
  set_bounding_width(12);
  set_score(0);
  set_enemy_value(1);
}

void BossShot::update() {
  move(dir_);
  vec2 p = shape().centre;
  if ((p.x < -10 && dir_.x < 0) || (p.x > Lib::kWidth + 10 && dir_.x > 0) ||
      (p.y < -10 && dir_.y < 0) || (p.y > Lib::kHeight + 10 && dir_.y > 0)) {
    destroy();
  }
  shape().set_rotation(shape().rotation() + fixed_c::hundredth * 2);
}