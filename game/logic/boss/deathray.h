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
  std::int32_t timer_ = 0;
  bool laser_ = false;
  bool dir_ = true;
  std::int32_t pos_ = 1;
  ii::SimInterface::ship_list arms_;
  std::int32_t arm_timer_ = 0;
  std::int32_t shot_timer_ = 0;

  std::int32_t ray_attack_timer_ = 0;
  vec2 ray_src1_;
  vec2 ray_src2_;
  vec2 ray_dest_;

  std::vector<std::pair<std::int32_t, std::int32_t>> shot_queue_;
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
  DeathRayBoss* boss_ = nullptr;
  bool top_ = false;
  std::int32_t timer_ = 0;
  bool attacking_ = false;
  vec2 dir_;
  std::int32_t start_ = 30;

  vec2 target_;
  std::int32_t shots_ = 0;
};

#endif