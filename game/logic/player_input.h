#ifndef IISPACE_GAME_LOGIC_PLAYER_INPUT_H
#define IISPACE_GAME_LOGIC_PLAYER_INPUT_H
#include "game/common/z.h"
#include "game/core/replay.h"

class Lib;
class Player;

struct PlayerInput {
  virtual void get(const Player& player, vec2& velocity, vec2& target, std::int32_t& keys) = 0;
  virtual ~PlayerInput() {}
};

struct ReplayPlayerInput : PlayerInput {
  ReplayPlayerInput(const Replay& replay);
  void get(const Player& player, vec2& velocity, vec2& target, std::int32_t& keys) override;

  const Replay& replay;
  std::size_t replay_frame;
};

struct LibPlayerInput : PlayerInput {
  LibPlayerInput(Lib& lib, Replay& replay);
  void get(const Player& player, vec2& velocity, vec2& target, std::int32_t& keys) override;

  Lib& lib;
  Replay& replay;
};

#endif