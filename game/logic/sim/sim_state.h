#ifndef IISPACE_GAME_LOGIC_SIM_SIM_STATE_H
#define IISPACE_GAME_LOGIC_SIM_SIM_STATE_H
#include "game/common/z.h"
#include "game/logic/sim/sim_io.h"
#include "game/mixer/sound.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

class Overmind;
class Ship;

namespace ii {
class SimInterface;
struct SimInternals;

class SimState {
public:
  ~SimState();
  SimState(const initial_conditions& conditions, InputAdapter& input);

  const SimInterface& interface() const {
    return *interface_;
  }

  SimInterface& interface() {
    return *interface_;
  }

  void update();
  void render() const;
  std::int32_t frame_count() const;

  game_mode mode() const;  // TODO: necessary?
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
    game_mode mode = game_mode::kNormal;
    std::int32_t elapsed_time = 0;
    std::int32_t lives_remaining = 0;
    std::int32_t overmind_timer = 0;
    std::optional<float> boss_hp_bar;
    std::int32_t colour_cycle = 0;
  };
  render_output get_render_output() const;

  struct results {
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
  initial_conditions conditions_;
  InputAdapter& input_;
  std::int32_t kill_timer_ = 0;
  bool game_over_ = false;
  std::int32_t colour_cycle_ = 0;

  std::unique_ptr<Overmind> overmind_;
  std::unique_ptr<SimInternals> internals_;
  std::unique_ptr<SimInterface> interface_;
};

}  // namespace ii

#endif