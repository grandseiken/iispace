#ifndef II_GAME_LOGIC_V0_PLAYER_UPGRADE_H
#define II_GAME_LOGIC_V0_PLAYER_UPGRADE_H
#include <cstdint>

namespace ii {
class SimInterface;
namespace v0 {
enum class mod_id : std::uint32_t;
enum class upgrade_position {
  kL0 = 0,
  kR0 = 1,
  kL1 = 2,
  kR1 = 3,
};

void spawn_mod_upgrade(SimInterface&, mod_id, upgrade_position);
}  // namespace v0
}  // namespace ii

#endif
