#ifndef IISPACE_GAME_LOGIC_BOSS_SHIELD_H
#define IISPACE_GAME_LOGIC_BOSS_SHIELD_H
#include "game/logic/boss.h"

class ShieldBombBoss : public Boss {
public:
  ShieldBombBoss(std::int32_t players, std::int32_t cycle);

  void update() override;
  std::int32_t get_damage(std::int32_t damage, bool magic) override;

private:
  std::int32_t timer_;
  std::int32_t count_;
  std::int32_t unshielded_;
  std::int32_t attack_;
  bool side_;
  vec2 attack_dir_;
  bool shot_alternate_;
};

#endif