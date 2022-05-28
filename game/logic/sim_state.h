#ifndef IISPACE_GAME_LOGIC_SIM_STATE_H
#define IISPACE_GAME_LOGIC_SIM_STATE_H
#include "game/common/z.h"
#include "game/core/replay.h"
#include "game/mixer/sound.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

class Lib;
class Overmind;
class Player;
class PlayerInput;
class Ship;

namespace ii {
class SimInterface;
struct SimInternals;

class SimState {
public:
  using ship_list = std::vector<Ship*>;

  // TODO: move replay handling out of here.
  SimState(Lib& lib, std::int32_t* frame_count, game_mode mode, std::int32_t player_count,
           bool can_face_secret_boss);
  SimState(Lib& lib, std::int32_t* frame_count, const std::string& replay_path);
  ~SimState();

  const SimInterface& interface() const {
    return *interface_;
  }

  SimInterface& interface() {
    return *interface_;
  }

  void update();
  void render() const;

  game_mode mode() const;
  void write_replay(const std::string& team_name, std::int64_t score) const;
  bool game_over() const;

  struct sound_t {
    float volume = 0.f;
    float pan = 0.f;
    float pitch = 0.f;
  };
  std::unordered_map<sound, sound_t> get_sound_output() const;
  std::unordered_map<std::int32_t, std::int32_t> get_rumble_output() const;

  struct render_output {
    struct player_info {
      colour_t colour = 0;
      std::int64_t score = 0;
      std::int32_t multiplier = 0;
      float timer = 0.f;
    };
    struct line_t {
      fvec2 a;
      fvec2 b;
      colour_t c = 0;
    };
    std::vector<player_info> players;
    std::vector<line_t> lines;
    std::optional<float> replay_progress;  // TODO: remove.
    game_mode mode = game_mode::kNormal;
    std::int32_t elapsed_time = 0;
    std::int32_t lives_remaining = 0;
    std::int32_t overmind_timer = 0;
    std::optional<float> boss_hp_bar;
  };
  render_output get_render_output() const;

  struct results {
    bool is_replay = false;
    game_mode mode = game_mode::kNormal;
    std::int32_t seed = 0;
    std::int32_t elapsed_time = 0;
    std::int32_t killed_bosses = 0;
    std::int32_t lives_remaining = 0;

    std::int32_t bosses_killed = 0;
    std::int32_t hard_mode_bosses_killed = 0;

    struct player_result {
      std::int32_t number = 0;
      std::int64_t score = 0;
      std::int32_t deaths = 0;
    };
    std::vector<player_result> players;
  };

  results get_results() const;

private:
  SimState(Lib& lib, std::int32_t* frame_count, Replay&& replay, bool replay_recording);

  Lib& lib_;
  std::int32_t* frame_count_ = nullptr;
  std::int32_t kill_timer_ = 0;
  bool game_over_ = false;

  Replay replay_;
  bool replay_recording_ = false;
  std::unique_ptr<PlayerInput> input_;
  std::unique_ptr<Overmind> overmind_;
  std::unique_ptr<SimInternals> internals_;
  std::unique_ptr<SimInterface> interface_;
};

}  // namespace ii

#endif