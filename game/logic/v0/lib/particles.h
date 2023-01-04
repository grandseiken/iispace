#ifndef II_GAME_LOGIC_V0_LIB_PARTICLES_H
#define II_GAME_LOGIC_V0_LIB_PARTICLES_H
#include "game/common/math.h"
#include "game/geometry/node.h"
#include "game/logic/ecs/call.h"
#include "game/logic/ecs/id.h"
#include "game/logic/sim/components.h"
#include "game/logic/v0/lib/ship_template.h"
#include <sfn/functional.h>
#include <optional>

namespace ii::v0 {

//////////////////////////////////////////////////////////////////////////////////
// Particles.
//////////////////////////////////////////////////////////////////////////////////
template <geom::ShapeNode S>
void explode_shapes(EmitHandle& e, const auto& parameters,
                    const std::optional<cvec4> colour_override = std::nullopt,
                    std::uint32_t time = 10, const std::optional<vec2>& towards = std::nullopt,
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
                           std::uint32_t time = 10,
                           const std::optional<vec2>& towards = std::nullopt,
                           std::optional<float> speed = std::nullopt) {
  explode_shapes<S>(e, get_shape_parameters<Logic>(h), colour_override, time, towards, speed);
}

inline void add_line_particle(EmitHandle& e, const fvec2& source, const fvec2& a, const fvec2& b,
                              const cvec4& c, float w, float z, std::uint32_t time) {
  auto& r = e.random();
  auto position = (a + b) / 2.f;
  auto velocity = (1.75f + .6f * r.fixed().to_float()) * normalise(position - source) +
      from_polar((2 * pi<fixed> * r.fixed()).to_float(), (r.fixed() / 10).to_float());
  auto diameter = distance(a, b);
  auto d = distance(source, a) - distance(source, b);
  if (angle_diff(angle(a - source), angle(b - source)) > 0) {
    d = -d;
  }
  auto angular_velocity = (d / (30.f * diameter)) + r.fixed().to_float() / 60.f;
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
                    std::uint32_t time = 24) {
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
  explode_entity_shapes<Logic, S>(h, e, std::nullopt, 10, std::nullopt, 1.4f);
  destruct_entity_lines<Logic, S>(h, e, source);
}

}  // namespace ii::v0

#endif