#ifndef II_GAME_LOGIC_PLAYER_POWERUP_H
#define II_GAME_LOGIC_PLAYER_POWERUP_H
#include "game/common/math.h"
#include "game/logic/ship/components.h"

namespace ii {
class SimInterface;
void spawn_powerup(SimInterface&, const vec2& position, powerup_type type);
}  // namespace ii

#endif
