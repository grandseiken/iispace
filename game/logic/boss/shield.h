#ifndef IISPACE_GAME_LOGIC_BOSS_SHIELD_H
#define IISPACE_GAME_LOGIC_BOSS_SHIELD_H
#include "game/logic/boss.h"

class ShieldBombBoss : public Boss {
public:
  ShieldBombBoss(std::int32_t players, std::int32_t cycle);

  void update() override;
  std::int32_t get_damage(std::int32_t damage, bool magic) override;

private:
  std::int32_t _timer;
  std::int32_t _count;
  std::int32_t _unshielded;
  std::int32_t _attack;
  bool _side;
  vec2 _attack_dir;
  bool _shot_alternate;
};

#endif