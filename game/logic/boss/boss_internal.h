#ifndef II_GAME_LOGIC_BOSS_BOSS_INTERNAL_H
#define II_GAME_LOGIC_BOSS_BOSS_INTERNAL_H
#include "game/logic/boss/boss.h"
#include "game/logic/ship/ship.h"

class Boss : public ii::Ship {
public:
  Boss(ii::SimInterface& sim, const vec2& position);
  virtual void on_destroy(bool bomb);
  void render() const override;

  static std::vector<std::pair<std::uint32_t, std::pair<vec2, glm::vec4>>> fireworks_;
  static std::vector<vec2> warnings_;
};

namespace ii {
std::uint32_t calculate_boss_score(boss_flag boss, std::uint32_t players, std::uint32_t cycle);
std::uint32_t calculate_boss_hp(std::uint32_t base, std::uint32_t players, std::uint32_t cycle);
std::uint32_t
scale_boss_damage(ecs::handle h, SimInterface&, damage_type type, std::uint32_t damage);

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
