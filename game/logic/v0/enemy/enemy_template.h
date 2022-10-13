#ifndef GAME_LOGIC_V0_ENEMY_ENEMY_TEMPLATE_H
#define GAME_LOGIC_V0_ENEMY_ENEMY_TEMPLATE_H
#include "game/logic/ecs/index.h"
#include "game/logic/ship/components.h"
#include "game/logic/sim/sim_interface.h"
#include "game/logic/v0/particles.h"
#include "game/logic/v0/ship_template.h"
#include <sfn/functional.h>

namespace ii::v0 {

inline vec2 direction_to_screen(const SimInterface& sim, const vec2& position) {
  auto dim = sim.dimensions();
  if (position.x <= 0 && position.y <= 0) {
    return vec2{1, 1};
  }
  if (position.x <= 0 && position.y >= dim.y) {
    return vec2{1, -1};
  }
  if (position.x >= dim.x && position.y <= 0) {
    return vec2{-1, 1};
  }
  if (position.x >= dim.x && position.y >= dim.y) {
    return vec2{-1, -1};
  }
  if (position.x <= 0) {
    return vec2{1, 0};
  }
  if (position.y <= 0) {
    return vec2{0, 1};
  }
  if (position.x >= dim.x) {
    return vec2{-1, 0};
  }
  if (position.y >= dim.y) {
    return vec2{0, -1};
  }
  return vec2{0};
}

inline bool
check_direction_to_screen(const SimInterface& sim, const vec2& position, const vec2& direction) {
  auto dim = sim.dimensions();
  return (position.x >= 0 || direction.x > 0) && (position.y >= 0 || direction.y > 0) &&
      (position.x <= dim.x || direction.x < 0) && (position.y <= dim.y || direction.y < 0);
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void add_enemy_health(ecs::handle h, std::uint32_t hp,
                      std::optional<sound> destroy_sound = std::nullopt,
                      std::optional<rumble_type> destroy_rumble = std::nullopt) {
  destroy_sound = destroy_sound ? *destroy_sound : Logic::kDestroySound;
  destroy_rumble = destroy_rumble ? *destroy_rumble : Logic::kDestroyRumble;
  using on_destroy_t =
      void(ecs::const_handle, SimInterface&, EmitHandle&, damage_type, const vec2&);
  constexpr auto on_destroy = sfn::cast<on_destroy_t, &destruct_entity_default<Logic, S>>;
  if constexpr (requires { &Logic::on_destroy; }) {
    h.add(Health{
        .hp = hp,
        .destroy_sound = destroy_sound,
        .destroy_rumble = destroy_rumble,
        .on_destroy =
            sfn::sequence<on_destroy, sfn::cast<on_destroy_t, ecs::call<&Logic::on_destroy>>>});
  } else {
    h.add(Health{.hp = hp,
                 .destroy_sound = destroy_sound,
                 .destroy_rumble = destroy_rumble,
                 .on_destroy = on_destroy});
  }
}

}  // namespace ii::v0

#endif