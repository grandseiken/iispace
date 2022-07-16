#ifndef II_GAME_LOGIC_PLAYER_PLAYER_H
#define II_GAME_LOGIC_PLAYER_PLAYER_H
#include "game/common/math.h"

namespace ii {
class SimInterface;

enum class powerup_type {
  kExtraLife,
  kMagicShots,
  kShield,
  kBomb,
};

void spawn_powerup(SimInterface&, const vec2& position, powerup_type type);
void spawn_player(SimInterface&, const vec2& position, std::uint32_t player_number);
}  // namespace ii

#endif
