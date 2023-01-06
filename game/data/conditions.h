#ifndef II_GAME_DATA_CONDITIONS_H
#define II_GAME_DATA_CONDITIONS_H
#include "game/common/result.h"
#include "game/common/ustring_convert.h"
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
  case proto::GameMode::kStandardRun:
    return game_mode::kStandardRun;
  case proto::GameMode::kLegacy_Normal:
    return game_mode::kLegacy_Normal;
  case proto::GameMode::kLegacy_Boss:
    return game_mode::kLegacy_Boss;
  case proto::GameMode::kLegacy_Hard:
    return game_mode::kLegacy_Hard;
  case proto::GameMode::kLegacy_Fast:
    return game_mode::kLegacy_Fast;
  case proto::GameMode::kLegacy_What:
    return game_mode::kLegacy_What;
  default:
    return unexpected("unknown replay game mode");
  }
}

inline proto::GameMode::Enum write_game_mode(game_mode value) {
  switch (value) {
  case game_mode::kStandardRun:
    return proto::GameMode::kStandardRun;
  case game_mode::kLegacy_Normal:
    return proto::GameMode::kLegacy_Normal;
  case game_mode::kLegacy_Boss:
    return proto::GameMode::kLegacy_Boss;
  case game_mode::kLegacy_Hard:
    return proto::GameMode::kLegacy_Hard;
  case game_mode::kLegacy_Fast:
    return proto::GameMode::kLegacy_Fast;
  case game_mode::kLegacy_What:
    return proto::GameMode::kLegacy_What;
  }
  return proto::GameMode::kStandardRun;
}

inline player_conditions read_player_conditions(const proto::PlayerConditions& proto) {
  player_conditions data;
  data.player_name = ustring::utf8(proto.player_name());
  return data;
}

inline proto::PlayerConditions write_player_conditions(const player_conditions& data) {
  proto::PlayerConditions proto;
  proto.set_player_name(to_utf8(data.player_name));
  return proto;
}

inline run_modifiers read_run_modifiers(const proto::RunModifiers&) {
  run_modifiers data;
  return data;
}

inline proto::RunModifiers write_run_modifiers(const run_modifiers&) {
  proto::RunModifiers proto;
  return proto;
}

inline game_tech_tree read_game_tech_tree(const proto::GameTechTree&) {
  game_tech_tree data;
  return data;
}

inline proto::GameTechTree write_game_tech_tree(const game_tech_tree&) {
  proto::GameTechTree proto;
  return proto;
}

inline result<run_biome> read_run_biome(proto::RunBiome::Enum value) {
  switch (value) {
  case proto::RunBiome::kTesting:
    return run_biome::kTesting;
  case proto::RunBiome::kBiome0_Uplink:
    return run_biome::kBiome0_Uplink;
  case proto::RunBiome::kBiome1_Edge:
    return run_biome::kBiome1_Edge;
  case proto::RunBiome::kBiome2_Fabric:
    return run_biome::kBiome2_Fabric;
  case proto::RunBiome::kBiome3_Archive:
    return run_biome::kBiome3_Archive;
  case proto::RunBiome::kBiome4_Firewall:
    return run_biome::kBiome4_Firewall;
  case proto::RunBiome::kBiome5_Prism:
    return run_biome::kBiome5_Prism;
  case proto::RunBiome::kBiome6_SystemCore:
    return run_biome::kBiome6_SystemCore;
  case proto::RunBiome::kBiome7_DarkNet:
    return run_biome::kBiome7_DarkNet;
  case proto::RunBiome::kBiome8_Paradise:
    return run_biome::kBiome8_Paradise;
  case proto::RunBiome::kBiome9_Minus:
    return run_biome::kBiome9_Minus;
  default:
    return unexpected("unknown run biome");
  }
}

inline proto::RunBiome::Enum write_run_biome(run_biome value) {
  switch (value) {
  case run_biome::kTesting:
    return proto::RunBiome::kTesting;
  case run_biome::kBiome0_Uplink:
    return proto::RunBiome::kBiome0_Uplink;
  case run_biome::kBiome1_Edge:
    return proto::RunBiome::kBiome1_Edge;
  case run_biome::kBiome2_Fabric:
    return proto::RunBiome::kBiome2_Fabric;
  case run_biome::kBiome3_Archive:
    return proto::RunBiome::kBiome3_Archive;
  case run_biome::kBiome4_Firewall:
    return proto::RunBiome::kBiome4_Firewall;
  case run_biome::kBiome5_Prism:
    return proto::RunBiome::kBiome5_Prism;
  case run_biome::kBiome6_SystemCore:
    return proto::RunBiome::kBiome6_SystemCore;
  case run_biome::kBiome7_DarkNet:
    return proto::RunBiome::kBiome7_DarkNet;
  case run_biome::kBiome8_Paradise:
    return proto::RunBiome::kBiome8_Paradise;
  case run_biome::kBiome9_Minus:
    return proto::RunBiome::kBiome9_Minus;
  }
  return proto::RunBiome::kNone;
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
  data.seed = proto.seed();
  data.player_count = proto.player_count();
  for (const auto& player : proto.player()) {
    data.players.emplace_back(read_player_conditions(player));
  }

  data.mode = *mode;
  data.modifiers = read_run_modifiers(proto.modifiers());
  data.tech_tree = read_game_tech_tree(proto.tech_tree());
  for (const auto& biome : proto.biome()) {
    auto result = read_run_biome(biome.value());
    if (!result) {
      return unexpected(result.error());
    }
    data.biomes.emplace_back(*result);
  }

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
  for (const auto& player : data.players) {
    *proto.add_player() = write_player_conditions(player);
  }

  proto.set_game_mode(write_game_mode(data.mode));
  *proto.mutable_modifiers() = write_run_modifiers(data.modifiers);
  *proto.mutable_tech_tree() = write_game_tech_tree(data.tech_tree);
  for (auto biome : data.biomes) {
    proto.add_biome()->set_value(write_run_biome(biome));
  }

  std::uint32_t flags = 0;
  if (+(data.flags & initial_conditions::flag::kLegacy_CanFaceSecretBoss)) {
    flags |= proto::InitialConditions::kLegacy_CanFaceSecretBoss;
  }
  proto.set_flags(static_cast<proto::InitialConditions::Flag>(flags));
  return proto;
}

}  // namespace ii::data

#endif