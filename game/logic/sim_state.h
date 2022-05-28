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

  struct results {
    bool is_replay = false;
    float replay_progress = 0.f;
    game_mode mode = game_mode::kNormal;
    std::int32_t seed = 0;
    std::int32_t elapsed_time = 0;
    std::int32_t killed_bosses = 0;
    std::int32_t lives_remaining = 0;
    std::int32_t overmind_timer = 0;
    std::optional<float> boss_hp_bar;

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