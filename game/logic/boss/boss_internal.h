#ifndef II_GAME_LOGIC_BOSS_BOSS_INTERNAL_H
#define II_GAME_LOGIC_BOSS_BOSS_INTERNAL_H
#include "game/logic/boss.h"
#include "game/logic/enemy.h"
#include "game/logic/ship/ship.h"

namespace ii {
std::uint32_t calculate_boss_score(boss_flag boss, std::uint32_t players, std::uint32_t cycle);
std::uint32_t calculate_boss_hp(std::uint32_t base, std::uint32_t players, std::uint32_t cycle);
std::uint32_t
scale_boss_damage(SimInterface&, ecs::handle h, damage_type type, std::uint32_t damage);

std::function<void(SimInterface&, ecs::handle, damage_type)>
make_legacy_boss_on_hit(bool explode_on_bomb_damage);
void legacy_boss_on_destroy(SimInterface&, ecs::handle h, damage_type type);
}  // namespace ii

class Boss : public Enemy {
public:
  Boss(ii::SimInterface& sim, const vec2& position);
  void on_destroy(bool bomb) override;

  static std::vector<std::pair<std::uint32_t, std::pair<vec2, glm::vec4>>> fireworks_;
  static std::vector<vec2> warnings_;
};

#endif
