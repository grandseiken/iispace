#ifndef II_GAME_LOGIC_SHIP_SHIP_TEMPLATE_H
#define II_GAME_LOGIC_SHIP_SHIP_TEMPLATE_H
#include "game/logic/ecs/call.h"
#include "game/logic/ecs/index.h"
#include "game/logic/ship/components.h"
#include "game/logic/ship/geometry.h"
#include "game/logic/ship/ship.h"
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
void render_shape(const SimInterface& sim, const auto& parameters,
                  const std::optional<glm::vec4>& c_override = std::nullopt) {
  geom::iterate(
      S{}, geom::iterate_lines, parameters, {},
      [&](const vec2& a, const vec2& b, const glm::vec4& c) {
        auto colour = c_override ? glm::vec4{c_override->r, c_override->g, c_override->b, c.a} : c;
        sim.render_line(to_float(a), to_float(b), colour);
      });
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void render_entity_shape(ecs::const_handle h, const Render& render, const Health* health,
                         const SimInterface& sim) {
  auto c_override = render.colour_override;
  if (!c_override && health && health->hit_timer) {
    c_override = glm::vec4{1.f};
  }
  render_shape<S>(sim, get_shape_parameters<Logic>(h), c_override);
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void explode_entity_shapes_with_colour(ecs::const_handle h, SimInterface& sim, const glm::vec4& c,
                                       std::uint32_t time = 8) {
  geom::iterate(S{}, geom::iterate_centres, get_shape_parameters<Logic>(h), {},
                [&](const vec2& v, const glm::vec4&) { sim.explosion(to_float(v), c, time); });
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void explode_entity_shapes(ecs::const_handle h, SimInterface& sim) {
  geom::iterate(S{}, geom::iterate_centres, get_shape_parameters<Logic>(h), {},
                [&](const vec2& v, const glm::vec4& c) { sim.explosion(to_float(v), c); });
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void explode_entity_shapes_towards(ecs::const_handle h, SimInterface& sim, std::uint32_t time,
                                   const vec2& towards) {
  geom::iterate(S{}, geom::iterate_centres, get_shape_parameters<Logic>(h), {},
                [&](const vec2& v, const glm::vec4& c) {
                  sim.explosion(to_float(v), c, time, to_float(towards));
                });
}

template <ecs::Component Logic, geom::ShapeNode S>
bool ship_check_point(ecs::const_handle h, const vec2& v, shape_flag mask) {
  return geom::check_point(S{}, get_shape_parameters<Logic>(h), v, mask);
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
ecs::handle create_ship(SimInterface& sim, const vec2& position, fixed rotation = 0) {
  auto h = sim.index().create();
  h.add(ShipFlags{.flags = Logic::kShipFlags});
  h.add(LegacyShip{.ship = std::make_unique<ShipForwarder>(sim, h)});

  h.add(Update{.update = ecs::call<&Logic::update>});
  h.add(Transform{.centre = position, .rotation = rotation});

  constexpr auto collision_shape_flags = [&]() constexpr {
    shape_flag result = shape_flag::kNone;
    geom::iterate(
        S{}, geom::iterate_flags, geom::arbitrary_parameters{},
        {}, [&](shape_flag f) constexpr { result |= f; });
    return result;
  }
  ();
  if constexpr (+collision_shape_flags) {
    h.add(Collision{.flags = collision_shape_flags,
                    .bounding_width = Logic::kBoundingWidth,
                    .check = &ship_check_point<Logic, S>});
  }

  constexpr auto render = ecs::call<&render_entity_shape<Logic, S>>;
  if constexpr (requires { &Logic::render; }) {
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
  constexpr auto explode_shapes = cast<on_destroy_t, &explode_entity_shapes<Logic, S>>;
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