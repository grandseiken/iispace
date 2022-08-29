#ifndef II_GAME_LOGIC_BOSS_BOSS_INTERNAL_H
#define II_GAME_LOGIC_BOSS_BOSS_INTERNAL_H
#include "game/logic/ecs/index.h"
#include "game/logic/ship/ship_template.h"
#include "game/logic/sim/sim_interface.h"

namespace ii {
std::uint32_t calculate_boss_score(boss_flag boss, std::uint32_t players, std::uint32_t cycle);
std::uint32_t calculate_boss_hp(std::uint32_t base, std::uint32_t players, std::uint32_t cycle);
std::uint32_t
scale_boss_damage(ecs::handle h, SimInterface&, damage_type type, std::uint32_t damage);

template <bool ExplodeOnBombDamage, ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void boss_on_hit(ecs::handle h, SimInterface& sim, EmitHandle& e, damage_type type, const vec2&) {
  if (type == damage_type::kBomb && ExplodeOnBombDamage) {
    explode_entity_shapes<Logic, S>(h, e);
    explode_entity_shapes<Logic, S>(h, e, glm::vec4{1.f}, 12);
    explode_entity_shapes<Logic, S>(h, e, std::nullopt, 24);
  }
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void boss_on_destroy(ecs::const_handle h, const Transform& transform, SimInterface& sim,
                     EmitHandle& e, damage_type, const vec2& source) {
  sim.index().iterate_dispatch_if<Enemy>([&](ecs::handle eh, Health& health) {
    if (eh.id() != h.id()) {
      health.damage(eh, sim, Player::kBombDamage, damage_type::kBomb, h.id());
    }
  });
  std::optional<glm::vec4> boss_colour;
  geom::iterate(S{}, geom::iterate_centres, get_shape_parameters<Logic>(h), geom::transform{},
                [&](const vec2&, const glm::vec4& c) {
                  if (!boss_colour) {
                    boss_colour = c;
                  }
                });

  explode_entity_shapes<Logic, S>(h, e);
  explode_entity_shapes<Logic, S>(h, e, glm::vec4{1.f}, 12);
  explode_entity_shapes<Logic, S>(h, e, boss_colour, 24);
  explode_entity_shapes<Logic, S>(h, e, glm::vec4{1.f}, 36);
  explode_entity_shapes<Logic, S>(h, e, boss_colour, 48);
  destruct_entity_lines<Logic, S>(h, e, source, 128);
  std::uint32_t n = 1;
  auto& random = sim.random(random_source::kLegacyAesthetic);
  for (std::uint32_t i = 0; i < 16; ++i) {
    auto v = from_polar(random.fixed() * (2 * fixed_c::pi),
                        fixed{8 + random.uint(64) + random.uint(64)});
    sim.global_entity().get<GlobalData>()->fireworks.push_back(
        GlobalData::fireworks_entry{.time = n,
                                    .position = transform.centre + v,
                                    .colour = boss_colour.value_or(glm::vec4{1.f})});
    n += i;
  }
  e.rumble_all(30, 1.f, 1.f).play(sound::kExplosion, transform.centre);
}

}  // namespace ii

#endif
