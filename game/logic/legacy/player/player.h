#ifndef II_GAME_LOGIC_LEGACY_PLAYER_PLAYER_H
#define II_GAME_LOGIC_LEGACY_PLAYER_PLAYER_H
#include "game/common/math.h"
#include "game/logic/ecs/index.h"
#include "game/logic/sim/io/player.h"
#include <optional>

namespace ii {
class SimInterface;
namespace legacy {
static constexpr fixed kPlayerSpeed = 5;
void spawn_player(SimInterface&, const vec2& position, std::uint32_t player_number, bool is_ai);
std::optional<input_frame> ai_think(const SimInterface& sim, ecs::handle h);
}  // namespace legacy
}  // namespace ii

#endif
