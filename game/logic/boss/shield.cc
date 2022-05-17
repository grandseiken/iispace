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
, _timer(0)
, _count(0)
, _unshielded(0)
, _attack(0)
, _side(false)
, _shot_alternate(false) {
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
  if (!_side && shape().centre.x < Lib::kWidth * fixed_c::tenth * 6) {
    move(vec2(1, 0) * kSbbSpeed);
  } else if (_side && shape().centre.x > Lib::kWidth * fixed_c::tenth * 4) {
    move(vec2(-1, 0) * kSbbSpeed);
  }

  shape().rotate(-fixed_c::hundredth * 2);
  shapes()[0]->rotate(-fixed_c::hundredth * 4);
  shapes()[20]->set_rotation(shapes()[0]->rotation());

  if (!is_on_screen()) {
    return;
  }

  if (is_hp_low()) {
    _timer++;
  }

  if (_unshielded) {
    _timer++;

    _unshielded--;
    for (std::size_t i = 0; i < 3; i++) {
      shapes()[i + 17]->colour = (_unshielded / 2) % 4 ? 0x00000000 : 0x666666ff;
    }
    for (std::size_t i = 0; i < 16; i++) {
      shapes()[i + 1]->colour = (_unshielded / 2) % 4 ? 0x00000000 : 0x333333ff;
    }

    if (!_unshielded) {
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

  if (_attack) {
    vec2 d = _attack_dir.rotated((kSbbAttackTime - _attack) * fixed_c::half * fixed_c::pi /
                                 kSbbAttackTime);
    spawn(new BossShot(shape().centre, d));
    _attack--;
    play_sound_random(Lib::sound::kBossFire);
  }

  _timer++;
  if (_timer > kSbbTimer) {
    _count++;
    _timer = 0;

    if (_count >= 4 && (!z::rand_int(4) || _count >= 8)) {
      _count = 0;
      if (!_unshielded) {
        if (game().all_ships(kShipPowerup).size() < 5) {
          spawn(new Powerup(shape().centre, Powerup::type::kBomb));
        }
      }
    }

    if (!z::rand_int(3)) {
      _side = !_side;
    }

    if (z::rand_int(2)) {
      vec2 d = vec2(5, 0).rotated(shape().rotation());
      for (std::int32_t i = 0; i < 12; i++) {
        spawn(new BossShot(shape().centre, d));
        d = d.rotated(2 * fixed_c::pi / 12);
      }
      play_sound(Lib::sound::kBossAttack);
    } else {
      _attack = kSbbAttackTime;
      _attack_dir = vec2::from_polar(z::rand_fixed() * (2 * fixed_c::pi), 5);
    }
  }
}

std::int32_t ShieldBombBoss::get_damage(std::int32_t damage, bool magic) {
  if (_unshielded) {
    return damage;
  }
  if (damage >= Player::kBombDamage && !_unshielded) {
    _unshielded = kSbbUnshieldTime;
    shapes()[0]->category = kVulnerable | kDangerous;
    shapes()[17]->category = 0;
  }
  if (!magic) {
    return 0;
  }
  _shot_alternate = !_shot_alternate;
  if (_shot_alternate) {
    restore_hp(60 / (1 + (game().get_lives() ? game().players().size() : game().alive_players())));
  }
  return damage;
}