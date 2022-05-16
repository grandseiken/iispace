#ifndef IISPACE_GAME_LOGIC_SIM_STATE_H
#define IISPACE_GAME_LOGIC_SIM_STATE_H
#include "game/common/z.h"
#include "game/core/replay.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class Lib;
class Overmind;
class Player;
class PlayerInput;
class SaveData;
class Ship;

struct Particle {
  Particle(const fvec2& position, colour_t colour, const fvec2& velocity, int32_t time)
  : destroy(false), position(position), velocity(velocity), timer(time), colour(colour) {}

  bool destroy;
  fvec2 position;
  fvec2 velocity;
  int32_t timer;
  colour_t colour;
};

class SimState {
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

  SimState(Lib& lib, SaveData& save, int32_t* frame_count, Mode::mode mode, int32_t player_count,
           bool can_face_secret_boss);
  SimState(Lib& lib, SaveData& save, int32_t* frame_count, const std::string& replay_path);
  ~SimState();

  void update(Lib& lib);
  void render(Lib& lib) const;

  Lib& lib();
  Mode::mode mode() const;
  void write_replay(const std::string& team_name, int64_t score) const;

  typedef std::vector<Ship*> ship_list;
  void add_ship(Ship* ship);
  void add_particle(const Particle& particle);
  int32_t get_non_wall_count() const;

  ship_list all_ships(int32_t ship_mask = 0) const;
  ship_list ships_in_radius(const vec2& point, fixed radius, int32_t ship_mask = 0) const;
  ship_list collision_list(const vec2& point, int32_t category) const;
  bool any_collision(const vec2& point, int32_t category) const;

  int32_t alive_players() const;
  int32_t killed_players() const;
  Player* nearest_player(const vec2& point) const;
  const ship_list& players() const;

  bool game_over() const;
  void add_life();
  void sub_life();
  int32_t get_lives() const;

  void render_hp_bar(float fill);
  void set_boss_killed(boss_list boss);

  struct results {
    bool is_replay = false;
    float replay_progress = 0.f;
    Mode::mode mode = Mode::NORMAL;
    std::int32_t seed = 0;
    std::int32_t elapsed_time = 0;
    std::int32_t killed_bosses = 0;
    std::int32_t lives_remaining = 0;
    std::int32_t overmind_timer = 0;
    std::optional<float> boss_hp_bar;

    struct player_result {
      std::int32_t number = 0;
      std::int64_t score = 0;
      std::int32_t deaths = 0;
    };
    std::vector<player_result> players;
  };

  results get_results() const;

private:
  SimState(Lib& lib, SaveData& save, int32_t* frame_count, Replay&& replay, bool replay_recording);

  Lib& _lib;
  SaveData& _save;
  int32_t* _frame_count = nullptr;
  int32_t _kill_timer = 0;
  bool _game_over = false;

  Replay _replay;
  bool _replay_recording = false;
  std::unique_ptr<PlayerInput> _input;

  int32_t _lives = 0;
  std::unique_ptr<Overmind> _overmind;
  std::vector<Particle> _particles;
  std::vector<std::unique_ptr<Ship>> _ships;
  ship_list _player_list;
  ship_list _collisions;

  mutable std::optional<float> _boss_hp_bar;
};

#endif