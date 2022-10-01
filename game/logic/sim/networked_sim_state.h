#ifndef II_GAME_LOGIC_SIM_NETWORKED_SIM_STATE_H
#define II_GAME_LOGIC_SIM_NETWORKED_SIM_STATE_H
#include "game/data/packet.h"
#include "game/logic/sim/io/output.h"
#include "game/logic/sim/io/player.h"
#include "game/logic/sim/sim_state.h"
#include <cstdint>
#include <deque>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace ii {
namespace data {
class ReplayWriter;
}  // namespace data

// TODO:
// - reconcile effects to be applied at last-known position of enemy?
// - interpolate positions of shots and/or enemies (e.g. follow)?
class NetworkedSimState : public ISimState {
public:
  ~NetworkedSimState() override = default;
  NetworkedSimState(NetworkedSimState&&) noexcept = default;
  NetworkedSimState(const NetworkedSimState&) = delete;
  NetworkedSimState& operator=(NetworkedSimState&&) noexcept = default;
  NetworkedSimState& operator=(const NetworkedSimState&) = delete;

  NetworkedSimState(const initial_conditions& conditions, network_input_mapping mapping,
                    data::ReplayWriter* writer = nullptr, std::span<std::uint32_t> ai_players = {});

  const std::unordered_set<std::string>& checksum_failed_remote_ids() const;
  // Ticks by which to delay input (increases prediction accuracy in exchange for small amounts of
  // input lag).
  void set_input_delay_ticks(std::uint64_t delay_ticks);
  // May update canonical state; never updates predicted state.
  void input_packet(const std::string& remote_id, const data::sim_packet& packet);
  // Always updates predicted state, advancing its tick count by exactly one. May or may not update
  // canonical state.
  std::vector<data::sim_packet> update(std::vector<input_frame> local_input);

  const SimState& canonical() const { return canonical_state_; }
  const SimState& predicted() const { return predicted_state_; }

  std::pair<std::string, std::uint64_t> min_remote_latest_tick() const;
  std::pair<std::string, std::uint64_t> min_remote_canonical_tick() const;

  // ISimState implementation.
  bool game_over() const override { return canonical_state_.game_over(); }
  glm::uvec2 dimensions() const override { return predicted_state_.dimensions(); }
  std::uint64_t tick_count() const override { return predicted_state_.tick_count(); }
  std::uint32_t fps() const override { return predicted_state_.fps(); }

  const render_output& render(bool paused) const override {
    return predicted_state_.render(paused);
  }
  aggregate_output& output() override { return merged_output_; }
  const sim_results& results() const override { return canonical_state_.results(); }

private:
  static constexpr std::uint64_t kMaxReconcileTickDifference = 16;
  void handle_predicted_output(std::uint64_t tick_count, aggregate_output& output);
  void handle_canonical_output(std::uint64_t tick_count, aggregate_output& output);
  void handle_replay_output(std::uint64_t tick_count, aggregate_output& output);
  void handle_dual_output(aggregate_output& output);

  std::vector<std::uint32_t> local_ai_players_;
  SimState canonical_state_;
  SimState predicted_state_;
  aggregate_output merged_output_;
  network_input_mapping mapping_;
  std::uint32_t player_count_ = 0;
  std::uint64_t input_delay_ticks_ = 0;
  std::uint64_t predicted_tick_base_ = 0;

  struct resolve_key_hash {
    std::size_t operator()(const resolve_key& k) const { return k.hash(); }
  };

  SimState::smoothing_data smoothing_data_;
  std::unordered_map<resolve_key, std::uint64_t, resolve_key_hash> reconciliation_map_;

  struct partial_frame {
    std::vector<std::optional<input_frame>> input_frames;
  };

  struct tick_checksum {
    tick_checksum(std::uint64_t tick_count, std::uint32_t checksum)
    : tick_count{tick_count}, checksum{checksum} {}
    std::uint64_t tick_count = 0;
    std::uint32_t checksum = 0;
  };

  struct remote_info {
    std::deque<tick_checksum> checksums;
    // TODO: which of these do we actually need to adjust timings?
    std::uint64_t latest_tick = 0;
    std::uint64_t canonical_tick = 0;
  };
  std::deque<tick_checksum> local_checksums_;
  std::unordered_map<std::string, remote_info> remotes_;
  std::unordered_set<std::string> checksum_failed_remote_ids_;

  // Latest known input for each player.
  std::vector<input_frame> latest_input_;
  // Partial input data, starting at canonical tick.
  std::deque<partial_frame> partial_frames_;
  // Queue of delayed local inputs.
  std::deque<std::vector<input_frame>> input_delayed_frames_;
};

}  // namespace ii

#endif