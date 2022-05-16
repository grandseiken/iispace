#ifndef IISPACE_GAME_LOGIC_BOSS_GHOST_H
#define IISPACE_GAME_LOGIC_BOSS_GHOST_H
#include "game/logic/boss.h"
#include "game/logic/enemy.h"

class GhostBoss : public Boss {
public:
  GhostBoss(int32_t players, int32_t cycle);

  void update() override;
  void render() const override;
  int32_t get_damage(int32_t damage, bool magic) override;

private:
  bool _visible;
  int32_t _vtime;
  int32_t _timer;
  int32_t _attack_time;
  int32_t _attack;
  bool _rDir;
  int32_t _start_time;
  int32_t _danger_circle;
  int32_t _danger_offset1;
  int32_t _danger_offset2;
  int32_t _danger_offset3;
  int32_t _danger_offset4;
  bool _shot_type;
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

#endif