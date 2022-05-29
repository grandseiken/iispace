#include "game/logic/sim/input_adapter.h"
#include "game/core/lib.h"
#include "game/logic/player.h"

namespace ii {

ReplayInputAdapter::ReplayInputAdapter(const Replay& replay) : replay_{replay}, replay_frame_{0} {}

std::vector<input_frame> ReplayInputAdapter::get() {
  std::vector<input_frame> result;
  for (std::size_t i = 0; i < replay_.replay.players(); ++i) {
    auto& f = result.emplace_back();
    if (replay_frame_ >= replay_.replay.player_frame_size()) {
      continue;
    }
    const auto& rf = replay_.replay.player_frame(replay_frame_++);
    vec2 target{fixed::from_internal(rf.target_x()), fixed::from_internal(rf.target_y())};
    if (rf.target_relative()) {
      f.target_relative = target;
    } else {
      f.target_absolute = target;
    }
    f.velocity = {fixed::from_internal(rf.velocity_x()), fixed::from_internal(rf.velocity_y())};
    f.keys = rf.keys();
  }
  return result;
}

float ReplayInputAdapter::progress() const {
  return static_cast<float>(replay_frame_) / replay_.replay.player_frame_size();
}

LibInputAdapter::LibInputAdapter(Lib& lib, Replay& replay) : lib_{lib}, replay_{replay} {}

std::vector<input_frame> LibInputAdapter::get() {
  std::vector<input_frame> result;
  for (std::int32_t i = 0; i < lib_.player_count(); ++i) {
    auto& f = result.emplace_back();
    f.velocity = lib_.get_move_velocity(i);
    auto target = lib_.get_fire_target(i);
    if (target.first) {
      f.target_relative = target.second;
    } else {
      f.target_absolute = target.second;
    }
    f.keys = static_cast<std::int32_t>(lib_.is_key_held(i, Lib::key::kFire)) |
        (lib_.is_key_pressed(i, Lib::key::kBomb) << 1);
    replay_.record(f.velocity, target.first, target.second, f.keys);
  }
  return result;
}

}  // namespace ii