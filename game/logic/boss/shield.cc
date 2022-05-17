#include "game/logic/boss/shield.h"
#include "game/logic/enemy.h"
#include "game/logic/player.h"

namespace {
const std::int32_t kSbbBaseHp = 320;
const std::int32_t kSbbTimer = 100;
const std::int32_t kSbbUnshieldTime = 300;
const std::int32_t kSbbAttackTime = 80;
const fixed kSbbSpeed = 1;
}  // namespace

ShieldBombBoss::ShieldBombBoss(std::int32_t players, std::int32_t cycle)
: Boss(vec2(-Lib::kWidth / 2, Lib::kHeight / 2), SimState::BOSS_1B, kSbbBaseHp, players, cycle)
, timer_(0)
, count_(0)
, unshielded_(0)
, attack_(0)
, side_(false)
, shot_alternate_(false) {
  add_shape(
      new Polygon(vec2(), 48, 8, 0x339966ff, 0, kDangerous | kVulnerable, Polygon::T::kPolygram));

  for (std::int32_t i = 0; i < 16; i++) {
    vec2 a = vec2(120, 0).rotated(i * fixed_c::pi / 8);
    vec2 b = vec2(80, 0).rotated(i * fixed_c::pi / 8);

    add_shape(new Line(vec2(), a, b, 0x999999ff, 0));
  }

  add_shape(new Polygon(vec2(), 130, 16, 0xccccccff, 0, kVulnShield | kDangerous));
  add_shape(new Polygon(vec2(), 125, 16, 0xccccccff, 0, 0));
  add_shape(new Polygon(vec2(), 120, 16, 0xccccccff, 0, 0));

  add_shape(new Polygon(vec2(), 42, 16, 0, 0, kShield));

  set_ignore_damage_colour_index(1);
}

void ShieldBombBoss::update() {
  if (!side_ && shape().centre.x < Lib::kWidth * fixed_c::tenth * 6) {
    move(vec2(1, 0) * kSbbSpeed);
  } else if (side_ && shape().centre.x > Lib::kWidth * fixed_c::tenth * 4) {
    move(vec2(-1, 0) * kSbbSpeed);
  }

  shape().rotate(-fixed_c::hundredth * 2);
  shapes()[0]->rotate(-fixed_c::hundredth * 4);
  shapes()[20]->set_rotation(shapes()[0]->rotation());

  if (!is_on_screen()) {
    return;
  }

  if (is_hp_low()) {
    timer_++;
  }

  if (unshielded_) {
    timer_++;

    unshielded_--;
    for (std::size_t i = 0; i < 3; i++) {
      shapes()[i + 17]->colour = (unshielded_ / 2) % 4 ? 0x00000000 : 0x666666ff;
    }
    for (std::size_t i = 0; i < 16; i++) {
      shapes()[i + 1]->colour = (unshielded_ / 2) % 4 ? 0x00000000 : 0x333333ff;
    }

    if (!unshielded_) {
      shapes()[0]->category = kDangerous | kVulnerable;
      shapes()[17]->category = kDangerous | kVulnShield;

      for (std::size_t i = 0; i < 3; i++) {
        shapes()[i + 17]->colour = 0xccccccff;
      }
      for (std::size_t i = 0; i < 16; i++) {
        shapes()[i + 1]->colour = 0x999999ff;
      }
    }
  }

  if (attack_) {
    vec2 d = attack_dir_.rotated((kSbbAttackTime - attack_) * fixed_c::half * fixed_c::pi /
                                 kSbbAttackTime);
    spawn(new BossShot(shape().centre, d));
    attack_--;
    play_sound_random(Lib::sound::kBossFire);
  }

  timer_++;
  if (timer_ > kSbbTimer) {
    count_++;
    timer_ = 0;

    if (count_ >= 4 && (!z::rand_int(4) || count_ >= 8)) {
      count_ = 0;
      if (!unshielded_) {
        if (game().all_ships(kShipPowerup).size() < 5) {
          spawn(new Powerup(shape().centre, Powerup::type::kBomb));
        }
      }
    }

    if (!z::rand_int(3)) {
      side_ = !side_;
    }

    if (z::rand_int(2)) {
      vec2 d = vec2(5, 0).rotated(shape().rotation());
      for (std::int32_t i = 0; i < 12; i++) {
        spawn(new BossShot(shape().centre, d));
        d = d.rotated(2 * fixed_c::pi / 12);
      }
      play_sound(Lib::sound::kBossAttack);
    } else {
      attack_ = kSbbAttackTime;
      attack_dir_ = vec2::from_polar(z::rand_fixed() * (2 * fixed_c::pi), 5);
    }
  }
}

std::int32_t ShieldBombBoss::get_damage(std::int32_t damage, bool magic) {
  if (unshielded_) {
    return damage;
  }
  if (damage >= Player::kBombDamage && !unshielded_) {
    unshielded_ = kSbbUnshieldTime;
    shapes()[0]->category = kVulnerable | kDangerous;
    shapes()[17]->category = 0;
  }
  if (!magic) {
    return 0;
  }
  shot_alternate_ = !shot_alternate_;
  if (shot_alternate_) {
    restore_hp(60 / (1 + (game().get_lives() ? game().players().size() : game().alive_players())));
  }
  return damage;
}