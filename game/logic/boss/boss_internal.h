#ifndef II_GAME_LOGIC_BOSS_BOSS_INTERNAL_H
#define II_GAME_LOGIC_BOSS_BOSS_INTERNAL_H
#include "game/logic/ecs/index.h"
#include "game/logic/ship/geometry.h"
#include "game/logic/ship/ship.h"
#include "game/logic/ship/ship_template.h"
#include "game/logic/sim/sim_interface.h"

class Boss : public ii::Ship {
public:
  Boss(ii::SimInterface& sim, const vec2& position);
  virtual void on_destroy(bool bomb);
  void render() const override;
};

namespace ii {
std::uint32_t calculate_boss_score(boss_flag boss, std::uint32_t players, std::uint32_t cycle);
std::uint32_t calculate_boss_hp(std::uint32_t base, std::uint32_t players, std::uint32_t cycle);
std::uint32_t
scale_boss_damage(ecs::handle h, SimInterface&, damage_type type, std::uint32_t damage);

template <bool ExplodeOnBombDamage, ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void boss_on_hit(ecs::handle h, SimInterface& sim, damage_type type) {
  if (type == damage_type::kBomb && ExplodeOnBombDamage) {
    explode_entity_shapes<Logic, S>(h, sim);
    explode_entity_shapes<Logic, S>(h, sim, glm::vec4{1.f}, 12);
    explode_entity_shapes<Logic, S>(h, sim, std::nullopt, 24);
  }
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
void boss_on_destroy(ecs::const_handle h, const Transform& transform, SimInterface& sim,
                     damage_type) {
  sim.index().iterate_dispatch_if<Enemy>([&](ecs::handle eh, Health& health) {
    if (eh.id() != h.id()) {
      health.damage(eh, sim, Player::kBombDamage, damage_type::kBomb, std::nullopt);
    }
  });
  std::optional<glm::vec4> boss_colour;
  geom::iterate(S{}, geom::iterate_centres, get_shape_parameters<Logic>(h), {},
                [&](const vec2&, const glm::vec4& c) {
                  if (!boss_colour) {
                    boss_colour = c;
                  }
                });

  explode_entity_shapes<Logic, S>(h, sim);
  explode_entity_shapes<Logic, S>(h, sim, glm::vec4{1.f}, 12);
  explode_entity_shapes<Logic, S>(h, sim, boss_colour, 24);
  explode_entity_shapes<Logic, S>(h, sim, glm::vec4{1.f}, 36);
  explode_entity_shapes<Logic, S>(h, sim, boss_colour, 48);
  std::uint32_t n = 1;
  for (std::uint32_t i = 0; i < 16; ++i) {
    auto v = from_polar(sim.random_fixed() * (2 * fixed_c::pi),
                        fixed{8 + sim.random(64) + sim.random(64)});
    sim.global_entity().get<GlobalData>()->fireworks.push_back(
        GlobalData::fireworks_entry{.time = n,
                                    .position = transform.centre + v,
                                    .colour = boss_colour.value_or(glm::vec4{1.f})});
    n += i;
  }
  sim.rumble_all(25);
  sim.play_sound(sound::kExplosion, transform.centre);
}

template <bool ExplodeOnBombDamage>
void legacy_boss_on_hit(ecs::handle h, SimInterface&, damage_type type) {
  auto* boss = static_cast<::Boss*>(h.get<LegacyShip>()->ship.get());
  if (type == damage_type::kBomb && ExplodeOnBombDamage) {
    boss->explosion();
    boss->explosion(glm::vec4{1.f}, 16);
    boss->explosion(std::nullopt, 24);
  }
}

inline void legacy_boss_on_destroy(ecs::const_handle h, SimInterface&, damage_type type) {
  auto* boss = static_cast<::Boss*>(h.get<LegacyShip>()->ship.get());
  boss->on_destroy(type == damage_type::kBomb);
}
}  // namespace ii

#endif
