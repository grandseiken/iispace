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
    return boss_mod_bosses_ - 1;
  }

  std::int32_t get_elapsed_time() const {
    return elapsed_time_;
  }

  std::int32_t get_timer() const {
    return is_boss_level_ ? -1 : kTimer - timer_;
  }

  // Enemy-counting
  //------------------------------
  void on_enemy_destroy(const Ship& ship);
  void on_enemy_create(const Ship& ship);

  std::int32_t count_non_wall_enemies() const {
    return non_wall_count_;
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

  SimState& game_;
  std::int32_t power_;
  std::int32_t timer_;
  std::int32_t count_;
  std::int32_t non_wall_count_;
  std::int32_t levels_mod_;
  std::int32_t groups_mod_;
  std::int32_t boss_mod_bosses_;
  std::int32_t boss_mod_fights_;
  std::int32_t boss_mod_secret_;
  bool can_face_secret_boss_;
  std::int32_t powerup_mod_;
  std::int32_t lives_target_;
  bool is_boss_next_;
  bool is_boss_level_;
  std::int32_t elapsed_time_;
  std::int32_t boss_rest_timer_;
  std::int32_t waves_total_;
  std::int32_t hard_already_;

  std::vector<std::int32_t> boss1_queue_;
  std::vector<std::int32_t> boss2_queue_;
  std::int32_t bosses_to_go_;
  std::vector<entry> formations_;

  template <typename F>
  void add_formation();
  void add_formations();
};

#endif
