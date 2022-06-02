#ifndef II_GAME_LOGIC_BOSS_TRACTOR_H
#define II_GAME_LOGIC_BOSS_TRACTOR_H
#include "game/logic/boss.h"
#include "game/logic/enemy.h"

class TractorBoss : public Boss {
public:
  TractorBoss(ii::SimInterface& sim, std::int32_t players, std::int32_t cycle);

  void update() override;
  void render() const override;
  std::int32_t get_damage(std::int32_t damage, bool magic) override;

private:
  CompoundShape* s1_ = nullptr;
  CompoundShape* s2_ = nullptr;
  Polygon* sattack_ = nullptr;
  bool will_attack_ = false;
  bool stopped_ = false;
  bool generating_ = false;
  bool attacking_ = false;
  bool continue_ = false;
  bool gen_dir_ = false;
  std::int32_t shoot_type_ = 0;
  bool sound_ = true;
  std::int32_t timer_ = 0;
  std::int32_t attack_size_ = 0;
  std::size_t attack_shapes_ = 0;

  std::vector<vec2> targets_;
};

class TBossShot : public Enemy {
public:
  TBossShot(ii::SimInterface& sim, const vec2& position, fixed angle);
  void update() override;

private:
  vec2 dir_;
};

#endif