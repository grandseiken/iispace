#ifndef IISPACE_GAME_LOGIC_OVERMIND_H
#define IISPACE_GAME_LOGIC_OVERMIND_H
#include "game/common/z.h"
#include <vector>

class Ship;
class SimState;

class formation_base;
class Overmind {
public:
  static constexpr std::int32_t kTimer = 2800;
  static constexpr std::int32_t kPowerupTime = 1200;
  static constexpr std::int32_t kBossRestTime = 240;

  static constexpr std::int32_t kInitialPower = 16;
  static constexpr std::int32_t kInitialTriggerVal = 0;
  static constexpr std::int32_t kLevelsPerGroup = 4;
  static constexpr std::int32_t kBaseGroupsPerBoss = 4;

  Overmind(SimState& game, bool can_face_secret_boss);
  ~Overmind();

  // General
  //------------------------------
  void update();

  std::int32_t get_killed_bosses() const {
    return _boss_mod_bosses - 1;
  }

  std::int32_t get_elapsed_time() const {
    return _elapsed_time;
  }

  std::int32_t get_timer() const {
    return _is_boss_level ? -1 : kTimer - _timer;
  }

  // Enemy-counting
  //------------------------------
  void on_enemy_destroy(const Ship& ship);
  void on_enemy_create(const Ship& ship);

  std::int32_t count_non_wall_enemies() const {
    return _non_wall_count;
  }

  struct entry {
    std::int32_t id;
    std::int32_t cost;
    std::int32_t min_resource;
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
  std::int32_t _power;
  std::int32_t _timer;
  std::int32_t _count;
  std::int32_t _non_wall_count;
  std::int32_t _levels_mod;
  std::int32_t _groups_mod;
  std::int32_t _boss_mod_bosses;
  std::int32_t _boss_mod_fights;
  std::int32_t _boss_mod_secret;
  bool _can_face_secret_boss;
  std::int32_t _powerup_mod;
  std::int32_t _lives_target;
  bool _is_boss_next;
  bool _is_boss_level;
  std::int32_t _elapsed_time;
  std::int32_t _boss_rest_timer;
  std::int32_t _waves_total;
  std::int32_t _hard_already;

  std::vector<std::int32_t> _boss1_queue;
  std::vector<std::int32_t> _boss2_queue;
  std::int32_t _bosses_to_go;
  std::vector<entry> _formations;

  template <typename F>
  void add_formation();
  void add_formations();
};

#endif
