#ifndef II_GAME_LOGIC_V0_LIB_COMPONENTS_H
#define II_GAME_LOGIC_V0_LIB_COMPONENTS_H
#include "game/common/math.h"
#include "game/common/struct_tuple.h"
#include "game/logic/ecs/index.h"
#include "game/logic/sim/components.h"
#include <optional>

namespace ii::v0 {
struct drop_data {
  std::int32_t counter = 0;
  std::int32_t compensation = 0;
};
DEBUG_STRUCT_TUPLE(drop_data, counter, compensation);

struct AiFocusTag : ecs::component {
  std::uint32_t priority = 1;
};
DEBUG_STRUCT_TUPLE(AiFocusTag, priority);

struct AiClickTag : ecs::component {
  std::optional<vec2> position;
};
DEBUG_STRUCT_TUPLE(AiClickTag, position);

struct GlobalData : ecs::component {
  bool is_run_complete = false;
  drop_data shield_drop;
  drop_data bomb_drop;

  bool walls_vulnerable = false;
  std::optional<std::uint32_t> overmind_wave_count;
  std::string debug_text;
};
DEBUG_STRUCT_TUPLE(GlobalData, is_run_complete, shield_drop, bomb_drop, walls_vulnerable,
                   overmind_wave_count, debug_text);

struct DropTable : ecs::component {
  std::uint32_t shield_drop_chance = 0;
  std::uint32_t bomb_drop_chance = 0;
};
DEBUG_STRUCT_TUPLE(DropTable, shield_drop_chance, bomb_drop_chance);

struct ColourOverride : ecs::component {
  cvec4 colour = colour::kWhite0;
};
DEBUG_STRUCT_TUPLE(ColourOverride);

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
  static constexpr std::uint32_t kStunTicks = 40u;
  static constexpr std::uint32_t kStunHit = 8u;
  static constexpr std::uint32_t kStunResistDecayTime = 12u;

  std::uint32_t stun_resist_base = 0u;
  std::uint32_t stun_resist_bonus = 20u;
  std::uint32_t stun_resist_extra = 0u;
  std::uint32_t stun_resist_decay = 0u;
  std::uint32_t stun_counter = 0u;
  std::uint32_t stun_ticks = 0u;

  struct destroy_timer_t {
    std::optional<vec2> source;
    std::uint32_t timer = 0;
  };
  std::optional<destroy_timer_t> destroy_timer;
  std::uint32_t shielded_ticks = 0u;

  void inflict_stun() {
    stun_counter += kStunHit;
    if (stun_counter >= stun_resist_base + stun_resist_extra) {
      stun_ticks = std::max(stun_ticks, kStunTicks);
      stun_counter = 0;
      stun_resist_extra += stun_resist_bonus;
    }
  }

  void update(ecs::handle h, Update* update, SimInterface& sim) {
    stun_counter && --stun_counter;
    stun_ticks && --stun_ticks;
    shielded_ticks && --shielded_ticks;
    if (stun_resist_extra && ++stun_resist_decay == kStunResistDecayTime) {
      stun_resist_decay = 0u;
      --stun_resist_extra;
    }
    if (update) {
      update->skip_update = stun_ticks > 0;
    }
    destroy_timer && destroy_timer->timer && --destroy_timer->timer;
    if (destroy_timer && !destroy_timer->timer) {
      if (auto* health = h.get<Health>(); health) {
        health->damage(h, sim, 12u * health->max_hp, damage_type::kBomb, h.id(),
                       destroy_timer->source);
      } else {
        h.emplace<Destroy>();
      }
      destroy_timer.reset();
    }
  }
};
DEBUG_STRUCT_TUPLE(EnemyStatus::destroy_timer_t, source, timer);
DEBUG_STRUCT_TUPLE(EnemyStatus, stun_resist_base, stun_resist_bonus, stun_resist_extra,
                   stun_resist_decay, stun_counter, stun_ticks, destroy_timer, shielded_ticks);

}  // namespace ii::v0

#endif
