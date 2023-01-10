#ifndef II_GAME_LOGIC_V0_BOSS_BOSS_TEMPLATE_H
#define II_GAME_LOGIC_V0_BOSS_BOSS_TEMPLATE_H
#include "game/common/ustring.h"
#include "game/geometry/concepts.h"
#include "game/logic/ecs/index.h"
#include "game/logic/v0/lib/components.h"
#include "game/logic/v0/lib/particles.h"
#include "game/mixer/sound.h"
#include <cstdint>
#include <optional>

namespace ii::v0 {
constexpr std::uint32_t kBossThreatValue = 1000;
constexpr std::uint32_t kBossHpMultiplier = 12u;

inline std::uint32_t
scale_boss_damage(ecs::handle, SimInterface& sim, damage_type type, std::uint32_t damage) {
  if (type == damage_type::kBomb) {
    damage *= std::max(1u, kBossHpMultiplier);
    switch (sim.alive_players()) {
    case 0:
    case 1:
      return 12 * damage / 16;
    case 2:
      return 9 * damage / 16;
    case 3:
      return 7 * damage / 16;
    case 4:
      return 6 * damage / 16;
    case 5:
      return 5 * damage / 16;
    default:
      return 4 * damage / 16;
    }
  }
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
void boss_on_hit(ecs::handle h, SimInterface&, EmitHandle& e, damage_type type,
                 const vec2& source) {
  if (type == damage_type::kBomb) {
    explode_entity_shapes<Logic, S>(h, e);
    explode_entity_shapes<Logic, S>(h, e, cvec4{1.f}, 12);
    explode_entity_shapes<Logic, S>(h, e, std::nullopt, 24);
    destruct_entity_lines<Logic, S>(h, e, source);
  }
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void boss_on_destroy(ecs::const_handle h, const Transform& transform, SimInterface& sim,
                     EmitHandle& e, damage_type, const vec2& source) {
  sim.index().iterate_dispatch_if<EnemyStatus>(
      [&](ecs::handle eh, EnemyStatus& status, const Transform* e_transform) {
        if (eh.id() != h.id()) {
          status.destroy_timer.emplace();
          status.destroy_timer->source = transform.centre;
          status.destroy_timer->timer = 4u;
          if (e_transform) {
            status.destroy_timer->timer +=
                (length(e_transform->centre - transform.centre) / 8).to_int();
          }
        }
      });

  auto boss_colour = get_boss_colour<Logic, S>(h);
  explode_entity_shapes<Logic, S>(h, e);
  explode_entity_shapes<Logic, S>(h, e, cvec4{1.f}, 12);
  explode_entity_shapes<Logic, S>(h, e, boss_colour, 24);
  explode_entity_shapes<Logic, S>(h, e, cvec4{1.f}, 36);
  explode_entity_shapes<Logic, S>(h, e, boss_colour, 48);
  destruct_entity_lines<Logic, S>(h, e, source, 128);

  std::uint32_t n = 1;
  for (std::uint32_t i = 0; i < 16; ++i) {
    auto e =
        sim.emit(resolve_key::reconcile(h.id(), resolve_tag::kOnDestroy, i + 1)).set_delay_ticks(n);
    auto v = transform.centre +
        from_polar(e.random().fixed() * (2 * pi<fixed>),
                   fixed{8 + e.random().uint(64) + e.random().uint(64)});
    auto explode = [&](const fvec2& v, const cvec4& c, std::uint32_t time) {
      auto c_dark = c;
      c_dark.a = .5f;
      e.explosion(v, c, time)
          .explosion(v + fvec2{8.f, 0.f}, c, time)
          .explosion(v + fvec2{0.f, 8.f}, c, time)
          .explosion(v + fvec2{8.f, 0.f}, c_dark, time)
          .explosion(v + fvec2{0.f, 8.f}, c_dark, time);
    };
    explode(to_float(v), cvec4{1.f}, 16);
    explode(to_float(v), boss_colour, 32);
    explode(to_float(v), cvec4{1.f}, 64);
    explode(to_float(v), boss_colour, 128);
    e.rumble_all(15, .25f, .5f);
    n += i;
  }

  e.rumble_all(30, 1.f, 1.f).play(sound::kExplosion, transform.centre);
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void add_boss_data(ecs::handle h, ustring_view name, std::uint32_t base_hp) {
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
  h.add(EnemyStatus{.stun_resist_base = 100u, .stun_resist_bonus = 60u});
  h.add(Enemy{.threat_value = kBossThreatValue});
  h.add(Boss{.name = ustring{name}});
  h.add(PostUpdate{.post_update = [](ecs::handle h, SimInterface&) {
    h.get<Boss>()->colour = get_boss_colour<Logic, S>(h);
  }});
}

}  // namespace ii::v0

#endif
