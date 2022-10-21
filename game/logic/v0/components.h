#ifndef II_GAME_LOGIC_V0_COMPONENTS_H
#define II_GAME_LOGIC_V0_COMPONENTS_H
#include "game/common/math.h"
#include "game/common/struct_tuple.h"
#include "game/logic/ecs/index.h"
#include "game/logic/sim/components.h"

namespace ii::v0 {

struct AiFocusTag : ecs::component {
  std::uint32_t priority = 1;
};
DEBUG_STRUCT_TUPLE(AiFocusTag, priority);

struct GlobalData : ecs::component {
  std::uint32_t non_wall_enemy_count = 0;
  std::optional<std::uint32_t> overmind_wave_count;
  void pre_update(SimInterface&);
};
DEBUG_STRUCT_TUPLE(GlobalData, non_wall_enemy_count, overmind_wave_count);

}  // namespace ii::v0

#endif
