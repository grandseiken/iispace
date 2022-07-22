#ifndef II_GAME_LOGIC_PLAYER_PLAYER_H
#define II_GAME_LOGIC_PLAYER_PLAYER_H
#include "game/common/math.h"
#include "game/logic/ship/components.h"

namespace ii {
class SimInterface;

void spawn_powerup(SimInterface&, const vec2& position, powerup_type type);
void spawn_player(SimInterface&, const vec2& position, std::uint32_t player_number, bool is_ai);
}  // namespace ii

#endif
