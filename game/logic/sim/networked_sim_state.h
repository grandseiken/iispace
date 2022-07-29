#ifndef II_GAME_LOGIC_SIM_NETWORKED_SIM_STATE_H
#define II_GAME_LOGIC_SIM_NETWORKED_SIM_STATE_H
#include "game/data/packet.h"
#include "game/logic/sim/sim_io.h"
#include "game/logic/sim/sim_state.h"
#include <deque>
#include <string>
#include <unordered_map>
#include <vector>

namespace ii {

class NetworkedSimState {
public:
  ~NetworkedSimState();
  NetworkedSimState(NetworkedSimState&&) noexcept = default;
  NetworkedSimState(const NetworkedSimState&) = delete;
  NetworkedSimState& operator=(NetworkedSimState&&) noexcept = default;
  NetworkedSimState& operator=(const NetworkedSimState&) = delete;

  struct source_mapping {
    std::vector<std::uint32_t> player_numbers;
  };
  struct player_mapping {
    source_mapping local;
    std::unordered_map</* remote ID */ std::string, source_mapping> remote;
  };

  NetworkedSimState(const initial_conditions& conditions, const player_mapping& mapping);
  void input_packet(const std::string& remote_id, const sim_packet& packet);
  sim_packet update(InputAdapter& local_input);

  const SimState& canonical() const {
    return canonical_state_;
  }

  const SimState& predicted() const {
    return predicted_state_;
  }

private:
  SimState canonical_state_;
  SimState predicted_state_;

  struct partial_frame {
    std::optional<std::uint32_t> checksum;
    std::vector<std::optional<input_frame>> input_frames;
  };
  std::deque<partial_frame> partial_frames_;
};

}  // namespace ii

#endif