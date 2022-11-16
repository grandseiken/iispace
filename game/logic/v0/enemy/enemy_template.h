#ifndef GAME_LOGIC_V0_ENEMY_ENEMY_TEMPLATE_H
#define GAME_LOGIC_V0_ENEMY_ENEMY_TEMPLATE_H
#include "game/logic/ecs/index.h"
#include "game/logic/sim/sim_interface.h"
#include "game/logic/v0/components.h"
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

// TODO: put this all into a struct for easyness.
template <typename F>
inline auto spread_default_coefficient(fixed min_distance, F&& f) {
  return [min_distance, f](const auto& logic, fixed d_sq) {
    return f(logic) / (sqrt(d_sq) * std::max(min_distance * min_distance, d_sq));
  };
}
inline auto spread_default_coefficient(fixed min_distance) {
  return spread_default_coefficient(min_distance, [](const auto&) { return 1_fx; });
}
template <typename F>
inline auto spread_linear_coefficient(fixed min_distance, F&& f) {
  return [min_distance, f](const auto& logic, fixed d_sq) {
    return f(logic) / std::max(min_distance * min_distance, d_sq);
  };
}
inline auto spread_linear_coefficient(fixed min_distance) {
  return spread_linear_coefficient(min_distance, [](const auto&) { return 1_fx; });
}

template <typename Logic, typename F>
vec2 spread_closest(ecs::handle h, const Transform& transform, SimInterface& sim,
                    std::vector<ecs::entity_id>& id_storage, fixed max_distance, std::size_t max_n,
                    F&& coefficient_function, std::uint64_t tick_interval = 16) {
  if ((static_cast<std::uint64_t>(+h.id()) + sim.tick_count()) % tick_interval == 0u) {
    thread_local std::vector<SimInterface::range_info> range_output;
    range_output.clear();
    id_storage.clear();
    sim.in_range(transform.centre, max_distance, ecs::id<Logic>(), max_n, range_output);
    for (const auto& e : range_output) {
      id_storage.emplace_back(e.h.id());
    }
  }
  vec2 target_spread_velocity{0};
  for (auto id : id_storage) {
    if (auto eh = sim.index().get(id); eh && !eh->has<Destroy>()) {
      auto d = eh->get<Transform>()->centre - transform.centre;
      auto d_sq = length_squared(d);
      if (d != vec2{0}) {
        target_spread_velocity -= d * coefficient_function(*eh->get<Logic>(), d_sq);
      }
    }
  }
  return target_spread_velocity;
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

  sfn::ptr<Health::on_destroy_t> on_destroy = nullptr;
  sfn::ptr<Health::on_hit_t> on_hit = nullptr;

  constexpr auto default_destroy =
      sfn::cast<Health::on_destroy_t, &destruct_entity_default<Logic, S>>;
  if constexpr (requires { &Logic::on_destroy; }) {
    on_destroy = sfn::sequence<default_destroy,
                               sfn::cast<Health::on_destroy_t, ecs::call<&Logic::on_destroy>>>;
  } else {
    on_destroy = default_destroy;
  }
  if constexpr (requires { &Logic::on_hit; }) {
    on_hit = sfn::cast<Health::on_hit_t, ecs::call<&Logic::on_hit>>;
  }
  h.add(Health{.hp = hp,
               .destroy_sound = destroy_sound,
               .destroy_rumble = destroy_rumble,
               .on_hit = on_hit,
               .on_destroy = on_destroy});
}

}  // namespace ii::v0

#endif