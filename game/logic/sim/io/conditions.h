#ifndef II_GAME_LOGIC_SIM_IO_CONDITIONS_H
#define II_GAME_LOGIC_SIM_IO_CONDITIONS_H
#include "game/common/enum.h"
#include <cstdint>
#include <vector>

namespace ii {

enum class compatibility_level {
  kLegacy,
  kIispaceV0,
};

enum class game_mode : std::uint32_t {
  // New game modes.
  kStandardRun,
  // Legacy game modes.
  kLegacy_Normal,
  kLegacy_Boss,
  kLegacy_Hard,
  kLegacy_Fast,
  kLegacy_What,
};

enum class run_biome : std::uint32_t {
  kTesting,
};

struct run_modifiers {};
struct game_tech_tree {};

struct initial_conditions {
  enum class flag : std::uint32_t {
    kNone = 0,
    kLegacy_CanFaceSecretBoss = 0b00000001,
  };
  compatibility_level compatibility = compatibility_level::kIispaceV0;
  std::uint32_t seed = 0;
  std::uint32_t player_count = 0;

  game_mode mode = game_mode::kStandardRun;
  game_tech_tree tech_tree;
  run_modifiers modifiers;
  flag flags = flag::kNone;
  std::vector<run_biome> biomes;
};

template <>
struct bitmask_enum<initial_conditions::flag> : std::true_type {};

inline bool is_legacy_mode(game_mode mode) {
  switch (mode) {
  case game_mode::kStandardRun:
    return false;
  case game_mode::kLegacy_Normal:
  case game_mode::kLegacy_Boss:
  case game_mode::kLegacy_Hard:
  case game_mode::kLegacy_Fast:
  case game_mode::kLegacy_What:
    return true;
  }
}

}  // namespace ii

#endif
