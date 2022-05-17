#ifndef IISPACE_GAME_LOGIC_BOSS_H
#define IISPACE_GAME_LOGIC_BOSS_H
#include "game/logic/ship.h"

class Boss : public Ship {
public:
  Boss(const vec2& position, SimState::boss_list boss, std::int32_t hp, std::int32_t players,
       std::int32_t cycle = 0, bool explode_on_damage = true);

  void set_killed() {
    game().set_boss_killed(_flag);
  }

  std::int64_t get_score() {
    return _score;
  }

  std::int32_t get_remaining_hp() const {
    return _hp / 30;
  }

  std::int32_t get_max_hp() const {
    return _max_hp / 30;
  }

  void restore_hp(std::int32_t hp) {
    _hp += hp;
  }

  bool is_hp_low() const {
    return (_max_hp * 1.2f) / _hp >= 3;
  }

  void set_ignore_damage_colour_index(std::int32_t shape_index) {
    _ignore_damage_colour = shape_index;
  }

  void render_hp_bar() const;

  void damage(std::int32_t damage, bool magic, Player* source) override;
  void render() const override;
  void render(bool hp_bar) const;
  virtual std::int32_t get_damage(std::int32_t damage, bool magic) = 0;
  virtual void on_destroy();

  static std::vector<std::pair<std::int32_t, std::pair<vec2, colour_t>>> _fireworks;
  static std::vector<vec2> _warnings;

protected:
  static std::int32_t CalculateHP(std::int32_t base, std::int32_t players, std::int32_t cycle);

private:
  std::int32_t _hp;
  std::int32_t _max_hp;
  SimState::boss_list _flag;
  std::int64_t _score;
  std::int32_t _ignore_damage_colour;
  mutable std::int32_t _damaged;
  mutable bool _show_hp;
  bool _explode_on_damage;
};

#endif
