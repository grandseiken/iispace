#ifndef II_GAME_LOGIC_SIM_SIM_STATE_H
#define II_GAME_LOGIC_SIM_SIM_STATE_H
#include "game/logic/sim/sim_io.h"
#include "game/mixer/sound.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

class Overmind;

namespace ii {
class Ship;
class SimInterface;
struct SimInternals;

class SimState {
public:
  ~SimState();
  SimState(const initial_conditions& conditions, InputAdapter& input);

  void update();
  void render() const;
  std::uint32_t frame_count() const;
  bool game_over() const;

  void clear_output();
  std::unordered_map<sound, sound_out> get_sound_output() const;
  std::unordered_map<std::uint32_t, std::uint32_t> get_rumble_output() const;
  render_output get_render_output() const;
  sim_results get_results() const;

private:
  initial_conditions conditions_;
  InputAdapter& input_;
  std::uint32_t kill_timer_ = 0;
  bool game_over_ = false;
  std::uint32_t colour_cycle_ = 0;

  std::unique_ptr<Overmind> overmind_;
  std::unique_ptr<SimInternals> internals_;
  std::unique_ptr<SimInterface> interface_;
};

}  // namespace ii

#endif