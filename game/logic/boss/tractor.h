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
  CompoundShape* _s1;
  CompoundShape* _s2;
  Polygon* _sattack;
  bool _will_attack;
  bool _stopped;
  bool _generating;
  bool _attacking;
  bool _continue;
  bool _gen_dir;
  std::int32_t _shoot_type;
  bool _sound;
  std::int32_t _timer;
  std::int32_t _attack_size;
  std::size_t _attack_shapes;

  std::vector<vec2> _targets;
};

class TBossShot : public Enemy {
public:
  TBossShot(const vec2& position, fixed angle);
  void update() override;

private:
  vec2 _dir;
};

#endif