#ifndef IISPACE_SRC_BOSS_ENEMY_H
#define IISPACE_SRC_BOSS_ENEMY_H

#include "enemy.h"
class Boss;

class BigFollow : public Follow {
public:

  BigFollow(const vec2& position, bool has_score);
  void on_destroy(bool bomb) override;

private:

  bool _has_score;

};

class SBBossShot : public Enemy {
public:

  SBBossShot(const vec2& position, const vec2& velocity,
             colour_t c = 0x999999ff);
  void update() override;

protected:

  vec2 _dir;

};

class TBossShot : public Enemy {
public:

  TBossShot(const vec2& position, fixed angle);
  void update() override;

private:

  vec2 _dir;

};

class GhostWall : public Enemy {
public:

  GhostWall(bool swap, bool no_gap, bool ignored);
  GhostWall(bool swap, bool swap_gap);
  void update() override;

private:

  vec2 _dir;

};

class GhostMine : public Enemy {
public:

  GhostMine(const vec2& position, Boss* ghost);
  void update() override;
  void render() const override;

private:

  int32_t _timer;
  Boss* _ghost;

};

class DeathRay : public Enemy {
public:

  DeathRay(const vec2& position);
  void update() override;

};

class DeathRayBoss;
class DeathArm : public Enemy {
public:

  DeathArm(DeathRayBoss* boss, bool top, int32_t hp);
  void update() override;
  void on_destroy(bool bomb) override;

private:

  DeathRayBoss* _boss;
  bool _top;
  int32_t _timer;
  bool _attacking;
  vec2 _dir;
  int32_t _start;

  vec2 _target;
  int32_t _shots;

};

class SnakeTail : public Enemy {
  friend class Snake;
public:

  SnakeTail(const vec2& position, colour_t colour);
  void update() override;
  void on_destroy(bool bomb) override;

private:

  SnakeTail* _tail;
  SnakeTail* _head;
  int32_t _timer;
  int32_t _dtimer;

};

class Snake : public Enemy {
public:

  Snake(const vec2& position, colour_t colour,
        const vec2& dir = vec2(), fixed rot = 0);
  void update() override;
  void on_destroy(bool bomb) override;

private:

  SnakeTail* _tail;
  int32_t _timer;
  vec2 _dir;
  int32_t _count;
  colour_t _colour;
  bool _shot_snake;
  fixed _shot_rot;

};

class RainbowShot : public SBBossShot {
public:

  RainbowShot(const vec2& position, const vec2& velocity, Ship* boss);
  void update() override;

private:

  Ship* _boss;
  int32_t _timer;

};

#endif
