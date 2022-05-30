#ifndef IISPACE_GAME_LOGIC_SIM_SIM_IO_H
#define IISPACE_GAME_LOGIC_SIM_SIM_IO_H
#include "game/common/z.h"
#include <cstdint>
#include <optional>
#include <vector>

namespace ii {

enum class game_mode {
  kNormal,
  kBoss,
  kHard,
  kFast,
  kWhat,
  kMax,
};

struct initial_conditions {
  std::int32_t seed = 0;
  game_mode mode = game_mode::kNormal;
  std::int32_t player_count = 0;
  bool can_face_secret_boss = false;
};

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

}  // namespace ii

#endif