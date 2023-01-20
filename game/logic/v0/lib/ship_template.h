#ifndef II_GAME_LOGIC_V0_LIB_SHIP_TEMPLATE_H
#define II_GAME_LOGIC_V0_LIB_SHIP_TEMPLATE_H
#include "game/common/colour.h"
#include "game/common/math.h"
#include "game/geometry/enums.h"
#include "game/geometry/node.h"
#include "game/geometry/node_transform.h"
#include "game/logic/ecs/call.h"
#include "game/logic/ecs/index.h"
#include "game/logic/sim/sim_interface.h"
#include "game/logic/v0/lib/components.h"
#include <sfn/functional.h>
#include <cstddef>
#include <optional>
#include <span>
#include <vector>

// TODO: basically, need to do less template instantiation. Just have a single geometry resolve
// function with callbacks and then iterate over shape output in non-template code? Probably
// means we need to cache the resolved shape data in a component or something.
namespace ii::v0 {

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

//////////////////////////////////////////////////////////////////////////////////
// Rendering.
//////////////////////////////////////////////////////////////////////////////////
template <geom::ShapeNode S>
void render_shape(std::vector<render::shape>& output, const auto& parameters,
                  const geom::transform& t = {},
                  const std::optional<float>& hit_alpha = std::nullopt,
                  const std::optional<float>& shield_alpha = std::nullopt,
                  const std::optional<std::size_t>& c_override_max_index = std::nullopt) {
  std::size_t i = 0;
  geom::transform transform = t;
  transform.index_out = &i;
  geom::iterate(S{}, geom::iterate_shapes, parameters, transform, [&](const render::shape& shape) {
    render::shape shape_copy = shape;
    bool override = !c_override_max_index || i < *c_override_max_index;
    if (shape.z_index > colour::kZTrails && hit_alpha && override) {
      shape_copy.apply_hit_flash(*hit_alpha);
    }
    if (shape.z_index <= colour::kZTrails && shield_alpha && override) {
      auto shield_copy = shape_copy;
      if (shape_copy.apply_shield(shield_copy, *shield_alpha)) {
        output.emplace_back(shield_copy);
      }
    }
    output.emplace_back(shape_copy);
  });
}

template <geom::ShapeNode S>
void render_entity_shape_override(std::vector<render::shape>& output, const Health* health,
                                  const EnemyStatus* status, const auto& parameters,
                                  const geom::transform& t = {}) {
  std::optional<float> hit_alpha;
  std::optional<float> shield_alpha;
  std::optional<std::size_t> c_override_max_index;
  if (status && status->shielded_ticks) {
    shield_alpha = std::min(1.f, status->shielded_ticks / 16.f);
  }
  if (health) {
    if (health->hit_timer) {
      hit_alpha = std::min(1.f, health->hit_timer / 10.f);
    }
    c_override_max_index = health->hit_flash_ignore_index;
  }
  render_shape<S>(output, parameters, t, hit_alpha, shield_alpha, c_override_max_index);
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void render_entity_shape(ecs::const_handle h, const Health* health, const EnemyStatus* status,
                         std::vector<render::shape>& output) {
  render_entity_shape_override<S>(output, health, status, get_shape_parameters<Logic>(h), {});
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
Collision::hit_result shape_check_point(const auto& parameters, const vec2& v, shape_flag mask) {
  Collision::hit_result result;
  geom::iterate(S{}, geom::iterate_check_point(mask), parameters, geom::convert_local_transform{v},
                [&](shape_flag f, const vec2& c) {
                  result.mask |= f;
                  result.shape_centres.emplace_back(c);
                });
  return result;
}

template <geom::ShapeNode S>
Collision::hit_result
shape_check_line(const auto& parameters, const vec2& a, const vec2& b, shape_flag mask) {
  Collision::hit_result result;
  geom::iterate(S{}, geom::iterate_check_line(mask), parameters,
                geom::convert_local_line_transform{a, b}, [&](shape_flag f, const vec2& c) {
                  result.mask |= f;
                  result.shape_centres.emplace_back(c);
                });
  return result;
}

template <geom::ShapeNode S>
Collision::hit_result
shape_check_ball(const auto& parameters, const vec2& centre, fixed radius, shape_flag mask) {
  Collision::hit_result result;
  geom::iterate(S{}, geom::iterate_check_ball(mask), parameters,
                geom::convert_local_transform{centre, radius}, [&](shape_flag f, const vec2& c) {
                  result.mask |= f;
                  result.shape_centres.emplace_back(c);
                });
  return result;
}

template <geom::ShapeNode S>
Collision::hit_result
shape_check_convex(const auto& parameters, std::span<const vec2> convex, shape_flag mask) {
  Collision::hit_result result;
  geom::iterate(S{}, geom::iterate_check_convex(mask), parameters,
                geom::convert_local_convex_transform{convex}, [&](shape_flag f, const vec2& c) {
                  result.mask |= f;
                  result.shape_centres.emplace_back(c);
                });
  return result;
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
Collision::hit_result ship_check_point(ecs::const_handle h, const vec2& v, shape_flag mask) {
  return shape_check_point<S>(get_shape_parameters<Logic>(h), v, mask);
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
Collision::hit_result
ship_check_line(ecs::const_handle h, const vec2& a, const vec2& b, shape_flag mask) {
  return shape_check_line<S>(get_shape_parameters<Logic>(h), a, b, mask);
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
Collision::hit_result
ship_check_ball(ecs::const_handle h, const vec2& centre, fixed radius, shape_flag mask) {
  return shape_check_ball<S>(get_shape_parameters<Logic>(h), centre, radius, mask);
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
Collision::hit_result
ship_check_convex(ecs::const_handle h, std::span<const vec2> convex, shape_flag mask) {
  return shape_check_convex<S>(get_shape_parameters<Logic>(h), convex, mask);
}

//////////////////////////////////////////////////////////////////////////////////
// Creation templates.
//////////////////////////////////////////////////////////////////////////////////
template <ecs::Component Logic>
ecs::handle create_ship(SimInterface& sim, const vec2& position, fixed rotation = 0) {
  auto h = sim.index().create();
  h.add(Update{.update = ecs::call<&Logic::update>});
  h.add(Transform{.centre = position, .rotation = rotation});
  return h;
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
ecs::handle add_collision(ecs::handle h, fixed bounding_width) {
  static constexpr auto collision_flags = get_shape_flags<Logic, S>();
  static_assert(+collision_flags);
  h.add(Collision{.flags = collision_flags,
                  .bounding_width = bounding_width,
                  .check_point = &ship_check_point<Logic, S>,
                  .check_line = &ship_check_line<Logic, S>,
                  .check_ball = &ship_check_ball<Logic, S>,
                  .check_convex = &ship_check_convex<Logic, S>});
  return h;
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
ecs::handle add_collision(ecs::handle h) {
  static_assert(requires { Logic::kBoundingWidth; });
  add_collision<Logic, S>(h, Logic::kBoundingWidth);
  return h;
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
ecs::handle add_render(ecs::handle h) {
  constexpr auto render_shape =
      sfn::cast<Render::render_t, ecs::call<&render_entity_shape<Logic, S>>>;

  sfn::ptr<Render::render_t> render_ptr = render_shape;
  sfn::ptr<Render::render_panel_t> render_panel_ptr = nullptr;
  if constexpr (requires { &Logic::render; }) {
    constexpr auto render_custom = sfn::cast<Render::render_t, ecs::call<&Logic::render>>;
    render_ptr = sfn::sequence<render_shape, render_custom>;
  }
  if constexpr (requires { &Logic::render_panel; }) {
    render_panel_ptr = sfn::cast<Render::render_panel_t, ecs::call<&Logic::render_panel>>;
  }
  h.add(Render{.render = render_ptr, .render_panel = render_panel_ptr});
  return h;
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
ecs::handle create_ship_default(SimInterface& sim, const vec2& position, fixed rotation = 0) {
  auto h = create_ship<Logic>(sim, position, rotation);
  if constexpr (+get_shape_flags<Logic, S>()) {
    add_collision<Logic, S>(h);
  }
  return add_render<Logic, S>(h);
}

}  // namespace ii::v0

#endif
