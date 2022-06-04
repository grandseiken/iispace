#ifndef II_GAME_LOGIC_BOSS_TRACTOR_H
#define II_GAME_LOGIC_BOSS_TRACTOR_H
#include "game/logic/boss.h"
#include "game/logic/enemy.h"

class TractorBoss : public Boss {
public:
  TractorBoss(ii::SimInterface& sim, std::uint32_t players, std::uint32_t cycle);

  void update() override;
  void render() const override;
  std::uint32_t get_damage(std::uint32_t damage, bool magic) override;

private:
  ii::CompoundShape* s1_ = nullptr;
  ii::CompoundShape* s2_ = nullptr;
  ii::Polygon* sattack_ = nullptr;
  bool will_attack_ = false;
  bool stopped_ = false;
  bool generating_ = false;
  bool attacking_ = false;
  bool continue_ = false;
  bool gen_dir_ = false;
  std::uint32_t shoot_type_ = 0;
  bool sound_ = true;
  std::uint32_t timer_ = 0;
  std::uint32_t attack_size_ = 0;
  std::size_t attack_shapes_ = 0;

  std::vector<vec2> targets_;
};

class TBossShot : public Enemy {
public:
  TBossShot(ii::SimInterface& sim, const vec2& position, fixed angle);
  void update() override;

private:
  vec2 dir_{0};
};

#endif