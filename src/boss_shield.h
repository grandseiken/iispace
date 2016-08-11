#ifndef IISPACE_SRC_BOSS_SHIELD_H
#define IISPACE_SRC_BOSS_SHIELD_H

#include "boss.h"

class ShieldBombBoss : public Boss {
public:
  ShieldBombBoss(int32_t players, int32_t cycle);

  void update() override;
  int32_t get_damage(int32_t damage, bool magic) override;

private:
  int32_t _timer;
  int32_t _count;
  int32_t _unshielded;
  int32_t _attack;
  bool _side;
  vec2 _attack_dir;
  bool _shot_alternate;
};

#endif