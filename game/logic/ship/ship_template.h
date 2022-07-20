#ifndef II_GAME_LOGIC_SHIP_SHIP_TEMPLATE_H
#define II_GAME_LOGIC_SHIP_SHIP_TEMPLATE_H
#include "game/logic/ecs/call.h"
#include "game/logic/ecs/index.h"
#include "game/logic/ship/components.h"
#include "game/logic/ship/geometry.h"
#include "game/logic/sim/sim_interface.h"

namespace ii {

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

template <geom::ShapeNode S>
void render_shape(const SimInterface& sim, const auto& parameters, const geom::transform& t = {},
                  const std::optional<glm::vec4>& c_override = std::nullopt,
                  const std::optional<std::size_t>& c_override_max_index = std::nullopt) {
  std::size_t i = 0;
  geom::transform transform = t;
  transform.shape_index_out = &i;
  geom::iterate(S{}, geom::iterate_lines, parameters, transform,
                [&](const vec2& a, const vec2& b, const glm::vec4& c) {
                  auto colour = c_override && (!c_override_max_index || i < *c_override_max_index)
                      ? glm::vec4{c_override->r, c_override->g, c_override->b, c.a}
                      : c;
                  sim.render_line(to_float(a), to_float(b), colour);
                });
}

template <geom::ShapeNode S>
void render_entity_shape_override(const SimInterface& sim, const Health* health,
                                  const auto& parameters, const geom::transform& t = {},
                                  const std::optional<glm::vec4>& colour_override = std::nullopt) {
  std::optional<glm::vec4> c_override = colour_override;
  std::optional<std::size_t> c_override_max_index;
  if (!colour_override && health && health->hit_timer) {
    c_override = glm::vec4{1.f};
    c_override_max_index = health->hit_flash_ignore_index;
  }
  render_shape<S>(sim, parameters, t, c_override, c_override_max_index);
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void render_entity_shape(ecs::const_handle h, const Health* health, const SimInterface& sim) {
  render_entity_shape_override<S>(sim, health, get_shape_parameters<Logic>(h), {}, std::nullopt);
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void iterate_entity_attachment_points(ecs::const_handle h,
                                      const geom::AttachmentPointFunction auto& f) {
  geom::iterate(S{}, geom::iterate_attachment_points, get_shape_parameters<Logic>(h),
                geom::transform{}, f);
}

template <geom::ShapeNode S>
void explode_shapes(SimInterface& sim, const auto& parameters,
                    const std::optional<glm::vec4> colour_override = std::nullopt,
                    std::uint32_t time = 8, const std::optional<vec2>& towards = std::nullopt) {
  std::optional<glm::vec2> towards_float;
  if (towards) {
    towards_float = to_float(*towards);
  }
  geom::iterate(S{}, geom::iterate_centres, parameters, {}, [&](const vec2& v, const glm::vec4& c) {
    sim.explosion(to_float(v), colour_override.value_or(c), time, towards_float);
  });
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void explode_entity_shapes(ecs::const_handle h, SimInterface& sim,
                           const std::optional<glm::vec4> colour_override = std::nullopt,
                           std::uint32_t time = 8,
                           const std::optional<vec2>& towards = std::nullopt) {
  if constexpr (requires { &Logic::explode_shapes; }) {
    ecs::call<&Logic::explode_shapes>(h, sim, colour_override, time, towards);
  } else {
    explode_shapes<S>(sim, get_shape_parameters<Logic>(h), colour_override, time, towards);
  }
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void explode_entity_shapes_default(ecs::const_handle h, SimInterface& sim) {
  return explode_entity_shapes<Logic, S>(h, sim);
}

template <ecs::Component Logic, geom::ShapeNode S>
bool ship_check_point(ecs::const_handle h, const vec2& v, shape_flag mask) {
  if constexpr (requires { &Logic::check_point; }) {
    return ecs::call<&Logic::check_point>(h, v, mask);
  } else {
    return geom::check_point(S{}, get_shape_parameters<Logic>(h), v, mask);
  }
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
ecs::handle create_ship(SimInterface& sim, const vec2& position, fixed rotation = 0) {
  auto h = sim.index().create();
  h.add(Update{.update = ecs::call<&Logic::update>});
  h.add(Transform{.centre = position, .rotation = rotation});

  if constexpr (requires { &Logic::kBoundingWidth; }) {
    constexpr auto collision_shape_flags = [&]() constexpr {
      shape_flag result = shape_flag::kNone;
      geom::iterate(
          S{}, geom::iterate_flags, geom::arbitrary_parameters{},
          {}, [&](shape_flag f) constexpr { result |= f; });
      return result;
    }
    ();
    auto extra_shape_flags = shape_flag::kNone;
    if constexpr (requires { Logic::kShapeFlags; }) {
      extra_shape_flags |= Logic::kShapeFlags;
    }
    if constexpr (+collision_shape_flags || requires { Logic::kShapeFlags; }) {
      h.add(Collision{.flags = collision_shape_flags | extra_shape_flags,
                      .bounding_width = Logic::kBoundingWidth,
                      .check = &ship_check_point<Logic, S>});
    }
  }

  constexpr auto render = ecs::call<&render_entity_shape<Logic, S>>;
  if constexpr (requires { &Logic::render_override; }) {
    h.add(Render{.render = ecs::call<&Logic::render_override>});
  } else if constexpr (requires { &Logic::render; }) {
    h.add(Render{.render = sequence<render, ecs::call<&Logic::render>>});
  } else {
    h.add(Render{.render = render});
  }
  return h;
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void add_enemy_health(ecs::handle h, std::uint32_t hp,
                      std::optional<sound> destroy_sound = std::nullopt) {
  destroy_sound = destroy_sound ? *destroy_sound : Logic::kDestroySound;
  using on_destroy_t = void(ecs::const_handle, SimInterface & sim, damage_type);
  constexpr auto explode_shapes = cast<on_destroy_t, &explode_entity_shapes_default<Logic, S>>;
  if constexpr (requires { &Logic::on_destroy; }) {
    h.add(Health{.hp = hp,
                 .destroy_sound = destroy_sound,
                 .on_destroy =
                     sequence<explode_shapes, cast<on_destroy_t, ecs::call<&Logic::on_destroy>>>});
  } else {
    h.add(Health{.hp = hp, .destroy_sound = destroy_sound, .on_destroy = explode_shapes});
  }
}

}  // namespace ii

#endif