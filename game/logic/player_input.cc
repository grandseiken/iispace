#include "game/logic/player_input.h"
#include "game/core/lib.h"
#include "game/logic/player.h"

ReplayPlayerInput::ReplayPlayerInput(const Replay& replay) : replay(replay), replay_frame(0) {}

void ReplayPlayerInput::get(const Player& player, vec2& velocity, vec2& target,
                            std::int32_t& keys) {
  const auto& pf = replay.replay.player_frame(replay_frame++);
  velocity = vec2(fixed::from_internal(pf.velocity_x()), fixed::from_internal(pf.velocity_y()));
  target = vec2(fixed::from_internal(pf.target_x()), fixed::from_internal(pf.target_y()));
  keys = pf.keys();
}

LibPlayerInput::LibPlayerInput(Lib& lib, Replay& replay) : lib(lib), replay(replay) {}

void LibPlayerInput::get(const Player& player, vec2& velocity, vec2& target, std::int32_t& keys) {
  velocity = lib.get_move_velocity(player.player_number());
  target = lib.get_fire_target(player.player_number(), player.shape().centre);
  keys = std::int32_t(lib.is_key_held(player.player_number(), Lib::key::kFire)) |
      (lib.is_key_pressed(player.player_number(), Lib::key::kBomb) << 1);
  replay.record(velocity, target, keys);
}