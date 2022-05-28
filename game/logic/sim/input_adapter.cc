#include "game/logic/sim/input_adapter.h"
#include "game/core/lib.h"
#include "game/logic/player.h"

namespace ii {

ReplayInputAdapter::ReplayInputAdapter(const Replay& replay) : replay_{replay}, replay_frame_{0} {}

void ReplayInputAdapter::get(const Player& player, vec2& velocity, vec2& target,
                            std::int32_t& keys) {
  const auto& pf = replay_.replay.player_frame(replay_frame_++);
  velocity = {fixed::from_internal(pf.velocity_x()), fixed::from_internal(pf.velocity_y())};
  target = {fixed::from_internal(pf.target_x()), fixed::from_internal(pf.target_y())};
  keys = pf.keys();
}

float ReplayInputAdapter::progress() const {
  return static_cast<float>(replay_frame_) / replay_.replay.player_frame_size();
}

LibInputAdapter::LibInputAdapter(Lib& lib, Replay& replay) : lib_{lib}, replay_{replay} {}

void LibInputAdapter::get(const Player& player, vec2& velocity, vec2& target, std::int32_t& keys) {
  velocity = lib_.get_move_velocity(player.player_number());
  target = lib_.get_fire_target(player.player_number(), player.shape().centre);
  keys = static_cast<std::int32_t>(lib_.is_key_held(player.player_number(), Lib::key::kFire)) |
      (lib_.is_key_pressed(player.player_number(), Lib::key::kBomb) << 1);
  replay_.record(velocity, target, keys);
}

}  // namespace ii