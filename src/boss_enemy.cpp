#include "boss_enemy.h"
#include "boss.h"
#include "player.h"

const fixed GhostWall::SPEED = 3 + fixed(1) / 2;
const fixed DeathRay::SPEED = 10;

// Big follower
//------------------------------
BigFollow::BigFollow(const vec2& position, bool hasScore)
  : Follow(position, 20, 3)
  , _hasScore(hasScore)
{
  SetScore(hasScore ? 20 : 0);
  SetDestroySound(Lib::SOUND_PLAYER_DESTROY);
  SetEnemyValue(3);
}

void BigFollow::OnDestroy(bool bomb)
{
  if (bomb) {
    return;
  }

  vec2 d(10, 0);
  d.rotate(GetRotation());
  for (int i = 0; i < 3; i++) {
    Follow* s = new Follow(GetPosition() + d);
    if (!_hasScore) {
      s->SetScore(0);
    }
    Spawn(s);
    d.rotate(2 * fixed::pi / 3);
  }
}

// Generic boss projectile
//------------------------------
SBBossShot::SBBossShot(const vec2& position, const vec2& velocity, colour c)
  : Enemy(position, Ship::SHIP_WALL, 0)
  , _dir(velocity)
{
  AddShape(new Polystar(vec2(), 16, 8, c, 0, 0));
  AddShape(new Polygon(vec2(), 10, 8, c, 0, 0));
  AddShape(new Polygon(vec2(), 12, 8, 0, 0, DANGEROUS));
  SetBoundingWidth(12);
  SetScore(0);
  SetEnemyValue(1);
}

void SBBossShot::Update()
{
  Move(_dir);
  vec2 p = GetPosition();
  if ((p.x < -10 && _dir.x < 0) ||
      (p.x > Lib::WIDTH + 10 && _dir.x > 0) ||
      (p.y < -10 && _dir.y < 0) ||
      (p.y > Lib::HEIGHT + 10 && _dir.y > 0)) {
    destroy();
  }
  SetRotation(GetRotation() + fixed::hundredth * 2);
}

// Tractor beam minion
//------------------------------
TBossShot::TBossShot(const vec2& position, fixed angle)
  : Enemy(position, Ship::SHIP_NONE, 1)
{
  AddShape(new Polygon(vec2(), 8, 6, 0xcc33ccff, 0, DANGEROUS | VULNERABLE));
  _dir.set_polar(angle, 3);
  SetBoundingWidth(8);
  SetScore(0);
  SetDestroySound(Lib::SOUND_ENEMY_SHATTER);
}

void TBossShot::Update()
{
  if ((GetPosition().x > Lib::WIDTH && _dir.x > 0) ||
      (GetPosition().x < 0 && _dir.x < 0)) {
    _dir.x = -_dir.x;
  }
  if ((GetPosition().y > Lib::HEIGHT && _dir.y > 0) ||
      (GetPosition().y < 0 && _dir.y < 0)) {
    _dir.y = -_dir.y;
  }

  Move(_dir);
}

// Ghost boss wall
//------------------------------
GhostWall::GhostWall(bool swap, bool noGap, bool ignored)
  : Enemy(vec2(Lib::WIDTH / 2, swap ? -10 : 10 + Lib::HEIGHT),
          Ship::SHIP_NONE, 0)
  , _dir(0, swap ? 1 : -1)
{
  if (noGap) {
    AddShape(new Box(vec2(), fixed(Lib::WIDTH), 10, 0xcc66ffff,
                     0, DANGEROUS | SHIELD));
    AddShape(new Box(vec2(), fixed(Lib::WIDTH), 7, 0xcc66ffff, 0, 0));
    AddShape(new Box(vec2(), fixed(Lib::WIDTH), 4, 0xcc66ffff, 0, 0));
  }
  else {
    AddShape(new Box(vec2(), Lib::HEIGHT / 2, 10, 0xcc66ffff,
                     0, DANGEROUS | SHIELD));
    AddShape(new Box(vec2(), Lib::HEIGHT / 2, 7, 0xcc66ffff,
                     0, DANGEROUS | SHIELD));
    AddShape(new Box(vec2(), Lib::HEIGHT / 2, 4, 0xcc66ffff,
                     0, DANGEROUS | SHIELD));
  }
  SetBoundingWidth(fixed(Lib::WIDTH));
}

GhostWall::GhostWall(bool swap, bool swapGap)
  : Enemy(vec2(swap ? -10 : 10 + Lib::WIDTH, Lib::HEIGHT / 2),
          Ship::SHIP_NONE, 0)
  , _dir(swap ? 1 : -1, 0)
{
  SetBoundingWidth(fixed(Lib::HEIGHT));
  fixed off = swapGap ? -100 : 100;

  AddShape(new Box(vec2(0, off - 20 - Lib::HEIGHT / 2), 10,
                   Lib::HEIGHT / 2, 0xcc66ffff, 0, DANGEROUS | SHIELD));
  AddShape(new Box(vec2(0, off + 20 + Lib::HEIGHT / 2), 10,
                   Lib::HEIGHT / 2, 0xcc66ffff, 0, DANGEROUS | SHIELD));

  AddShape(new Box(vec2(0, off - 20 - Lib::HEIGHT / 2), 7,
                   Lib::HEIGHT / 2, 0xcc66ffff, 0, 0));
  AddShape(new Box(vec2(0, off + 20 + Lib::HEIGHT / 2), 7,
                   Lib::HEIGHT / 2, 0xcc66ffff, 0, 0));

  AddShape(new Box(vec2(0, off - 20 - Lib::HEIGHT / 2), 4,
                   Lib::HEIGHT / 2, 0xcc66ffff, 0, 0));
  AddShape(new Box(vec2(0, off + 20 + Lib::HEIGHT / 2), 4,
                   Lib::HEIGHT / 2, 0xcc66ffff, 0, 0));
}

void GhostWall::Update()
{
  if ((_dir.x > 0 && GetPosition().x < 32) ||
      (_dir.y > 0 && GetPosition().y < 16) ||
      (_dir.x < 0 && GetPosition().x >= Lib::WIDTH - 32) ||
      (_dir.y < 0 && GetPosition().y >= Lib::HEIGHT - 16)) {
    Move(_dir * SPEED / 2);
  }
  else {
    Move(_dir * SPEED);
  }

  if ((_dir.x > 0 && GetPosition().x > Lib::WIDTH + 10) ||
      (_dir.y > 0 && GetPosition().y > Lib::HEIGHT + 10) ||
      (_dir.x < 0 && GetPosition().x < -10) ||
      (_dir.y < 0 && GetPosition().y < -10)) {
    destroy();
  }
}

// Ghost boss mine
//------------------------------
GhostMine::GhostMine(const vec2& position, Boss* ghost)
  : Enemy(position, Ship::SHIP_NONE, 0)
  , _timer(80)
  , _ghost(ghost)
{
  AddShape(new Polygon(vec2(), 24, 8, 0x9933ccff, 0, 0));
  AddShape(new Polygon(vec2(), 20, 8, 0x9933ccff, 0, 0));
  SetBoundingWidth(24);
  SetScore(0);
}

void GhostMine::Update()
{
  if (_timer == 80) {
    Explosion();
    SetRotation(z::rand_fixed() * 2 * fixed::pi);
  }
  if (_timer) {
    _timer--;
    if (!_timer) {
      GetShape(0).SetCategory(DANGEROUS | SHIELD | VULNSHIELD);
    }
  }
  z0Game::ShipList s = GetCollisionList(GetPosition(), DANGEROUS);
  for (unsigned int i = 0; i < s.size(); i++) {
    if (s[i] == _ghost) {
      Enemy* e = z::rand_int(6) == 0 ||
          (_ghost->IsHPLow() && z::rand_int(5) == 0) ?
              new BigFollow(GetPosition(), false) : new Follow(GetPosition());
      e->SetScore(0);
      Spawn(e);
      Damage(1, false, 0);
      break;
    }
  }
}

void GhostMine::Render() const
{
  if (!((_timer / 4) % 2)) {
    Enemy::Render();
  }
}

// Death ray
//------------------------------
DeathRay::DeathRay(const vec2& position)
  : Enemy(position, Ship::SHIP_NONE, 0)
{
  AddShape(new Box(vec2(), 10, 48, 0, 0, DANGEROUS));
  AddShape(new Line(vec2(), vec2(0, -48), vec2(0, 48), 0xffffffff, 0));
  SetBoundingWidth(48);
}

void DeathRay::Update()
{
  Move(vec2(1, 0) * SPEED);
  if (GetPosition().x > Lib::WIDTH + 20) {
    destroy();
  }
}

// Death arm
//------------------------------
DeathArm::DeathArm(DeathRayBoss* boss, bool top, int hp)
  : Enemy(vec2(), Ship::SHIP_NONE, hp)
  , _boss(boss)
  , _top(top)
  , _timer(top ? 2 * DeathRayBoss::ARM_ATIMER / 3 : 0)
  , _attacking(false)
  , _dir()
  , _start(30)
  , _shots(0)
{
  AddShape(new Polygon(vec2(), 60, 4, 0x33ff99ff, 0, 0));
  AddShape(new Polygram(vec2(), 50, 4, 0x228855ff, 0, VULNERABLE));
  AddShape(new Polygon(vec2(), 40, 4, 0, 0, SHIELD));
  AddShape(new Polygon(vec2(), 20, 4, 0x33ff99ff, 0, 0));
  AddShape(new Polygon(vec2(), 18, 4, 0x228855ff, 0, 0));
  SetBoundingWidth(60);
  SetDestroySound(Lib::SOUND_PLAYER_DESTROY);
}

void DeathArm::Update()
{
  if (_timer % (DeathRayBoss::ARM_ATIMER / 2) == DeathRayBoss::ARM_ATIMER / 4) {
    PlaySoundRandom(Lib::SOUND_BOSS_FIRE);
    _target = GetNearestPlayer()->GetPosition();
    _shots = 16;
  }
  if (_shots > 0) {
    vec2 d = _target - GetPosition();
    d.normalise();
    d *= 5;
    Ship* s = new SBBossShot(GetPosition(), d, 0x33ff99ff);
    Spawn(s);
    --_shots;
  }

  Rotate(fixed::hundredth * 5);
  if (_attacking) {
    _timer++;
    if (_timer < DeathRayBoss::ARM_ATIMER / 4) {
      Player* p = GetNearestPlayer();
      vec2 d = p->GetPosition() - GetPosition();
      if (d.length() != 0) {
        _dir = d;
        _dir.normalise();
        Move(_dir * DeathRayBoss::ARM_SPEED);
      }
    }
    else if (_timer < DeathRayBoss::ARM_ATIMER / 2) {
      Move(_dir * DeathRayBoss::ARM_SPEED);
    }
    else if (_timer < DeathRayBoss::ARM_ATIMER) {
      vec2 d = _boss->GetPosition() +
          vec2(80, _top ? 80 : -80) - GetPosition();
      if (d.length() > DeathRayBoss::ARM_SPEED) {
        d.normalise();
        Move(d * DeathRayBoss::ARM_SPEED);
      }
      else {
        _attacking = false;
        _timer = 0;
      }
    }
    else if (_timer >= DeathRayBoss::ARM_ATIMER) {
      _attacking = false;
      _timer = 0;
    }
    return;
  }

  _timer++;
  if (_timer >= DeathRayBoss::ARM_ATIMER) {
    _timer = 0;
    _attacking = true;
    _dir.set(0, 0);
    PlaySound(Lib::SOUND_BOSS_ATTACK);
  }
  SetPosition(_boss->GetPosition() + vec2(80, _top ? 80 : -80));

  if (_start) {
    if (_start == 30) {
      Explosion();
      Explosion(0xffffffff);
    }
    _start--;
    if (!_start) {
      GetShape(1).SetCategory(DANGEROUS | VULNERABLE);
    }
  }
}

void DeathArm::OnDestroy(bool bomb)
{
  _boss->OnArmDeath(this);
  Explosion();
  Explosion(0xffffffff, 12);
  Explosion(GetShape(0).GetColour(), 24);
}

// Snake tail
//------------------------------
SnakeTail::SnakeTail(const vec2& position, colour colour)
  : Enemy(position, Ship::SHIP_NONE, 1)
  , _tail(0)
  , _head(0)
  , _timer(150)
  , _dTimer(0)
{
  AddShape(
      new Polygon(vec2(), 10, 4, colour, 0, DANGEROUS | SHIELD | VULNSHIELD));
  SetBoundingWidth(22);
  SetScore(0);
}

void SnakeTail::Update()
{
  static const fixed z15 = fixed::hundredth * 15;
  Rotate(z15);
  --_timer;
  if (!_timer) {
    OnDestroy(false);
    destroy();
    Explosion();
  }
  if (_dTimer) {
    --_dTimer;
    if (!_dTimer) {
      if (_tail) {
        _tail->_dTimer = 4;
      }
      OnDestroy(false);
      destroy();
      Explosion();
      PlaySoundRandom(Lib::SOUND_ENEMY_DESTROY);
    }
  }
}

void SnakeTail::OnDestroy(bool bomb)
{
  if (_head) {
    _head->_tail = 0;
  }
  if (_tail) {
    _tail->_head = 0;
  }
  _head = 0;
  _tail = 0;
}

// Snake
//------------------------------
Snake::Snake(const vec2& position, colour colour, const vec2& dir, fixed rot)
  : Enemy(position, Ship::SHIP_NONE, 5)
  , _tail(0)
  , _timer(0)
  , _dir(0, 0)
  , _count(0)
  , _colour(colour)
  , _shotSnake(false)
  , _shotRot(rot)
{
  AddShape(new Polygon(vec2(), 14, 3, colour, 0, VULNERABLE));
  AddShape(new Polygon(vec2(), 10, 3, 0, 0, DANGEROUS));
  SetScore(0);
  SetBoundingWidth(32);
  SetEnemyValue(5);
  SetDestroySound(Lib::SOUND_PLAYER_DESTROY);
  if (dir == vec2()) {
    int r = z::rand_int(4);
    if (r == 0) {
      _dir = vec2(1, 0);
    }
    else if (r == 1) {
      _dir = vec2(-1, 0);
    }
    else if (r == 2) {
      _dir = vec2(0, 1);
    }
    else {
      _dir = vec2(0, -1);
    }
  }
  else {
    _dir = dir;
    _dir.normalise();
    _shotSnake = true;
  }
  SetRotation(_dir.angle());
}

void Snake::Update()
{
  if (!(GetPosition().x >= -8 && GetPosition().x <= Lib::WIDTH + 8 &&
        GetPosition().y >= -8 && GetPosition().y <= Lib::HEIGHT + 8)) {
    _tail = 0;
    destroy();
    return;
  }

  colour c = z::colour_cycle(_colour, _timer % 256);
  GetShape(0).SetColour(c);
  _timer++;
  if (_timer % (_shotSnake ? 4 : 8) == 0) {
    SnakeTail* t = new SnakeTail(GetPosition(), (c & 0xffffff00) | 0x00000099);
    if (_tail != 0) {
      _tail->_head = t;
      t->_tail = _tail;
    }
    PlaySoundRandom(Lib::SOUND_BOSS_FIRE, 0, .5f);
    _tail = t;
    Spawn(t);
  }
  if (!_shotSnake && _timer % 48 == 0 && !z::rand_int(3)) {
    _dir.rotate((z::rand_int(2) ? 1 : -1) * fixed::pi / 2);
    SetRotation(_dir.angle());
  }
  Move(_dir * (_shotSnake ? 4 : 2));
  if (_timer % 8 == 0) {
    _dir.rotate(8 * _shotRot);
    SetRotation(_dir.angle());
  }
}

void Snake::OnDestroy(bool bomb)
{
  if (_tail) {
    _tail->_dTimer = 4;
  }
}

// Rainbow projectile
//------------------------------
RainbowShot::RainbowShot(const vec2& position, const vec2& velocity, Ship* boss)
  : SBBossShot(position, velocity)
  , _boss(boss)
  , _timer(0)
{
}

void RainbowShot::Update()
{
  SBBossShot::Update();
  static const vec2 center = vec2(Lib::WIDTH / 2, Lib::HEIGHT / 2);

  if ((GetPosition() - center).length() > 100 && _timer % 2 == 0) {
    z0Game::ShipList list = GetCollisionList(GetPosition(), SHIELD);
    SuperBoss* s = (SuperBoss*) _boss;
    for (std::size_t i = 0; i < list.size(); ++i) {
      bool boss = false;
      for (std::size_t j = 0; j < s->_arcs.size(); ++j) {
        if (list[i] == s->_arcs[j]) {
          boss = true;
          break;
        }
      }
      if (!boss) {
        continue;
      }

      Explosion(0, 4, true, to_float(GetPosition() - _dir));
      destroy();
      return;
    }
  }

  ++_timer;
  static const fixed r = 6 * (8 * fixed::hundredth / 10);
  if (_timer % 8 == 0) {
    _dir.rotate(r);
  }
}
