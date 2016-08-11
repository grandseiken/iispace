#ifndef IISPACE_SRC_BOSS_DEATHRAY_H
#define IISPACE_SRC_BOSS_DEATHRAY_H

#include "boss.h"
#include "enemy.h"

class DeathRayBoss : public Boss {
public:
  DeathRayBoss(int32_t players, int32_t cycle);

  void update() override;
  void render() const override;
  int32_t get_damage(int32_t damage, bool magic) override;

  void on_arm_death(Ship* arm);

private:
  int32_t _timer;
  bool _laser;
  bool _dir;
  int32_t _pos;
  GameModal::ship_list _arms;
  int32_t _arm_timer;
  int32_t _shot_timer;

  int32_t _ray_attack_timer;
  vec2 _ray_src1;
  vec2 _ray_src2;
  vec2 _ray_dest;

  std::vector<std::pair<int32_t, int32_t>> _shot_queue;
};

class DeathRay : public Enemy {
public:
  DeathRay(const vec2& position);
  void update() override;
};

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

#endif