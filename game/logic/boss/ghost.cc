#include "game/logic/boss/ghost.h"
#include "game/logic/player.h"

namespace {
const std::int32_t kGbBaseHp = 700;
const std::int32_t kGbTimer = 250;
const std::int32_t kGbAttackTime = 100;
const fixed kGbWallSpeed = 3 + fixed(1) / 2;
}  // namespace

GhostBoss::GhostBoss(std::int32_t players, std::int32_t cycle)
: Boss(vec2(Lib::kWidth / 2, Lib::kHeight / 2), SimState::BOSS_2B, kGbBaseHp, players, cycle)
, visible_(false)
, vtime_(0)
, timer_(0)
, attack_time_(kGbAttackTime)
, attack_(0)
, _rDir(false)
, start_time_(120)
, danger_circle_(0)
, danger_offset1_(0)
, danger_offset2_(0)
, danger_offset3_(0)
, danger_offset4_(0)
, shot_type_(false) {
  add_shape(new Polygon(vec2(), 32, 8, 0x9933ccff, 0, 0));
  add_shape(new Polygon(vec2(), 48, 8, 0xcc66ffff, 0, 0));

  for (std::int32_t i = 0; i < 8; i++) {
    vec2 c = vec2::from_polar(i * fixed_c::pi / 4, 48);

    CompoundShape* s = new CompoundShape(c, 0, 0);
    for (std::int32_t j = 0; j < 8; j++) {
      vec2 d = vec2::from_polar(j * fixed_c::pi / 4, 1);
      s->add_shape(new Line(vec2(), d * 10, d * 10 * 2, 0x9933ccff, 0));
    }
    add_shape(s);
  }

  add_shape(new Polygon(vec2(), 24, 8, 0xcc66ffff, 0, 0, Polygon::T::kPolygram));

  for (std::int32_t n = 0; n < 5; n++) {
    CompoundShape* s = new CompoundShape(vec2(), 0, 0);
    for (std::int32_t i = 0; i < 16 + n * 6; i++) {
      vec2 d = vec2::from_polar(i * 2 * fixed_c::pi / (16 + n * 6), 100 + n * 60);
      s->add_shape(new Polygon(d, 16, 8, n ? 0x330066ff : 0x9933ccff, 0, 0, Polygon::T::kPolystar));
      s->add_shape(new Polygon(d, 12, 8, n ? 0x330066ff : 0x9933ccff));
    }
    add_shape(s);
  }

  for (std::int32_t n = 0; n < 5; n++) {
    CompoundShape* s = new CompoundShape(vec2(), 0, kDangerous);
    add_shape(s);
  }

  set_ignore_damage_colour_index(12);
}

void GhostBoss::update() {
  for (std::int32_t n = 1; n < 5; ++n) {
    CompoundShape* s = (CompoundShape*)shapes()[11 + n].get();
    CompoundShape* c = (CompoundShape*)shapes()[16 + n].get();
    c->clear_shapes();

    for (std::int32_t i = 0; i < 16 + n * 6; ++i) {
      Shape& s1 = *s->shapes()[2 * i];
      Shape& s2 = *s->shapes()[1 + 2 * i];
      std::int32_t t = n == 1 ? (i + danger_offset1_) % 11
          : n == 2            ? (i + danger_offset2_) % 7
          : n == 3            ? (i + danger_offset3_) % 17
                              : (i + danger_offset4_) % 10;

      bool b = n == 1 ? (t % 11 == 0 || t % 11 == 5)
          : n == 2    ? (t % 7 == 0)
          : n == 3    ? (t % 17 == 0 || t % 17 == 8)
                      : (t % 10 == 0);

      if (((n == 4 && attack_ != 2) || (n + (n == 3)) & danger_circle_) && visible_ && b) {
        s1.colour = 0xcc66ffff;
        s2.colour = 0xcc66ffff;
        if (vtime_ == 0) {
          vec2 d = vec2::from_polar(i * 2 * fixed_c::pi / (16 + n * 6), 100 + n * 60);
          c->add_shape(new Polygon(d, 9, 8, 0));
          warnings_.push_back(c->convert_point(shape().centre, shape().rotation(), d));
        }
      } else {
        s1.colour = 0x330066ff;
        s2.colour = 0x330066ff;
      }
    }
  }
  if (!(attack_ == 2 && attack_time_ < kGbAttackTime * 4 && attack_time_)) {
    shape().rotate(-fixed_c::hundredth * 4);
  }

  for (std::size_t i = 0; i < 8; i++) {
    shapes()[i + 2]->rotate(fixed_c::hundredth * 4);
  }
  shapes()[10]->rotate(fixed_c::hundredth * 6);
  for (std::int32_t n = 0; n < 5; n++) {
    CompoundShape* s = (CompoundShape*)shapes()[11 + n].get();
    if (n % 2) {
      s->rotate(fixed_c::hundredth * 3 + (fixed(3) / 2000) * n);
    } else {
      s->rotate(fixed_c::hundredth * 5 - (fixed(3) / 2000) * n);
    }
    for (const auto& t : s->shapes()) {
      t->rotate(-fixed_c::tenth);
    }

    s = (CompoundShape*)shapes()[16 + n].get();
    if (n % 2) {
      s->rotate(fixed_c::hundredth * 3 + (fixed(3) / 2000) * n);
    } else {
      s->rotate(fixed_c::hundredth * 4 - (fixed(3) / 2000) * n);
    }
    for (const auto& t : s->shapes()) {
      t->rotate(-fixed_c::tenth);
    }
  }

  if (start_time_) {
    start_time_--;
    return;
  }

  if (!attack_time_) {
    timer_++;
    if (timer_ == 9 * kGbTimer / 10 ||
        (timer_ >= kGbTimer / 10 && timer_ < 9 * kGbTimer / 10 - 16 &&
         ((timer_ % 16 == 0 && attack_ == 2) || (timer_ % 32 == 0 && shot_type_)))) {
      for (std::int32_t i = 0; i < 8; i++) {
        vec2 d = vec2::from_polar(i * fixed_c::pi / 4 + shape().rotation(), 5);
        spawn(new BossShot(shape().centre, d, 0xcc66ffff));
      }
      play_sound_random(Lib::sound::kBossFire);
    }
    if (timer_ != 9 * kGbTimer / 10 && timer_ >= kGbTimer / 10 && timer_ < 9 * kGbTimer / 10 - 16 &&
        timer_ % 16 == 0 && (!shot_type_ || attack_ == 2)) {
      Player* p = nearest_player();
      vec2 d = (p->shape().centre - shape().centre).normalised();

      if (d.length() > fixed_c::half) {
        spawn(new BossShot(shape().centre, d * 5, 0xcc66ffff));
        spawn(new BossShot(shape().centre, d * -5, 0xcc66ffff));
        play_sound_random(Lib::sound::kBossFire);
      }
    }
  } else {
    attack_time_--;

    if (attack_ == 2) {
      if (attack_time_ >= kGbAttackTime * 4 && attack_time_ % 8 == 0) {
        vec2 pos(z::rand_int(Lib::kWidth + 1), z::rand_int(Lib::kHeight + 1));
        spawn(new GhostMine(pos, this));
        play_sound_random(Lib::sound::kEnemySpawn);
        danger_circle_ = 0;
      } else if (attack_time_ == kGbAttackTime * 4 - 1) {
        visible_ = true;
        vtime_ = 60;
        add_shape(new Box(vec2(Lib::kWidth / 2 + 32, 0), Lib::kWidth / 2, 10, 0xcc66ffff, 0, 0));
        add_shape(new Box(vec2(Lib::kWidth / 2 + 32, 0), Lib::kWidth / 2, 7, 0xcc66ffff, 0, 0));
        add_shape(new Box(vec2(Lib::kWidth / 2 + 32, 0), Lib::kWidth / 2, 4, 0xcc66ffff, 0, 0));

        add_shape(new Box(vec2(-Lib::kWidth / 2 - 32, 0), Lib::kWidth / 2, 10, 0xcc66ffff, 0, 0));
        add_shape(new Box(vec2(-Lib::kWidth / 2 - 32, 0), Lib::kWidth / 2, 7, 0xcc66ffff, 0, 0));
        add_shape(new Box(vec2(-Lib::kWidth / 2 - 32, 0), Lib::kWidth / 2, 4, 0xcc66ffff, 0, 0));
        play_sound(Lib::sound::kBossFire);
      } else if (attack_time_ < kGbAttackTime * 3) {
        if (attack_time_ == kGbAttackTime * 3 - 1) {
          play_sound(Lib::sound::kBossAttack);
        }
        shape().rotate((_rDir ? 1 : -1) * 2 * fixed_c::pi / (kGbAttackTime * 6));
        if (!attack_time_) {
          for (std::int32_t i = 0; i < 6; i++) {
            destroy_shape(21);
          }
        }
        danger_offset1_ = z::rand_int(11);
        danger_offset2_ = z::rand_int(7);
        danger_offset3_ = z::rand_int(17);
        danger_offset3_ = z::rand_int(10);
      }
    }

    if (attack_ == 0 && attack_time_ == kGbAttackTime * 2) {
      bool r = z::rand_int(2) != 0;
      spawn(new GhostWall(r, true));
      spawn(new GhostWall(!r, false));
    }
    if (attack_ == 1 && attack_time_ == kGbAttackTime * 2) {
      _rDir = z::rand_int(2) != 0;
      spawn(new GhostWall(!_rDir, false, 0));
    }
    if (attack_ == 1 && attack_time_ == kGbAttackTime * 1) {
      spawn(new GhostWall(_rDir, true, 0));
    }

    if ((attack_ == 0 || attack_ == 1) && is_hp_low() && attack_time_ == kGbAttackTime * 1) {
      std::int32_t r = z::rand_int(4);
      vec2 v = r == 0 ? vec2(-Lib::kWidth / 4, Lib::kHeight / 2)
          : r == 1    ? vec2(Lib::kWidth + Lib::kWidth / 4, Lib::kHeight / 2)
          : r == 2    ? vec2(Lib::kWidth / 2, -Lib::kHeight / 4)
                      : vec2(Lib::kWidth / 2, Lib::kHeight + Lib::kHeight / 4);
      spawn(new BigFollow(v, false));
    }
    if (!attack_time_ && !visible_) {
      visible_ = true;
      vtime_ = 60;
    }
    if (attack_time_ == 0 && attack_ != 2) {
      std::int32_t r = z::rand_int(3);
      danger_circle_ |= 1 + (r == 2 ? 3 : r);
      if (z::rand_int(2) || is_hp_low()) {
        r = z::rand_int(3);
        danger_circle_ |= 1 + (r == 2 ? 3 : r);
      }
      if (z::rand_int(2) || is_hp_low()) {
        r = z::rand_int(3);
        danger_circle_ |= 1 + (r == 2 ? 3 : r);
      }
    } else {
      danger_circle_ = 0;
    }
  }

  if (timer_ >= kGbTimer) {
    timer_ = 0;
    visible_ = false;
    vtime_ = 60;
    attack_ = z::rand_int(3);
    shot_type_ = z::rand_int(2) == 0;
    shapes()[0]->category = 0;
    shapes()[1]->category = 0;
    shapes()[11]->category = 0;
    if (shapes().size() >= 22) {
      shapes()[21]->category = 0;
      shapes()[24]->category = 0;
    }
    play_sound(Lib::sound::kBossAttack);

    if (attack_ == 0) {
      attack_time_ = kGbAttackTime * 2 + 50;
    }
    if (attack_ == 1) {
      attack_time_ = kGbAttackTime * 3;
      for (std::size_t i = 0; i < game().players().size(); i++) {
        spawn(new Powerup(shape().centre, Powerup::type::kShield));
      }
    }
    if (attack_ == 2) {
      attack_time_ = kGbAttackTime * 6;
      _rDir = z::rand_int(2) != 0;
    }
  }

  if (vtime_) {
    vtime_--;
    if (!vtime_ && visible_) {
      shapes()[0]->category = kShield;
      shapes()[1]->category = kDangerous | kVulnerable;
      shapes()[11]->category = kDangerous;
      if (shapes().size() >= 22) {
        shapes()[21]->category = kDangerous;
        shapes()[24]->category = kDangerous;
      }
    }
  }
}

void GhostBoss::render() const {
  if ((start_time_ / 4) % 2 == 1) {
    render_with_colour(1);
    return;
  }
  if ((visible_ && ((vtime_ / 4) % 2 == 0)) || (!visible_ && ((vtime_ / 4) % 2 == 1))) {
    Boss::render();
    return;
  }

  render_with_colour(0x330066ff);
  if (!start_time_) {
    render_hp_bar();
  }
}

std::int32_t GhostBoss::get_damage(std::int32_t damage, bool magic) {
  return damage;
}

GhostWall::GhostWall(bool swap, bool no_gap, bool ignored)
: Enemy(vec2(Lib::kWidth / 2, swap ? -10 : 10 + Lib::kHeight), kShipNone, 0)
, dir_(0, swap ? 1 : -1) {
  if (no_gap) {
    add_shape(new Box(vec2(), fixed(Lib::kWidth), 10, 0xcc66ffff, 0, kDangerous | kShield));
    add_shape(new Box(vec2(), fixed(Lib::kWidth), 7, 0xcc66ffff, 0, 0));
    add_shape(new Box(vec2(), fixed(Lib::kWidth), 4, 0xcc66ffff, 0, 0));
  } else {
    add_shape(new Box(vec2(), Lib::kHeight / 2, 10, 0xcc66ffff, 0, kDangerous | kShield));
    add_shape(new Box(vec2(), Lib::kHeight / 2, 7, 0xcc66ffff, 0, kDangerous | kShield));
    add_shape(new Box(vec2(), Lib::kHeight / 2, 4, 0xcc66ffff, 0, kDangerous | kShield));
  }
  set_bounding_width(fixed(Lib::kWidth));
}

GhostWall::GhostWall(bool swap, bool swap_gap)
: Enemy(vec2(swap ? -10 : 10 + Lib::kWidth, Lib::kHeight / 2), kShipNone, 0)
, dir_(swap ? 1 : -1, 0) {
  set_bounding_width(fixed(Lib::kHeight));
  fixed off = swap_gap ? -100 : 100;

  add_shape(new Box(vec2(0, off - 20 - Lib::kHeight / 2), 10, Lib::kHeight / 2, 0xcc66ffff, 0,
                    kDangerous | kShield));
  add_shape(new Box(vec2(0, off + 20 + Lib::kHeight / 2), 10, Lib::kHeight / 2, 0xcc66ffff, 0,
                    kDangerous | kShield));

  add_shape(new Box(vec2(0, off - 20 - Lib::kHeight / 2), 7, Lib::kHeight / 2, 0xcc66ffff, 0, 0));
  add_shape(new Box(vec2(0, off + 20 + Lib::kHeight / 2), 7, Lib::kHeight / 2, 0xcc66ffff, 0, 0));

  add_shape(new Box(vec2(0, off - 20 - Lib::kHeight / 2), 4, Lib::kHeight / 2, 0xcc66ffff, 0, 0));
  add_shape(new Box(vec2(0, off + 20 + Lib::kHeight / 2), 4, Lib::kHeight / 2, 0xcc66ffff, 0, 0));
}

void GhostWall::update() {
  if ((dir_.x > 0 && shape().centre.x < 32) || (dir_.y > 0 && shape().centre.y < 16) ||
      (dir_.x < 0 && shape().centre.x >= Lib::kWidth - 32) ||
      (dir_.y < 0 && shape().centre.y >= Lib::kHeight - 16)) {
    move(dir_ * kGbWallSpeed / 2);
  } else {
    move(dir_ * kGbWallSpeed);
  }

  if ((dir_.x > 0 && shape().centre.x > Lib::kWidth + 10) ||
      (dir_.y > 0 && shape().centre.y > Lib::kHeight + 10) ||
      (dir_.x < 0 && shape().centre.x < -10) || (dir_.y < 0 && shape().centre.y < -10)) {
    destroy();
  }
}

GhostMine::GhostMine(const vec2& position, Boss* ghost)
: Enemy(position, kShipNone, 0), timer_(80), ghost_(ghost) {
  add_shape(new Polygon(vec2(), 24, 8, 0x9933ccff, 0, 0));
  add_shape(new Polygon(vec2(), 20, 8, 0x9933ccff, 0, 0));
  set_bounding_width(24);
  set_score(0);
}

void GhostMine::update() {
  if (timer_ == 80) {
    explosion();
    shape().set_rotation(z::rand_fixed() * 2 * fixed_c::pi);
  }
  if (timer_) {
    timer_--;
    if (!timer_) {
      shapes()[0]->category = kDangerous | kShield | kVulnShield;
    }
  }
  for (const auto& ship : game().collision_list(shape().centre, kDangerous)) {
    if (ship == ghost_) {
      Enemy* e = z::rand_int(6) == 0 || (ghost_->is_hp_low() && z::rand_int(5) == 0)
          ? new BigFollow(shape().centre, false)
          : new Follow(shape().centre);
      e->set_score(0);
      spawn(e);
      damage(1, false, 0);
      break;
    }
  }
}

void GhostMine::render() const {
  if (!((timer_ / 4) % 2)) {
    Enemy::render();
  }
}
