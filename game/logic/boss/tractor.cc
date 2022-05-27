#include "game/logic/boss/tractor.h"
#include "game/logic/player.h"

namespace {
const std::int32_t kTbBaseHp = 900;
const std::int32_t kTbTimer = 100;
const fixed kTbSpeed = 2;
}  // namespace

TractorBoss::TractorBoss(std::int32_t players, std::int32_t cycle)
: Boss{{ii::kSimWidth * (1 + fixed_c::half), ii::kSimHeight / 2},
       SimState::BOSS_2A,
       kTbBaseHp,
       players,
       cycle}
, shoot_type_{z::rand_int(2)} {
  auto s1 = std::make_unique<CompoundShape>(vec2{0, -96}, 0, kDangerous | kVulnerable);
  s1_ = s1.get();

  s1_->add_new_shape<Polygon>(vec2{}, 12, 6, 0x882288ff, 0, 0, Polygon::T::kPolygram);
  s1_->add_new_shape<Polygon>(vec2{}, 12, 12, 0x882288ff, 0, 0);
  s1_->add_new_shape<Polygon>(vec2{}, 2, 6, 0x882288ff, 0, 0);
  s1_->add_new_shape<Polygon>(vec2{}, 36, 12, 0xcc33ccff, 0, 0);
  s1_->add_new_shape<Polygon>(vec2{}, 34, 12, 0xcc33ccff, 0, 0);
  s1_->add_new_shape<Polygon>(vec2{}, 32, 12, 0xcc33ccff, 0, 0);
  for (std::int32_t i = 0; i < 8; ++i) {
    vec2 d = vec2{24, 0}.rotated(i * fixed_c::pi / 4);
    s1_->add_new_shape<Polygon>(d, 12, 6, 0xcc33ccff, 0, 0, Polygon::T::kPolygram);
  }

  auto s2 = std::make_unique<CompoundShape>(vec2{0, 96}, 0, kDangerous | kVulnerable);
  s2_ = s2.get();

  s2_->add_new_shape<Polygon>(vec2{}, 12, 6, 0x882288ff, 0, 0, Polygon::T::kPolygram);
  s2_->add_new_shape<Polygon>(vec2{}, 12, 12, 0x882288ff, 0, 0);
  s2_->add_new_shape<Polygon>(vec2{}, 2, 6, 0x882288ff, 0, 0);

  s2_->add_new_shape<Polygon>(vec2{}, 36, 12, 0xcc33ccff, 0, 0);
  s2_->add_new_shape<Polygon>(vec2{}, 34, 12, 0xcc33ccff, 0, 0);
  s2_->add_new_shape<Polygon>(vec2{}, 32, 12, 0xcc33ccff, 0, 0);
  for (std::int32_t i = 0; i < 8; ++i) {
    vec2 d = vec2{24, 0}.rotated(i * fixed_c::pi / 4);
    s2_->add_new_shape<Polygon>(d, 12, 6, 0xcc33ccff, 0, 0, Polygon::T::kPolygram);
  }

  add_shape(std::move(s1));
  add_shape(std::move(s2));

  auto sattack = std::make_unique<Polygon>(vec2{}, 0, 16, 0x993399ff);
  sattack_ = sattack.get();
  add_shape(std::move(sattack));

  add_new_shape<Line>(vec2{}, vec2{-2, -96}, vec2{-2, 96}, 0xcc33ccff, 0);
  add_new_shape<Line>(vec2{}, vec2{0, -96}, vec2{0, 96}, 0x882288ff, 0);
  add_new_shape<Line>(vec2{}, vec2{2, -96}, vec2{2, 96}, 0xcc33ccff, 0);

  add_new_shape<Polygon>(vec2{0, 96}, 30, 12, 0, 0, kShield);
  add_new_shape<Polygon>(vec2{0, -96}, 30, 12, 0, 0, kShield);

  attack_shapes_ = shapes().size();
}

void TractorBoss::update() {
  if (shape().centre.x <= ii::kSimWidth / 2 && will_attack_ && !stopped_ && !continue_) {
    stopped_ = true;
    generating_ = true;
    gen_dir_ = z::rand_int(2) == 0;
    timer_ = 0;
  }

  if (shape().centre.x < -150) {
    shape().centre.x = ii::kSimWidth + 150;
    will_attack_ = !will_attack_;
    shoot_type_ = z::rand_int(2);
    if (will_attack_) {
      shoot_type_ = 0;
    }
    continue_ = false;
    sound_ = !will_attack_;
    shape().rotate(z::rand_fixed() * 2 * fixed_c::pi);
  }

  ++timer_;
  if (!stopped_) {
    move(kTbSpeed * vec2{-1, 0});
    if (!will_attack_ && is_on_screen() && timer_ % (16 - sim().state().alive_players() * 2) == 0) {
      if (shoot_type_ == 0 || (is_hp_low() && shoot_type_ == 1)) {
        Player* p = nearest_player();

        vec2 v = s1_->convert_point(shape().centre, shape().rotation(), {});
        vec2 d = (p->shape().centre - v).normalised();
        spawn_new<BossShot>(v, d * 5, 0xcc33ccff);
        spawn_new<BossShot>(v, d * -5, 0xcc33ccff);

        v = s2_->convert_point(shape().centre, shape().rotation(), {});
        d = (p->shape().centre - v).normalised();
        spawn_new<BossShot>(v, d * 5, 0xcc33ccff);
        spawn_new<BossShot>(v, d * -5, 0xcc33ccff);

        play_sound_random(ii::sound::kBossFire);
      }
      if (shoot_type_ == 1 || is_hp_low()) {
        Player* p = nearest_player();
        vec2 v = shape().centre;

        vec2 d = (p->shape().centre - v).normalised();
        spawn_new<BossShot>(v, d * 5, 0xcc33ccff);
        spawn_new<BossShot>(v, d * -5, 0xcc33ccff);
        play_sound_random(ii::sound::kBossFire);
      }
    }
    if ((!will_attack_ || continue_) && is_on_screen()) {
      if (sound_) {
        play_sound(ii::sound::kBossAttack);
        sound_ = false;
      }
      targets_.clear();
      for (const auto& ship : sim().state().all_ships(kShipPlayer)) {
        if (((Player*)ship)->is_killed()) {
          continue;
        }
        vec2 pos = ship->shape().centre;
        targets_.push_back(pos);
        vec2 d = (shape().centre - pos).normalised();
        ship->move(d * Tractor::kTractorBeamSpeed);
      }
    }
  } else {
    if (generating_) {
      if (timer_ >= kTbTimer * 5) {
        timer_ = 0;
        generating_ = false;
        attacking_ = false;
        attack_size_ = 0;
        play_sound(ii::sound::kBossAttack);
      }

      if (timer_ < kTbTimer * 4 && timer_ % (10 - 2 * sim().state().alive_players()) == 0) {
        spawn_new<TBossShot>(s1_->convert_point(shape().centre, shape().rotation(), {}),
                             gen_dir_ ? shape().rotation() + fixed_c::pi : shape().rotation());

        spawn_new<TBossShot>(s2_->convert_point(shape().centre, shape().rotation(), {}),
                             shape().rotation() + (gen_dir_ ? 0 : fixed_c::pi));
        play_sound_random(ii::sound::kEnemySpawn);
      }

      if (is_hp_low() && timer_ % (20 - sim().state().alive_players() * 2) == 0) {
        Player* p = nearest_player();
        vec2 v = shape().centre;

        vec2 d = (p->shape().centre - v).normalised();
        spawn_new<BossShot>(v, d * 5, 0xcc33ccff);
        spawn_new<BossShot>(v, d * -5, 0xcc33ccff);
        play_sound_random(ii::sound::kBossFire);
      }
    } else {
      if (!attacking_) {
        if (timer_ >= kTbTimer * 4) {
          timer_ = 0;
          attacking_ = true;
        }
        if (timer_ % (kTbTimer / (1 + fixed_c::half)).to_int() == kTbTimer / 8) {
          vec2 v = s1_->convert_point(shape().centre, shape().rotation(), {});
          vec2 d = vec2::from_polar(z::rand_fixed() * (2 * fixed_c::pi), 5);
          spawn_new<BossShot>(v, d, 0xcc33ccff);
          d = d.rotated(fixed_c::pi / 2);
          spawn_new<BossShot>(v, d, 0xcc33ccff);
          d = d.rotated(fixed_c::pi / 2);
          spawn_new<BossShot>(v, d, 0xcc33ccff);
          d = d.rotated(fixed_c::pi / 2);
          spawn_new<BossShot>(v, d, 0xcc33ccff);

          v = s2_->convert_point(shape().centre, shape().rotation(), {});
          d = vec2::from_polar(z::rand_fixed() * (2 * fixed_c::pi), 5);
          spawn_new<BossShot>(v, d, 0xcc33ccff);
          d = d.rotated(fixed_c::pi / 2);
          spawn_new<BossShot>(v, d, 0xcc33ccff);
          d = d.rotated(fixed_c::pi / 2);
          spawn_new<BossShot>(v, d, 0xcc33ccff);
          d = d.rotated(fixed_c::pi / 2);
          spawn_new<BossShot>(v, d, 0xcc33ccff);
          play_sound_random(ii::sound::kBossFire);
        }
        targets_.clear();
        for (const auto& ship : sim().state().all_ships(kShipPlayer | kShipEnemy)) {
          if (ship == this || ((ship->type() & kShipPlayer) && ((Player*)ship)->is_killed())) {
            continue;
          }

          if (ship->type() & kShipEnemy) {
            play_sound_random(ii::sound::kBossAttack, 0, 0.3f);
          }
          vec2 pos = ship->shape().centre;
          targets_.push_back(pos);
          fixed speed = 0;
          if (ship->type() & kShipPlayer) {
            speed = Tractor::kTractorBeamSpeed;
          }
          if (ship->type() & kShipEnemy) {
            speed = 4 + fixed_c::half;
          }
          vec2 d = (shape().centre - pos).normalised();
          ship->move(d * speed);

          if ((ship->type() & kShipEnemy) && !(ship->type() & kShipWall) &&
              (ship->shape().centre - shape().centre).length() <= 40) {
            ship->destroy();
            ++attack_size_;
            sattack_->radius = attack_size_ / (1 + fixed_c::half);
            add_new_shape<Polygon>(vec2{}, 8, 6, 0xcc33ccff, 0, 0);
          }
        }
      } else {
        timer_ = 0;
        stopped_ = false;
        continue_ = true;
        for (std::int32_t i = 0; i < attack_size_; ++i) {
          vec2 d = vec2::from_polar(i * (2 * fixed_c::pi) / attack_size_, 5);
          spawn_new<BossShot>(shape().centre, d, 0xcc33ccff);
        }
        play_sound(ii::sound::kBossFire);
        play_sound_random(ii::sound::kExplosion);
        attack_size_ = 0;
        sattack_->radius = 0;
        while (attack_shapes_ < shapes().size()) {
          destroy_shape(attack_shapes_);
        }
      }
    }
  }

  for (std::size_t i = attack_shapes_; i < shapes().size(); ++i) {
    vec2 v = vec2::from_polar(
        z::rand_fixed() * (2 * fixed_c::pi),
        2 * (z::rand_fixed() - fixed_c::half) * fixed(attack_size_) / (1 + fixed_c::half));
    shapes()[i]->centre = v;
  }

  fixed r = 0;
  if (!will_attack_ || (stopped_ && !generating_ && !attacking_)) {
    r = fixed_c::hundredth;
  } else if (stopped_ && attacking_ && !generating_) {
    r = 0;
  } else {
    r = fixed_c::hundredth / 2;
  }
  shape().rotate(r);

  s1_->rotate(fixed_c::tenth / 2);
  s2_->rotate(-fixed_c::tenth / 2);
  for (std::size_t i = 0; i < 8; ++i) {
    s1_->shapes()[4 + i]->rotate(-fixed_c::tenth);
    s2_->shapes()[4 + i]->rotate(fixed_c::tenth);
  }
}

void TractorBoss::render() const {
  Boss::render();
  if ((stopped_ && !generating_ && !attacking_) ||
      (!stopped_ && (continue_ || !will_attack_) && is_on_screen())) {
    for (std::size_t i = 0; i < targets_.size(); ++i) {
      if (((timer_ + i * 4) / 4) % 2) {
        sim().lib().render_line(to_float(shape().centre), to_float(targets_[i]), 0xcc33ccff);
      }
    }
  }
}

std::int32_t TractorBoss::get_damage(std::int32_t damage, bool magic) {
  return damage;
}

TBossShot::TBossShot(const vec2& position, fixed angle) : Enemy{position, kShipNone, 1} {
  add_new_shape<Polygon>(vec2{}, 8, 6, 0xcc33ccff, 0, kDangerous | kVulnerable);
  dir_ = vec2::from_polar(angle, 3);
  set_bounding_width(8);
  set_score(0);
  set_destroy_sound(ii::sound::kEnemyShatter);
}

void TBossShot::update() {
  if ((shape().centre.x > ii::kSimWidth && dir_.x > 0) || (shape().centre.x < 0 && dir_.x < 0)) {
    dir_.x = -dir_.x;
  }
  if ((shape().centre.y > ii::kSimHeight && dir_.y > 0) || (shape().centre.y < 0 && dir_.y < 0)) {
    dir_.y = -dir_.y;
  }

  move(dir_);
}