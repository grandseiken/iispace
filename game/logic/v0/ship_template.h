#ifndef II_GAME_LOGIC_V0_SHIP_TEMPLATE_H
#define II_GAME_LOGIC_V0_SHIP_TEMPLATE_H
#include "game/common/colour.h"
#include "game/common/math.h"
#include "game/logic/ecs/call.h"
#include "game/logic/ecs/index.h"
#include "game/logic/geometry/enums.h"
#include "game/logic/geometry/node.h"
#include "game/logic/geometry/node_transform.h"
#include "game/logic/sim/io/render.h"
#include "game/logic/sim/sim_interface.h"
#include "game/logic/v0/components.h"
#include <sfn/functional.h>
#include <cstddef>
#include <optional>
#include <vector>

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
    output.emplace_back(shape_copy);
    if (shape.z_index <= colour::kZTrails && shield_alpha && override) {
      shape_copy = shape;
      if (shape_copy.apply_shield(*shield_alpha)) {
        output.emplace_back(shape_copy);
      }
    }
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
ecs::handle add_collision(ecs::handle h) {
  static constexpr auto collision_flags = get_shape_flags<Logic, S>();
  static_assert(+collision_flags);
  static_assert(requires { Logic::kBoundingWidth; });
  h.add(Collision{.flags = collision_flags,
                  .bounding_width = Logic::kBoundingWidth,
                  .check = &ship_check_point<Logic, S>});
  return h;
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
ecs::handle add_render(ecs::handle h) {
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
ecs::handle create_ship_default(SimInterface& sim, const vec2& position, fixed rotation = 0) {
  auto h = create_ship<Logic>(sim, position, rotation);
  if constexpr (+get_shape_flags<Logic, S>()) {
    add_collision<Logic, S>(h);
  }
  return add_render<Logic, S>(h);
}

}  // namespace ii::v0

#endif
