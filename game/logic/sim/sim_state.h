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

  void update();
  void render() const;
  std::int32_t frame_count() const;
  bool game_over() const;

  std::unordered_map<sound, sound_out> get_sound_output() const;
  std::unordered_map<std::int32_t, std::int32_t> get_rumble_output() const;
  render_output get_render_output() const;
  sim_results get_results() const;

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