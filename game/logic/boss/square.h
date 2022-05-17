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
  vec2 dir_;
  bool reverse_;
  std::int32_t timer_;
  std::int32_t spawn_timer_;
  std::int32_t special_timer_;
  bool special_attack_;
  bool special_attack_rotate_;
  Player* attack_player_;
};

#endif