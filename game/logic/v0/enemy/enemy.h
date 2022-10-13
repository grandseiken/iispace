#ifndef II_GAME_LOGIC_V0_ENEMY_ENEMY_H
#define II_GAME_LOGIC_V0_ENEMY_ENEMY_H
#include "game/common/math.h"
#include <optional>

namespace ii {
class SimInterface;

namespace v0 {
void spawn_follow(SimInterface&, const vec2& position,
                  std::optional<vec2> direction = std::nullopt);
void spawn_big_follow(SimInterface&, const vec2& position,
                      std::optional<vec2> direction = std::nullopt);
void spawn_huge_follow(SimInterface&, const vec2& position,
                       std::optional<vec2> direction = std::nullopt);

void spawn_chaser(SimInterface&, const vec2& position);
void spawn_big_chaser(SimInterface&, const vec2& position);
}  // namespace v0
}  // namespace ii

#endif
