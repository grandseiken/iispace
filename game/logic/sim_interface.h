#ifndef IISPACE_GAME_LOGIC_SIM_INTERFACE_H
#define IISPACE_GAME_LOGIC_SIM_INTERFACE_H
#include "game/common/z.h"
#include "game/mixer/sound.h"
#include <cstdint>
#include <memory>
#include <vector>

class Lib;
class Player;
class Ship;

struct Particle {
  Particle(const fvec2& position, colour_t colour, const fvec2& velocity, std::int32_t time)
  : position{position}, velocity{velocity}, timer{time}, colour{colour} {}

  bool destroy = false;
  std::int32_t timer = 0;
  colour_t colour = 0;
  fvec2 position;
  fvec2 velocity;
};

namespace ii {
class SimInternals;

constexpr std::int32_t kSimWidth = 640;
constexpr std::int32_t kSimHeight = 480;

class SimInterface {
public:
  enum boss_list {
    BOSS_1A = 1,
    BOSS_1B = 2,
    BOSS_1C = 4,
    BOSS_2A = 8,
    BOSS_2B = 16,
    BOSS_2C = 32,
    BOSS_3A = 64,
  };
  using ship_list = std::vector<Ship*>;

  SimInterface(SimInternals* internals) : internals_{internals} {}

  // State manipulation.
  game_mode mode() const;

  // TODO: const/non-const versions of retrieval functions.
  ship_list all_ships(std::int32_t ship_mask = 0) const;
  ship_list ships_in_radius(const vec2& point, fixed radius, std::int32_t ship_mask = 0) const;
  ship_list collision_list(const vec2& point, std::int32_t category) const;
  bool any_collision(const vec2& point, std::int32_t category) const;
  std::int32_t get_non_wall_count() const;

  std::int32_t alive_players() const;
  std::int32_t killed_players() const;
  Player* nearest_player(const vec2& point) const;
  const ship_list& players() const;

  void add_life();
  void sub_life();
  std::int32_t get_lives() const;
  void set_boss_killed(boss_list boss);

  void add_ship(std::unique_ptr<Ship> ship);
  void add_particle(const Particle& particle);

  template <typename T, typename... Args>
  void add_new_ship(Args&&... args) {
    add_ship(std::make_unique<T>(std::forward<Args>(args)...));
  }

  // Simulation output.
  void rumble(std::int32_t player, std::int32_t time) const;
  void play_sound(sound, float volume = 1.f, float pan = 0.f, float repitch = 0.f) const;
  void render_hp_bar(float fill) const;
  void render_line(const fvec2& a, const fvec2& b, colour_t c) const;
  void render_line_rect(const fvec2& lo, const fvec2& hi, colour_t c) const;
  void render_player_info(std::int32_t player_number, colour_t colour, std::int64_t score,
                          std::int32_t multiplier, float timer);

private:
  SimInternals* internals_;
};

}  // namespace ii

#endif