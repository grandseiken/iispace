#ifndef II_GAME_LOGIC_LEGACY_PLAYER_POWERUP_H
#define II_GAME_LOGIC_LEGACY_PLAYER_POWERUP_H
#include "game/common/math.h"
#include "game/logic/sim/components.h"

namespace ii {
class SimInterface;
namespace legacy {

enum class powerup_type {
  kExtraLife,
  kMagicShots,
  kShield,
  kBomb,
};

void spawn_powerup(SimInterface&, const vec2& position, powerup_type type);
}  // namespace legacy
}  // namespace ii

#endif
