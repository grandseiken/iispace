#ifndef II_GAME_LOGIC_V0_ENEMY_ENEMY_H
#define II_GAME_LOGIC_V0_ENEMY_ENEMY_H
#include "game/common/math.h"
#include <optional>

namespace ii {
class SimInterface;

namespace v0 {
void spawn_square(SimInterface& sim, const vec2& position, const vec2& dir, bool drop = true);
void spawn_wall(SimInterface& sim, const vec2& position, const vec2& dir, bool anti);

void spawn_follow(SimInterface&, const vec2& position, std::optional<vec2> direction = std::nullopt,
                  bool drop = true);
void spawn_big_follow(SimInterface&, const vec2& position,
                      std::optional<vec2> direction = std::nullopt, bool drop = true);
void spawn_huge_follow(SimInterface&, const vec2& position,
                       std::optional<vec2> direction = std::nullopt, bool drop = true);

void spawn_chaser(SimInterface&, const vec2& position, bool drop = true);
void spawn_big_chaser(SimInterface&, const vec2& position, bool drop = true);

void spawn_follow_hub(SimInterface& sim, const vec2& position, bool fast = false);
void spawn_big_follow_hub(SimInterface& sim, const vec2& position, bool fast = false);
void spawn_chaser_hub(SimInterface& sim, const vec2& position, bool fast = false);
void spawn_shielder(SimInterface& sim, const vec2& position, bool power = false);
void spawn_tractor(SimInterface& sim, const vec2& position, bool power = false);
}  // namespace v0
}  // namespace ii

#endif
