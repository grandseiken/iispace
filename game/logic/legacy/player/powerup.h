#ifndef II_GAME_LOGIC_LEGACY_PLAYER_POWERUP_H
#define II_GAME_LOGIC_LEGACY_PLAYER_POWERUP_H
#include "game/common/math.h"
#include "game/logic/ship/components.h"

namespace ii {
class SimInterface;
namespace legacy {
void spawn_powerup(SimInterface&, const vec2& position, powerup_type type);
}  // namespace legacy
}  // namespace ii

#endif
