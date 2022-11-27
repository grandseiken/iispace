#ifndef II_GAME_LOGIC_V0_PLAYER_LOADOUT_MODS_H
#define II_GAME_LOGIC_V0_PLAYER_LOADOUT_MODS_H
#include "game/common/ustring.h"
#include <optional>

namespace ii::v0 {

enum class mod_category {
  kGeneral,
  kCorruption,
  kCloseCombat,
  kLightning,
  kSniper,
  kGravity,
  kLaser,
  kPummel,
  kRemote,
  kUnknown0,
  kUnknown1,
  kDefender,
  kCluster,
};

enum class mod_slot {
  kWeapon,
  kSuper,
  kBomb,
  kShield,
  kBonus,
  kWeaponCombo,
  kSuperCombo,
  kBombCombo,
  kShieldCombo,
  kBonusCombo,
};

enum class mod_id : std::uint32_t {
  kNone,
  kBackShots,
  kFrontShots,
  kBounceShots,
  kHomingShots,
  kSuperCapacity,
  kSuperRefill,
  kBombCapacity,
  kBombRadius,
  kBombSpeedClearCharge,
  kBombDoubleTrigger,
  kShieldCapacity,
  kShieldRefill,
  kShieldRespawn,
  kPowerupDrops,
  kCurrencyDrops,
  kCorruptionWeapon,
  kCorruptionSuper,
  kCorruptionBomb,
  kCorruptionShield,
  kCorruptionBonus,
  kCloseCombatWeapon,
  kCloseCombatSuper,
  kCloseCombatBomb,
  kCloseCombatShield,
  kCloseCombatBonus,
  kLightningWeapon,
  kLightningSuper,
  kLightningBomb,
  kLightningShield,
  kLightningBonus,
  kSniperWeapon,
  kSniperSuper,
  kSniperBomb,
  kSniperShield,
  kSniperBonus,
  kLaserWeapon,
  kLaserSuper,
  kLaserBomb,
  kLaserShield,
  kLaserBonus,
  kClusterWeapon,
  kClusterSuper,
  kClusterBomb,
  kClusterShield,
  kClusterBonus,
};

struct mod_options {
  bool multiplayer_only = false;
  bool allow_multiple = false;
  bool allow_multiple_per_player = false;
  std::optional<mod_slot> slot_dependency;
  std::optional<mod_category> category_dependency;
};

struct mod_data {
  mod_id id = mod_id::kNone;
  mod_slot slot = mod_slot::kBonusCombo;
  mod_category category = mod_category::kGeneral;
  mod_options options;
  ustring_view name;
  ustring_view description;
};

const mod_data& mod_lookup(mod_id);

}  // namespace ii::v0

#endif
