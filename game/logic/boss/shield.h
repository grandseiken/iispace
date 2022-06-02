#ifndef II_GAME_LOGIC_BOSS_SHIELD_H
#define II_GAME_LOGIC_BOSS_SHIELD_H
#include "game/logic/boss.h"

class ShieldBombBoss : public Boss {
public:
  ShieldBombBoss(ii::SimInterface& sim, std::int32_t players, std::int32_t cycle);

  void update() override;
  std::int32_t get_damage(std::int32_t damage, bool magic) override;

private:
  std::int32_t timer_ = 0;
  std::int32_t count_ = 0;
  std::int32_t unshielded_ = 0;
  std::int32_t attack_ = 0;
  bool side_ = false;
  vec2 attack_dir_;
  bool shot_alternate_ = false;
};

#endif