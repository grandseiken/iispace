#ifndef II_GAME_LOGIC_LEGACY_PLAYER_POWERUP_H
#define II_GAME_LOGIC_LEGACY_PLAYER_POWERUP_H
#include "game/common/math.h"

namespace ii {
class SimInterface;
namespace v0 {
void spawn_powerup(SimInterface&, const vec2& position);
}  // namespace v0
}  // namespace ii

#endif
