#ifndef II_GAME_LOGIC_BOSS_BOSS_INTERNAL_H
#define II_GAME_LOGIC_BOSS_BOSS_INTERNAL_H
#include "game/logic/boss.h"
#include "game/logic/ship/ship.h"

std::uint32_t
calculate_boss_score(ii::SimInterface::boss_list boss, std::uint32_t players, std::uint32_t cycle);

class Boss : public ii::Ship {
public:
  Boss(ii::SimInterface& sim, const vec2& position, ii::SimInterface::boss_list boss,
       std::uint32_t hp, std::uint32_t players, std::uint32_t cycle = 0,
       bool explode_on_damage = true);

  void set_killed() {
    sim().set_boss_killed(flag_);
  }

  std::uint32_t get_remaining_hp() const {
    return hp_ / 30;
  }

  std::uint32_t get_max_hp() const {
    return max_hp_ / 30;
  }

  void restore_hp(std::uint32_t hp) {
    hp_ += hp;
  }

  bool is_hp_low() const {
    return 3 * hp_ <= max_hp_ + max_hp_ / 5;
  }

  void set_ignore_damage_colour_index(std::uint32_t shape_index) {
    ignore_damage_colour_ = shape_index;
  }

  void render_hp_bar() const;

  void damage(std::uint32_t damage, bool magic, Player* source) override;
  void render() const override;
  void render(bool hp_bar) const;
  virtual std::uint32_t get_damage(std::uint32_t damage, bool magic) = 0;
  virtual void on_destroy();

  static std::vector<std::pair<std::uint32_t, std::pair<vec2, glm::vec4>>> fireworks_;
  static std::vector<vec2> warnings_;

protected:
  static std::uint32_t CalculateHP(std::uint32_t base, std::uint32_t players, std::uint32_t cycle);

private:
  std::uint32_t hp_ = 0;
  std::uint32_t max_hp_ = 0;
  ii::SimInterface::boss_list flag_ = static_cast<ii::SimInterface::boss_list>(0);
  std::uint32_t ignore_damage_colour_ = 256;
  mutable std::uint32_t damaged_ = 0;
  mutable bool show_hp_ = false;
  bool explode_on_damage_ = false;
};

#endif
