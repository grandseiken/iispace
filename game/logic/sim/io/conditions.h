#ifndef II_GAME_LOGIC_SIM_IO_CONDITIONS_H
#define II_GAME_LOGIC_SIM_IO_CONDITIONS_H
#include "game/common/colour.h"
#include "game/common/enum.h"
#include "game/common/ustring.h"
#include <glm/glm.hpp>
#include <array>
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
struct player_conditions {
  ustring player_name;
};

struct initial_conditions {
  enum class flag : std::uint32_t {
    kNone = 0,
    kLegacy_CanFaceSecretBoss = 0b00000001,
  };
  compatibility_level compatibility = compatibility_level::kIispaceV0;
  std::uint32_t seed = 0;
  std::uint32_t player_count = 0;
  std::vector<player_conditions> players;

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
  return false;
}

inline glm::vec4 legacy_player_colour(std::uint32_t player_number) {
  return colour::hue360((20 * player_number) % 360);
}

inline glm::vec4 v0_player_colour(std::uint32_t player_number) {
  static constexpr auto kNewPlayer0 =
      colour::clamp(colour::kSolarizedDarkRed + glm::vec4{0.f, .15f, 0.f, 0.f});
  static constexpr auto kNewPlayer1 =
      colour::clamp(colour::kSolarizedDarkOrange + glm::vec4{0.f, .2f, 0.f, 0.f});
  static constexpr auto kNewPlayer2 =
      colour::clamp(colour::kSolarizedDarkYellow + glm::vec4{0.f, .25f, 0.f, 0.f});
  static constexpr std::array colours = {
      kNewPlayer0,
      colour::hsl_mix(kNewPlayer0, kNewPlayer1, 2.f / 3.f),
      colour::hsl_mix(kNewPlayer1, kNewPlayer2, 1.f / 3.f),
      kNewPlayer2,
  };
  return colours[player_number % colours.size()];
}

inline glm::vec4 player_colour(game_mode mode, std::uint32_t player_number) {
  return is_legacy_mode(mode) ? legacy_player_colour(player_number)
                              : v0_player_colour(player_number);
}

}  // namespace ii

#endif
