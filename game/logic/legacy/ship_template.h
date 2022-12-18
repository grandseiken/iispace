#ifndef II_GAME_LOGIC_LEGACY_SHIP_TEMPLATE_H
#define II_GAME_LOGIC_LEGACY_SHIP_TEMPLATE_H
#include "game/common/colour.h"
#include "game/common/math.h"
#include "game/geometry/enums.h"
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
                    const std::optional<cvec4> colour_override = std::nullopt,
                    std::uint32_t time = 8, const std::optional<vec2>& towards = std::nullopt,
                    std::optional<float> speed = std::nullopt) {
  std::optional<fvec2> towards_float;
  if (towards) {
    towards_float = to_float(*towards);
  }
  geom::iterate(S{}, geom::iterate_centres, parameters, geom::transform{},
                [&](const vec2& v, const cvec4& c) {
                  e.explosion(to_float(v), colour_override.value_or(c), time, towards_float, speed);
                });
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

inline void add_line_particle(EmitHandle& e, const fvec2& source, const fvec2& a, const fvec2& b,
                              const cvec4& c, float w, float z, std::uint32_t time) {
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
              .width = w,
          },
      .end_time = time + r.uint(time),
      .flash_time = z > colour::kZTrails ? 16u : 0u,
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
                [&](const vec2& a, const vec2& b, const cvec4& c, float w, float z) {
                  add_line_particle(e, f_source, to_float(a), to_float(b), c, w, z, time);
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
template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
constexpr shape_flag get_shape_flags() {
  shape_flag result = shape_flag::kNone;
  geom::iterate(S{}, geom::iterate_flags, geom::arbitrary_parameters{}, geom::transform{},
                [&](shape_flag f) constexpr { result |= f; });
  if constexpr (requires { Logic::kShapeFlags; }) {
    result |= Logic::kShapeFlags;
  }
  return result;
}

template <geom::ShapeNode S>
shape_flag shape_check_point(const auto& parameters, const vec2& v, shape_flag mask) {
  shape_flag result = shape_flag::kNone;
  geom::iterate(S{}, geom::iterate_collision(mask), parameters, geom::convert_local_transform{v},
                [&](shape_flag f) { result |= f; });
  return result;
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
shape_flag ship_check_point(ecs::const_handle h, const vec2& v, shape_flag mask) {
  return shape_check_point<S>(get_shape_parameters<Logic>(h), v, mask);
}

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
// Rendering.
//////////////////////////////////////////////////////////////////////////////////
template <geom::ShapeNode S>
void render_shape(std::vector<render::shape>& output, const auto& parameters, float z_index = 0.f,
                  const geom::transform& t = {},
                  const std::optional<float> hit_alpha = std::nullopt,
                  const std::optional<cvec4>& c_override = std::nullopt,
                  const std::optional<std::size_t>& c_override_max_index = std::nullopt) {
  std::size_t i = 0;
  geom::transform transform = t;
  transform.index_out = &i;
  geom::iterate(S{}, geom::iterate_shapes, parameters, transform, [&](const render::shape& shape) {
    render::shape shape_copy = shape;
    shape_copy.z_index = z_index;
    if ((c_override || hit_alpha) && (!c_override_max_index || i < *c_override_max_index)) {
      if (c_override) {
        shape_copy.colour = cvec4{c_override->r, c_override->g, c_override->b, shape.colour.a};
      }
      if (hit_alpha) {
        shape_copy.apply_hit_flash(*hit_alpha);
      }
    }
    output.emplace_back(shape_copy);
  });
}

template <geom::ShapeNode S>
void render_entity_shape_override(std::vector<render::shape>& output, const Health* health,
                                  const auto& parameters, float z_index = 0.f,
                                  const geom::transform& t = {},
                                  const std::optional<cvec4>& colour_override = std::nullopt) {
  std::optional<float> hit_alpha;
  std::optional<std::size_t> c_override_max_index;
  if (!colour_override && health && health->hit_timer) {
    hit_alpha = std::min(1.f, health->hit_timer / 10.f);
    c_override_max_index = health->hit_flash_ignore_index;
  }
  render_shape<S>(output, parameters, z_index, t, hit_alpha, colour_override, c_override_max_index);
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void render_entity_shape(ecs::const_handle h, const Health* health,
                         std::vector<render::shape>& output) {
  render_entity_shape_override<S>(output, health, get_shape_parameters<Logic>(h), Logic::kZIndex,
                                  {}, std::nullopt);
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
                    .check = sim.is_legacy() ? &ship_check_point_legacy<Logic, S>
                                             : &ship_check_point<Logic, S>});
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
