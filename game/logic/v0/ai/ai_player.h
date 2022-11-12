#ifndef II_GAME_LOGIC_V0_AI_AI_PLAYER_H
#define II_GAME_LOGIC_V0_AI_AI_PLAYER_H
#include "game/logic/ecs/index.h"
#include "game/logic/sim/io/player.h"

namespace ii {
class SimInterface;
namespace v0 {

void add_ai(ecs::handle h);
std::optional<input_frame> ai_think(const SimInterface& sim, ecs::handle h, ai_state& state);
}  // namespace v0
}  // namespace ii

#endif
