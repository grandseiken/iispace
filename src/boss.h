#ifndef IISPACE_SRC_BOSS_H
#define IISPACE_SRC_BOSS_H

#include "ship.h"

class Boss : public Ship {
public:
  Boss(const vec2& position, GameModal::boss_list boss, int32_t hp, int32_t players,
       int32_t cycle = 0, bool explode_on_damage = true);

  void set_killed() {
    game().set_boss_killed(_flag);
  }

  int64_t get_score() {
    return _score;
  }

  int32_t get_remaining_hp() const {
    return _hp / 30;
  }

  int32_t get_max_hp() const {
    return _max_hp / 30;
  }

  void restore_hp(int32_t hp) {
    _hp += hp;
  }

  bool is_hp_low() const {
    return (_max_hp * 1.2f) / _hp >= 3;
  }

  void set_ignore_damage_colour_index(int32_t shape_index) {
    _ignore_damage_colour = shape_index;
  }

  void render_hp_bar() const;

  void damage(int32_t damage, bool magic, Player* source) override;
  void render() const override;
  void render(bool hp_bar) const;
  virtual int32_t get_damage(int32_t damage, bool magic) = 0;
  virtual void on_destroy();

  static std::vector<std::pair<int32_t, std::pair<vec2, colour_t>>> _fireworks;
  static std::vector<vec2> _warnings;

protected:
  static int32_t CalculateHP(int32_t base, int32_t players, int32_t cycle);

private:
  int32_t _hp;
  int32_t _max_hp;
  GameModal::boss_list _flag;
  int64_t _score;
  int32_t _ignore_damage_colour;
  mutable int32_t _damaged;
  mutable bool _show_hp;
  bool _explode_on_damage;
};

#endif
