#ifndef II_GAME_LOGIC_V0_ENEMY_ENEMY_H
#define II_GAME_LOGIC_V0_ENEMY_ENEMY_H
#include "game/common/math.h"
#include "game/logic/ecs/index.h"
#include <optional>

namespace ii {
class SimInterface;

namespace v0 {
ecs::handle
spawn_square(SimInterface& sim, const vec2& position, const vec2& dir, bool drop = true);
ecs::handle spawn_wall(SimInterface& sim, const vec2& position, const vec2& dir, bool anti);

ecs::handle spawn_follow(SimInterface&, const vec2& position,
                         std::optional<vec2> direction = std::nullopt, bool drop = true);
ecs::handle spawn_big_follow(SimInterface&, const vec2& position,
                             std::optional<vec2> direction = std::nullopt, bool drop = true);
ecs::handle spawn_huge_follow(SimInterface&, const vec2& position,
                              std::optional<vec2> direction = std::nullopt, bool drop = true);

ecs::handle spawn_chaser(SimInterface&, const vec2& position, bool drop = true);
ecs::handle spawn_big_chaser(SimInterface&, const vec2& position, bool drop = true);
ecs::handle spawn_follow_sponge(SimInterface&, const vec2& position);

ecs::handle spawn_follow_hub(SimInterface& sim, const vec2& position, bool fast = false);
ecs::handle spawn_big_follow_hub(SimInterface& sim, const vec2& position, bool fast = false);
ecs::handle spawn_chaser_hub(SimInterface& sim, const vec2& position, bool fast = false);
ecs::handle spawn_shielder(SimInterface& sim, const vec2& position, bool power = false);
ecs::handle spawn_tractor(SimInterface& sim, const vec2& position, bool power = false);
ecs::handle spawn_shield_hub(SimInterface& sim, const vec2& position);
}  // namespace v0
}  // namespace ii

#endif
