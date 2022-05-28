#include "game/logic/boss/square.h"
#include "game/logic/enemy.h"
#include "game/logic/player.h"

namespace {
const std::int32_t kBsbBaseHp = 400;
const std::int32_t kBsbTimer = 100;
const std::int32_t kBsbSTimer = 80;
const std::int32_t kBsbAttackTime = 90;

const fixed kBsbSpeed = 2 + fixed(1) / 2;
const fixed kBsbAttackRadius = 120;
}  // namespace

BigSquareBoss::BigSquareBoss(std::int32_t players, std::int32_t cycle)
: Boss{{ii::kSimWidth * fixed_c::hundredth * 75, ii::kSimHeight * 2},
       ii::SimInterface::BOSS_1A,
       kBsbBaseHp,
       players,
       cycle}
, dir_{0, -1}
, timer_{kBsbTimer * 6} {
  add_new_shape<Polygon>(vec2{}, 160, 4, 0x9933ffff, 0, 0);
  add_new_shape<Polygon>(vec2{}, 140, 4, 0x9933ffff, 0, kDangerous);
  add_new_shape<Polygon>(vec2{}, 120, 4, 0x9933ffff, 0, kDangerous);
  add_new_shape<Polygon>(vec2{}, 100, 4, 0x9933ffff, 0, 0);
  add_new_shape<Polygon>(vec2{}, 80, 4, 0x9933ffff, 0, 0);
  add_new_shape<Polygon>(vec2{}, 60, 4, 0x9933ffff, 0, kVulnerable);

  add_new_shape<Polygon>(vec2{}, 155, 4, 0x9933ffff, 0, 0);
  add_new_shape<Polygon>(vec2{}, 135, 4, 0x9933ffff, 0, 0);
  add_new_shape<Polygon>(vec2{}, 115, 4, 0x9933ffff, 0, 0);
  add_new_shape<Polygon>(vec2{}, 95, 4, 0x6600ccff, 0, 0);
  add_new_shape<Polygon>(vec2{}, 75, 4, 0x6600ccff, 0, 0);
  add_new_shape<Polygon>(vec2{}, 55, 4, 0x330099ff, 0, kShield);
}

void BigSquareBoss::update() {
  const vec2& pos = shape().centre;
  if (pos.y < ii::kSimHeight * fixed_c::hundredth * 25 && dir_.y == -1) {
    dir_ = {reverse_ ? 1 : -1, 0};
  }
  if (pos.x < ii::kSimWidth * fixed_c::hundredth * 25 && dir_.x == -1) {
    dir_ = {0, reverse_ ? -1 : 1};
  }
  if (pos.y > ii::kSimHeight * fixed_c::hundredth * 75 && dir_.y == 1) {
    dir_ = {reverse_ ? -1 : 1, 0};
  }
  if (pos.x > ii::kSimWidth * fixed_c::hundredth * 75 && dir_.x == 1) {
    dir_ = {0, reverse_ ? 1 : -1};
  }

  if (special_attack_) {
    special_timer_--;
    if (attack_player_->is_killed()) {
      special_timer_ = 0;
      attack_player_ = 0;
    } else if (!special_timer_) {
      vec2 d(kBsbAttackRadius, 0);
      if (special_attack_rotate_) {
        d = d.rotated(fixed_c::pi / 2);
      }
      for (std::int32_t i = 0; i < 6; ++i) {
        auto s = std::make_unique<Follow>(attack_player_->shape().centre + d);
        s->shape().set_rotation(fixed_c::pi / 4);
        s->set_score(0);
        spawn(std::move(s));
        d = d.rotated(2 * fixed_c::pi / 6);
      }
      attack_player_ = 0;
      play_sound(ii::sound::kEnemySpawn);
    }
    if (!attack_player_) {
      special_attack_ = false;
    }
  } else if (is_on_screen()) {
    timer_--;
    if (timer_ <= 0) {
      timer_ = (z::rand_int(6) + 1) * kBsbTimer;
      dir_ = vec2{} - dir_;
      reverse_ = !reverse_;
    }
    ++spawn_timer_;
    std::int32_t t = (kBsbSTimer - sim().alive_players() * 10) / (is_hp_low() ? 2 : 1);
    if (spawn_timer_ >= t) {
      spawn_timer_ = 0;
      ++special_timer_;
      spawn_new<BigFollow>(shape().centre, false);
      play_sound_random(ii::sound::kBossFire);
    }
    if (special_timer_ >= 8 && z::rand_int(4)) {
      special_timer_ = kBsbAttackTime;
      special_attack_ = true;
      special_attack_rotate_ = z::rand_int(2) != 0;
      attack_player_ = nearest_player();
      play_sound(ii::sound::kBossAttack);
    }
  }

  if (special_attack_) {
    return;
  }

  move(dir_ * kBsbSpeed);
  for (std::size_t i = 0; i < 6; ++i) {
    shapes()[i]->rotate((i % 2 ? -1 : 1) * fixed_c::hundredth * ((1 + i) * 5));
  }
  for (std::size_t i = 0; i < 6; ++i) {
    shapes()[i + 6]->set_rotation(shapes()[i]->rotation());
  }
}

void BigSquareBoss::render() const {
  Boss::render();
  if ((special_timer_ / 4) % 2 && attack_player_) {
    fvec2 d(kBsbAttackRadius.to_float(), 0);
    if (special_attack_rotate_) {
      d = d.rotated(kPiFloat / 2);
    }
    for (std::int32_t i = 0; i < 6; ++i) {
      fvec2 p = to_float(attack_player_->shape().centre) + d;
      Polygon s{{}, 10, 4, 0x9933ffff, fixed_c::pi / 4, 0};
      s.render(sim(), p, 0);
      d = d.rotated(2 * kPiFloat / 6);
    }
  }
}

std::int32_t BigSquareBoss::get_damage(std::int32_t damage, bool magic) {
  return damage;
}