#ifndef IISPACE_GAME_LOGIC_OVERMIND_H
#define IISPACE_GAME_LOGIC_OVERMIND_H
#include "game/common/z.h"
#include <vector>

class Ship;
class SimState;

class formation_base;
class Overmind {
public:
  static const int32_t TIMER = 2800;
  static const int32_t POWERUP_TIME = 1200;
  static const int32_t BOSS_REST_TIME = 240;

  static const int32_t INITIAL_POWER = 16;
  static const int32_t INITIAL_TRIGGERVAL = 0;
  static const int32_t LEVELS_PER_GROUP = 4;
  static const int32_t BASE_GROUPS_PER_BOSS = 4;

  Overmind(SimState& game, bool can_face_secret_boss);
  ~Overmind();

  // General
  //------------------------------
  void update();

  int32_t get_killed_bosses() const {
    return _boss_mod_bosses - 1;
  }

  int32_t get_elapsed_time() const {
    return _elapsed_time;
  }

  int32_t get_timer() const {
    return _is_boss_level ? -1 : TIMER - _timer;
  }

  // Enemy-counting
  //------------------------------
  void on_enemy_destroy(const Ship& ship);
  void on_enemy_create(const Ship& ship);

  int32_t count_non_wall_enemies() const {
    return _non_wall_count;
  }

  struct entry {
    int32_t id;
    int32_t cost;
    int32_t min_resource;
    formation_base* function;
  };

private:
  void spawn(Ship* ship);
  void spawn_powerup();
  void spawn_boss_reward();

  void wave();
  void boss();
  void boss_mode_boss();

  SimState& _game;
  int32_t _power;
  int32_t _timer;
  int32_t _count;
  int32_t _non_wall_count;
  int32_t _levels_mod;
  int32_t _groups_mod;
  int32_t _boss_mod_bosses;
  int32_t _boss_mod_fights;
  int32_t _boss_mod_secret;
  bool _can_face_secret_boss;
  int32_t _powerup_mod;
  int32_t _lives_target;
  bool _is_boss_next;
  bool _is_boss_level;
  int32_t _elapsed_time;
  int32_t _boss_rest_timer;
  int32_t _waves_total;
  int32_t _hard_already;

  std::vector<int32_t> _boss1_queue;
  std::vector<int32_t> _boss2_queue;
  int32_t _bosses_to_go;
  std::vector<entry> _formations;

  template <typename F>
  void add_formation();
  void add_formations();
};

#endif
