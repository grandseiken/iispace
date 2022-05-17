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
  bool visible_ = false;
  std::int32_t vtime_ = 0;
  std::int32_t timer_ = 0;
  std::int32_t attack_time_ = 0;
  std::int32_t attack_ = 0;
  bool _rdir = false;
  std::int32_t start_time_ = 120;
  std::int32_t danger_circle_ = 0;
  std::int32_t danger_offset1_ = 0;
  std::int32_t danger_offset2_ = 0;
  std::int32_t danger_offset3_ = 0;
  std::int32_t danger_offset4_ = 0;
  bool shot_type_ = false;
};

class GhostWall : public Enemy {
public:
  GhostWall(bool swap, bool no_gap, bool ignored);
  GhostWall(bool swap, bool swap_gap);
  void update() override;

private:
  vec2 dir_;
};

class GhostMine : public Enemy {
public:
  GhostMine(const vec2& position, Boss* ghost);
  void update() override;
  void render() const override;

private:
  std::int32_t timer_ = 80;
  Boss* ghost_ = nullptr;
};

#endif