#ifndef IISPACE_GAME_LOGIC_SIM_INPUT_ADAPTER_H
#define IISPACE_GAME_LOGIC_SIM_INPUT_ADAPTER_H
#include "game/common/z.h"
#include "game/core/replay.h"
#include <cstdint>
#include <optional>
#include <vector>

class Lib;
class Replay;

namespace ii {

struct input_frame {
  vec2 velocity;
  std::optional<vec2> target_absolute;
  std::optional<vec2> target_relative;
  std::int32_t keys = 0;
};

class InputAdapter {
public:
  virtual ~InputAdapter() {}
  virtual std::vector<input_frame> get() = 0;
};

class ReplayInputAdapter : public InputAdapter {
public:
  ReplayInputAdapter(const Replay& replay);
  std::vector<input_frame> get() override;
  float progress() const;

private:
  const Replay& replay_;
  std::size_t replay_frame_;
};

class LibInputAdapter : public InputAdapter {
public:
  LibInputAdapter(Lib& lib, Replay& replay);
  std::vector<input_frame> get() override;

private:
  Lib& lib_;
  Replay& replay_;
};

}  // namespace ii

#endif