#ifndef II_GAME_LOGIC_OVERMIND_H
#define II_GAME_LOGIC_OVERMIND_H
#include <cstdint>
#include <optional>
#include <vector>

class Ship;
namespace ii {
class SimInterface;
}

class formation_base;
class Overmind {
public:
  Overmind(ii::SimInterface& sim, bool can_face_secret_boss);
  ~Overmind();

  // General
  //------------------------------
  void update();

  std::uint32_t get_killed_bosses() const;
  std::uint32_t get_elapsed_time() const;
  std::optional<std::uint32_t> get_timer() const;

  // Enemy-counting
  //------------------------------
  // TODO: get rid of this.
  void on_enemy_destroy(const Ship& ship);
  void on_enemy_create(const Ship& ship);

  std::uint32_t count_non_wall_enemies() const {
    return non_wall_count_;
  }

  struct entry {
    std::uint32_t id;
    std::uint32_t cost;
    std::uint32_t min_resource;
    formation_base* function;
  };

private:
  void spawn_powerup();
  void spawn_boss_reward();

  void wave();
  void boss();
  void boss_mode_boss();

  ii::SimInterface& sim_;
  std::uint32_t power_ = 0;
  std::uint32_t timer_ = 0;
  std::uint32_t count_ = 0;
  std::uint32_t non_wall_count_ = 0;
  std::uint32_t levels_mod_ = 0;
  std::uint32_t groups_mod_ = 0;
  std::uint32_t boss_mod_bosses_ = 0;
  std::uint32_t boss_mod_fights_ = 0;
  std::uint32_t boss_mod_secret_ = 0;
  bool can_face_secret_boss_ = false;
  std::uint32_t powerup_mod_ = 0;
  std::uint32_t lives_spawned_ = 0;
  std::uint32_t lives_target_ = 0;
  bool is_boss_next_ = false;
  bool is_boss_level_ = false;
  std::uint32_t elapsed_time_ = 0;
  std::uint32_t boss_rest_timer_ = 0;
  std::uint32_t waves_total_ = 0;
  std::uint32_t hard_already_ = 0;

  std::vector<std::uint32_t> boss1_queue_;
  std::vector<std::uint32_t> boss2_queue_;
  std::uint32_t bosses_to_go_ = 0;
  std::vector<entry> formations_;

  void add_formations();
};

#endif
