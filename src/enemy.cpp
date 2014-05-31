#include "enemy.h"
#include "boss_enemy.h"
#include "player.h"

#include <algorithm>

const int Follow::TIME = 90;
const fixed Follow::SPEED = 2;
const int Chaser::TIME = 60;
const fixed Chaser::SPEED = 4;
const fixed Square::SPEED = 2 + fixed(1) / 4;
const int Wall::TIMER = 80;
const fixed Wall::SPEED = 1 + fixed(1) / 4;
const int FollowHub::TIMER = 170;
const fixed FollowHub::SPEED = 1;
const int Shielder::TIMER = 80;
const fixed Shielder::SPEED = 2;
const int Tractor::TIMER = 50;
const fixed Tractor::SPEED = 6 * (fixed(1) / 10);
const fixed Tractor::TRACTOR_SPEED = 2 + fixed(1) / 2;

// Basic enemy
//------------------------------
Enemy::Enemy(const vec2& position, Ship::ship_category type,
             int hp, bool explodeOnDestroy)
  : Ship(position, Ship::ship_category(type | SHIP_ENEMY))
  , _hp(hp)
  , _score(0)
  , _damaged(false)
  , _destroySound(Lib::SOUND_ENEMY_DESTROY)
  , _explodeOnDestroy(explodeOnDestroy)
{
}

void Enemy::damage(int damage, bool magic, Player* source)
{
  _hp -= std::max(damage, 0);
  if (damage > 0) {
    play_sound_random(Lib::SOUND_ENEMY_HIT);
  }

  if (_hp <= 0 && !is_destroyed()) {
    play_sound_random(_destroySound);
    if (source && GetScore() > 0) {
      source->add_score(GetScore());
    }
    if (_explodeOnDestroy) {
      explosion();
    }
    else {
      explosion(0, 4, true, to_float(shape().centre));
    }
    OnDestroy(damage >= Player::bomb_damage);
    destroy();
  }
  else if (!is_destroyed()) {
    if (damage > 0) {
      play_sound_random(Lib::SOUND_ENEMY_HIT);
    }
    _damaged = damage >= Player::bomb_damage ? 25 : 1;
  }
}

void Enemy::render() const
{
  if (!_damaged) {
    Ship::render();
    return;
  }
  render_with_colour(0xffffffff);
  --_damaged;
}

// Follower enemy
//------------------------------
Follow::Follow(const vec2& position, fixed radius, int hp)
  : Enemy(position, SHIP_NONE, hp)
  , _timer(0)
  , _target(0)
{
  add_shape(
      new Polygon(vec2(), radius, 4, 0x9933ffff, 0, DANGEROUS | VULNERABLE));
  SetScore(15);
  set_bounding_width(10);
  SetDestroySound(Lib::SOUND_ENEMY_SHATTER);
  set_enemy_value(1);
}

void Follow::update()
{
  shape().rotate(fixed::tenth);
  if (!z0().alive_players()) {
    return;
  }

  _timer++;
  if (!_target || _timer > TIME) {
    _target = nearest_player();
    _timer = 0;
  }
  vec2 d = _target->shape().centre - shape().centre;
  if (d.length() > 0) {
    move(d.normalised() * SPEED);
  }
}

// Chaser enemy
//------------------------------
Chaser::Chaser(const vec2& position)
  : Enemy(position, SHIP_NONE, 2)
  , _move(false)
  , _timer(TIME)
  , _dir()
{
  add_shape(new Polygon(vec2(), 10, 4, 0x3399ffff,
                        0, DANGEROUS | VULNERABLE, Polygon::T::POLYGRAM));
  SetScore(30);
  set_bounding_width(10);
  SetDestroySound(Lib::SOUND_ENEMY_SHATTER);
  set_enemy_value(2);
}

void Chaser::update()
{
  bool before = is_on_screen();

  if (_timer) {
    _timer--;
  }
  if (_timer <= 0) {
    _timer = TIME * (_move + 1);
    _move = !_move;
    if (_move) {
      _dir = SPEED *
          (nearest_player()->shape().centre - shape().centre).normalised();
    }
  }
  if (_move) {
    move(_dir);
    if (!before && is_on_screen()) {
      _move = false;
    }
  }
  else {
    shape().rotate(fixed::tenth);
  }
}

// Square enemy
//------------------------------
Square::Square(const vec2& position, fixed rotation)
  : Enemy(position, SHIP_WALL, 4)
  , _dir()
  , _timer(z::rand_int(80) + 40)
{
  add_shape(new Box(vec2(), 10, 10, 0x33ff33ff, 0, DANGEROUS | VULNERABLE));
  _dir = vec2::from_polar(rotation, 1);
  SetScore(25);
  set_bounding_width(15);
  set_enemy_value(2);
}

void Square::update()
{
  if (is_on_screen() && z0().get_non_wall_count() == 0) {
    _timer--;
  }
  else {
    _timer = z::rand_int(80) + 40;
  }

  if (_timer == 0) {
    damage(4, false, 0);
  }

  const vec2& pos = shape().centre;
  if (pos.x < 0 && _dir.x <= 0) {
    _dir.x = -_dir.x;
    if (_dir.x <= 0) {
      _dir.x = 1;
    }
  }
  if (pos.y < 0 && _dir.y <= 0) {
    _dir.y = -_dir.y;
    if (_dir.y <= 0) {
      _dir.y = 1;
    }
  }

  if (pos.x > Lib::WIDTH && _dir.x >= 0) {
    _dir.x = -_dir.x;
    if (_dir.x >= 0) {
      _dir.x = -1;
    }
  }
  if (pos.y > Lib::HEIGHT && _dir.y >= 0) {
    _dir.y = -_dir.y;
    if (_dir.y >= 0) {
      _dir.y = -1;
    }
  }
  _dir = _dir.normalised();
  move(_dir * SPEED);
  shape().set_rotation(_dir.angle());
}

void Square::render() const
{
  if (z0().get_non_wall_count() == 0 && (_timer % 4 == 1 || _timer % 4 == 2)) {
    render_with_colour(0x33333300);
  }
  else {
    Enemy::render();
  }
}

// Wall enemy
//------------------------------
Wall::Wall(const vec2& position, bool rdir)
  : Enemy(position, SHIP_WALL, 10)
  , _dir(0, 1)
  , _timer(0)
  , _rotate(false)
  , _rdir(rdir)
{
  add_shape(new Box(vec2(), 10, 40, 0x33cc33ff, 0, DANGEROUS | VULNERABLE));
  SetScore(20);
  set_bounding_width(50);
  set_enemy_value(4);
}

void Wall::update()
{
  if (z0().get_non_wall_count() == 0 && _timer % 8 < 2) {
    if (GetHP() > 2) {
      play_sound(Lib::SOUND_ENEMY_SPAWN);
    }
    damage(GetHP() - 2, false, 0);
  }

  if (_rotate) {
    vec2 d = _dir.rotated(
        (_rdir ? _timer - TIMER : TIMER - _timer) * fixed::pi / (4 * TIMER));

    shape().set_rotation(d.angle());
    _timer--;
    if (_timer <= 0) {
      _timer = 0;
      _rotate = false;
      _dir = _dir.rotated(_rdir ? -fixed::pi / 4 : fixed::pi / 4);
    }
    return;
  }
  else {
    _timer++;
    if (_timer > TIMER * 6) {
      if (is_on_screen()) {
        _timer = TIMER;
        _rotate = true;
      }
      else {
        _timer = 0;
      }
    }
  }

  vec2 pos = shape().centre;
  if ((pos.x < 0 && _dir.x < -fixed::hundredth) ||
      (pos.y < 0 && _dir.y < -fixed::hundredth) ||
      (pos.x > Lib::WIDTH && _dir.x > fixed::hundredth) ||
      (pos.y > Lib::HEIGHT && _dir.y > fixed::hundredth)) {
    _dir = -_dir.normalised();
  }

  move(_dir * SPEED);
  shape().set_rotation(_dir.angle());
}

void Wall::OnDestroy(bool bomb)
{
  if (bomb) {
    return;
  }
  vec2 d = _dir.rotated(fixed::pi / 2);

  vec2 v = shape().centre + d * 10 * 3;
  if (v.x >= 0 && v.x <= Lib::WIDTH && v.y >= 0 && v.y <= Lib::HEIGHT) {
    spawn(new Square(v, shape().rotation()));
  }

  v = shape().centre - d * 10 * 3;
  if (v.x >= 0 && v.x <= Lib::WIDTH && v.y >= 0 && v.y <= Lib::HEIGHT) {
    spawn(new Square(v, shape().rotation()));
  }
}

// Follow spawner enemy
//------------------------------
FollowHub::FollowHub(const vec2& position, bool powerA, bool powerB)
  : Enemy(position, SHIP_NONE, 14)
  , _timer(0)
  , _dir(0, 0)
  , _count(0)
  , _powerA(powerA)
  , _powerB(powerB)
{
  add_shape(new Polygon(vec2(), 16, 4, 0x6666ffff, fixed::pi / 4,
                        DANGEROUS | VULNERABLE, Polygon::T::POLYGRAM));
  if (_powerB) {
    add_shape(new Polygon(vec2(16, 0), 8, 4, 0x6666ffff,
                          fixed::pi / 4, 0, Polygon::T::POLYSTAR));
    add_shape(new Polygon(vec2(-16, 0), 8, 4, 0x6666ffff,
                          fixed::pi / 4, 0, Polygon::T::POLYSTAR));
    add_shape(new Polygon(vec2(0, 16), 8, 4, 0x6666ffff,
                          fixed::pi / 4, 0, Polygon::T::POLYSTAR));
    add_shape(new Polygon(vec2(0, -16), 8, 4, 0x6666ffff,
                          fixed::pi / 4, 0, Polygon::T::POLYSTAR));
  }

  add_shape(new Polygon(vec2(16, 0), 8, 4, 0x6666ffff, fixed::pi / 4));
  add_shape(new Polygon(vec2(-16, 0), 8, 4, 0x6666ffff, fixed::pi / 4));
  add_shape(new Polygon(vec2(0, 16), 8, 4, 0x6666ffff, fixed::pi / 4));
  add_shape(new Polygon(vec2(0, -16), 8, 4, 0x6666ffff, fixed::pi / 4));
  SetScore(50 + _powerA * 10 + _powerB * 10);
  set_bounding_width(16);
  SetDestroySound(Lib::SOUND_PLAYER_DESTROY);
  set_enemy_value(6 + (powerA ? 2 : 0) + (powerB ? 2 : 0));
}

void FollowHub::update()
{
  _timer++;
  if (_timer > (_powerA ? TIMER / 2 : TIMER)) {
    _timer = 0;
    _count++;
    if (is_on_screen()) {
      if (_powerB) {
        spawn(new Chaser(shape().centre));
      }
      else {
        spawn(new Follow(shape().centre));
      }
      play_sound_random(Lib::SOUND_ENEMY_SPAWN);
    }
  }

  _dir = shape().centre.x < 0 ? vec2(1, 0) :
         shape().centre.x > Lib::WIDTH ? vec2(-1, 0) :
         shape().centre.y < 0 ? vec2(0, 1) :
         shape().centre.y > Lib::HEIGHT ? vec2(0, -1) :
         _count > 3 ? (_count = 0, _dir.rotated(-fixed::pi / 2)) : _dir;

  fixed s = _powerA ?
      fixed::hundredth * 5 + fixed::tenth : fixed::hundredth * 5;
  shape().rotate(s);
  shapes()[0]->rotate(-s);

  move(_dir * SPEED);
}

void FollowHub::OnDestroy(bool bomb)
{
  if (bomb) {
    return;
  }
  if (_powerB) {
    spawn(new BigFollow(shape().centre, true));
  }
  spawn(new Chaser(shape().centre));
}

// Shielder enemy
//------------------------------
Shielder::Shielder(const vec2& position, bool power)
  : Enemy(position, SHIP_NONE, 16)
  , _dir(0, 1)
  , _timer(0)
  , _rotate(false)
  , _rDir(false)
  , _power(power)
{
  add_shape(new Polygon(vec2(24, 0), 8, 6, 0x006633ff,
                        0, VULNSHIELD, Polygon::T::POLYSTAR));
  add_shape(new Polygon(vec2(-24, 0), 8, 6, 0x006633ff,
                        0, VULNSHIELD, Polygon::T::POLYSTAR));
  add_shape(new Polygon(vec2(0, 24), 8, 6, 0x006633ff,
                        fixed::pi / 2, VULNSHIELD, Polygon::T::POLYSTAR));
  add_shape(new Polygon(vec2(0, -24), 8, 6, 0x006633ff,
                        fixed::pi / 2, VULNSHIELD, Polygon::T::POLYSTAR));
  add_shape(new Polygon(vec2(24, 0), 8, 6, 0x33cc99ff, 0, 0));
  add_shape(new Polygon(vec2(-24, 0), 8, 6, 0x33cc99ff, 0, 0));
  add_shape(new Polygon(vec2(0, 24), 8, 6, 0x33cc99ff, fixed::pi / 2, 0));
  add_shape(new Polygon(vec2(0, -24), 8, 6, 0x33cc99ff, fixed::pi / 2, 0));

  add_shape(new Polygon(vec2(0, 0), 24, 4, 0x006633ff,
                        0, 0, Polygon::T::POLYSTAR));
  add_shape(new Polygon(vec2(0, 0), 14, 8, power ? 0x33cc99ff : 0x006633ff,
                        0, DANGEROUS | VULNERABLE));
  SetScore(60 + _power * 40);
  set_bounding_width(36);
  SetDestroySound(Lib::SOUND_PLAYER_DESTROY);
  set_enemy_value(8 + (power ? 2 : 0));
}

void Shielder::update()
{
  fixed s = _power ? fixed::hundredth * 12 : fixed::hundredth * 4;
  shape().rotate(s);
  shapes()[9]->rotate(-2 * s);
  for (int i = 0; i < 8; i++) {
    shapes()[i]->rotate(-s);
  }

  bool onScreen = false;
  _dir = shape().centre.x < 0 ? vec2(1, 0) :
         shape().centre.x > Lib::WIDTH ? vec2(-1, 0) :
         shape().centre.y < 0 ? vec2(0, 1) :
         shape().centre.y > Lib::HEIGHT ? vec2(0, -1) :
         (onScreen = true, _dir);

  if (!onScreen && _rotate) {
    _timer = 0;
    _rotate = false;
  }

  fixed speed = SPEED +
      (_power ? fixed::tenth * 3 : fixed::tenth * 2) * (16 - GetHP());
  if (_rotate) {
    vec2 d = _dir.rotated(
        (_rDir ? 1 : -1) * (TIMER - _timer) * fixed::pi / (2 * TIMER));
    _timer--;
    if (_timer <= 0) {
      _timer = 0;
      _rotate = false;
      _dir = _dir.rotated((_rDir ? 1 : -1) * fixed::pi / 2);
    }
    move(d * speed);
  }
  else {
    _timer++;
    if (_timer > TIMER * 2) {
      _timer = TIMER;
      _rotate = true;
      _rDir = z::rand_int(2) != 0;
    }
    if (is_on_screen() && _timer % TIMER == TIMER / 2 && _power) {
      Player* p = nearest_player();
      vec2 v = shape().centre;

      vec2 d = (p->shape().centre - v).normalised();
      spawn(new SBBossShot(v, d * 3, 0x33cc99ff));
      play_sound_random(Lib::SOUND_BOSS_FIRE);
    }
    move(_dir * speed);
  }
  _dir = _dir.normalised();
}

// Tractor beam enemy
//------------------------------
Tractor::Tractor(const vec2& position, bool power)
  : Enemy(position, SHIP_NONE, 50)
  , _timer(TIMER * 4)
  , _dir(0, 0)
  , _power(power)
  , _ready(false)
  , _spinning(false)
{
  add_shape(new Polygon(vec2(24, 0), 12, 6, 0xcc33ccff, 0,
                        DANGEROUS | VULNERABLE, Polygon::T::POLYGRAM));
  add_shape(new Polygon(vec2(-24, 0), 12, 6, 0xcc33ccff, 0,
                        DANGEROUS | VULNERABLE, Polygon::T::POLYGRAM));
  add_shape(new Line(vec2(0, 0), vec2(24, 0), vec2(-24, 0), 0xcc33ccff));
  if (power) {
    add_shape(new Polygon(vec2(24, 0), 16, 6,
                         0xcc33ccff, 0, 0, Polygon::T::POLYSTAR));
    add_shape(new Polygon(vec2(-24, 0), 16, 6,
                          0xcc33ccff, 0, 0, Polygon::T::POLYSTAR));
  }
  SetScore(85 + power * 40);
  set_bounding_width(36);
  SetDestroySound(Lib::SOUND_PLAYER_DESTROY);
  set_enemy_value(10 + (power ? 2 : 0));
}

void Tractor::update()
{
  shapes()[0]->rotate(fixed::hundredth * 5);
  shapes()[1]->rotate(-fixed::hundredth * 5);
  if (_power) {
    shapes()[3]->rotate(-fixed::hundredth * 8);
    shapes()[4]->rotate(fixed::hundredth * 8);
  }

  if (shape().centre.x < 0) {
    _dir = vec2(1, 0);
  }
  else if (shape().centre.x > Lib::WIDTH) {
    _dir = vec2(-1, 0);
  }
  else if (shape().centre.y < 0) {
    _dir = vec2(0, 1);
  }
  else if (shape().centre.y > Lib::HEIGHT) {
    _dir = vec2(0, -1);
  }
  else {
    _timer++;
  }

  if (!_ready && !_spinning) {
    move(_dir * SPEED * (is_on_screen() ? 1 : 2 + fixed::half));

    if (_timer > TIMER * 8) {
      _ready = true;
      _timer = 0;
    }
  }
  else if (_ready) {
    if (_timer > TIMER) {
      _ready = false;
      _spinning = true;
      _timer = 0;
      _players = z0().get_players();
      play_sound(Lib::SOUND_BOSS_FIRE);
    }
  }
  else if (_spinning) {
    shape().rotate(fixed::tenth * 3);
    for (unsigned int i = 0; i < _players.size(); i++) {
      if (!((Player*) _players[i])->is_killed()) {
        vec2 d = (shape().centre - _players[i]->shape().centre).normalised();
        _players[i]->move(d * TRACTOR_SPEED);
      }
    }

    if (_timer % (TIMER / 2) == 0 && is_on_screen() && _power) {
      Player* p = nearest_player();
      vec2 d = (p->shape().centre - shape().centre).normalised();
      spawn(new SBBossShot(shape().centre, d * 4, 0xcc33ccff));
      play_sound_random(Lib::SOUND_BOSS_FIRE);
    }

    if (_timer > TIMER * 5) {
      _spinning = false;
      _timer = 0;
    }
  }
}

void Tractor::render() const
{
  Enemy::render();
  if (_spinning) {
    for (unsigned int i = 0; i < _players.size(); i++) {
      if (((_timer + i * 4) / 4) % 2 && !((Player*) _players[i])->is_killed()) {
        lib().render_line(to_float(shape().centre),
                          to_float(_players[i]->shape().centre), 0xcc33ccff);
      }
    }
  }
}
