#include "game/logic/sim/networked_sim_state.h"

namespace ii {

NetworkedSimState::NetworkedSimState(const initial_conditions& conditions, const player_mapping&)
: canonical_state_{conditions}, predicted_state_{conditions} {}

void NetworkedSimState::input_packet(const std::string& remote_id, const sim_packet& packet) {}

sim_packet NetworkedSimState::update(InputAdapter& local_input) {
  return {};
}

}  // namespace ii