#include "boss_shield.h"
#include "enemy.h"
#include "player.h"

static const int32_t SBB_BASE_HP = 320;
static const int32_t SBB_TIMER = 100;
static const int32_t SBB_UNSHIELD_TIME = 300;
static const int32_t SBB_ATTACK_TIME = 80;
static const fixed SBB_SPEED = 1;

ShieldBombBoss::ShieldBombBoss(int32_t players, int32_t cycle)
: Boss(vec2(-Lib::WIDTH / 2, Lib::HEIGHT / 2), GameModal::BOSS_1B, SBB_BASE_HP, players, cycle)
, _timer(0)
, _count(0)
, _unshielded(0)
, _attack(0)
, _side(false)
, _shot_alternate(false) {
  add_shape(
      new Polygon(vec2(), 48, 8, 0x339966ff, 0, DANGEROUS | VULNERABLE, Polygon::T::POLYGRAM));

  for (int32_t i = 0; i < 16; i++) {
    vec2 a = vec2(120, 0).rotated(i * fixed_c::pi / 8);
    vec2 b = vec2(80, 0).rotated(i * fixed_c::pi / 8);

    add_shape(new Line(vec2(), a, b, 0x999999ff, 0));
  }

  add_shape(new Polygon(vec2(), 130, 16, 0xccccccff, 0, VULNSHIELD | DANGEROUS));
  add_shape(new Polygon(vec2(), 125, 16, 0xccccccff, 0, 0));
  add_shape(new Polygon(vec2(), 120, 16, 0xccccccff, 0, 0));

  add_shape(new Polygon(vec2(), 42, 16, 0, 0, SHIELD));

  set_ignore_damage_colour_index(1);
}

void ShieldBombBoss::update() {
  if (!_side && shape().centre.x < Lib::WIDTH * fixed_c::tenth * 6) {
    move(vec2(1, 0) * SBB_SPEED);
  } else if (_side && shape().centre.x > Lib::WIDTH * fixed_c::tenth * 4) {
    move(vec2(-1, 0) * SBB_SPEED);
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
      shapes()[0]->category = DANGEROUS | VULNERABLE;
      shapes()[17]->category = DANGEROUS | VULNSHIELD;

      for (std::size_t i = 0; i < 3; i++) {
        shapes()[i + 17]->colour = 0xccccccff;
      }
      for (std::size_t i = 0; i < 16; i++) {
        shapes()[i + 1]->colour = 0x999999ff;
      }
    }
  }

  if (_attack) {
    vec2 d = _attack_dir.rotated((SBB_ATTACK_TIME - _attack) * fixed_c::half * fixed_c::pi /
                                 SBB_ATTACK_TIME);
    spawn(new BossShot(shape().centre, d));
    _attack--;
    play_sound_random(Lib::SOUND_BOSS_FIRE);
  }

  _timer++;
  if (_timer > SBB_TIMER) {
    _count++;
    _timer = 0;

    if (_count >= 4 && (!z::rand_int(4) || _count >= 8)) {
      _count = 0;
      if (!_unshielded) {
        if (game().all_ships(SHIP_POWERUP).size() < 5) {
          spawn(new Powerup(shape().centre, Powerup::BOMB));
        }
      }
    }

    if (!z::rand_int(3)) {
      _side = !_side;
    }

    if (z::rand_int(2)) {
      vec2 d = vec2(5, 0).rotated(shape().rotation());
      for (int32_t i = 0; i < 12; i++) {
        spawn(new BossShot(shape().centre, d));
        d = d.rotated(2 * fixed_c::pi / 12);
      }
      play_sound(Lib::SOUND_BOSS_ATTACK);
    } else {
      _attack = SBB_ATTACK_TIME;
      _attack_dir = vec2::from_polar(z::rand_fixed() * (2 * fixed_c::pi), 5);
    }
  }
}

int32_t ShieldBombBoss::get_damage(int32_t damage, bool magic) {
  if (_unshielded) {
    return damage;
  }
  if (damage >= Player::BOMB_DAMAGE && !_unshielded) {
    _unshielded = SBB_UNSHIELD_TIME;
    shapes()[0]->category = VULNERABLE | DANGEROUS;
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