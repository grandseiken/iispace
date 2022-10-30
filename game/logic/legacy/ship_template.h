#ifndef II_GAME_LOGIC_LEGACY_SHIP_TEMPLATE_H
#define II_GAME_LOGIC_LEGACY_SHIP_TEMPLATE_H
#include "game/common/math.h"
#include "game/logic/ecs/call.h"
#include "game/logic/ecs/index.h"
#include "game/logic/geometry/enums.h"
#include "game/logic/geometry/node.h"
#include "game/logic/geometry/node_transform.h"
#include "game/logic/sim/components.h"
#include "game/logic/sim/io/render.h"
#include "game/logic/sim/sim_interface.h"
#include "game/logic/v0/ship_template.h"
#include <sfn/functional.h>
#include <cstddef>
#include <optional>
#include <vector>

namespace ii::legacy {

using v0::get_shape_parameters;
using v0::render_entity_shape;
using v0::render_entity_shape_override;
using v0::render_shape;
using v0::shape_check_point;
using v0::ship_check_point;
using v0::standard_transform;

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void iterate_entity_attachment_points(ecs::const_handle h,
                                      const geom::AttachmentPointFunction auto& f) {
  geom::iterate(S{}, geom::iterate_attachment_points, get_shape_parameters<Logic>(h),
                geom::transform{}, f);
}

//////////////////////////////////////////////////////////////////////////////////
// Particles.
//////////////////////////////////////////////////////////////////////////////////
template <geom::ShapeNode S>
void explode_shapes(EmitHandle& e, const auto& parameters,
                    const std::optional<glm::vec4> colour_override = std::nullopt,
                    std::uint32_t time = 8, const std::optional<vec2>& towards = std::nullopt,
                    std::optional<float> speed = std::nullopt) {
  std::optional<glm::vec2> towards_float;
  if (towards) {
    towards_float = to_float(*towards);
  }
  geom::iterate(S{}, geom::iterate_centres, parameters, geom::transform{},
                [&](const vec2& v, const glm::vec4& c) {
                  e.explosion(to_float(v), colour_override.value_or(c), time, towards_float, speed);
                });
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void explode_entity_shapes(ecs::const_handle h, EmitHandle& e,
                           const std::optional<glm::vec4> colour_override = std::nullopt,
                           std::uint32_t time = 8,
                           const std::optional<vec2>& towards = std::nullopt,
                           std::optional<float> speed = std::nullopt) {
  if constexpr (requires { &Logic::explode_shapes; }) {
    ecs::call<&Logic::explode_shapes>(h, e, colour_override, time, towards, speed);
  } else {
    explode_shapes<S>(e, get_shape_parameters<Logic>(h), colour_override, time, towards, speed);
  }
}

inline void add_line_particle(EmitHandle& e, const glm::vec2& source, const glm::vec2& a,
                              const glm::vec2& b, const glm::vec4& c, std::uint32_t time) {
  auto& r = e.random();
  auto position = (a + b) / 2.f;
  auto velocity = (2.f + .5f * r.fixed().to_float()) * normalise(position - source) +
      from_polar((2 * fixed_c::pi * r.fixed()).to_float(), (r.fixed() / 8).to_float());
  auto diameter = distance(a, b);
  auto d = distance(source, a) - distance(source, b);
  if (angle_diff(angle(a - source), angle(b - source)) > 0) {
    d = -d;
  }
  auto angular_velocity = (d / (32.f * diameter)) + r.fixed().to_float() / 64.f;
  e.add(particle{
      .position = position - velocity,
      .velocity = velocity,
      .colour = c,
      .data =
          line_particle{
              .radius = diameter / 2.f,
              .rotation = angle(b - a) - angular_velocity,
              .angular_velocity = angular_velocity,
          },
      .end_time = time + r.uint(time),
      .flash_time = 3,
      .fade = true,
  });
}

template <geom::ShapeNode S>
void destruct_lines(EmitHandle& e, const auto& parameters, const vec2& source,
                    std::uint32_t time = 20) {
  auto f_source = to_float(source);
  // TODO: something a bit cleverer here? Take velocity of shot into account?
  // Take velocity of destructed shape into account (maybe using same system as will handle
  // motion trails)?
  // Make destruct particles similarly velocified?
  geom::iterate(S{}, geom::iterate_lines, parameters, geom::transform{},
                [&](const vec2& a, const vec2& b, const glm::vec4& c) {
                  add_line_particle(e, f_source, to_float(a), to_float(b), c, time);
                });
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
template <geom::ShapeNode S>
shape_flag shape_check_point_legacy(const auto& parameters, const vec2& v, shape_flag mask) {
  shape_flag result = shape_flag::kNone;
  geom::iterate(S{}, geom::iterate_collision(mask), parameters,
                geom::legacy_convert_local_transform{v}, [&](shape_flag f) { result |= f; });
  return result;
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
shape_flag ship_check_point_legacy(ecs::const_handle h, const vec2& v, shape_flag mask) {
  return shape_check_point_legacy<S>(get_shape_parameters<Logic>(h), v, mask);
}

//////////////////////////////////////////////////////////////////////////////////
// Creation templates.
//////////////////////////////////////////////////////////////////////////////////
template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
ecs::handle create_ship(SimInterface& sim, const vec2& position, fixed rotation = 0) {
  auto h = v0::create_ship<Logic>(sim, position, rotation);

  static constexpr auto collision_flags = v0::get_shape_flags<Logic, S>();
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
                    .check = sim.is_legacy() ? &ship_check_point_legacy<Logic, S>
                                             : &v0::ship_check_point<Logic, S>});
  }

  v0::add_render<Logic, S>(h);
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
