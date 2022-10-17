#ifndef II_GAME_LOGIC_V0_PLAYER_BUBBLE_H
#define II_GAME_LOGIC_V0_PLAYER_BUBBLE_H
#include "game/logic/ecs/index.h"

namespace ii {
class SimInterface;
namespace v0 {
ecs::handle spawn_player_bubble(SimInterface& sim, ecs::handle player);
}  // namespace v0
}  // namespace ii

#endif
