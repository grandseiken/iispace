#ifndef II_GAME_LOGIC_SIM_SIM_STATE_H
#define II_GAME_LOGIC_SIM_SIM_STATE_H
#include "game/common/math.h"
#include "game/logic/sim/sim_io.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <span>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace ii {
class Printer;
class ReplayWriter;
class SimInterface;
struct SimInternals;

class ISimState {
public:
  virtual ~ISimState() = default;

  virtual bool game_over() const = 0;
  virtual std::uint64_t tick_count() const = 0;
  virtual std::uint32_t frame_count() const = 0;
  virtual render_output render() const = 0;

  virtual aggregate_output& output() = 0;
  virtual sim_results results() const = 0;
};

class SimState : public ISimState {
public:
  ~SimState() override;
  SimState(SimState&&) noexcept;
  SimState(const SimState&) = delete;
  SimState& operator=(SimState&&) noexcept;
  SimState& operator=(const SimState&) = delete;

  SimState();  // Empty state for double-buffering. Behaviour undefined until copy_to().
  SimState(const initial_conditions& conditions, ReplayWriter* replay_writer = nullptr,
           std::span<const std::uint32_t> ai_players = {});

  std::uint64_t tick_count() const override;
  std::uint32_t checksum() const;  // Fast checksum.
  void copy_to(SimState&) const;
  void ai_think(std::vector<input_frame>& input) const;
  void update(std::vector<input_frame> input);
  bool game_over() const override;
  render_output render() const override;
  std::uint32_t frame_count() const override;

  aggregate_output& output() override;
  sim_results results() const override;

  // Functionality provided specifically for use by NetworkedSimState.
  struct smoothing_data {
    struct player_data {
      std::optional<vec2> position;
      vec2 velocity{0, 0};
      fixed rotation = 0;
    };
    std::unordered_map<std::uint32_t, player_data> players;
  };

  void set_predicted_players(std::span<const std::uint32_t>);
  void update_smoothing(smoothing_data& data);

  // Debug query API.
  struct query {
    query() : portable{false} {}
    bool portable;  // Make output identical across runs/compilers.
    std::unordered_set<std::uint32_t> entity_ids;
    std::unordered_set<std::string> component_names;
  };
  void dump(Printer&, const query& q = {}) const;

private:
  ReplayWriter* replay_writer_ = nullptr;
  std::uint32_t kill_timer_ = 0;
  std::uint32_t colour_cycle_ = 0;
  std::size_t compact_counter_ = 0;
  bool game_over_ = false;
  smoothing_data smoothing_data_;

  std::unique_ptr<SimInternals> internals_;
  std::unique_ptr<SimInterface> interface_;
};

}  // namespace ii

#endif
