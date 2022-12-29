#ifndef II_GAME_LOGIC_V0_BOSS_BOSS_TEMPLATE_H
#define II_GAME_LOGIC_V0_BOSS_BOSS_TEMPLATE_H
#include "game/geometry/concepts.h"
#include "game/logic/ecs/index.h"
#include "game/logic/v0/components.h"
#include "game/logic/v0/particles.h"
#include "game/mixer/sound.h"
#include <cstdint>
#include <optional>

namespace ii::v0 {
constexpr std::uint32_t kBossThreatValue = 1000;
constexpr std::uint32_t kBossHpMultiplier = 12u;

inline std::uint32_t
scale_boss_damage(ecs::handle, SimInterface& sim, damage_type, std::uint32_t damage) {
  return damage * std::max(1u, kBossHpMultiplier / std::max(1u, sim.alive_players()));
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::Shape>
inline cvec4 get_boss_colour(ecs::const_handle h) {
  std::optional<cvec4> result;
  geom::iterate(S{}, geom::iterate_centres, get_shape_parameters<Logic>(h), geom::transform{},
                [&](const vec2&, const cvec4& c) {
                  if (!result) {
                    result = c;
                  }
                });
  return result.value_or(colour::kWhite0);
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void boss_on_hit(ecs::handle h, SimInterface&, EmitHandle& e, damage_type type, const vec2&) {
  if (type == damage_type::kBomb) {
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
      health.damage(eh, sim, health.max_hp, damage_type::kBomb, h.id());
    }
  });

  auto boss_colour = get_boss_colour<Logic, S>(h);
  explode_entity_shapes<Logic, S>(h, e);
  explode_entity_shapes<Logic, S>(h, e, cvec4{1.f}, 12);
  explode_entity_shapes<Logic, S>(h, e, boss_colour, 24);
  explode_entity_shapes<Logic, S>(h, e, cvec4{1.f}, 36);
  explode_entity_shapes<Logic, S>(h, e, boss_colour, 48);
  destruct_entity_lines<Logic, S>(h, e, source, 128);
  /*std::uint32_t n = 1;
  auto& random = sim.random(random_source::kLegacyAesthetic);
  for (std::uint32_t i = 0; i < 16; ++i) {
    auto v =
        from_polar(random.fixed() * (2 * pi<fixed>), fixed{8 + random.uint(64) + random.uint(64)});
    sim.global_entity().get<GlobalData>()->fireworks.push_back(GlobalData::fireworks_entry{
        .time = n, .position = transform.centre + v, .colour = boss_colour.value_or(cvec4{1.f})});
    n += i;
  }*/
  e.rumble_all(30, 1.f, 1.f).play(sound::kExplosion, transform.centre);
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void add_boss_data(ecs::handle h, std::uint32_t base_hp) {
  h.add(Health{
      .hp = kBossHpMultiplier * base_hp,
      .hit_sound0 = std::nullopt,
      .hit_sound1 = sound::kEnemyShatter,
      .destroy_sound = std::nullopt,
      .destroy_rumble = std::nullopt,
      .damage_transform = &scale_boss_damage,
      .on_hit = &boss_on_hit<Logic, S>,
      .on_destroy = ecs::call<&boss_on_destroy<Logic, S>>,
  });
  h.add(EnemyStatus{});
  h.add(Enemy{.threat_value = kBossThreatValue});
  h.add(Boss{});
}

}  // namespace ii::v0

#endif
