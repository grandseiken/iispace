#ifndef II_GAME_LOGIC_PLAYER_AI_PLAYER_H
#define II_GAME_LOGIC_PLAYER_AI_PLAYER_H
#include "game/common/math.h"
#include "game/logic/ecs/index.h"
#include "game/logic/ship/components.h"
#include "game/logic/sim/sim_io.h"

namespace ii {
class SimInterface;

struct AiPlayer : ecs::component {
  vec2 velocity{0};
  input_frame think(ecs::const_handle h, const Transform& transform, const Player& player,
                    const SimInterface& sim);
};
DEBUG_STRUCT_TUPLE(AiPlayer, velocity);

}  // namespace ii

#endif
