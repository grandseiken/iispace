#ifndef II_GAME_LOGIC_OVERMIND_OVERMIND_H
#define II_GAME_LOGIC_OVERMIND_OVERMIND_H
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace ii {
class Ship;
class SimInterface;
}  // namespace ii
class Stars;

class formation_base;
// TODO: make Overmind an entity/component.
class Overmind {
public:
  Overmind(ii::SimInterface& sim);
  ~Overmind();

  void update();
  void render() const;

  std::uint32_t get_killed_bosses() const;
  std::uint32_t get_elapsed_time() const;
  std::optional<std::uint32_t> get_timer() const;

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
  std::unique_ptr<Stars> stars_;

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
