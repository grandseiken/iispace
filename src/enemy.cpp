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
  : Ship(position, Ship::ship_category(type | Ship::SHIP_ENEMY))
  , _hp(hp)
  , _score(0)
  , _damaged(false)
  , _destroySound(Lib::SOUND_ENEMY_DESTROY)
  , _explodeOnDestroy(explodeOnDestroy)
{
}

void Enemy::Damage(int damage, bool magic, Player* source)
{
  _hp -= std::max(damage, 0);
  if (damage > 0) {
    PlaySoundRandom(Lib::SOUND_ENEMY_HIT);
  }

  if (_hp <= 0 && !is_destroyed()) {
    PlaySoundRandom(_destroySound);
    if (source && GetScore() > 0) {
      source->AddScore(GetScore());
    }
    if (_explodeOnDestroy) {
      Explosion();
    }
    else {
      Explosion(0, 4, true, to_float(GetPosition()));
    }
    OnDestroy(damage >= Player::BOMB_DAMAGE);
    destroy();
  }
  else if (!is_destroyed()) {
    if (damage > 0) {
      PlaySoundRandom(Lib::SOUND_ENEMY_HIT);
    }
    _damaged = damage >= Player::BOMB_DAMAGE ? 25 : 1;
  }
}

void Enemy::Render() const
{
  if (!_damaged) {
    Ship::Render();
    return;
  }
  RenderWithColour(0xffffffff);
  --_damaged;
}

// Follower enemy
//------------------------------
Follow::Follow(const vec2& position, fixed radius, int hp)
  : Enemy(position, Ship::SHIP_NONE, hp)
  , _timer(0)
  , _target(0)
{
  AddShape(
      new Polygon(vec2(), radius, 4, 0x9933ffff, 0, DANGEROUS | VULNERABLE));
  SetScore(15);
  SetBoundingWidth(10);
  SetDestroySound(Lib::SOUND_ENEMY_SHATTER);
  SetEnemyValue(1);
}

void Follow::Update()
{
  Rotate(fixed::tenth);
  if (Player::CountKilledPlayers() >= GetPlayers().size()) {
    return;
  }

  _timer++;
  if (!_target || _timer > TIME) {
    _target = GetNearestPlayer();
    _timer = 0;
  }
  vec2 d = _target->GetPosition() - GetPosition();
  if (d.length() > 0) {
    d.normalise();
    d *= SPEED;

    Move(d);
  }
}

// Chaser enemy
//------------------------------
Chaser::Chaser(const vec2& position)
  : Enemy(position, Ship::SHIP_NONE, 2)
  , _move(false)
  , _timer(TIME)
  , _dir()
{
  AddShape(new Polygram(vec2(), 10, 4, 0x3399ffff, 0, DANGEROUS | VULNERABLE));
  SetScore(30);
  SetBoundingWidth(10);
  SetDestroySound(Lib::SOUND_ENEMY_SHATTER);
  SetEnemyValue(2);
}

void Chaser::Update()
{
  bool before = IsOnScreen();

  if (_timer) {
    _timer--;
  }
  if (_timer <= 0) {
    _timer = TIME * (_move + 1);
    _move = !_move;
    if (_move) {
      Ship* p = GetNearestPlayer();
      _dir = p->GetPosition() - GetPosition();
      _dir.normalise();
      _dir *= SPEED;
    }
  }
  if (_move) {
    Move(_dir);
    if (!before && IsOnScreen()) {
      _move = false;
    }
  }
  else {
    Rotate(fixed::tenth);
  }
}

// Square enemy
//------------------------------
Square::Square(const vec2& position, fixed rotation)
  : Enemy(position, Ship::SHIP_WALL, 4)
  , _dir()
  , _timer(z::rand_int(80) + 40)
{
  AddShape(new Box(vec2(), 10, 10, 0x33ff33ff, 0, DANGEROUS | VULNERABLE));
  _dir.set_polar(rotation, 1);
  SetScore(25);
  SetBoundingWidth(15);
  SetEnemyValue(2);
}

void Square::Update()
{
  if (GetNonWallCount() == 0 && IsOnScreen()) {
    _timer--;
  }
  else {
    _timer = z::rand_int(80) + 40;
  }

  if (_timer == 0) {
    Damage(4, false, 0);
  }

  vec2 pos = GetPosition();
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
  _dir.normalise();

  Move(_dir * SPEED);
  SetRotation(_dir.angle());
}

void Square::Render() const
{
  if (GetNonWallCount() == 0 && (_timer % 4 == 1 || _timer % 4 == 2)) {
    RenderWithColour(0x33333300);
  }
  else {
    Enemy::Render();
  }
}

// Wall enemy
//------------------------------
Wall::Wall(const vec2& position, bool rdir)
  : Enemy(position, Ship::SHIP_WALL, 10)
  , _dir(0, 1)
  , _timer(0)
  , _rotate(false)
  , _rdir(rdir)
{
  AddShape(new Box(vec2(), 10, 40, 0x33cc33ff, 0, DANGEROUS | VULNERABLE));
  SetScore(20);
  SetBoundingWidth(50);
  SetEnemyValue(4);
}

void Wall::Update()
{
  if (GetNonWallCount() == 0 && _timer % 8 < 2) {
    if (GetHP() > 2) {
      PlaySound(Lib::SOUND_ENEMY_SPAWN);
    }
    Damage(GetHP() - 2, false, 0);
  }

  if (_rotate) {
    vec2 d(_dir);
    d.rotate(
        (_rdir ? _timer - TIMER : TIMER - _timer) * fixed::pi / (4 * TIMER));

    SetRotation(d.angle());
    _timer--;
    if (_timer <= 0) {
      _timer = 0;
      _rotate = false;
      _dir.rotate(_rdir ? -fixed::pi / 4 : fixed::pi / 4);
    }
    return;
  }
  else {
    _timer++;
    if (_timer > TIMER * 6) {
      if (IsOnScreen()) {
        _timer = TIMER;
        _rotate = true;
      }
      else {
        _timer = 0;
      }
    }
  }

  vec2 pos = GetPosition();
  if ((pos.x < 0 && _dir.x < -fixed::hundredth) ||
      (pos.y < 0 && _dir.y < -fixed::hundredth) ||
      (pos.x > Lib::WIDTH && _dir.x > fixed::hundredth) ||
      (pos.y > Lib::HEIGHT && _dir.y > fixed::hundredth)) {
    _dir = vec2() - _dir;
    _dir.normalise();
  }

  Move(_dir * SPEED);
  SetRotation(_dir.angle());
}

void Wall::OnDestroy(bool bomb)
{
  if (bomb) {
    return;
  }
  vec2 d = _dir;
  d.rotate(fixed::pi / 2);

  vec2 v = GetPosition() + d * 10 * 3;
  if (v.x >= 0 && v.x <= Lib::WIDTH && v.y >= 0 && v.y <= Lib::HEIGHT) {
    Spawn(new Square(v, GetRotation()));
  }

  v = GetPosition() - d * 10 * 3;
  if (v.x >= 0 && v.x <= Lib::WIDTH && v.y >= 0 && v.y <= Lib::HEIGHT) {
    Spawn(new Square(v, GetRotation()));
  }
}

// Follow spawner enemy
//------------------------------
FollowHub::FollowHub(const vec2& position, bool powerA, bool powerB)
  : Enemy(position, Ship::SHIP_NONE, 14)
  , _timer(0)
  , _dir(0, 0)
  , _count(0)
  , _powerA(powerA)
  , _powerB(powerB)
{
  AddShape(new Polygram(
      vec2(), 16, 4, 0x6666ffff, fixed::pi / 4, DANGEROUS | VULNERABLE));
  if (_powerB) {
    AddShape(new Polystar(vec2(16, 0), 8, 4, 0x6666ffff, fixed::pi / 4));
    AddShape(new Polystar(vec2(-16, 0), 8, 4, 0x6666ffff, fixed::pi / 4));
    AddShape(new Polystar(vec2(0, 16), 8, 4, 0x6666ffff, fixed::pi / 4));
    AddShape(new Polystar(vec2(0, -16), 8, 4, 0x6666ffff, fixed::pi / 4));
  }

  AddShape(new Polygon(vec2(16, 0), 8, 4, 0x6666ffff, fixed::pi / 4));
  AddShape(new Polygon(vec2(-16, 0), 8, 4, 0x6666ffff, fixed::pi / 4));
  AddShape(new Polygon(vec2(0, 16), 8, 4, 0x6666ffff, fixed::pi / 4));
  AddShape(new Polygon(vec2(0, -16), 8, 4, 0x6666ffff, fixed::pi / 4));
  SetScore(50 + _powerA * 10 + _powerB * 10);
  SetBoundingWidth(16);
  SetDestroySound(Lib::SOUND_PLAYER_DESTROY);
  SetEnemyValue(6 + (powerA ? 2 : 0) + (powerB ? 2 : 0));
}

void FollowHub::Update()
{
  _timer++;
  if (_timer > (_powerA ? TIMER / 2 : TIMER)) {
    _timer = 0;
    _count++;
    if (IsOnScreen()) {
      if (_powerB) {
        Spawn(new Chaser(GetPosition()));
      }
      else {
        Spawn(new Follow(GetPosition()));
      }
      PlaySoundRandom(Lib::SOUND_ENEMY_SPAWN);
    }
  }

  if (GetPosition().x < 0) {
    _dir.set(1, 0);
  }
  else if (GetPosition().x > Lib::WIDTH) {
    _dir.set(-1, 0);
  }
  else if (GetPosition().y < 0) {
    _dir.set(0, 1);
  }
  else if (GetPosition().y > Lib::HEIGHT) {
    _dir.set(0, -1);
  }
  else if (_count > 3) {
    _dir.rotate(-fixed::pi / 2);
    _count = 0;
  }

  fixed s = _powerA ?
      fixed::hundredth * 5 + fixed::tenth : fixed::hundredth * 5;
  Rotate(s);
  GetShape(0).Rotate(-s);

  Move(_dir * SPEED);
}

void FollowHub::OnDestroy(bool bomb)
{
  if (bomb) {
    return;
  }
  if (_powerB) {
    Spawn(new BigFollow(GetPosition(), true));
  }
  Spawn(new Chaser(GetPosition()));
}

// Shielder enemy
//------------------------------
Shielder::Shielder(const vec2& position, bool power)
  : Enemy(position, Ship::SHIP_NONE, 16)
  , _dir(0, 1)
  , _timer(0)
  , _rotate(false)
  , _rDir(false)
  , _power(power)
{
  AddShape(new Polystar(vec2(24, 0), 8, 6, 0x006633ff, 0, VULNSHIELD));
  AddShape(new Polystar(vec2(-24, 0), 8, 6, 0x006633ff, 0, VULNSHIELD));
  AddShape(new Polystar(vec2(0, 24), 8, 6, 0x006633ff,
                        fixed::pi / 2, VULNSHIELD));
  AddShape(new Polystar(vec2(0, -24), 8, 6, 0x006633ff,
                        fixed::pi / 2, VULNSHIELD));
  AddShape(new Polygon(vec2(24, 0), 8, 6, 0x33cc99ff, 0, 0));
  AddShape(new Polygon(vec2(-24, 0), 8, 6, 0x33cc99ff, 0, 0));
  AddShape(new Polygon(vec2(0, 24), 8, 6, 0x33cc99ff, fixed::pi / 2, 0));
  AddShape(new Polygon(vec2(0, -24), 8, 6, 0x33cc99ff, fixed::pi / 2, 0));

  AddShape(new Polystar(vec2(0, 0), 24, 4, 0x006633ff, 0, 0));
  AddShape(new Polygon(vec2(0, 0), 14, 8, power ? 0x33cc99ff : 0x006633ff,
                       0, DANGEROUS | VULNERABLE));
  SetScore(60 + _power * 40);
  SetBoundingWidth(36);
  SetDestroySound(Lib::SOUND_PLAYER_DESTROY);
  SetEnemyValue(8 + (power ? 2 : 0));
}

void Shielder::Update()
{
  fixed s = _power ? fixed::hundredth * 12 : fixed::hundredth * 4;
  Rotate(s);
  GetShape(9).Rotate(-2 * s);
  for (int i = 0; i < 8; i++) {
    GetShape(i).Rotate(-s);
  }

  bool onScreen = false;
  if (GetPosition().x < 0) {
    _dir.set(1, 0);
  }
  else if (GetPosition().x > Lib::WIDTH) {
    _dir.set(-1, 0);
  }
  else if (GetPosition().y < 0) {
    _dir.set(0, 1);
  }
  else if (GetPosition().y > Lib::HEIGHT) {
    _dir.set(0, -1);
  }
  else {
    onScreen = true;
  }

  if (!onScreen && _rotate) {
    _timer = 0;
    _rotate = false;
  }

  fixed speed = SPEED +
      (_power ? fixed::tenth * 3 : fixed::tenth * 2) * (16 - GetHP());
  if (_rotate) {
    vec2 d(_dir);
    d.rotate((_rDir ? 1 : -1) * (TIMER - _timer) * fixed::pi / (2 * TIMER));
    _timer--;
    if (_timer <= 0) {
      _timer = 0;
      _rotate = false;
      _dir.rotate((_rDir ? 1 : -1) * fixed::pi / 2);
    }
    Move(d * speed);
  }
  else {
    _timer++;
    if (_timer > TIMER * 2) {
      _timer = TIMER;
      _rotate = true;
      _rDir = z::rand_int(2) != 0;
    }
    if (IsOnScreen() && _timer % TIMER == TIMER / 2 && _power) {
      Player* p = GetNearestPlayer();
      vec2 v = GetPosition();

      vec2 d = p->GetPosition() - v;
      d.normalise();
      Spawn(new SBBossShot(v, d * 3, 0x33cc99ff));
      PlaySoundRandom(Lib::SOUND_BOSS_FIRE);
    }
    Move(_dir * speed);
  }
  _dir.normalise();

}

// Tractor beam enemy
//------------------------------
Tractor::Tractor(const vec2& position, bool power)
  : Enemy(position, Ship::SHIP_NONE, 50)
  , _timer(TIMER * 4)
  , _dir(0, 0)
  , _power(power)
  , _ready(false)
  , _spinning(false)
{
  AddShape(new Polygram(vec2(24, 0), 12, 6,
                        0xcc33ccff, 0, DANGEROUS | VULNERABLE));
  AddShape(new Polygram(vec2(-24, 0), 12, 6,
                        0xcc33ccff, 0, DANGEROUS | VULNERABLE));
  AddShape(new Line(vec2(0, 0), vec2(24, 0), vec2(-24, 0), 0xcc33ccff));
  if (power) {
    AddShape(new Polystar(vec2(24, 0), 16, 6, 0xcc33ccff, 0, 0));
    AddShape(new Polystar(vec2(-24, 0), 16, 6, 0xcc33ccff, 0, 0));
  }
  SetScore(85 + power * 40);
  SetBoundingWidth(36);
  SetDestroySound(Lib::SOUND_PLAYER_DESTROY);
  SetEnemyValue(10 + (power ? 2 : 0));
}

void Tractor::Update()
{
  GetShape(0).Rotate(fixed::hundredth * 5);
  GetShape(1).Rotate(-fixed::hundredth * 5);
  if (_power) {
    GetShape(3).Rotate(-fixed::hundredth * 8);
    GetShape(4).Rotate(fixed::hundredth * 8);
  }

  if (GetPosition().x < 0) {
    _dir.set(1, 0);
  }
  else if (GetPosition().x > Lib::WIDTH) {
    _dir.set(-1, 0);
  }
  else if (GetPosition().y < 0) {
    _dir.set(0, 1);
  }
  else if (GetPosition().y > Lib::HEIGHT) {
    _dir.set(0, -1);
  }
  else {
    _timer++;
  }

  if (!_ready && !_spinning) {
    Move(_dir * SPEED * (IsOnScreen() ? 1 : 2 + fixed::half));

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
      _players = GetPlayers();
      PlaySound(Lib::SOUND_BOSS_FIRE);
    }
  }
  else if (_spinning) {
    Rotate(fixed::tenth * 3);
    for (unsigned int i = 0; i < _players.size(); i++) {
      if (!((Player*) _players[i])->IsKilled()) {
        vec2 d = GetPosition() - _players[i]->GetPosition();
        d.normalise();
        _players[i]->Move(d * TRACTOR_SPEED);
      }
    }

    if (_timer % (TIMER / 2) == 0 && IsOnScreen() && _power) {
      Player* p = GetNearestPlayer();
      vec2 v = GetPosition();

      vec2 d = p->GetPosition() - v;
      d.normalise();
      Spawn(new SBBossShot(v, d * 4, 0xcc33ccff));
      PlaySoundRandom(Lib::SOUND_BOSS_FIRE);
    }

    if (_timer > TIMER * 5) {
      _spinning = false;
      _timer = 0;
    }
  }
}

void Tractor::Render() const
{
  Enemy::Render();
  if (_spinning) {
    for (unsigned int i = 0; i < _players.size(); i++) {
      if (((_timer + i * 4) / 4) % 2 && !((Player*) _players[i])->IsKilled()) {
        GetLib().RenderLine(to_float(GetPosition()),
                            to_float(_players[i]->GetPosition()), 0xcc33ccff);
      }
    }
  }
}
