#ifndef II_GAME_MODE_FLAGS_H
#define II_GAME_MODE_FLAGS_H
#include "game/common/result.h"
#include "game/flags.h"
#include "game/logic/sim/io/conditions.h"

namespace ii {

inline result<void> parse_game_mode(std::vector<std::string>& args, game_mode& mode) {
  std::optional<std::string> mode_string;
  if (auto r = flag_parse(args, "mode", mode_string); !r) {
    return unexpected(r.error());
  }
  if (mode_string) {
    if (*mode_string == "normal") {
      mode = game_mode::kNormal;
    } else if (*mode_string == "hard") {
      mode = game_mode::kHard;
    } else if (*mode_string == "boss") {
      mode = game_mode::kBoss;
    } else if (*mode_string == "what") {
      mode = game_mode::kWhat;
    } else {
      return unexpected("error: unknown game mode " + *mode_string);
    }
  }
  return {};
}

inline result<void>
parse_compatibility_level(std::vector<std::string>& args, compatibility_level& compatibility) {
  std::optional<std::string> compatibility_string;
  if (auto r = flag_parse(args, "compatibility", compatibility_string); !r) {
    return unexpected(r.error());
  }
  if (compatibility_string) {
    if (*compatibility_string == "legacy") {
      compatibility = compatibility_level::kLegacy;
    } else if (*compatibility_string == "v0") {
      compatibility = compatibility_level::kIispaceV0;
    } else {
      return unexpected("error: unknown compatibility level " + *compatibility_string);
    }
  }
  return {};
}

inline result<void>
parse_initial_conditions_flags(std::vector<std::string>& args, initial_conditions::flag& flags) {
  bool can_face_secret_boss = false;
  if (auto r = flag_parse<bool>(args, "can_face_secret_boss", can_face_secret_boss, false); !r) {
    return unexpected(r.error());
  }
  if (can_face_secret_boss) {
    flags |= initial_conditions::flag::kLegacy_CanFaceSecretBoss;
  }
  return {};
}

}  // namespace ii

#endif
