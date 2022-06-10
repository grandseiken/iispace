#ifndef II_GAME_LOGIC_BOSS_BOSS_INTERNAL_H
#define II_GAME_LOGIC_BOSS_BOSS_INTERNAL_H
#include "game/logic/boss.h"
#include "game/logic/ship/ship.h"

namespace ii {
std::uint32_t
calculate_boss_score(SimInterface::boss_list boss, std::uint32_t players, std::uint32_t cycle);
std::uint32_t calculate_boss_hp(std::uint32_t base, std::uint32_t players, std::uint32_t cycle);
std::uint32_t
scale_boss_damage(SimInterface&, ecs::handle h, damage_type type, std::uint32_t damage);

std::function<void(damage_type)>
make_legacy_boss_on_hit(ecs::handle h, bool explode_on_bomb_damage);
std::function<void(damage_type)> make_legacy_boss_on_destroy(ecs::handle h);
}  // namespace ii

class Boss : public ii::Ship {
public:
  Boss(ii::SimInterface& sim, const vec2& position, ii::SimInterface::boss_list boss);

  void set_killed() {
    sim().set_boss_killed(flag_);
  }

  void restore_hp(std::uint32_t hp) {
    if (auto* c = handle().get<ii::Health>(); c) {
      c->hp += hp;
    }
  }

  void set_ignore_damage_colour_index(std::uint32_t shape_index) {
    ignore_damage_colour_ = shape_index;
  }

  void render_hp_bar() const;
  void render() const override;
  void render(bool hp_bar) const;
  virtual void on_destroy();

  static std::vector<std::pair<std::uint32_t, std::pair<vec2, glm::vec4>>> fireworks_;
  static std::vector<vec2> warnings_;

private:
  ii::SimInterface::boss_list flag_ = static_cast<ii::SimInterface::boss_list>(0);
  std::uint32_t ignore_damage_colour_ = 256;
  mutable bool show_hp_ = false;
};

#endif
