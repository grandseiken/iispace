#ifndef II_GAME_LOGIC_LEGACY_BOSS_BOSS_H
#define II_GAME_LOGIC_LEGACY_BOSS_BOSS_H
#include <cstdint>

namespace ii {
class SimInterface;

namespace legacy {
void spawn_shield_bomb_boss(SimInterface&, std::uint32_t cycle);
void spawn_big_square_boss(SimInterface&, std::uint32_t cycle);
void spawn_chaser_boss(SimInterface&, std::uint32_t cycle);

void spawn_death_ray_boss(SimInterface&, std::uint32_t cycle);
void spawn_ghost_boss(SimInterface&, std::uint32_t cycle);
void spawn_tractor_boss(SimInterface&, std::uint32_t cycle);

void spawn_super_boss(SimInterface&, std::uint32_t cycle);
}  // namespace legacy
}  // namespace ii

#endif