#ifndef II_GAME_LOGIC_BOSS_H
#define II_GAME_LOGIC_BOSS_H
#include "game/logic/ship.h"

class Boss : public Ship {
public:
  Boss(ii::SimInterface& sim, const vec2& position, ii::SimInterface::boss_list boss,
       std::int32_t hp, std::int32_t players, std::int32_t cycle = 0,
       bool explode_on_damage = true);

  void set_killed() {
    sim().set_boss_killed(flag_);
  }

  std::int64_t get_score() {
    return score_;
  }

  std::int32_t get_remaining_hp() const {
    return hp_ / 30;
  }

  std::int32_t get_max_hp() const {
    return max_hp_ / 30;
  }

  void restore_hp(std::int32_t hp) {
    hp_ += hp;
  }

  bool is_hp_low() const {
    return (max_hp_ * 1.2f) / hp_ >= 3;
  }

  void set_ignore_damage_colour_index(std::int32_t shape_index) {
    ignore_damage_colour_ = shape_index;
  }

  void render_hp_bar() const;

  void damage(std::int32_t damage, bool magic, Player* source) override;
  void render() const override;
  void render(bool hp_bar) const;
  virtual std::int32_t get_damage(std::int32_t damage, bool magic) = 0;
  virtual void on_destroy();

  static std::vector<std::pair<std::int32_t, std::pair<vec2, colour_t>>> fireworks_;
  static std::vector<vec2> warnings_;

protected:
  static std::int32_t CalculateHP(std::int32_t base, std::int32_t players, std::int32_t cycle);

private:
  std::int32_t hp_ = 0;
  std::int32_t max_hp_ = 0;
  ii::SimInterface::boss_list flag_ = static_cast<ii::SimInterface::boss_list>(0);
  std::int64_t score_ = 0;
  std::int32_t ignore_damage_colour_ = 256;
  mutable std::int32_t damaged_ = 0;
  mutable bool show_hp_ = false;
  bool explode_on_damage_ = false;
};

#endif
