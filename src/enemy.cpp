#include "enemy.h"
#include "boss_enemy.h"
#include "player.h"

#include <algorithm>

static const int32_t FOLLOW_TIME = 90;
static const fixed FOLLOW_SPEED = 2;

static const int32_t CHASER_TIME = 60;
static const fixed CHASER_SPEED = 4;

static const fixed SQUARE_SPEED = 2 + fixed(1) / 4;

static const int32_t WALL_TIMER = 80;
static const fixed WALL_SPEED = 1 + fixed(1) / 4;

static const int32_t FOLLOW_HUB_TIMER = 170;
static const fixed FOLLOW_HUB_SPEED = 1;

static const int32_t SHIELDER_TIMER = 80;
static const fixed SHIELDER_SPEED = 2;

static const int32_t TRACTOR_TIMER = 50;
static const fixed TRACTOR_SPEED = 6 * (fixed(1) / 10);
const fixed Tractor::TRACTOR_BEAM_SPEED = 2 + fixed(1) / 2;

Enemy::Enemy(const vec2& position, Ship::ship_category type,
             int32_t hp, bool explode_on_destroy)
  : Ship(position, Ship::ship_category(type | SHIP_ENEMY))
  , _hp(hp)
  , _score(0)
  , _damaged(false)
  , _destroy_sound(Lib::SOUND_ENEMY_DESTROY)
  , _explode_on_destroy(explode_on_destroy)
{
}

void Enemy::damage(int32_t damage, bool magic, Player* source)
{
  _hp -= std::max(damage, 0);
  if (damage > 0) {
    play_sound_random(Lib::SOUND_ENEMY_HIT);
  }

  if (_hp <= 0 && !is_destroyed()) {
    play_sound_random(_destroy_sound);
    if (source && get_score() > 0) {
      source->add_score(get_score());
    }
    if (_explode_on_destroy) {
      explosion();
    }
    else {
      explosion(0, 4, true, to_float(shape().centre));
    }
    on_destroy(damage >= Player::BOMB_DAMAGE);
    destroy();
  }
  else if (!is_destroyed()) {
    if (damage > 0) {
      play_sound_random(Lib::SOUND_ENEMY_HIT);
    }
    _damaged = damage >= Player::BOMB_DAMAGE ? 25 : 1;
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

Follow::Follow(const vec2& position, fixed radius, int32_t hp)
  : Enemy(position, SHIP_NONE, hp)
  , _timer(0)
  , _target(0)
{
  add_shape(
      new Polygon(vec2(), radius, 4, 0x9933ffff, 0, DANGEROUS | VULNERABLE));
  set_score(15);
  set_bounding_width(10);
  set_destroy_sound(Lib::SOUND_ENEMY_SHATTER);
  set_enemy_value(1);
}

void Follow::update()
{
  shape().rotate(fixed::tenth);
  if (!z0().alive_players()) {
    return;
  }

  _timer++;
  if (!_target || _timer > FOLLOW_TIME) {
    _target = nearest_player();
    _timer = 0;
  }
  vec2 d = _target->shape().centre - shape().centre;
  move(d.normalised() * FOLLOW_SPEED);
}

Chaser::Chaser(const vec2& position)
  : Enemy(position, SHIP_NONE, 2)
  , _move(false)
  , _timer(CHASER_TIME)
  , _dir()
{
  add_shape(new Polygon(vec2(), 10, 4, 0x3399ffff,
                        0, DANGEROUS | VULNERABLE, Polygon::T::POLYGRAM));
  set_score(30);
  set_bounding_width(10);
  set_destroy_sound(Lib::SOUND_ENEMY_SHATTER);
  set_enemy_value(2);
}

void Chaser::update()
{
  bool before = is_on_screen();

  if (_timer) {
    _timer--;
  }
  if (_timer <= 0) {
    _timer = CHASER_TIME * (_move + 1);
    _move = !_move;
    if (_move) {
      _dir = CHASER_SPEED *
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

Square::Square(const vec2& position, fixed rotation)
  : Enemy(position, SHIP_WALL, 4)
  , _dir()
  , _timer(z::rand_int(80) + 40)
{
  add_shape(new Box(vec2(), 10, 10, 0x33ff33ff, 0, DANGEROUS | VULNERABLE));
  _dir = vec2::from_polar(rotation, 1);
  set_score(25);
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
  move(_dir * SQUARE_SPEED);
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

Wall::Wall(const vec2& position, bool rdir)
  : Enemy(position, SHIP_WALL, 10)
  , _dir(0, 1)
  , _timer(0)
  , _rotate(false)
  , _rdir(rdir)
{
  add_shape(new Box(vec2(), 10, 40, 0x33cc33ff, 0, DANGEROUS | VULNERABLE));
  set_score(20);
  set_bounding_width(50);
  set_enemy_value(4);
}

void Wall::update()
{
  if (z0().get_non_wall_count() == 0 && _timer % 8 < 2) {
    if (get_hp() > 2) {
      play_sound(Lib::SOUND_ENEMY_SPAWN);
    }
    damage(get_hp() - 2, false, 0);
  }

  if (_rotate) {
    vec2 d = _dir.rotated((_rdir ? 1 : -1) * (_timer - WALL_TIMER) *
                          fixed::pi / (4 * WALL_TIMER));

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
    if (_timer > WALL_TIMER * 6) {
      if (is_on_screen()) {
        _timer = WALL_TIMER;
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

  move(_dir * WALL_SPEED);
  shape().set_rotation(_dir.angle());
}

void Wall::on_destroy(bool bomb)
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

FollowHub::FollowHub(const vec2& position, bool powera, bool powerb)
  : Enemy(position, SHIP_NONE, 14)
  , _timer(0)
  , _dir(0, 0)
  , _count(0)
  , _powera(powera)
  , _powerb(powerb)
{
  add_shape(new Polygon(vec2(), 16, 4, 0x6666ffff, fixed::pi / 4,
                        DANGEROUS | VULNERABLE, Polygon::T::POLYGRAM));
  if (_powerb) {
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
  set_score(50 + _powera * 10 + _powerb * 10);
  set_bounding_width(16);
  set_destroy_sound(Lib::SOUND_PLAYER_DESTROY);
  set_enemy_value(6 + (powera ? 2 : 0) + (powerb ? 2 : 0));
}

void FollowHub::update()
{
  _timer++;
  if (_timer > (_powera ? FOLLOW_HUB_TIMER / 2 : FOLLOW_HUB_TIMER)) {
    _timer = 0;
    _count++;
    if (is_on_screen()) {
      if (_powerb) {
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

  fixed s = _powera ?
      fixed::hundredth * 5 + fixed::tenth : fixed::hundredth * 5;
  shape().rotate(s);
  shapes()[0]->rotate(-s);

  move(_dir * FOLLOW_HUB_SPEED);
}

void FollowHub::on_destroy(bool bomb)
{
  if (bomb) {
    return;
  }
  if (_powerb) {
    spawn(new BigFollow(shape().centre, true));
  }
  spawn(new Chaser(shape().centre));
}

Shielder::Shielder(const vec2& position, bool power)
  : Enemy(position, SHIP_NONE, 16)
  , _dir(0, 1)
  , _timer(0)
  , _rotate(false)
  , _rdir(false)
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
  set_score(60 + _power * 40);
  set_bounding_width(36);
  set_destroy_sound(Lib::SOUND_PLAYER_DESTROY);
  set_enemy_value(8 + (power ? 2 : 0));
}

void Shielder::update()
{
  fixed s = _power ? fixed::hundredth * 12 : fixed::hundredth * 4;
  shape().rotate(s);
  shapes()[9]->rotate(-2 * s);
  for (int32_t i = 0; i < 8; i++) {
    shapes()[i]->rotate(-s);
  }

  bool on_screen = false;
  _dir = shape().centre.x < 0 ? vec2(1, 0) :
         shape().centre.x > Lib::WIDTH ? vec2(-1, 0) :
         shape().centre.y < 0 ? vec2(0, 1) :
         shape().centre.y > Lib::HEIGHT ? vec2(0, -1) :
         (on_screen = true, _dir);

  if (!on_screen && _rotate) {
    _timer = 0;
    _rotate = false;
  }

  fixed speed = SHIELDER_SPEED +
      (_power ? fixed::tenth * 3 : fixed::tenth * 2) * (16 - get_hp());
  if (_rotate) {
    vec2 d = _dir.rotated((_rdir ? 1 : -1) * (SHIELDER_TIMER - _timer) *
                          fixed::pi / (2 * SHIELDER_TIMER));
    _timer--;
    if (_timer <= 0) {
      _timer = 0;
      _rotate = false;
      _dir = _dir.rotated((_rdir ? 1 : -1) * fixed::pi / 2);
    }
    move(d * speed);
  }
  else {
    _timer++;
    if (_timer > SHIELDER_TIMER * 2) {
      _timer = SHIELDER_TIMER;
      _rotate = true;
      _rdir = z::rand_int(2) != 0;
    }
    if (is_on_screen() && _power &&
        _timer % SHIELDER_TIMER == SHIELDER_TIMER / 2) {
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

Tractor::Tractor(const vec2& position, bool power)
  : Enemy(position, SHIP_NONE, 50)
  , _timer(TRACTOR_TIMER * 4)
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
  set_score(85 + power * 40);
  set_bounding_width(36);
  set_destroy_sound(Lib::SOUND_PLAYER_DESTROY);
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
    move(_dir * TRACTOR_SPEED * (is_on_screen() ? 1 : 2 + fixed::half));

    if (_timer > TRACTOR_TIMER * 8) {
      _ready = true;
      _timer = 0;
    }
  }
  else if (_ready) {
    if (_timer > TRACTOR_TIMER) {
      _ready = false;
      _spinning = true;
      _timer = 0;
      _players = z0().get_players();
      play_sound(Lib::SOUND_BOSS_FIRE);
    }
  }
  else if (_spinning) {
    shape().rotate(fixed::tenth * 3);
    for (const auto& ship : _players) {
      if (!((Player*) ship)->is_killed()) {
        vec2 d = (shape().centre - ship->shape().centre).normalised();
        ship->move(d * TRACTOR_BEAM_SPEED);
      }
    }

    if (_timer % (TRACTOR_TIMER / 2) == 0 && is_on_screen() && _power) {
      Player* p = nearest_player();
      vec2 d = (p->shape().centre - shape().centre).normalised();
      spawn(new SBBossShot(shape().centre, d * 4, 0xcc33ccff));
      play_sound_random(Lib::SOUND_BOSS_FIRE);
    }

    if (_timer > TRACTOR_TIMER * 5) {
      _spinning = false;
      _timer = 0;
    }
  }
}

void Tractor::render() const
{
  Enemy::render();
  if (_spinning) {
    for (std::size_t i = 0; i < _players.size(); ++i) {
      if (((_timer + i * 4) / 4) % 2 && !((Player*) _players[i])->is_killed()) {
        lib().render_line(to_float(shape().centre),
                          to_float(_players[i]->shape().centre), 0xcc33ccff);
      }
    }
  }
}
