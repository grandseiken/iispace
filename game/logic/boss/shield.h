#ifndef II_GAME_LOGIC_BOSS_SHIELD_H
#define II_GAME_LOGIC_BOSS_SHIELD_H
#include "game/logic/boss.h"

class ShieldBombBoss : public Boss {
public:
  ShieldBombBoss(ii::SimInterface& sim, std::uint32_t players, std::uint32_t cycle);

  void update() override;
  std::uint32_t get_damage(std::uint32_t damage, bool magic) override;

private:
  std::uint32_t timer_ = 0;
  std::uint32_t count_ = 0;
  std::uint32_t unshielded_ = 0;
  std::uint32_t attack_ = 0;
  bool side_ = false;
  vec2 attack_dir_{0};
  bool shot_alternate_ = false;
};

#endif