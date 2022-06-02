#ifndef II_GAME_LOGIC_BOSS_SQUARE_H
#define II_GAME_LOGIC_BOSS_SQUARE_H
#include "game/logic/boss.h"

class BigSquareBoss : public Boss {
public:
  BigSquareBoss(ii::SimInterface& sim, std::int32_t players, std::int32_t cycle);

  void update() override;
  void render() const override;
  std::int32_t get_damage(std::int32_t damage, bool magic) override;

private:
  vec2 dir_;
  bool reverse_ = false;
  std::int32_t timer_ = 0;
  std::int32_t spawn_timer_ = 0;
  std::int32_t special_timer_ = 0;
  bool special_attack_ = false;
  bool special_attack_rotate_ = false;
  Player* attack_player_ = nullptr;
};

#endif