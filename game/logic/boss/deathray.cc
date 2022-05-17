#include "game/logic/boss/deathray.h"
#include "game/logic/player.h"

namespace {
const std::int32_t kDrbBaseHp = 600;
const std::int32_t kDrbArmHp = 100;
const std::int32_t kDrbRayTimer = 100;
const std::int32_t kDrbTimer = 100;
const std::int32_t kDrbArmATimer = 300;
const std::int32_t kDrbArmRTimer = 400;

const fixed kDrbSpeed = 5;
const fixed kDrbArmSpeed = 4;
const fixed kDrbRaySpeed = 10;
}  // namespace

DeathRayBoss::DeathRayBoss(std::int32_t players, std::int32_t cycle)
: Boss{{Lib::kWidth * (fixed(3) / 20), -Lib::kHeight},
       SimState::BOSS_2C,
       kDrbBaseHp,
       players,
       cycle}
, timer_{kDrbTimer * 2} {
  add_new_shape<Polygon>(vec2{}, 110, 12, 0x228855ff, fixed_c::pi / 12, 0, Polygon::T::kPolystar);
  add_new_shape<Polygon>(vec2{}, 70, 12, 0x33ff99ff, fixed_c::pi / 12, 0, Polygon::T::kPolygram);
  add_new_shape<Polygon>(vec2{}, 120, 12, 0x33ff99ff, fixed_c::pi / 12, kDangerous | kVulnerable);
  add_new_shape<Polygon>(vec2{}, 115, 12, 0x33ff99ff, fixed_c::pi / 12, 0);
  add_new_shape<Polygon>(vec2{}, 110, 12, 0x33ff99ff, fixed_c::pi / 12, kShield);

  auto s1 = std::make_unique<CompoundShape>(vec2{}, 0, kDangerous);
  for (std::int32_t i = 1; i < 12; ++i) {
    auto s2 = std::make_unique<CompoundShape>(vec2{}, i * fixed_c::pi / 6, 0);
    s2->add_new_shape<Box>(vec2{130, 0}, 10, 24, 0x33ff99ff, 0, 0);
    s2->add_new_shape<Box>(vec2{130, 0}, 8, 22, 0x228855ff, 0, 0);
    s1->add_shape(std::move(s2));
  }
  add_shape(std::move(s1));

  set_ignore_damage_colour_index(5);
  shape().rotate(2 * fixed_c::pi * fixed(z::rand_int()) / z::rand_max);
}

void DeathRayBoss::update() {
  bool positioned = true;
  fixed d = pos_ == 0 ? 1 * Lib::kHeight / 4 - shape().centre.y
      : pos_ == 1     ? 2 * Lib::kHeight / 4 - shape().centre.y
      : pos_ == 2     ? 3 * Lib::kHeight / 4 - shape().centre.y
      : pos_ == 3     ? 1 * Lib::kHeight / 8 - shape().centre.y
      : pos_ == 4     ? 3 * Lib::kHeight / 8 - shape().centre.y
      : pos_ == 5     ? 5 * Lib::kHeight / 8 - shape().centre.y
                      : 7 * Lib::kHeight / 8 - shape().centre.y;

  if (abs(d) > 3) {
    move(vec2{0, d / abs(d)} * kDrbSpeed);
    positioned = false;
  }

  if (ray_attack_timer_) {
    ray_attack_timer_--;
    if (ray_attack_timer_ == 40) {
      ray_dest_ = nearest_player()->shape().centre;
    }
    if (ray_attack_timer_ < 40) {
      vec2 d = (ray_dest_ - shape().centre).normalised();
      spawn_new<BossShot>(shape().centre, d * 10, 0xccccccff);
      play_sound_random(Lib::sound::kBossAttack);
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
        spawn_new<DeathRay>(shape().centre + vec2{100, 0});
        play_sound_random(Lib::sound::kBossFire);
      }
      if (!timer_) {
        laser_ = false;
        timer_ = kDrbTimer * (2 + z::rand_int(3));
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
      if (timer_ % kDrbTimer == 0 && timer_ != 0 && !z::rand_int(4)) {
        dir_ = z::rand_int(2) != 0;
        pos_ = z::rand_int(7);
      }
      if (timer_ == kDrbTimer * 2 + 50 && arms_.size() == 2) {
        ray_attack_timer_ = kDrbRayTimer;
        ray_src1_ = arms_[0]->shape().centre;
        ray_src2_ = arms_[1]->shape().centre;
        play_sound(Lib::sound::kEnemySpawn);
      }
    }
    if (timer_ <= 0) {
      laser_ = true;
      timer_ = 0;
      pos_ = z::rand_int(7);
    }
  }

  ++shot_timer_;
  if (arms_.empty() && !is_hp_low()) {
    if (shot_timer_ % 8 == 0) {
      shot_queue_.push_back(std::pair<std::int32_t, std::int32_t>((shot_timer_ / 8) % 12, 16));
      play_sound_random(Lib::sound::kBossFire);
    }
  }
  if (arms_.empty() && is_hp_low()) {
    if (shot_timer_ % 48 == 0) {
      for (std::int32_t i = 1; i < 16; i += 3) {
        shot_queue_.push_back(std::pair<std::int32_t, std::int32_t>(i, 16));
      }
      play_sound_random(Lib::sound::kBossFire);
    }
    if (shot_timer_ % 48 == 16) {
      for (std::int32_t i = 2; i < 12; i += 3) {
        shot_queue_.push_back(std::pair<std::int32_t, std::int32_t>(i, 16));
      }
      play_sound_random(Lib::sound::kBossFire);
    }
    if (shot_timer_ % 48 == 32) {
      for (std::int32_t i = 3; i < 12; i += 3) {
        shot_queue_.push_back(std::pair<std::int32_t, std::int32_t>(i, 16));
      }
      play_sound_random(Lib::sound::kBossFire);
    }
    if (shot_timer_ % 128 == 0) {
      ray_attack_timer_ = kDrbRayTimer;
      vec2 d1 = vec2::from_polar(z::rand_fixed() * 2 * fixed_c::pi, 110);
      vec2 d2 = vec2::from_polar(z::rand_fixed() * 2 * fixed_c::pi, 110);
      ray_src1_ = shape().centre + d1;
      ray_src2_ = shape().centre + d2;
      play_sound(Lib::sound::kEnemySpawn);
    }
  }
  if (arms_.empty()) {
    ++arm_timer_;
    if (arm_timer_ >= kDrbArmRTimer) {
      arm_timer_ = 0;
      if (!is_hp_low()) {
        std::int32_t players =
            game().get_lives() ? game().players().size() : game().alive_players();
        std::int32_t hp =
            (kDrbArmHp * (7 * fixed_c::tenth + 3 * fixed_c::tenth * players)).to_int();
        auto arm0 = std::make_unique<DeathArm>(this, true, hp);
        auto arm1 = std::make_unique<DeathArm>(this, false, hp);
        arms_.push_back(arm0.get());
        arms_.push_back(arm1.get());
        play_sound(Lib::sound::kEnemySpawn);
        spawn(std::move(arm0));
        spawn(std::move(arm1));
      }
    }
  }

  for (std::size_t i = 0; i < shot_queue_.size(); ++i) {
    if (!going_fast || shot_timer_ % 2) {
      std::int32_t n = shot_queue_[i].first;
      vec2 d = vec2{1, 0}.rotated(shape().rotation() + n * fixed_c::pi / 6);
      spawn_new<BossShot>(shape().centre + d * 120, d * 5, 0x33ff99ff);
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
  for (std::int32_t i = ray_attack_timer_ - 8; i <= ray_attack_timer_ + 8; ++i) {
    if (i < 40 || i > kDrbRayTimer) {
      continue;
    }

    fvec2 pos = to_float(shape().centre);
    fvec2 d = to_float(ray_src1_) - pos;
    d *= static_cast<float>(i - 40) / (kDrbRayTimer - 40);
    Polygon s{{}, 10, 6, 0x999999ff, 0, 0, Polygon::T::kPolystar};
    s.render(lib(), d + pos, 0);

    d = to_float(ray_src2_) - pos;
    d *= static_cast<float>(i - 40) / (kDrbRayTimer - 40);
    Polygon s2{{}, 10, 6, 0x999999ff, 0, 0, Polygon::T::kPolystar};
    s2.render(lib(), d + pos, 0);
  }
}

std::int32_t DeathRayBoss::get_damage(std::int32_t damage, bool magic) {
  return arms_.size() ? 0 : damage;
}

void DeathRayBoss::on_arm_death(Ship* arm) {
  for (auto it = arms_.begin(); it != arms_.end(); ++it) {
    if (*it == arm) {
      arms_.erase(it);
      break;
    }
  }
}

DeathRay::DeathRay(const vec2& position) : Enemy{position, kShipNone, 0} {
  add_new_shape<Box>(vec2{}, 10, 48, 0, 0, kDangerous);
  add_new_shape<Line>(vec2{}, vec2{0, -48}, vec2{0, 48}, 0xffffffff, 0);
  set_bounding_width(48);
}

void DeathRay::update() {
  move(vec2{1, 0} * kDrbRaySpeed);
  if (shape().centre.x > Lib::kWidth + 20) {
    destroy();
  }
}

DeathArm::DeathArm(DeathRayBoss* boss, bool top, std::int32_t hp)
: Enemy{{}, kShipNone, hp}
, boss_{boss}
, top_{top}
, timer_{top ? 2 * kDrbArmATimer / 3 : 0}
, start_{30} {
  add_new_shape<Polygon>(vec2{}, 60, 4, 0x33ff99ff, 0, 0);
  add_new_shape<Polygon>(vec2{}, 50, 4, 0x228855ff, 0, kVulnerable, Polygon::T::kPolygram);
  add_new_shape<Polygon>(vec2{}, 40, 4, 0, 0, kShield);
  add_new_shape<Polygon>(vec2{}, 20, 4, 0x33ff99ff, 0, 0);
  add_new_shape<Polygon>(vec2{}, 18, 4, 0x228855ff, 0, 0);
  set_bounding_width(60);
  set_destroy_sound(Lib::sound::kPlayerDestroy);
}

void DeathArm::update() {
  if (timer_ % (kDrbArmATimer / 2) == kDrbArmATimer / 4) {
    play_sound_random(Lib::sound::kBossFire);
    target_ = nearest_player()->shape().centre;
    shots_ = 16;
  }
  if (shots_ > 0) {
    vec2 d = (target_ - shape().centre).normalised() * 5;
    spawn_new<BossShot>(shape().centre, d, 0x33ff99ff);
    --shots_;
  }

  shape().rotate(fixed_c::hundredth * 5);
  if (attacking_) {
    ++timer_;
    if (timer_ < kDrbArmATimer / 4) {
      Player* p = nearest_player();
      vec2 d = p->shape().centre - shape().centre;
      if (d.length() != 0) {
        dir_ = d.normalised();
        move(dir_ * kDrbArmSpeed);
      }
    } else if (timer_ < kDrbArmATimer / 2) {
      move(dir_ * kDrbArmSpeed);
    } else if (timer_ < kDrbArmATimer) {
      vec2 d = boss_->shape().centre + vec2{80, top_ ? 80 : -80} - shape().centre;
      if (d.length() > kDrbArmSpeed) {
        move(d.normalised() * kDrbArmSpeed);
      } else {
        attacking_ = false;
        timer_ = 0;
      }
    } else if (timer_ >= kDrbArmATimer) {
      attacking_ = false;
      timer_ = 0;
    }
    return;
  }

  ++timer_;
  if (timer_ >= kDrbArmATimer) {
    timer_ = 0;
    attacking_ = true;
    dir_ = {};
    play_sound(Lib::sound::kBossAttack);
  }
  shape().centre = boss_->shape().centre + vec2{80, top_ ? 80 : -80};

  if (start_) {
    if (start_ == 30) {
      explosion();
      explosion(0xffffffff);
    }
    start_--;
    if (!start_) {
      shapes()[1]->category = kDangerous | kVulnerable;
    }
  }
}

void DeathArm::on_destroy(bool bomb) {
  boss_->on_arm_death(this);
  explosion();
  explosion(0xffffffff, 12);
  explosion(shapes()[0]->colour, 24);
}
