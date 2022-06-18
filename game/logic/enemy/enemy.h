#ifndef II_GAME_LOGIC_ENEMY_ENEMY_H
#define II_GAME_LOGIC_ENEMY_ENEMY_H
#include "game/common/math.h"

namespace ii {
class SimInterface;

void spawn_bounce(SimInterface& sim, const vec2& position, fixed angle);
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
                     const glm::vec4& c = {0.f, 0.f, .6f, 1.f}, fixed rotate_speed = 0);

void spawn_snake(SimInterface& sim, const vec2& position, const glm::vec4& colour,
                 const vec2& direction = vec2{0}, fixed rotation = 0);
}  // namespace ii

#endif
