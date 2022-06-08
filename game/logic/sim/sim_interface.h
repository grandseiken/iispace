#ifndef II_GAME_LOGIC_SIM_SIM_INTERFACE_H
#define II_GAME_LOGIC_SIM_SIM_INTERFACE_H
#include "game/common/math.h"
#include "game/logic/ship/ecs_index.h"
#include "game/logic/sim/sim_io.h"
#include "game/mixer/sound.h"
#include <cstdint>
#include <memory>
#include <vector>

class Player;

namespace ii {
namespace ecs {
class EntityIndex;
}  // namespace ecs
enum class shape_flag : std::uint32_t;
enum class ship_flag : std::uint32_t;
class Ship;
class SimInternals;

constexpr glm::ivec2 kSimDimensions = {640, 480};

struct particle {
  particle(const glm::vec2& position, const glm::vec4& colour, const glm::vec2& velocity,
           std::uint32_t time)
  : position{position}, velocity{velocity}, timer{time}, colour{colour} {}

  bool destroy = false;
  std::uint32_t timer = 0;
  glm::vec4 colour{0.f};
  glm::vec2 position{0.f};
  glm::vec2 velocity{0.f};
};

class SimInterface {
public:
  // TODO: move to sim_io.h?
  static glm::vec4 player_colour(std::size_t player_number);
  enum boss_list {
    kBoss1A = 1,
    kBoss1B = 2,
    kBoss1C = 4,
    kBoss2A = 8,
    kBoss2B = 16,
    kBoss2C = 32,
    kBoss3A = 64,
  };
  using ship_list = std::vector<Ship*>;

  SimInterface(SimInternals* internals) : internals_{internals} {}

  // Input.
  input_frame input(std::uint32_t player_number);

  // State manipulation.
  const initial_conditions& conditions() const;
  ecs::EntityIndex& index();
  const ecs::EntityIndex& index() const;

  std::uint32_t random(std::uint32_t max);
  fixed random_fixed();

  // TODO: const/non-const versions of retrieval functions?
  std::size_t
  count_ships(ship_flag mask = ship_flag{0}, ship_flag exclude_mask = ship_flag{0}) const;
  ship_list all_ships(ship_flag mask = ship_flag{0}) const;
  ship_list ships_in_radius(const vec2& point, fixed radius, ship_flag mask = ship_flag{0}) const;
  ship_list collision_list(const vec2& point, shape_flag category) const;
  bool any_collision(const vec2& point, shape_flag category) const;
  std::uint32_t get_non_wall_count() const;

  std::uint32_t player_count() const;
  std::uint32_t alive_players() const;
  std::uint32_t killed_players() const;
  ::Player* nearest_player(const vec2& point) const;
  ship_list players() const;

  void add_life();
  void sub_life();
  std::uint32_t get_lives() const;
  void set_boss_killed(boss_list boss);

  ecs::handle create_legacy(std::unique_ptr<Ship> ship);
  void add_particle(const particle& particle);

  // Simulation output.
  void rumble_all(std::uint32_t time) const;
  void rumble(std::uint32_t player, std::uint32_t time) const;
  void play_sound(sound, float volume = 1.f, float pan = 0.f, float repitch = 0.f) const;
  void render_hp_bar(float fill) const;
  void render_line(const glm::vec2& a, const glm::vec2& b, const glm::vec4& c) const;
  void render_line_rect(const glm::vec2& lo, const glm::vec2& hi, const glm::vec4& c) const;
  void render_player_info(std::uint32_t player_number, const glm::vec4& colour, std::uint64_t score,
                          std::uint32_t multiplier, float timer);

private:
  SimInternals* internals_;
};

}  // namespace ii

#endif