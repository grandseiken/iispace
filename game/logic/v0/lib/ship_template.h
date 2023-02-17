#ifndef II_GAME_LOGIC_V0_LIB_SHIP_TEMPLATE_H
#define II_GAME_LOGIC_V0_LIB_SHIP_TEMPLATE_H
#include "game/common/colour.h"
#include "game/common/math.h"
#include "game/geom2/enums.h"
#include "game/geometry/node.h"
#include "game/geometry/node_transform.h"
#include "game/geometry/resolve.h"
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

inline geom::resolve_result& local_resolve() {
  static thread_local geom::resolve_result r;
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
// Rendering.
//////////////////////////////////////////////////////////////////////////////////
std::optional<cvec4> get_shape_colour(const geom::resolve_result& r);

void render_shape(std::vector<render::shape>& output, const geom::resolve_result& r,
                  const std::optional<float>& hit_alpha = std::nullopt,
                  const std::optional<float>& shield_alpha = std::nullopt);

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void render_entity_shape(ecs::const_handle h, const Health* health, const EnemyStatus* status,
                         std::vector<render::shape>& output) {
  std::optional<float> hit_alpha;
  std::optional<float> shield_alpha;
  if (status && status->shielded_ticks) {
    shield_alpha = std::min(1.f, status->shielded_ticks / 16.f);
  }
  if (health && health->hit_timer) {
    hit_alpha = std::min(1.f, health->hit_timer / 10.f);
  }
  auto& r = resolve_entity_shape<Logic, S>(h);
  render_shape(output, r, hit_alpha, shield_alpha);
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
geom::hit_result
check_shape_collision(const auto& parameters, const geom::iterate_check_collision_t& it) {
  geom::hit_result result;
  geom::iterate(S{}, it, parameters, geom::convert_local_transform{}, result);
  return result;
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
geom::hit_result
check_entity_collision(ecs::const_handle h, const geom::iterate_check_collision_t& it) {
  return check_shape_collision<S>(get_shape_parameters<Logic>(h), it);
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
                  .check_collision = &check_entity_collision<Logic, S>});
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
