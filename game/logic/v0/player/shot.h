#ifndef II_GAME_LOGIC_V0_PLAYER_SHOT_H
#define II_GAME_LOGIC_V0_PLAYER_SHOT_H
#include "game/common/math.h"
#include "game/logic/ecs/index.h"

namespace ii {
class SimInterface;
struct PlayerLoadout;
namespace v0 {
void spawn_player_shot(SimInterface& sim, const vec2& position, ecs::handle player,
                       const PlayerLoadout&, const vec2& direction);
}  // namespace v0
}  // namespace ii

#endif
