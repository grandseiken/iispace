#ifndef II_GAME_DATA_CONDITIONS_H
#define II_GAME_DATA_CONDITIONS_H
#include "game/common/result.h"
#include "game/data/proto/conditions.pb.h"
#include "game/logic/sim/io/conditions.h"

namespace ii::data {

inline result<compatibility_level> read_compatibility_level(proto::CompatibilityLevel::Enum value) {
  switch (value) {
  case proto::CompatibilityLevel::kLegacy:
    return compatibility_level::kLegacy;
  case proto::CompatibilityLevel::kIispaceV0:
    return compatibility_level::kIispaceV0;
  default:
    return unexpected("unknown replay compatibility level");
  }
}

inline proto::CompatibilityLevel::Enum write_compatibility_level(compatibility_level value) {
  switch (value) {
  case compatibility_level::kLegacy:
    return proto::CompatibilityLevel::kLegacy;
  case compatibility_level::kIispaceV0:
    return proto::CompatibilityLevel::kIispaceV0;
  }
  return proto::CompatibilityLevel::kIispaceV0;
}

inline result<game_mode> read_game_mode(proto::GameMode::Enum value) {
  switch (value) {
  case proto::GameMode::kNormal:
    return game_mode::kNormal;
  case proto::GameMode::kBoss:
    return game_mode::kBoss;
  case proto::GameMode::kHard:
    return game_mode::kHard;
  case proto::GameMode::kFast:
    return game_mode::kFast;
  case proto::GameMode::kWhat:
    return game_mode::kWhat;
  default:
    return unexpected("unknown replay game mode");
  }
}

inline proto::GameMode::Enum write_game_mode(game_mode value) {
  switch (value) {
  case game_mode::kMax:
  case game_mode::kNormal:
    return proto::GameMode::kNormal;
  case game_mode::kBoss:
    return proto::GameMode::kBoss;
  case game_mode::kHard:
    return proto::GameMode::kHard;
  case game_mode::kFast:
    return proto::GameMode::kFast;
  case game_mode::kWhat:
    return proto::GameMode::kWhat;
  }
  return proto::GameMode::kNormal;
}

inline result<initial_conditions> read_initial_conditions(const proto::InitialConditions& proto) {
  auto compatibility = read_compatibility_level(proto.compatibility());
  if (!compatibility) {
    return unexpected(compatibility.error());
  }
  auto mode = read_game_mode(proto.game_mode());
  if (!mode) {
    return unexpected(mode.error());
  }

  initial_conditions data;
  data.compatibility = *compatibility;
  data.player_count = proto.player_count();
  data.seed = proto.seed();
  data.mode = *mode;

  if (proto.flags() & proto::InitialConditions::kLegacy_CanFaceSecretBoss) {
    data.flags |= initial_conditions::flag::kLegacy_CanFaceSecretBoss;
  }
  return data;
}

inline proto::InitialConditions write_initial_conditions(const initial_conditions& data) {
  proto::InitialConditions proto;
  proto.set_compatibility(write_compatibility_level(data.compatibility));
  proto.set_seed(data.seed);
  proto.set_player_count(data.player_count);
  proto.set_game_mode(write_game_mode(data.mode));

  std::uint32_t flags = 0;
  if (+(data.flags & initial_conditions::flag::kLegacy_CanFaceSecretBoss)) {
    flags |= proto::InitialConditions::kLegacy_CanFaceSecretBoss;
  }
  proto.set_flags(static_cast<proto::InitialConditions::Flag>(flags));
  return proto;
}

}  // namespace ii::data

#endif