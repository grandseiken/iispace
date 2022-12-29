#ifndef II_GAME_LOGIC_V0_BOSS_BOSS_H
#define II_GAME_LOGIC_V0_BOSS_BOSS_H
#include <cstdint>

namespace ii {
class SimInterface;

namespace v0 {
void spawn_biome0_square_boss(SimInterface&, std::uint32_t biome_index);
}  // namespace v0
}  // namespace ii

#endif