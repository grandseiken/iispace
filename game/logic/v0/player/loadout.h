#ifndef II_GAME_LOGIC_V0_PLAYER_LOADOUT_H
#define II_GAME_LOGIC_V0_PLAYER_LOADOUT_H
#include "game/common/struct_tuple.h"
#include "game/logic/ecs/index.h"
#include "game/logic/sim/io/conditions.h"
#include "game/logic/v0/player/loadout_mods.h"
#include <span>
#include <unordered_map>

namespace ii {
class RandomEngine;
class SimInterface;
struct initial_conditions;
namespace v0 {

using player_loadout = std::unordered_map<mod_id, std::uint32_t>;

struct PlayerLoadout : ecs::component {
  player_loadout loadout;
  void add(ecs::handle h, mod_id, const SimInterface&);
  bool has(mod_id) const;
  std::uint32_t count(mod_id) const;

  std::uint32_t max_shield_capacity(const SimInterface&) const;
  std::uint32_t max_bomb_capacity(const SimInterface&) const;
  fixed bomb_radius_multiplier() const;
};
DEBUG_STRUCT_TUPLE(PlayerLoadout);

std::vector<mod_id> mod_selection(const initial_conditions& conditions, RandomEngine& random,
                                  const player_loadout& combined_loadout);

}  // namespace v0
}  // namespace ii

#endif
