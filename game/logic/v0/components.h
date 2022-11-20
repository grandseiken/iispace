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
  std::string debug_text;

  void pre_update(SimInterface&);
};
DEBUG_STRUCT_TUPLE(GlobalData, non_wall_enemy_count, overmind_wave_count, debug_text);

struct DropTable : ecs::component {
  std::uint32_t shield_drop_chance = 0;
  std::uint32_t bomb_drop_chance = 0;
};
DEBUG_STRUCT_TUPLE(DropTable, shield_drop_chance, bomb_drop_chance);

struct Physics : ecs::component {
  fixed mass = 1_fx;
  fixed drag_coefficient = mass;
  vec2 velocity{0};

  void apply_impulse(vec2 impulse) { velocity += impulse / mass; }
  void update(Transform& transform) {
    static constexpr auto kBaseDragCoefficient = 1_fx / 32_fx;
    transform.move(velocity);
    velocity *= std::max(0_fx, 1_fx - kBaseDragCoefficient * drag_coefficient / mass);
    if (length_squared(velocity) < 1_fx / (128_fx * 128_fx)) {
      velocity = vec2{0};
    }
  }
};
DEBUG_STRUCT_TUPLE(Physics, mass, drag_coefficient, velocity);

struct EnemyStatus : ecs::component {
  std::uint32_t shielded_ticks = 0;
};

}  // namespace ii::v0

#endif
