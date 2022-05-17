#ifndef IISPACE_GAME_LOGIC_BOSS_SQUARE_H
#define IISPACE_GAME_LOGIC_BOSS_SQUARE_H
#include "game/logic/boss.h"

class BigSquareBoss : public Boss {
public:
  BigSquareBoss(std::int32_t players, std::int32_t cycle);

  void update() override;
  void render() const override;
  std::int32_t get_damage(std::int32_t damage, bool magic) override;

private:
  vec2 _dir;
  bool _reverse;
  std::int32_t _timer;
  std::int32_t _spawn_timer;
  std::int32_t _special_timer;
  bool _special_attack;
  bool _special_attack_rotate;
  Player* _attack_player;
};

#endif