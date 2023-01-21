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

void add_line_particle(EmitHandle& e, const fvec2& source, const fvec2& a, const fvec2& b,
                       const cvec4& c, float w, float z, std::uint32_t time);
void add_explode_particle(EmitHandle& e, const fvec2& source, const fvec2& position, float radius,
                          const cvec4& c, std::uint32_t time);

template <geom::ShapeNode S>
void explode_shapes(EmitHandle& e, const auto& parameters,
                    const std::optional<cvec4> colour_override = std::nullopt,
                    std::uint32_t time = 10, const std::optional<vec2>& towards = std::nullopt,
                    std::optional<float> speed = std::nullopt) {
  std::optional<fvec2> towards_float;
  if (towards) {
    towards_float = to_float(*towards);
  }
  geom::iterate(S{}, geom::iterate_volumes, parameters, geom::transform{},
                [&](const vec2& v, fixed, const cvec4& c0, const cvec4& c1) {
                  // TODO: maybe ignore outlines?
                  e.explosion(to_float(v), colour_override.value_or(c0.a ? c0 : c1), time,
                              towards_float, speed);
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

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void destruct_entity_lines(ecs::const_handle h, EmitHandle& e, const vec2& source,
                           std::uint32_t time = 20) {
  // TODO: something a bit cleverer here? Take velocity of shot into account?
  // Take velocity of destructed shape into account (maybe using same system as motion trails)?
  // Make destruct particles similarly velocified?
  auto source_f = to_float(source);
  geom::iterate(S{}, geom::iterate_lines, get_shape_parameters<Logic>(h), geom::transform{},
                [&](const vec2& a, const vec2& b, const cvec4& c, float w, float z) {
                  if (z > colour::kZOutline) {
                    add_line_particle(e, source_f, to_float(a), to_float(b), c, w, z, time);
                  }
                });
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void explode_entity_volumes(ecs::const_handle h, EmitHandle& e, const vec2& source,
                            std::uint32_t time = 20) {
  auto source_f = to_float(source);
  geom::iterate(S{}, geom::iterate_volumes, get_shape_parameters<Logic>(h), geom::transform{},
                [&](const vec2& v, fixed r, const cvec4&, const cvec4& c) {
                  if (c.a) {
                    add_explode_particle(e, source_f, to_float(v), 2.f * r.to_float(), c, time);
                  }
                });
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void destruct_entity_default(ecs::const_handle h, SimInterface&, EmitHandle& e, damage_type,
                             const vec2& source) {
  explode_entity_shapes<Logic, S>(h, e, std::nullopt, 10, std::nullopt, 1.4f);
  destruct_entity_lines<Logic, S>(h, e, source);
  explode_entity_volumes<Logic, S>(h, e, source);  // TODO: box explode should use a box FX shape.
}

}  // namespace ii::v0

#endif