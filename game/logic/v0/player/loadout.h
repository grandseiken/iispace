#ifndef II_GAME_LOGIC_V0_PLAYER_LOADOUT_H
#define II_GAME_LOGIC_V0_PLAYER_LOADOUT_H
#include "game/logic/ecs/index.h"

namespace ii {
class SimInterface;
namespace v0 {

struct PlayerLoadout : ecs::component {
  std::uint32_t max_shield_capacity(const SimInterface&) const;
  std::uint32_t max_bomb_capacity(const SimInterface&) const;
};

}  // namespace v0
}  // namespace ii

#endif
