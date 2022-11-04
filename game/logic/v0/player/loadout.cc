#include "game/logic/v0/player/loadout.h"

namespace ii::v0 {

std::uint32_t PlayerLoadout::max_shield_capacity(const SimInterface&) const {
  return 3u;  // TODO
}

std::uint32_t PlayerLoadout::max_bomb_capacity(const SimInterface&) const {
  return 3u;  // TODO
}

}  // namespace ii::v0