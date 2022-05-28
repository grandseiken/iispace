#ifndef IISPACE_GAME_LOGIC_SIM_INPUT_ADAPTER_H
#define IISPACE_GAME_LOGIC_SIM_INPUT_ADAPTER_H
#include "game/common/z.h"
#include "game/core/replay.h"

class Lib;
class Player;

namespace ii {

class InputAdapter {
public:
  virtual ~InputAdapter() {}
  virtual void get(const Player& player, vec2& velocity, vec2& target, std::int32_t& keys) = 0;
};

class ReplayInputAdapter : public InputAdapter {
public:
  ReplayInputAdapter(const Replay& replay);
  void get(const Player& player, vec2& velocity, vec2& target, std::int32_t& keys) override;
  float progress() const;

private:
  const Replay& replay_;
  std::size_t replay_frame_;
};

class LibInputAdapter : public InputAdapter {
public:
  LibInputAdapter(Lib& lib, Replay& replay);
  void get(const Player& player, vec2& velocity, vec2& target, std::int32_t& keys) override;

private:
  Lib& lib_;
  Replay& replay_;
};

}  // namespace ii

#endif