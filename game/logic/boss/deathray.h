#ifndef IISPACE_GAME_LOGIC_BOSS_DEATHRAY_H
#define IISPACE_GAME_LOGIC_BOSS_DEATHRAY_H
#include "game/logic/boss.h"
#include "game/logic/enemy.h"

class DeathRayBoss : public Boss {
public:
  DeathRayBoss(std::int32_t players, std::int32_t cycle);

  void update() override;
  void render() const override;
  std::int32_t get_damage(std::int32_t damage, bool magic) override;

  void on_arm_death(Ship* arm);

private:
  std::int32_t _timer;
  bool _laser;
  bool _dir;
  std::int32_t _pos;
  SimState::ship_list _arms;
  std::int32_t _arm_timer;
  std::int32_t _shot_timer;

  std::int32_t _ray_attack_timer;
  vec2 _ray_src1;
  vec2 _ray_src2;
  vec2 _ray_dest;

  std::vector<std::pair<std::int32_t, std::int32_t>> _shot_queue;
};

class DeathRay : public Enemy {
public:
  DeathRay(const vec2& position);
  void update() override;
};

class DeathArm : public Enemy {
public:
  DeathArm(DeathRayBoss* boss, bool top, std::int32_t hp);
  void update() override;
  void on_destroy(bool bomb) override;

private:
  DeathRayBoss* _boss;
  bool _top;
  std::int32_t _timer;
  bool _attacking;
  vec2 _dir;
  std::int32_t _start;

  vec2 _target;
  std::int32_t _shots;
};

#endif