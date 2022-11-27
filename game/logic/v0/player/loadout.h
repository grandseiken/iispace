#ifndef II_GAME_LOGIC_V0_PLAYER_LOADOUT_H
#define II_GAME_LOGIC_V0_PLAYER_LOADOUT_H
#include "game/common/struct_tuple.h"
#include "game/logic/ecs/index.h"
#include "game/logic/sim/io/conditions.h"
#include "game/logic/v0/player/loadout_mods.h"
#include <span>
#include <vector>

namespace ii {
class RandomEngine;
class SimInterface;
struct initial_conditions;
namespace v0 {

using player_loadout = std::span<const mod_id>;
std::vector<mod_id> mod_selection(const initial_conditions& conditions, RandomEngine& random,
                                  std::span<const player_loadout> loadouts);

struct PlayerLoadout : ecs::component {
  std::vector<mod_id> loadout;
  void add_mod(mod_id);

  std::uint32_t max_shield_capacity(const SimInterface&) const;
  std::uint32_t max_bomb_capacity(const SimInterface&) const;
};
DEBUG_STRUCT_TUPLE(PlayerLoadout);

}  // namespace v0
}  // namespace ii

#endif
