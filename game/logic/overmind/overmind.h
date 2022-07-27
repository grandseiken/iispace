#ifndef II_GAME_LOGIC_OVERMIND_OVERMIND_H
#define II_GAME_LOGIC_OVERMIND_OVERMIND_H
#include "game/common/functional.h"
#include <cstdint>
#include <optional>
#include <vector>

namespace ii {
class SimInterface;
class SpawnContext;

// TODO: make Overmind an entity/component.
class Overmind {
public:
  Overmind(SimInterface& sim);
  ~Overmind();

  void update();

private:
  struct entry {
    std::uint32_t id = 0;
    std::uint32_t cost = 0;
    std::uint32_t min_resource = 0;
    function_ptr<void(const SpawnContext&)> function = nullptr;
  };

  void spawn_powerup();
  void spawn_boss_reward();

  void wave();
  void boss();
  void boss_mode_boss();

  SimInterface& sim_;

  std::uint32_t power_ = 0;
  std::uint32_t timer_ = 0;
  std::uint32_t levels_mod_ = 0;
  std::uint32_t groups_mod_ = 0;
  std::uint32_t boss_mod_bosses_ = 0;
  std::uint32_t boss_mod_fights_ = 0;
  std::uint32_t boss_mod_secret_ = 0;
  std::uint32_t powerup_mod_ = 0;
  std::uint32_t lives_spawned_ = 0;
  std::uint32_t lives_target_ = 0;
  bool is_boss_next_ = false;
  bool is_boss_level_ = false;
  std::uint32_t boss_rest_timer_ = 0;
  std::uint32_t waves_total_ = 0;
  std::uint32_t hard_already_ = 0;

  std::vector<std::uint32_t> boss1_queue_;
  std::vector<std::uint32_t> boss2_queue_;
  std::uint32_t bosses_to_go_ = 0;
  std::vector<entry> formations_;

  void add_formations();
};

}  // namespace ii

#endif
