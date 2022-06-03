#include "game/logic/boss/ghost.h"
#include "game/logic/player.h"

namespace {
const std::int32_t kGbBaseHp = 700;
const std::int32_t kGbTimer = 250;
const std::int32_t kGbAttackTime = 100;
const fixed kGbWallSpeed = 3 + 1_fx / 2;
}  // namespace

GhostBoss::GhostBoss(ii::SimInterface& sim, std::int32_t players, std::int32_t cycle)
: Boss{sim,  {ii::kSimWidth / 2, ii::kSimHeight / 2}, ii::SimInterface::kBoss2B, kGbBaseHp, players,
       cycle}
, attack_time_{kGbAttackTime} {
  add_new_shape<Polygon>(vec2{0}, 32, 8, 0x9933ccff, 0, 0);
  add_new_shape<Polygon>(vec2{0}, 48, 8, 0xcc66ffff, 0, 0);

  for (std::int32_t i = 0; i < 8; ++i) {
    vec2 c = from_polar(i * fixed_c::pi / 4, 48_fx);

    auto* s = add_new_shape<CompoundShape>(c, 0, 0);
    for (std::int32_t j = 0; j < 8; ++j) {
      vec2 d = from_polar(j * fixed_c::pi / 4, 1_fx);
      s->add_new_shape<Line>(vec2{0}, d * 10, d * 10 * 2, 0x9933ccff, 0);
    }
  }

  add_new_shape<Polygon>(vec2{0}, 24, 8, 0xcc66ffff, 0, 0, Polygon::T::kPolygram);

  for (std::int32_t n = 0; n < 5; ++n) {
    auto* s = add_new_shape<CompoundShape>(vec2{0}, 0, 0);
    for (std::int32_t i = 0; i < 16 + n * 6; ++i) {
      vec2 d = from_polar(i * 2 * fixed_c::pi / (16 + n * 6), 100_fx + n * 60);
      s->add_new_shape<Polygon>(d, 16, 8, n ? 0x330066ff : 0x9933ccff, 0, 0, Polygon::T::kPolystar);
      s->add_new_shape<Polygon>(d, 12, 8, n ? 0x330066ff : 0x9933ccff);
    }
  }

  for (std::int32_t n = 0; n < 5; ++n) {
    add_new_shape<CompoundShape>(vec2{0}, 0, kDangerous);
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
          vec2 d = from_polar(i * 2 * fixed_c::pi / (16 + n * 6), 100_fx + n * 60);
          c->add_new_shape<Polygon>(d, 9, 8, 0);
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

  for (std::size_t i = 0; i < 8; ++i) {
    shapes()[i + 2]->rotate(fixed_c::hundredth * 4);
  }
  shapes()[10]->rotate(fixed_c::hundredth * 6);
  for (std::int32_t n = 0; n < 5; ++n) {
    CompoundShape* s = (CompoundShape*)shapes()[11 + n].get();
    if (n % 2) {
      s->rotate(fixed_c::hundredth * 3 + (3_fx / 2000) * n);
    } else {
      s->rotate(fixed_c::hundredth * 5 - (3_fx / 2000) * n);
    }
    for (const auto& t : s->shapes()) {
      t->rotate(-fixed_c::tenth);
    }

    s = (CompoundShape*)shapes()[16 + n].get();
    if (n % 2) {
      s->rotate(fixed_c::hundredth * 3 + (3_fx / 2000) * n);
    } else {
      s->rotate(fixed_c::hundredth * 4 - (3_fx / 2000) * n);
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
    ++timer_;
    if (timer_ == 9 * kGbTimer / 10 ||
        (timer_ >= kGbTimer / 10 && timer_ < 9 * kGbTimer / 10 - 16 &&
         ((timer_ % 16 == 0 && attack_ == 2) || (timer_ % 32 == 0 && shot_type_)))) {
      for (std::int32_t i = 0; i < 8; ++i) {
        auto d = from_polar(i * fixed_c::pi / 4 + shape().rotation(), 5_fx);
        spawn_new<BossShot>(shape().centre, d, 0xcc66ffff);
      }
      play_sound_random(ii::sound::kBossFire);
    }
    if (timer_ != 9 * kGbTimer / 10 && timer_ >= kGbTimer / 10 && timer_ < 9 * kGbTimer / 10 - 16 &&
        timer_ % 16 == 0 && (!shot_type_ || attack_ == 2)) {
      Player* p = nearest_player();
      auto d = normalise(p->shape().centre - shape().centre);

      if (length(d) > fixed_c::half) {
        spawn_new<BossShot>(shape().centre, d * 5, 0xcc66ffff);
        spawn_new<BossShot>(shape().centre, d * -5, 0xcc66ffff);
        play_sound_random(ii::sound::kBossFire);
      }
    }
  } else {
    attack_time_--;

    if (attack_ == 2) {
      if (attack_time_ >= kGbAttackTime * 4 && attack_time_ % 8 == 0) {
        vec2 pos(sim().random(ii::kSimWidth + 1), sim().random(ii::kSimHeight + 1));
        spawn_new<GhostMine>(pos, this);
        play_sound_random(ii::sound::kEnemySpawn);
        danger_circle_ = 0;
      } else if (attack_time_ == kGbAttackTime * 4 - 1) {
        visible_ = true;
        vtime_ = 60;
        add_new_shape<Box>(vec2{ii::kSimWidth / 2 + 32, 0}, ii::kSimWidth / 2, 10, 0xcc66ffff, 0,
                           0);
        add_new_shape<Box>(vec2{ii::kSimWidth / 2 + 32, 0}, ii::kSimWidth / 2, 7, 0xcc66ffff, 0, 0);
        add_new_shape<Box>(vec2{ii::kSimWidth / 2 + 32, 0}, ii::kSimWidth / 2, 4, 0xcc66ffff, 0, 0);

        add_new_shape<Box>(vec2{-ii::kSimWidth / 2 - 32, 0}, ii::kSimWidth / 2, 10, 0xcc66ffff, 0,
                           0);
        add_new_shape<Box>(vec2{-ii::kSimWidth / 2 - 32, 0}, ii::kSimWidth / 2, 7, 0xcc66ffff, 0,
                           0);
        add_new_shape<Box>(vec2{-ii::kSimWidth / 2 - 32, 0}, ii::kSimWidth / 2, 4, 0xcc66ffff, 0,
                           0);
        play_sound(ii::sound::kBossFire);
      } else if (attack_time_ < kGbAttackTime * 3) {
        if (attack_time_ == kGbAttackTime * 3 - 1) {
          play_sound(ii::sound::kBossAttack);
        }
        shape().rotate((_rdir ? 1 : -1) * 2 * fixed_c::pi / (kGbAttackTime * 6));
        if (!attack_time_) {
          for (std::int32_t i = 0; i < 6; ++i) {
            destroy_shape(21);
          }
        }
        danger_offset1_ = sim().random(11);
        danger_offset2_ = sim().random(7);
        danger_offset3_ = sim().random(17);
        danger_offset3_ = sim().random(10);
      }
    }

    if (attack_ == 0 && attack_time_ == kGbAttackTime * 2) {
      bool r = sim().random(2) != 0;
      spawn_new<GhostWall>(r, true);
      spawn_new<GhostWall>(!r, false);
    }
    if (attack_ == 1 && attack_time_ == kGbAttackTime * 2) {
      _rdir = sim().random(2) != 0;
      spawn_new<GhostWall>(!_rdir, false, 0);
    }
    if (attack_ == 1 && attack_time_ == kGbAttackTime * 1) {
      spawn_new<GhostWall>(_rdir, true, 0);
    }

    if ((attack_ == 0 || attack_ == 1) && is_hp_low() && attack_time_ == kGbAttackTime * 1) {
      std::int32_t r = sim().random(4);
      vec2 v = r == 0 ? vec2{-ii::kSimWidth / 4, ii::kSimHeight / 2}
          : r == 1    ? vec2{ii::kSimWidth + ii::kSimWidth / 4, ii::kSimHeight / 2}
          : r == 2    ? vec2{ii::kSimWidth / 2, -ii::kSimHeight / 4}
                      : vec2{ii::kSimWidth / 2, ii::kSimHeight + ii::kSimHeight / 4};
      spawn_new<BigFollow>(v, false);
    }
    if (!attack_time_ && !visible_) {
      visible_ = true;
      vtime_ = 60;
    }
    if (attack_time_ == 0 && attack_ != 2) {
      std::int32_t r = sim().random(3);
      danger_circle_ |= 1 + (r == 2 ? 3 : r);
      if (sim().random(2) || is_hp_low()) {
        r = sim().random(3);
        danger_circle_ |= 1 + (r == 2 ? 3 : r);
      }
      if (sim().random(2) || is_hp_low()) {
        r = sim().random(3);
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
    attack_ = sim().random(3);
    shot_type_ = sim().random(2) == 0;
    shapes()[0]->category = 0;
    shapes()[1]->category = 0;
    shapes()[11]->category = 0;
    if (shapes().size() >= 22) {
      shapes()[21]->category = 0;
      shapes()[24]->category = 0;
    }
    play_sound(ii::sound::kBossAttack);

    if (attack_ == 0) {
      attack_time_ = kGbAttackTime * 2 + 50;
    }
    if (attack_ == 1) {
      attack_time_ = kGbAttackTime * 3;
      for (std::size_t i = 0; i < sim().players().size(); ++i) {
        spawn_new<Powerup>(shape().centre, Powerup::type::kShield);
      }
    }
    if (attack_ == 2) {
      attack_time_ = kGbAttackTime * 6;
      _rdir = sim().random(2) != 0;
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

GhostWall::GhostWall(ii::SimInterface& sim, bool swap, bool no_gap, bool ignored)
: Enemy{sim, {ii::kSimWidth / 2, swap ? -10 : 10 + ii::kSimHeight}, kShipNone, 0}
, dir_{0, swap ? 1 : -1} {
  if (no_gap) {
    add_new_shape<Box>(vec2{0}, fixed{ii::kSimWidth}, 10, 0xcc66ffff, 0, kDangerous | kShield);
    add_new_shape<Box>(vec2{0}, fixed{ii::kSimWidth}, 7, 0xcc66ffff, 0, 0);
    add_new_shape<Box>(vec2{0}, fixed{ii::kSimWidth}, 4, 0xcc66ffff, 0, 0);
  } else {
    add_new_shape<Box>(vec2{0}, ii::kSimHeight / 2, 10, 0xcc66ffff, 0, kDangerous | kShield);
    add_new_shape<Box>(vec2{0}, ii::kSimHeight / 2, 7, 0xcc66ffff, 0, kDangerous | kShield);
    add_new_shape<Box>(vec2{0}, ii::kSimHeight / 2, 4, 0xcc66ffff, 0, kDangerous | kShield);
  }
  set_bounding_width(fixed{ii::kSimWidth});
}

GhostWall::GhostWall(ii::SimInterface& sim, bool swap, bool swap_gap)
: Enemy{sim, {swap ? -10 : 10 + ii::kSimWidth, ii::kSimHeight / 2}, kShipNone, 0}
, dir_{swap ? 1 : -1, 0} {
  set_bounding_width(fixed{ii::kSimHeight});
  fixed off = swap_gap ? -100 : 100;

  add_new_shape<Box>(vec2{0, off - 20 - ii::kSimHeight / 2}, 10, ii::kSimHeight / 2, 0xcc66ffff, 0,
                     kDangerous | kShield);
  add_new_shape<Box>(vec2{0, off + 20 + ii::kSimHeight / 2}, 10, ii::kSimHeight / 2, 0xcc66ffff, 0,
                     kDangerous | kShield);

  add_new_shape<Box>(vec2{0, off - 20 - ii::kSimHeight / 2}, 7, ii::kSimHeight / 2, 0xcc66ffff, 0,
                     0);
  add_new_shape<Box>(vec2{0, off + 20 + ii::kSimHeight / 2}, 7, ii::kSimHeight / 2, 0xcc66ffff, 0,
                     0);

  add_new_shape<Box>(vec2{0, off - 20 - ii::kSimHeight / 2}, 4, ii::kSimHeight / 2, 0xcc66ffff, 0,
                     0);
  add_new_shape<Box>(vec2{0, off + 20 + ii::kSimHeight / 2}, 4, ii::kSimHeight / 2, 0xcc66ffff, 0,
                     0);
}

void GhostWall::update() {
  if ((dir_.x > 0 && shape().centre.x < 32) || (dir_.y > 0 && shape().centre.y < 16) ||
      (dir_.x < 0 && shape().centre.x >= ii::kSimWidth - 32) ||
      (dir_.y < 0 && shape().centre.y >= ii::kSimHeight - 16)) {
    move(dir_ * kGbWallSpeed / 2);
  } else {
    move(dir_ * kGbWallSpeed);
  }

  if ((dir_.x > 0 && shape().centre.x > ii::kSimWidth + 10) ||
      (dir_.y > 0 && shape().centre.y > ii::kSimHeight + 10) ||
      (dir_.x < 0 && shape().centre.x < -10) || (dir_.y < 0 && shape().centre.y < -10)) {
    destroy();
  }
}

GhostMine::GhostMine(ii::SimInterface& sim, const vec2& position, Boss* ghost)
: Enemy{sim, position, kShipNone, 0}, ghost_{ghost} {
  add_new_shape<Polygon>(vec2{0}, 24, 8, 0x9933ccff, 0, 0);
  add_new_shape<Polygon>(vec2{0}, 20, 8, 0x9933ccff, 0, 0);
  set_bounding_width(24);
  set_score(0);
}

void GhostMine::update() {
  if (timer_ == 80) {
    explosion();
    shape().set_rotation(sim().random_fixed() * 2 * fixed_c::pi);
  }
  if (timer_) {
    timer_--;
    if (!timer_) {
      shapes()[0]->category = kDangerous | kShield | kVulnShield;
    }
  }
  for (const auto& ship : sim().collision_list(shape().centre, kDangerous)) {
    if (ship == ghost_) {
      Enemy* e;
      if (sim().random(6) == 0 || (ghost_->is_hp_low() && sim().random(5) == 0)) {
        e = spawn_new<BigFollow>(shape().centre, false);
      } else {
        e = spawn_new<Follow>(shape().centre);
      }
      e->set_score(0);
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
