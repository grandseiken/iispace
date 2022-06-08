#ifndef II_GAME_LOGIC_BOSS_H
#define II_GAME_LOGIC_BOSS_H
#include "game/logic/sim/sim_interface.h"

namespace ii {
void spawn_shield_bomb_boss(SimInterface&, std::uint32_t players, std::uint32_t cycle);
void spawn_big_square_boss(SimInterface&, std::uint32_t players, std::uint32_t cycle);
void spawn_chaser_boss(SimInterface&, std::uint32_t players, std::uint32_t cycle);
void chaser_boss_begin_frame();

void spawn_death_ray_boss(SimInterface&, std::uint32_t players, std::uint32_t cycle);
void spawn_ghost_boss(SimInterface&, std::uint32_t players, std::uint32_t cycle);
void spawn_tractor_boss(SimInterface&, std::uint32_t players, std::uint32_t cycle);

void spawn_super_boss(SimInterface&, std::uint32_t players, std::uint32_t cycle);

std::vector<std::pair<std::uint32_t, std::pair<vec2, glm::vec4>>>& boss_fireworks();
std::vector<vec2>& boss_warnings();
}  // namespace ii

#endif