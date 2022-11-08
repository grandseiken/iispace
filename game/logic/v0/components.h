#ifndef II_GAME_LOGIC_V0_COMPONENTS_H
#define II_GAME_LOGIC_V0_COMPONENTS_H
#include "game/common/math.h"
#include "game/common/struct_tuple.h"
#include "game/logic/ecs/index.h"
#include "game/logic/sim/components.h"

namespace ii::v0 {
struct drop_data {
  std::int32_t counter = 0;
  std::int32_t compensation = 0;
};

struct AiFocusTag : ecs::component {
  std::uint32_t priority = 1;
};
DEBUG_STRUCT_TUPLE(AiFocusTag, priority);

struct GlobalData : ecs::component {
  drop_data shield_drop;
  drop_data bomb_drop;

  std::uint32_t non_wall_enemy_count = 0;
  std::optional<std::uint32_t> overmind_wave_count;

  void pre_update(SimInterface&);
};
DEBUG_STRUCT_TUPLE(GlobalData, non_wall_enemy_count, overmind_wave_count);

struct DropTable : ecs::component {
  std::uint32_t shield_drop_chance = 0;
  std::uint32_t bomb_drop_chance = 0;
};
DEBUG_STRUCT_TUPLE(DropTable, shield_drop_chance, bomb_drop_chance);

}  // namespace ii::v0

#endif
