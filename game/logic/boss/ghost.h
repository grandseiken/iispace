#ifndef IISPACE_GAME_LOGIC_BOSS_GHOST_H
#define IISPACE_GAME_LOGIC_BOSS_GHOST_H
#include "game/logic/boss.h"
#include "game/logic/enemy.h"

class GhostBoss : public Boss {
public:
  GhostBoss(std::int32_t players, std::int32_t cycle);

  void update() override;
  void render() const override;
  std::int32_t get_damage(std::int32_t damage, bool magic) override;

private:
  bool _visible;
  std::int32_t _vtime;
  std::int32_t _timer;
  std::int32_t _attack_time;
  std::int32_t _attack;
  bool _rDir;
  std::int32_t _start_time;
  std::int32_t _danger_circle;
  std::int32_t _danger_offset1;
  std::int32_t _danger_offset2;
  std::int32_t _danger_offset3;
  std::int32_t _danger_offset4;
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
  std::int32_t _timer;
  Boss* _ghost;
};

#endif