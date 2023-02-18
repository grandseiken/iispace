#ifndef II_GAME_LOGIC_LEGACY_SHIP_TEMPLATE_H
#define II_GAME_LOGIC_LEGACY_SHIP_TEMPLATE_H
#include "game/common/colour.h"
#include "game/common/math.h"
#include "game/geom2/resolve.h"
#include "game/geom2/types.h"
#include "game/geometry/node.h"
#include "game/geometry/node_transform.h"
#include "game/logic/ecs/call.h"
#include "game/logic/ecs/index.h"
#include "game/logic/legacy/components.h"
#include "game/logic/sim/sim_interface.h"
#include <sfn/functional.h>
#include <cstddef>
#include <optional>
#include <vector>

namespace ii::legacy {

template <geom::ShapeNode... S>
using standard_transform = geom::translate_p<0, geom::rotate_p<1, S...>>;

template <ecs::Component Logic>
auto get_shape_parameters(ecs::const_handle h) {
  if constexpr (requires { &Logic::shape_parameters; }) {
    return ecs::call<&Logic::shape_parameters>(h);
  } else {
    const auto& transform = *h.get<Transform>();
    return std::tuple<vec2, fixed>{transform.centre, transform.rotation};
  }
}

inline geom::resolve_result& local_resolve() {
  static thread_local geom2::resolve_result r;
  r.entries.clear();
  return r;
}

template <geom::ShapeNode S>
void resolve_shape(const auto& parameters, geom::resolve_result& result) {
  geom::iterate(S{}, geom::iterate_resolve(), parameters, geom::transform{}, result);
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
geom::resolve_result& resolve_entity_shape(ecs::const_handle h) {
  auto& r = local_resolve();
  resolve_shape<S>(get_shape_parameters<Logic>(h), r);
  return r;
}

//////////////////////////////////////////////////////////////////////////////////
// Particles.
//////////////////////////////////////////////////////////////////////////////////
void explode_shapes(EmitHandle& e, const geom::resolve_result& r,
                    const std::optional<cvec4>& colour_override = std::nullopt,
                    std::uint32_t time = 8, const std::optional<fvec2>& towards = std::nullopt,
                    std::optional<float> speed = std::nullopt);

template <geom::ShapeNode S>
void explode_shapes(EmitHandle& e, const auto& parameters,
                    const std::optional<cvec4> colour_override = std::nullopt,
                    std::uint32_t time = 8, const std::optional<vec2>& towards = std::nullopt,
                    std::optional<float> speed = std::nullopt) {
  auto& r = local_resolve();
  resolve_shape<S>(parameters, r);
  std::optional<fvec2> towards_float;
  if (towards) {
    towards_float = to_float(*towards);
  }
  explode_shapes(e, r, colour_override, time, towards_float, speed);
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void explode_entity_shapes(ecs::const_handle h, EmitHandle& e,
                           const std::optional<cvec4> colour_override = std::nullopt,
                           std::uint32_t time = 8,
                           const std::optional<vec2>& towards = std::nullopt,
                           std::optional<float> speed = std::nullopt) {
  if constexpr (requires { &Logic::explode_shapes; }) {
    ecs::call<&Logic::explode_shapes>(h, e, colour_override, time, towards, speed);
  } else {
    explode_shapes<S>(e, get_shape_parameters<Logic>(h), colour_override, time, towards, speed);
  }
}

void destruct_lines(EmitHandle& e, const geom::resolve_result& r, const fvec2& source,
                    std::uint32_t time = 20);

template <geom::ShapeNode S>
void destruct_lines(EmitHandle& e, const auto& parameters, const vec2& source,
                    std::uint32_t time = 20) {
  // TODO: something a bit cleverer here? Take velocity of shot into account?
  // Take velocity of destructed shape into account (maybe using same system as will handle
  // motion trails)?
  // Make destruct particles similarly velocified?
  auto& r = local_resolve();
  resolve_shape<S>(parameters, r);
  destruct_lines(e, r, to_float(source), time);
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void destruct_entity_lines(ecs::const_handle h, EmitHandle& e, const vec2& source,
                           std::uint32_t time = 20) {
  if constexpr (requires { &Logic::destruct_lines; }) {
    ecs::call<&Logic::destruct_lines>(h, e, source, time);
  } else {
    destruct_lines<S>(e, get_shape_parameters<Logic>(h), source, time);
  }
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void destruct_entity_default(ecs::const_handle h, SimInterface&, EmitHandle& e, damage_type,
                             const vec2& source) {
  explode_entity_shapes<Logic, S>(h, e, std::nullopt, 10, std::nullopt, 1.5f);
  destruct_entity_lines<Logic, S>(h, e, source);
}

//////////////////////////////////////////////////////////////////////////////////
// Collision.
//////////////////////////////////////////////////////////////////////////////////
template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
constexpr shape_flag get_shape_flags() {
  shape_flag result = shape_flag::kNone;
  geom::iterate(S{}, geom::iterate_flags, geom::arbitrary_parameters{}, geom::null_transform{},
                result);
  if constexpr (requires { Logic::kShapeFlags; }) {
    result |= Logic::kShapeFlags;
  }
  return result;
}

template <geom::ShapeNode S>
geom::hit_result shape_check_collision(const auto& parameters, const geom::check_t& it) {
  geom::hit_result result;
  geom::iterate(S{}, it, parameters, geom::convert_local_transform{}, result);
  return result;
}

template <geom::ShapeNode S>
geom::hit_result shape_check_collision_legacy(const auto& parameters, const geom::check_t& it) {
  geom::hit_result result;
  if (const auto* c = std::get_if<geom::check_point_t>(&it.extent)) {
    geom::iterate(S{}, geom::check_point(it.mask, vec2{0}), parameters,
                  geom::legacy_convert_local_transform{c->v}, result);
  }
  return result;
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
geom::hit_result ship_check_collision(ecs::const_handle h, const geom::check_t& it) {
  return shape_check_collision<S>(get_shape_parameters<Logic>(h), it);
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
geom::hit_result ship_check_collision_legacy(ecs::const_handle h, const geom::check_t& it) {
  return shape_check_collision_legacy<S>(get_shape_parameters<Logic>(h), it);
}

//////////////////////////////////////////////////////////////////////////////////
// Rendering.
//////////////////////////////////////////////////////////////////////////////////
void render_shape(std::vector<render::shape>& output, const geom::resolve_result& r, float z = 0.f,
                  const std::optional<float>& hit_alpha = std::nullopt,
                  const std::optional<cvec4>& c_override = std::nullopt,
                  const std::optional<std::size_t>& c_override_max_index = std::nullopt);

template <geom::ShapeNode S>
void render_shape(std::vector<render::shape>& output, const auto& parameters, float z = 0.f,
                  const std::optional<float>& hit_alpha = std::nullopt,
                  const std::optional<cvec4>& c_override = std::nullopt,
                  const std::optional<std::size_t>& c_override_max_index = std::nullopt) {
  auto& r = local_resolve();
  resolve_shape<S>(parameters, r);
  render_shape(output, r, z, hit_alpha, c_override, c_override_max_index);
}

template <geom::ShapeNode S>
void render_entity_shape_override(std::vector<render::shape>& output, const Health* health,
                                  const auto& parameters, float z = 0.f,
                                  const std::optional<cvec4>& colour_override = std::nullopt) {
  std::optional<float> hit_alpha;
  std::optional<std::size_t> c_override_max_index;
  if (!colour_override && health && health->hit_timer) {
    hit_alpha = std::min(1.f, health->hit_timer / 10.f);
    c_override_max_index = health->hit_flash_ignore_index;
  }
  render_shape<S>(output, parameters, z, hit_alpha, colour_override, c_override_max_index);
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void render_entity_shape(ecs::const_handle h, const Health* health,
                         std::vector<render::shape>& output) {
  render_entity_shape_override<S>(output, health, get_shape_parameters<Logic>(h), Logic::kZIndex,
                                  std::nullopt);
}

//////////////////////////////////////////////////////////////////////////////////
// Creation templates.
//////////////////////////////////////////////////////////////////////////////////
template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
ecs::handle create_ship(SimInterface& sim, const vec2& position, fixed rotation = 0) {
  auto h = sim.index().create();
  h.add(Update{.update = ecs::call<&Logic::update>});
  h.add(Transform{.centre = position, .rotation = rotation});

  static constexpr auto collision_flags = get_shape_flags<Logic, S>();
  static constexpr bool has_constant_bw = requires { Logic::kBoundingWidth; };
  static constexpr bool has_function_bw = requires { &Logic::bounding_width; };

  if constexpr (+collision_flags && (has_constant_bw || has_function_bw)) {
    fixed bounding_width = 0;
    if constexpr (has_function_bw) {
      bounding_width = Logic::bounding_width(sim);
    } else {
      bounding_width = Logic::kBoundingWidth;
    }
    h.add(Collision{.flags = collision_flags,
                    .bounding_width = bounding_width,
                    .check_collision = sim.is_legacy() ? &ship_check_collision_legacy<Logic, S>
                                                       : &ship_check_collision<Logic, S>});
  }

  constexpr auto render = ecs::call<&render_entity_shape<Logic, S>>;
  if constexpr (requires { &Logic::render; }) {
    h.add(Render{.render = sfn::sequence<sfn::cast<Render::render_t, render>,
                                         sfn::cast<Render::render_t, ecs::call<&Logic::render>>>});
  } else {
    h.add(Render{.render = sfn::cast<Render::render_t, render>});
  }
  return h;
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

}  // namespace ii::legacy

#endif
