#ifndef II_GAME_LOGIC_LEGACY_PLAYER_PLAYER_H
#define II_GAME_LOGIC_LEGACY_PLAYER_PLAYER_H
#include "game/common/math.h"
#include "game/logic/ecs/index.h"
#include <optional>

namespace ii {
class SimInterface;
namespace legacy {
void spawn_player(SimInterface&, const vec2& position, std::uint32_t player_number);
}  // namespace legacy
}  // namespace ii

#endif
