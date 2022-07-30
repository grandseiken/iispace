#ifndef II_GAME_LOGIC_SIM_NETWORKED_SIM_STATE_H
#define II_GAME_LOGIC_SIM_NETWORKED_SIM_STATE_H
#include "game/data/packet.h"
#include "game/logic/sim/sim_io.h"
#include "game/logic/sim/sim_state.h"
#include <cstdint>
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

namespace ii {
class ReplayWriter;

class NetworkedSimState {
public:
  ~NetworkedSimState() = default;
  NetworkedSimState(NetworkedSimState&&) noexcept = default;
  NetworkedSimState(const NetworkedSimState&) = delete;
  NetworkedSimState& operator=(NetworkedSimState&&) noexcept = default;
  NetworkedSimState& operator=(const NetworkedSimState&) = delete;

  struct source_mapping {
    std::vector<std::uint32_t> player_numbers;
  };
  struct input_mapping {
    source_mapping local;
    std::unordered_map</* remote ID */ std::string, source_mapping> remote;
  };

  NetworkedSimState(const initial_conditions& conditions, input_mapping mapping,
                    ReplayWriter* writer = nullptr);

  const std::unordered_set<std::string>& checksum_failed_remote_ids() const;
  void input_packet(const std::string& remote_id, const sim_packet& packet);
  sim_packet update(std::vector<input_frame> local_input);

  const SimState& canonical() const {
    return canonical_state_;
  }

  const SimState& predicted() const {
    return predicted_state_;
  }

private:
  SimState canonical_state_;
  SimState predicted_state_;
  input_mapping mapping_;
  std::uint32_t player_count_ = 0;
  std::uint64_t predicted_tick_base_ = 0;

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
};

}  // namespace ii

#endif