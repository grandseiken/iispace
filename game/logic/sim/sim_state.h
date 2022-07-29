#ifndef II_GAME_LOGIC_SIM_SIM_STATE_H
#define II_GAME_LOGIC_SIM_SIM_STATE_H
#include "game/logic/sim/sim_io.h"
#include "game/mixer/sound.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

namespace ii {
class Overmind;
class Printer;
class Ship;
class SimInterface;
struct SimInternals;

class SimState {
public:
  ~SimState();
  SimState(SimState&&) noexcept;
  SimState(const SimState&) = delete;
  SimState& operator=(SimState&&) noexcept;
  SimState& operator=(const SimState&) = delete;

  SimState();  // Empty state for double-buffering. Behaviour undefined until copy_to().
  SimState(const initial_conditions& conditions, std::span<const std::uint32_t> ai_players = {});
  // TODO: some kind of fast state checksum.

  void copy_to(SimState&) const;
  void update(InputAdapter& input);
  void render() const;
  bool game_over() const;
  std::uint32_t frame_count() const;

  void clear_output();
  std::unordered_map<sound, sound_out> get_sound_output() const;
  std::unordered_map<std::uint32_t, std::uint32_t> get_rumble_output() const;
  render_output get_render_output() const;
  sim_results get_results() const;
  void dump(Printer&) const;

private:
  std::uint32_t kill_timer_ = 0;
  std::uint32_t colour_cycle_ = 0;
  std::size_t compact_counter_ = 0;
  bool game_over_ = false;

  std::unique_ptr<SimInternals> internals_;
  std::unique_ptr<SimInterface> interface_;
};

}  // namespace ii

#endif