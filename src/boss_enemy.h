#ifndef IISPACE_SRC_BOSS_ENEMY_H
#define IISPACE_SRC_BOSS_ENEMY_H

#include "enemy.h"
class Boss;

// Big follow
//------------------------------
class BigFollow : public Follow {
public:

  BigFollow(const Vec2& position, bool hasScore);
  virtual ~BigFollow() {}
  virtual void OnDestroy(bool bomb);

private:

  bool _hasScore;

};

// Generic boss shot
//------------------------------
class SBBossShot : public Enemy {
public:

  SBBossShot(const Vec2& position, const Vec2& velocity, Colour c = 0x999999ff);
  virtual ~SBBossShot() {}

  virtual void Update();

  virtual bool IsWall() const
  {
    return true;
  }

protected:

  Vec2 _dir;

};

// Tractor beam shot
//------------------------------
class TBossShot : public Enemy {
public:

  TBossShot(const Vec2& position, fixed angle);
  virtual ~TBossShot() {}

  virtual void Update();

private:

  Vec2 _dir;

};

// Ghost boss wall
//------------------------------
class GhostWall : public Enemy {
public:

  static const fixed SPEED;

  GhostWall(bool swap, bool noGap, bool ignored);
  GhostWall(bool swap, bool swapGap);
  virtual ~GhostWall() {}

  virtual void Update();

private:

  Vec2 _dir;

};

// Ghost boss mine
//------------------------------
class GhostMine : public Enemy {
public:

  GhostMine(const Vec2& position, Boss* ghost);
  virtual ~GhostMine() {}

  virtual void Update();
  virtual void Render() const;

private:

  int _timer;
  Boss* _ghost;

};

// Death ray
//------------------------------
class DeathRay : public Enemy {
public:

  static const fixed SPEED;

  DeathRay(const Vec2& position);
  virtual ~DeathRay() {}

  virtual void Update();

};

// Death ray boss arm
//------------------------------
class DeathArm : public Enemy {
public:

  DeathArm(DeathRayBoss* boss, bool top, int hp);
  virtual ~DeathArm() {}

  virtual void Update();
  virtual void OnDestroy(bool bomb);

private:

  DeathRayBoss* _boss;
  bool _top;
  int _timer;
  bool _attacking;
  Vec2 _dir;
  int _start;

  Vec2 _target;
  int _shots;

};

// Snake tail
//------------------------------
class SnakeTail : public Enemy {
  friend class Snake;
public:

  SnakeTail(const Vec2& position, Colour colour);
  virtual ~SnakeTail() {}

  virtual void Update();
  virtual void OnDestroy(bool bomb);

private:

  SnakeTail* _tail;
  SnakeTail* _head;
  int _timer;
  int _dTimer;

};

// Snake
//------------------------------
class Snake : public Enemy {
public:

  Snake(const Vec2& position, Colour colour,
        const Vec2& dir = Vec2(), fixed rot = M_ZERO);
  virtual ~Snake() {}

  virtual void Update();
  virtual void OnDestroy(bool bomb);

private:

  SnakeTail* _tail;
  int _timer;
  Vec2 _dir;
  int _count;
  Colour _colour;
  bool _shotSnake;
  fixed _shotRot;

};

// Rainbow shot
//------------------------------
class RainbowShot : public SBBossShot {
public:

  RainbowShot(const Vec2& position, const Vec2& velocity, Ship* boss);
  virtual ~RainbowShot() {}

  virtual void Update();

private:

  Ship* _boss;
  int _timer;

};

#endif
