#ifndef II_GAME_LOGIC_ENEMY_H
#define II_GAME_LOGIC_ENEMY_H
#include "game/logic/ship/ecs_index.h"
#include "game/logic/ship/ship.h"

namespace ii {
void legacy_enemy_on_destroy(SimInterface&, ecs::handle h, damage_type type);

void spawn_follow(SimInterface&, const vec2& position, bool has_score = true, fixed rotation = 0);
void spawn_big_follow(SimInterface&, const vec2& position, bool has_score);
void spawn_chaser(SimInterface&, const vec2& position);
void spawn_square(SimInterface&, const vec2& position, fixed rotation = fixed_c::pi / 2);
void spawn_wall(SimInterface&, const vec2& position, bool rdir);
void spawn_follow_hub(SimInterface&, const vec2& position, bool powera = false,
                      bool powerb = false);
void spawn_shielder(SimInterface&, const vec2& position, bool power = false);
void spawn_tractor(SimInterface&, const vec2& position, bool power = false);
void spawn_boss_shot(SimInterface&, const vec2& position, const vec2& velocity,
                     const glm::vec4& c = {0.f, 0.f, .6f, 1.f});
}  // namespace ii

class Enemy : public ii::Ship {
public:
  Enemy(ii::SimInterface& sim, const vec2& position, ii::ship_flag type);
  void render() const override;
  virtual void on_destroy(bool bomb) {}
};

class BossShot : public Enemy {
public:
  BossShot(ii::SimInterface& sim, const vec2& position, const vec2& velocity,
           const glm::vec4& c = {0.f, 0.f, .6f, 1.f});
  void update() override;

protected:
  vec2 dir_{0};
};

#endif
