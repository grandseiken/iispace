#ifndef II_GAME_LOGIC_V0_PLAYER_PLAYER_H
#define II_GAME_LOGIC_V0_PLAYER_PLAYER_H
#include "game/common/math.h"
#include "game/logic/ecs/index.h"
#include <optional>

namespace ii {
class SimInterface;
namespace v0 {
void spawn_player(SimInterface&, const vec2& position, std::uint32_t player_number);
}  // namespace v0
}  // namespace ii

#endif
