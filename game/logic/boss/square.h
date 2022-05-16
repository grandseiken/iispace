#ifndef IISPACE_GAME_LOGIC_BOSS_SQUARE_H
#define IISPACE_GAME_LOGIC_BOSS_SQUARE_H
#include "game/logic/boss.h"

class BigSquareBoss : public Boss {
public:
  BigSquareBoss(int32_t players, int32_t cycle);

  void update() override;
  void render() const override;
  int32_t get_damage(int32_t damage, bool magic) override;

private:
  vec2 _dir;
  bool _reverse;
  int32_t _timer;
  int32_t _spawn_timer;
  int32_t _special_timer;
  bool _special_attack;
  bool _special_attack_rotate;
  Player* _attack_player;
};

#endif