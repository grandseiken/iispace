#ifndef II_GAME_LOGIC_SIM_IO_PLAYER_H
#define II_GAME_LOGIC_SIM_IO_PLAYER_H
#include "game/common/colour.h"
#include "game/common/math.h"
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

namespace ii {
static constexpr std::uint32_t kMaxPlayers = 4;

struct ai_state {
  vec2 velocity{0};
};

struct input_frame {
  enum key : std::uint32_t {
    kNone = 0b000000,
    kFire = 0b000001,
    kBomb = 0b000010,
    kSuper = 0b000100,
    kPower = 0b001000,
    kClick = 0b010000,
  };
  vec2 velocity{0};
  std::optional<vec2> target_absolute;
  std::optional<vec2> target_relative;
  std::uint32_t keys = 0;
};

struct input_source_mapping {
  std::vector<std::uint32_t> player_numbers;
};

struct network_input_mapping {
  input_source_mapping local;
  std::unordered_map</* remote ID */ std::string, input_source_mapping> remote;
};

}  // namespace ii

#endif
