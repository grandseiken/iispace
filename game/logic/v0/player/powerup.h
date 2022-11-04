#ifndef II_GAME_LOGIC_V0_PLAYER_POWERUP_H
#define II_GAME_LOGIC_V0_PLAYER_POWERUP_H
#include "game/common/math.h"
#include "game/logic/ecs/index.h"

namespace ii {
class SimInterface;
namespace v0 {
ecs::handle spawn_player_bubble(SimInterface& sim, ecs::handle player);
void spawn_shield_powerup(SimInterface&, const vec2& position);
void spawn_bomb_powerup(SimInterface&, const vec2& position);
}  // namespace v0
}  // namespace ii

#endif
