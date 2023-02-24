#ifndef II_GAME_LOGIC_V0_LIB_SHIP_TEMPLATE_H
#define II_GAME_LOGIC_V0_LIB_SHIP_TEMPLATE_H
#include "game/common/colour.h"
#include "game/common/math.h"
#include "game/geometry/shape_bank.h"
#include "game/logic/ecs/call.h"
#include "game/logic/ecs/index.h"
#include "game/logic/sim/sim_interface.h"
#include "game/logic/v0/lib/components.h"
#include <sfn/functional.h>
#include <cstddef>
#include <optional>
#include <span>
#include <vector>

namespace ii::v0 {

template <sfn::ptr<void(geom::node&)> ConstructShape,
          sfn::ptr<void(ecs::const_handle, geom::parameter_set&)> SetParameters,
          fixed BoundingWidth, shape_flag Flags>
struct shape_definition {
  inline static constexpr auto construct_shape = ConstructShape;
  inline static constexpr auto set_parameters = SetParameters;
  inline static constexpr auto bounding_width = BoundingWidth;
  inline static constexpr auto flags = Flags;
};

template <ecs::Component Logic>
using default_shape_definition =
    shape_definition<&Logic::construct_shape, ecs::call<&Logic::set_parameters>,
                     Logic::kBoundingWidth, Logic::kFlags>;

inline geom::resolve_result& local_resolve() {
  static thread_local geom::resolve_result r;
  r.entries.clear();
  return r;
}

template <sfn::ptr<void(geom::node&)> ConstructShape, typename F>
geom::resolve_result& resolve_shape(const SimInterface& sim, F&& set_function) {
  auto& r = local_resolve();
  geom::resolve(r, sim.shape_bank(), ConstructShape, set_function);
  return r;
}

template <typename ShapeDefinition>
geom::resolve_result& resolve_entity_shape(ecs::const_handle h, const SimInterface& sim) {
  return resolve_shape<ShapeDefinition::construct_shape>(
      sim,
      [&h](geom::parameter_set& parameters) { ShapeDefinition::set_parameters(h, parameters); });
}

//////////////////////////////////////////////////////////////////////////////////
// Rendering.
//////////////////////////////////////////////////////////////////////////////////
std::optional<cvec4> get_shape_colour(const geom::resolve_result& r);

void render_shape(std::vector<render::shape>& output, const geom::resolve_result& r,
                  const std::optional<float>& hit_alpha = std::nullopt,
                  const std::optional<float>& shield_alpha = std::nullopt);

template <typename ShapeDefinition>
void render_entity_shape(ecs::const_handle h, const Health* health, const EnemyStatus* status,
                         std::vector<render::shape>& output, const SimInterface& sim) {
  std::optional<float> hit_alpha;
  std::optional<float> shield_alpha;
  if (status && status->shielded_ticks) {
    shield_alpha = std::min(1.f, status->shielded_ticks / 16.f);
  }
  if (health && health->hit_timer) {
    hit_alpha = std::min(1.f, health->hit_timer / 10.f);
  }
  auto& r = resolve_entity_shape<ShapeDefinition>(h, sim);
  render_shape(output, r, hit_alpha, shield_alpha);
}

//////////////////////////////////////////////////////////////////////////////////
// Collision.
//////////////////////////////////////////////////////////////////////////////////
template <typename ShapeDefinition>
geom::hit_result
check_entity_collision(ecs::const_handle h, const geom::check_t& check, const SimInterface& sim) {
  geom::hit_result result;
  geom::check_collision(
      result, check, sim.shape_bank(), ShapeDefinition::construct_shape,
      [&h](geom::parameter_set& parameters) { ShapeDefinition::set_parameters(h, parameters); });
  return result;
}

//////////////////////////////////////////////////////////////////////////////////
// Creation templates.
//////////////////////////////////////////////////////////////////////////////////
template <ecs::Component Logic>
ecs::handle create_ship(SimInterface& sim, const vec2& position, fixed rotation = 0) {
  auto h = sim.index().create();
  add(h, Update{.update = ecs::call<&Logic::update>});
  add(h, Transform{.centre = position, .rotation = rotation});
  return h;
}

template <typename Logic, typename ShapeDefinition = default_shape_definition<Logic>>
ecs::handle add_collision(ecs::handle h) {
  add(h,
      Collision{.flags = ShapeDefinition::flags,
                .bounding_width = ShapeDefinition::bounding_width,
                .check_collision = &check_entity_collision<ShapeDefinition>});
  return h;
}

template <ecs::Component Logic, typename ShapeDefinition = default_shape_definition<Logic>>
ecs::handle add_render(ecs::handle h) {
  constexpr auto render_shape = ecs::call<&render_entity_shape<ShapeDefinition>>;

  sfn::ptr<Render::render_t> render_ptr = render_shape;
  sfn::ptr<Render::render_panel_t> render_panel_ptr = nullptr;
  if constexpr (requires { &Logic::render; }) {
    constexpr auto render_custom = sfn::cast<Render::render_t, ecs::call<&Logic::render>>;
    render_ptr = sfn::sequence<render_shape, render_custom>;
  }
  if constexpr (requires { &Logic::render_panel; }) {
    render_panel_ptr = sfn::cast<Render::render_panel_t, ecs::call<&Logic::render_panel>>;
  }
  add(h, Render{.render = render_ptr, .render_panel = render_panel_ptr});
  return h;
}

template <ecs::Component Logic, typename ShapeDefinition = default_shape_definition<Logic>>
ecs::handle create_ship_default(SimInterface& sim, const vec2& position, fixed rotation = 0) {
  auto h = create_ship<Logic>(sim, position, rotation);
  if constexpr (+ShapeDefinition::flags) {
    add_collision<Logic, ShapeDefinition>(h);
  }
  return add_render<Logic, ShapeDefinition>(h);
}

}  // namespace ii::v0

#endif
