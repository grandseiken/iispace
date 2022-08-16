#ifndef II_GAME_LOGIC_SIM_IO_CONDITIONS_H
#define II_GAME_LOGIC_SIM_IO_CONDITIONS_H
#include "game/common/enum.h"
#include <cstdint>

namespace ii {

enum class compatibility_level {
  kLegacy,
  kIispaceV0,
};

enum class game_mode : std::uint32_t {
  kNormal,
  kBoss,
  kHard,
  kFast,
  kWhat,
  kMax,
};

struct initial_conditions {
  enum class flag : std::uint32_t {
    kNone = 0,
    kLegacy_CanFaceSecretBoss = 0b00000001,
  };
  compatibility_level compatibility = compatibility_level::kIispaceV0;
  std::uint32_t seed = 0;
  std::uint32_t player_count = 0;
  game_mode mode = game_mode::kNormal;
  flag flags = flag::kNone;
};

template <>
struct bitmask_enum<initial_conditions::flag> : std::true_type {};

}  // namespace ii

#endif
