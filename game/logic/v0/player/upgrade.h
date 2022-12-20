#ifndef II_GAME_LOGIC_V0_PLAYER_UPGRADE_H
#define II_GAME_LOGIC_V0_PLAYER_UPGRADE_H
#include <cstdint>
#include <span>

namespace ii {
class SimInterface;
namespace v0 {
enum class mod_id : std::uint32_t;
void spawn_mod_upgrades(SimInterface&, std::span<mod_id>);
void cleanup_mod_upgrades(SimInterface&);
bool is_mod_upgrade_choice_done(const SimInterface&);
}  // namespace v0
}  // namespace ii

#endif
