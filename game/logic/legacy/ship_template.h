#ifndef II_GAME_LOGIC_LEGACY_SHIP_TEMPLATE_H
#define II_GAME_LOGIC_LEGACY_SHIP_TEMPLATE_H
#include "game/common/colour.h"
#include "game/common/math.h"
#include "game/geometry/shape_bank.h"
#include "game/logic/ecs/call.h"
#include "game/logic/ecs/index.h"
#include "game/logic/legacy/components.h"
#include "game/logic/sim/sim_interface.h"
#include <sfn/functional.h>
#include <cstddef>
#include <optional>
#include <vector>

namespace ii::legacy {

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

template <ecs::Component Logic, fixed Width>
using shape_definition_with_width =
    shape_definition<&Logic::construct_shape, ecs::call<&Logic::set_parameters>, Width,
                     Logic::kFlags>;

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
// Particles.
//////////////////////////////////////////////////////////////////////////////////
void explode_shapes(EmitHandle& e, const geom::resolve_result& r,
                    const std::optional<cvec4>& colour_override = std::nullopt,
                    std::uint32_t time = 8, const std::optional<fvec2>& towards = std::nullopt,
                    std::optional<float> speed = std::nullopt);
void destruct_lines(EmitHandle& e, const geom::resolve_result& r, const fvec2& source,
                    std::uint32_t time = 20);

template <ecs::Component Logic, typename ShapeDefinition = default_shape_definition<Logic>>
void destruct_entity_default(ecs::const_handle h, SimInterface& sim, EmitHandle& e, damage_type,
                             const vec2& source) {
  // TODO: something a bit cleverer here? Take velocity of shot into account?
  // Take velocity of destructed shape into account (maybe using same system as will handle
  // motion trails)? Make destruct particles similarly velocified?
  auto& r = resolve_entity_shape<ShapeDefinition>(h, sim);
  explode_shapes(e, r, std::nullopt, 10, std::nullopt, 1.5f);
  destruct_lines(e, r, to_float(source));
}

//////////////////////////////////////////////////////////////////////////////////
// Collision.
//////////////////////////////////////////////////////////////////////////////////
template <typename ShapeDefinition>
geom::hit_result
ship_check_collision(ecs::const_handle h, const geom::check_t& check, const SimInterface& sim) {
  geom::hit_result result;
  geom::check_collision(
      result, check, sim.shape_bank(), ShapeDefinition::construct_shape,
      [&h](geom::parameter_set& parameters) { ShapeDefinition::set_parameters(h, parameters); });
  return result;
}

template <typename ShapeDefinition>
geom::hit_result ship_check_collision_legacy(ecs::const_handle h, const geom::check_t& check,
                                             const SimInterface& sim) {
  geom::hit_result result;
  auto legacy_check = check;
  legacy_check.legacy_algorithm = true;
  geom::check_collision(
      result, legacy_check, sim.shape_bank(), ShapeDefinition::construct_shape,
      [&h](geom::parameter_set& parameters) { ShapeDefinition::set_parameters(h, parameters); });
  return result;
}

//////////////////////////////////////////////////////////////////////////////////
// Rendering.
//////////////////////////////////////////////////////////////////////////////////
void render_shape(std::vector<render::shape>& output, const geom::resolve_result& r, float z = 0.f,
                  const std::optional<float>& hit_alpha = std::nullopt,
                  const std::optional<cvec4>& c_override = std::nullopt);

template <typename ShapeDefinition>
void render_entity_shape_override(std::vector<render::shape>& output, ecs::const_handle h,
                                  const Health* health, const SimInterface& sim, float z = 0.f,
                                  const std::optional<cvec4>& colour_override = std::nullopt) {
  std::optional<float> hit_alpha;
  if (!colour_override && health && health->hit_timer) {
    hit_alpha = std::min(1.f, health->hit_timer / 10.f);
  }
  auto& r = resolve_entity_shape<ShapeDefinition>(h, sim);
  render_shape(output, r, z, hit_alpha, colour_override);
}

template <typename Logic, typename ShapeDefinition = default_shape_definition<Logic>>
void render_entity_shape(ecs::const_handle h, const Health* health,
                         std::vector<render::shape>& output, const SimInterface& sim) {
  render_entity_shape_override<ShapeDefinition>(output, h, health, sim, Logic::kZIndex,
                                                std::nullopt);
}

//////////////////////////////////////////////////////////////////////////////////
// Creation templates.
//////////////////////////////////////////////////////////////////////////////////
template <ecs::Component Logic, typename ShapeDefinition = default_shape_definition<Logic>>
ecs::handle create_ship(SimInterface& sim, const vec2& position, fixed rotation = 0) {
  auto h = sim.index().create();
  h.add(Update{.update = ecs::call<&Logic::update>});
  h.add(Transform{.centre = position, .rotation = rotation});

  if constexpr (+ShapeDefinition::flags) {
    h.add(Collision{.flags = ShapeDefinition::flags,
                    .bounding_width = ShapeDefinition::bounding_width,
                    .check_collision = sim.is_legacy()
                        ? &ship_check_collision_legacy<ShapeDefinition>
                        : &ship_check_collision<ShapeDefinition>});
  }

  constexpr auto render = ecs::call<&render_entity_shape<Logic, ShapeDefinition>>;
  if constexpr (requires { &Logic::render; }) {
    h.add(Render{.render = sfn::sequence<sfn::cast<Render::render_t, render>,
                                         sfn::cast<Render::render_t, ecs::call<&Logic::render>>>});
  } else {
    h.add(Render{.render = sfn::cast<Render::render_t, render>});
  }
  return h;
}

template <ecs::Component Logic, typename ShapeDefinition = default_shape_definition<Logic>>
void add_enemy_health(ecs::handle h, std::uint32_t hp,
                      std::optional<sound> destroy_sound = std::nullopt,
                      std::optional<rumble_type> destroy_rumble = std::nullopt) {
  destroy_sound = destroy_sound ? *destroy_sound : Logic::kDestroySound;
  destroy_rumble = destroy_rumble ? *destroy_rumble : Logic::kDestroyRumble;
  using on_destroy_t =
      void(ecs::const_handle, SimInterface&, EmitHandle&, damage_type, const vec2&);
  constexpr auto on_destroy =
      sfn::cast<on_destroy_t, &destruct_entity_default<Logic, ShapeDefinition>>;
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

}  // namespace ii::legacy

#endif
