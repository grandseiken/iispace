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

void explode_shapes(EmitHandle& e, const geom::resolve_result& r,
                    const std::optional<cvec4>& colour_override = std::nullopt,
                    std::uint32_t time = 10, const std::optional<fvec2>& towards = std::nullopt,
                    std::optional<float> speed = std::nullopt);
void destruct_lines(EmitHandle& e, const geom::resolve_result& r, const fvec2& source,
                    std::uint32_t time = 20);
void explode_volumes(EmitHandle& e, const geom::resolve_result& r, const fvec2& source,
                     std::uint32_t time = 20);

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void destruct_entity_default(ecs::const_handle h, SimInterface&, EmitHandle& e, damage_type,
                             const vec2& source) {
  // TODO: something a bit cleverer here? Take velocity of shot into account?
  // Take velocity of destructed shape into account (maybe using same system as motion trails)?
  // Make destruct particles similarly velocified?
  // TODO: box explode should use a box FX shape.
  auto& r = resolve_entity_shape<Logic, S>(h);
  explode_shapes(e, r, std::nullopt, /* time */ 10, std::nullopt, 1.4f);
  destruct_lines(e, r, to_float(source), 20);
  explode_volumes(e, r, to_float(source), 20);
}

}  // namespace ii::v0

#endif