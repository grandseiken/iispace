#ifndef IISPACE_SRC_BOSS_ENEMY_H
#define IISPACE_SRC_BOSS_ENEMY_H

#include "enemy.h"
class Boss;

// Big follow
//------------------------------
class BigFollow : public Follow {
public:

  BigFollow(const vec2& position, bool has_score);
  void on_destroy(bool bomb) override;

private:

  bool _has_score;

};

// Generic boss shot
//------------------------------
class SBBossShot : public Enemy {
public:

  SBBossShot(const vec2& position, const vec2& velocity,
             colour_t c = 0x999999ff);
  void update() override;

protected:

  vec2 _dir;

};

// Tractor beam shot
//------------------------------
class TBossShot : public Enemy {
public:

  TBossShot(const vec2& position, fixed angle);
  void update() override;

private:

  vec2 _dir;

};

// Ghost boss wall
//------------------------------
class GhostWall : public Enemy {
public:

  static const fixed SPEED;

  GhostWall(bool swap, bool no_gap, bool ignored);
  GhostWall(bool swap, bool swap_gap);
  void update() override;

private:

  vec2 _dir;

};

// Ghost boss mine
//------------------------------
class GhostMine : public Enemy {
public:

  GhostMine(const vec2& position, Boss* ghost);
  void update() override;
  void render() const override;

private:

  int _timer;
  Boss* _ghost;

};

// Death ray
//------------------------------
class DeathRay : public Enemy {
public:

  static const fixed SPEED;

  DeathRay(const vec2& position);
  void update() override;

};

// Death ray boss arm
//------------------------------
class DeathRayBoss;
class DeathArm : public Enemy {
public:

  DeathArm(DeathRayBoss* boss, bool top, int hp);
  void update() override;
  void on_destroy(bool bomb) override;

private:

  DeathRayBoss* _boss;
  bool _top;
  int _timer;
  bool _attacking;
  vec2 _dir;
  int _start;

  vec2 _target;
  int _shots;

};

// Snake tail
//------------------------------
class SnakeTail : public Enemy {
  friend class Snake;
public:

  SnakeTail(const vec2& position, colour_t colour);
  void update() override;
  void on_destroy(bool bomb) override;

private:

  SnakeTail* _tail;
  SnakeTail* _head;
  int _timer;
  int _dtimer;

};

// Snake
//------------------------------
class Snake : public Enemy {
public:

  Snake(const vec2& position, colour_t colour,
        const vec2& dir = vec2(), fixed rot = 0);
  void update() override;
  void on_destroy(bool bomb) override;

private:

  SnakeTail* _tail;
  int _timer;
  vec2 _dir;
  int _count;
  colour_t _colour;
  bool _shot_snake;
  fixed _shot_rot;

};

// Rainbow shot
//------------------------------
class RainbowShot : public SBBossShot {
public:

  RainbowShot(const vec2& position, const vec2& velocity, Ship* boss);
  void update() override;

private:

  Ship* _boss;
  int _timer;

};

#endif
