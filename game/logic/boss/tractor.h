#ifndef IISPACE_GAME_LOGIC_BOSS_TRACTOR_H
#define IISPACE_GAME_LOGIC_BOSS_TRACTOR_H
#include "game/logic/boss.h"
#include "game/logic/enemy.h"

class TractorBoss : public Boss {
public:
  TractorBoss(std::int32_t players, std::int32_t cycle);

  void update() override;
  void render() const override;
  std::int32_t get_damage(std::int32_t damage, bool magic) override;

private:
  CompoundShape* s1_;
  CompoundShape* s2_;
  Polygon* sattack_;
  bool will_attack_;
  bool stopped_;
  bool generating_;
  bool attacking_;
  bool continue_;
  bool gen_dir_;
  std::int32_t shoot_type_;
  bool sound_;
  std::int32_t timer_;
  std::int32_t attack_size_;
  std::size_t attack_shapes_;

  std::vector<vec2> targets_;
};

class TBossShot : public Enemy {
public:
  TBossShot(const vec2& position, fixed angle);
  void update() override;

private:
  vec2 dir_;
};

#endif