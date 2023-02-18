#ifndef II_GAME_LOGIC_LEGACY_BOSS_BOSS_INTERNAL_H
#define II_GAME_LOGIC_LEGACY_BOSS_BOSS_INTERNAL_H
#include "game/logic/ecs/index.h"
#include "game/logic/legacy/components.h"
#include "game/logic/legacy/ship_template.h"
#include "game/logic/sim/sim_interface.h"

namespace ii::legacy {
std::uint32_t calculate_boss_score(boss_flag boss, std::uint32_t players, std::uint32_t cycle);
std::uint32_t calculate_boss_hp(std::uint32_t base, std::uint32_t players, std::uint32_t cycle);
std::uint32_t
scale_boss_damage(ecs::handle h, SimInterface&, damage_type type, std::uint32_t damage);

template <bool ExplodeOnBombDamage, ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void boss_on_hit(ecs::handle h, SimInterface& sim, EmitHandle& e, damage_type type, const vec2&) {
  if (type == damage_type::kBomb && ExplodeOnBombDamage) {
    explode_entity_shapes<Logic, S>(h, e);
    explode_entity_shapes<Logic, S>(h, e, cvec4{1.f}, 12);
    explode_entity_shapes<Logic, S>(h, e, std::nullopt, 24);
  }
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void boss_on_destroy(ecs::const_handle h, const Transform& transform, SimInterface& sim,
                     EmitHandle& e, damage_type, const vec2& source) {
  sim.index().iterate_dispatch_if<Enemy>([&](ecs::handle eh, Health& health) {
    if (eh.id() != h.id()) {
      health.damage(eh, sim, GlobalData::kBombDamage, damage_type::kBomb, h.id());
    }
  });

  auto& r = resolve_entity_shape<Logic, S>(h);
  std::optional<cvec4> boss_colour;
  for (const auto& e : r.entries) {
    if (const auto* d = std::get_if<geom::resolve_result::ball>(&e.data); d && d->line.colour0.a) {
      boss_colour = d->line.colour0;
      break;
    }
    if (const auto* d = std::get_if<geom::resolve_result::box>(&e.data); d && d->line.colour0.a) {
      boss_colour = d->line.colour0;
      break;
    }
    if (const auto* d = std::get_if<geom::resolve_result::ngon>(&e.data); d && d->line.colour0.a) {
      boss_colour = d->line.colour0;
      break;
    }
  }

  explode_entity_shapes<Logic, S>(h, e);
  explode_entity_shapes<Logic, S>(h, e, cvec4{1.f}, 12);
  explode_entity_shapes<Logic, S>(h, e, boss_colour, 24);
  explode_entity_shapes<Logic, S>(h, e, cvec4{1.f}, 36);
  explode_entity_shapes<Logic, S>(h, e, boss_colour, 48);
  destruct_entity_lines<Logic, S>(h, e, source, 128);
  std::uint32_t n = 1;
  auto& random = sim.random(random_source::kLegacyAesthetic);
  for (std::uint32_t i = 0; i < 16; ++i) {
    auto v = from_polar_legacy(random.fixed() * (2 * pi<fixed>),
                               fixed{8 + random.uint(64) + random.uint(64)});
    sim.global_entity().get<GlobalData>()->fireworks.push_back(GlobalData::fireworks_entry{
        .time = n, .position = transform.centre + v, .colour = boss_colour.value_or(cvec4{1.f})});
    n += i;
  }
  e.rumble_all(30, 1.f, 1.f).play(sound::kExplosion, transform.centre);
}

}  // namespace ii::legacy

#endif
