#include "boss_square.h"
#include "enemy.h"
#include "player.h"

static const int32_t BSB_BASE_HP = 400;
static const int32_t BSB_TIMER = 100;
static const int32_t BSB_STIMER = 80;
static const int32_t BSB_ATTACK_TIME = 90;

static const fixed BSB_SPEED = 2 + fixed(1) / 2;
static const fixed BSB_ATTACK_RADIUS = 120;

BigSquareBoss::BigSquareBoss(int32_t players, int32_t cycle)
  : Boss(vec2(Lib::WIDTH * fixed::hundredth * 75, Lib::HEIGHT * 2),
         z0Game::BOSS_1A, BSB_BASE_HP, players, cycle)
  , _dir(0, -1)
  , _reverse(false)
  , _timer(BSB_TIMER * 6)
  , _spawn_timer(0)
  , _special_timer(0)
  , _special_attack(false)
  , _special_attack_rotate(false)
  , _attack_player(0)
{
  add_shape(new Polygon(vec2(), 160, 4, 0x9933ffff, 0, 0));
  add_shape(new Polygon(vec2(), 140, 4, 0x9933ffff, 0, DANGEROUS));
  add_shape(new Polygon(vec2(), 120, 4, 0x9933ffff, 0, DANGEROUS));
  add_shape(new Polygon(vec2(), 100, 4, 0x9933ffff, 0, 0));
  add_shape(new Polygon(vec2(), 80, 4, 0x9933ffff, 0, 0));
  add_shape(new Polygon(vec2(), 60, 4, 0x9933ffff, 0, VULNERABLE));

  add_shape(new Polygon(vec2(), 155, 4, 0x9933ffff, 0, 0));
  add_shape(new Polygon(vec2(), 135, 4, 0x9933ffff, 0, 0));
  add_shape(new Polygon(vec2(), 115, 4, 0x9933ffff, 0, 0));
  add_shape(new Polygon(vec2(), 95, 4, 0x6600ccff, 0, 0));
  add_shape(new Polygon(vec2(), 75, 4, 0x6600ccff, 0, 0));
  add_shape(new Polygon(vec2(), 55, 4, 0x330099ff, 0, SHIELD));
}

void BigSquareBoss::update()
{
  const vec2& pos = shape().centre;
  if (pos.y < Lib::HEIGHT * fixed::hundredth * 25 && _dir.y == -1) {
    _dir = vec2(_reverse ? 1 : -1, 0);
  }
  if (pos.x < Lib::WIDTH * fixed::hundredth * 25 && _dir.x == -1) {
    _dir = vec2(0, _reverse ? -1 : 1);
  }
  if (pos.y > Lib::HEIGHT * fixed::hundredth * 75 && _dir.y == 1) {
    _dir = vec2(_reverse ? -1 : 1, 0);
  }
  if (pos.x > Lib::WIDTH * fixed::hundredth * 75 && _dir.x == 1) {
    _dir = vec2(0, _reverse ? 1 : -1);
  }

  if (_special_attack) {
    _special_timer--;
    if (_attack_player->is_killed()) {
      _special_timer = 0;
      _attack_player = 0;
    }
    else if (!_special_timer) {
      vec2 d(BSB_ATTACK_RADIUS, 0);
      if (_special_attack_rotate) {
        d = d.rotated(fixed::pi / 2);
      }
      for (int32_t i = 0; i < 6; i++) {
        Enemy* s = new Follow(_attack_player->shape().centre + d);
        s->shape().set_rotation(fixed::pi / 4);
        s->set_score(0);
        spawn(s);
        d = d.rotated(2 * fixed::pi / 6);
      }
      _attack_player = 0;
      play_sound(Lib::SOUND_ENEMY_SPAWN);
    }
    if (!_attack_player) {
      _special_attack = false;
    }
  }
  else if (is_on_screen()) {
    _timer--;
    if (_timer <= 0) {
      _timer = (z::rand_int(6) + 1) * BSB_TIMER;
      _dir = vec2() - _dir;
      _reverse = !_reverse;
    }
    _spawn_timer++;
    if (_spawn_timer >=
        int32_t(BSB_STIMER - z0().alive_players() * 10) / (is_hp_low() ? 2 : 1)) {
      _spawn_timer = 0;
      _special_timer++;
      spawn(new BigFollow(shape().centre, false));
      play_sound_random(Lib::SOUND_BOSS_FIRE);
    }
    if (_special_timer >= 8 && z::rand_int(4)) {
      _special_timer = BSB_ATTACK_TIME;
      _special_attack = true;
      _special_attack_rotate = z::rand_int(2) != 0;
      _attack_player = nearest_player();
      play_sound(Lib::SOUND_BOSS_ATTACK);
    }
  }

  if (_special_attack) {
    return;
  }

  move(_dir * BSB_SPEED);
  for (std::size_t i = 0; i < 6; ++i) {
    shapes()[i]->rotate((i % 2 ? -1 : 1) * fixed::hundredth * ((1 + i) * 5));
  }
  for (std::size_t i = 0; i < 6; ++i) {
    shapes()[i + 6]->set_rotation(shapes()[i]->rotation());
  }
}

void BigSquareBoss::render() const
{
  Boss::render();
  if ((_special_timer / 4) % 2 && _attack_player) {
    flvec2 d(BSB_ATTACK_RADIUS.to_float(), 0);
    if (_special_attack_rotate) {
      d = d.rotated(M_PIf / 2);
    }
    for (int32_t i = 0; i < 6; i++) {
      flvec2 p = to_float(_attack_player->shape().centre) + d;
      Polygon s(vec2(), 10, 4, 0x9933ffff, fixed::pi / 4, 0);
      s.render(lib(), p, 0);
      d = d.rotated(2 * M_PIf / 6);
    }
  }
}

int32_t BigSquareBoss::get_damage(int32_t damage, bool magic)
{
  return damage;
}