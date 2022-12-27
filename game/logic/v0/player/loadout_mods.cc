#include "game/logic/v0/player/loadout_mods.h"
#include "game/common/colour.h"
#include <unordered_map>

namespace ii::v0 {
namespace {

std::unordered_map<mod_id, mod_data> make_mod_map() {
  std::unordered_map<mod_id, mod_data> m;

  auto add_data = [&m](mod_id id, mod_slot slot, mod_category category, const char* name,
                       const char* description, const mod_options& options = {}) {
    m[id] = {
        .id = id,
        .slot = slot,
        .category = category,
        .options = options,
        .name = ustring_view::ascii(name),
        .description = ustring_view::ascii(description),
    };
  };

  add_data(mod_id::kBackShots, mod_slot::kWeaponCombo, mod_category::kGeneral, "Reversi",
           "You shoot both forwards and backwards simultaneously.");
  add_data(mod_id::kFrontShots, mod_slot::kWeaponCombo, mod_category::kGeneral, "Twin barrel",
           "You fire two shots instead of one, at a slightly reduced rate.");
  add_data(mod_id::kBounceShots, mod_slot::kWeaponCombo, mod_category::kGeneral, "Ricochet",
           "Your shots bounce on impact, and may strike a second target.");
  add_data(mod_id::kHomingShots, mod_slot::kWeaponCombo, mod_category::kGeneral, "Threatseeker",
           "Your shots are guided towards targets.");

  add_data(mod_id::kSuperCapacity, mod_slot::kSuperCombo, mod_category::kGeneral, "Megabuffer",
           "Your mega-weapon charge capacity is increased.",
           {.allow_multiple_per_player = true, .slot_dependency = mod_slot::kSuper});
  add_data(mod_id::kSuperRefill, mod_slot::kSuperCombo, mod_category::kGeneral, "Overcharge",
           "Your mega-weapon recharges automatically. Your shields are disabled.",
           {.slot_dependency = mod_slot::kSuper});

  add_data(mod_id::kBombCapacity, mod_slot::kBombCombo, mod_category::kGeneral, "Boombox",
           "You can carry an additional bomb. You gain a bomb now.",
           {.allow_multiple_per_player = true});
  add_data(mod_id::kBombRadius, mod_slot::kBombCombo, mod_category::kGeneral, "Blastwave",
           "Your bombs have a larger area-of-effect radius.", {.allow_multiple_per_player = true});
  add_data(mod_id::kBombSpeedClearCharge, mod_slot::kBombCombo, mod_category::kGeneral,
           "Blast synthesis", "Whenever a wave of enemies is cleared quickly, you gain a bomb.");
  add_data(mod_id::kBombDoubleTrigger, mod_slot::kBombCombo, mod_category::kGeneral, "Aftershock",
           "Your bombs explode twice, triggering a second time after a short delay.");

  add_data(mod_id::kShieldCapacity, mod_slot::kShieldCombo, mod_category::kGeneral,
           "Shell extension",
           "Your maximum number of shields is increased by one. You gain a shield now.",
           {.allow_multiple_per_player = true});
  add_data(mod_id::kShieldRefill, mod_slot::kShieldCombo, mod_category::kGeneral, "Recombinator",
           "Your shield regenerates automatically after a time, but you can only have one.");
  add_data(mod_id::kShieldRespawn, mod_slot::kShieldCombo, mod_category::kGeneral, "Safe boot",
           "You respawn with full shields.", {.multiplayer_only = true});

  add_data(mod_id::kPowerupDrops, mod_slot::kBonusCombo, mod_category::kGeneral, "Optimizer",
           "Enemies you destroy have a higher chance to drop power-ups.", {.allow_multiple = true});
  add_data(mod_id::kCurrencyDrops, mod_slot::kBonusCombo, mod_category::kGeneral, "Data scraper",
           "Enemies you destroy have a higher chance to drop currency.", {.allow_multiple = true});

  add_data(mod_id::kCorruptionWeapon, mod_slot::kWeapon, mod_category::kCorruption, "Corruptor",
           "Your shots deal bonus damage and inflict bit-rot.");
  add_data(mod_id::kCorruptionSuper, mod_slot::kSuper, mod_category::kCorruption, "Fragmenter",
           "Your mega-weapon damages enemies and inflicts bit-rot in a cone.");
  add_data(mod_id::kCorruptionBomb, mod_slot::kBomb, mod_category::kCorruption,
           "Incoherency vortex", "Your bomb creates a vortex that inflicts bit-rot.");
  add_data(mod_id::kCorruptionShield, mod_slot::kShield, mod_category::kCorruption, "Scrambler",
           "Your shield inflicts all nearby enemies with bit-rot.");
  add_data(mod_id::kCorruptionBonus, mod_slot::kBonus, mod_category::kCorruption,
           "Viral replication", "Bit-rot spreads from afflicted enemies to others nearby.",
           {.category_dependency = mod_category::kCorruption});

  add_data(mod_id::kCloseCombatWeapon, mod_slot::kWeapon, mod_category::kCloseCombat,
           "Close-combat cannon", "You fire a spread of shots in a close-range burst.");
  add_data(mod_id::kCloseCombatSuper, mod_slot::kSuper, mod_category::kCloseCombat, "Killdozer",
           "Your mega-weapon transforms you into an invulnerable bulldozer for a short time.");
  add_data(mod_id::kCloseCombatBomb, mod_slot::kBomb, mod_category::kCloseCombat, "???", "???");
  add_data(mod_id::kCloseCombatShield, mod_slot::kShield, mod_category::kCloseCombat,
           "Countermeasures", "Your shield triggers your bomb effect when hit.");
  add_data(mod_id::kCloseCombatBonus, mod_slot::kBonus, mod_category::kCloseCombat,
           "Catalytic converter",
           "Whenever you destroy an enemy at close range, you have a chance to gain a bomb.",
           {.category_dependency = mod_category::kCloseCombat});

  add_data(mod_id::kLightningWeapon, mod_slot::kWeapon, mod_category::kLightning, "Spark",
           "Your shots deal bonus damage and stun enemies.");
  add_data(mod_id::kLightningSuper, mod_slot::kSuper, mod_category::kLightning, "Chain lightning",
           "Your mega-weapon is a stream of chain-lightning that stuns and jumps between enemies.");
  add_data(
      mod_id::kLightningBomb, mod_slot::kBomb, mod_category::kLightning, "Thunderball",
      "Your bomb creates a slow-moving mass of ball lightning that damages and stuns enemies.");
  add_data(mod_id::kLightningShield, mod_slot::kShield, mod_category::kLightning, "Discharge",
           "Your shield damages and stuns nearby enemies at random.");
  add_data(mod_id::kLightningBonus, mod_slot::kBonus, mod_category::kLightning, "Feedback",
           "Stunned enemies take increased damage from all sources.",
           {.category_dependency = mod_category::kLightning});

  add_data(mod_id::kSniperWeapon, mod_slot::kWeapon, mod_category::kSniper, "Precision splitter",
           "Your shots multiply after some distance, dealing higher damage at long range.");
  add_data(mod_id::kSniperSuper, mod_slot::kSuper, mod_category::kSniper, "Nuclear incision",
           "Your mega-weapon is a single penetrating shot that deals massive damage.");
  add_data(mod_id::kSniperBomb, mod_slot::kBomb, mod_category::kSniper, "Tactical deployment",
           "Your bomb is fired a short distance.");
  add_data(mod_id::kSniperShield, mod_slot::kShield, mod_category::kSniper, "???", "???");
  add_data(mod_id::kSniperBonus, mod_slot::kBonus, mod_category::kSniper, "Enervation",
           "Whenever you destroy an enemy at long range, you have a chance to fully recharge your "
           "mega-weapon.",
           {.slot_dependency = mod_slot::kSuper, .category_dependency = mod_category::kSniper});

  add_data(mod_id::kLaserWeapon, mod_slot::kWeapon, mod_category::kLaser, "Tightbeam",
           "Your shots are replaced with a continuous laser-beam that penetrates targets.");
  add_data(mod_id::kLaserSuper, mod_slot::kSuper, mod_category::kLaser, "Mega-widebeam",
           "Your mega-weapon is a continuous massive laser-beam.");
  add_data(
      mod_id::kLaserBomb, mod_slot::kBomb, mod_category::kLaser, "Laserstorm",
      "Your bomb is a flurry of laser-beams, dealing moderate damage to all enemies on-screen.");
  add_data(mod_id::kLaserShield, mod_slot::kShield, mod_category::kLaser, "???", "???");
  add_data(mod_id::kLaserBonus, mod_slot::kBonus, mod_category::kLaser, "Optical amplification",
           "Your laser-beams that pass through an enemy deal bonus damage to subsequent targets.",
           {.category_dependency = mod_category::kLaser});

  add_data(
      mod_id::kClusterWeapon, mod_slot::kWeapon, mod_category::kCluster, "Compact rockets",
      "Your shots explode on impact, dealing area-of-effect knockback damage to nearby enemies.");
  add_data(mod_id::kClusterSuper, mod_slot::kSuper, mod_category::kCluster,
           "Very large missile array",
           "Your mega-weapon is a barrage of homing missiles that deal area-of-effect knockback "
           "damage.");
  add_data(mod_id::kClusterBomb, mod_slot::kBomb, mod_category::kCluster, "Cluster payload",
           "Your bomb releases a salvo of cluster grenades that deal heavy knockback damage all "
           "around you.");
  add_data(mod_id::kClusterShield, mod_slot::kShield, mod_category::kCluster, "Disperser",
           "Your shield periodically knocks back nearby enemies.");
  add_data(mod_id::kClusterBonus, mod_slot::kBonus, mod_category::kCluster, "Secondary detonation",
           "Enemies you destroy explode, dealing area-of-effect knockback damage to others nearby.",
           {.category_dependency = mod_category::kCluster});
  return m;
};

}  // namespace

const mod_data& mod_lookup(mod_id id) {
  static const auto map = make_mod_map();
  if (auto it = map.find(id); it != map.end()) {
    return it->second;
  }
  static const mod_data kNone;
  return kNone;
}

ustring_view mod_slot_name(mod_slot slot) {
  switch (slot) {
  case mod_slot::kWeapon:
    return ustring_view::ascii("Weapon");
  case mod_slot::kSuper:
    return ustring_view::ascii("Mega-weapon");
  case mod_slot::kBomb:
    return ustring_view::ascii("Bomb");
  case mod_slot::kShield:
    return ustring_view::ascii("Shield");
  case mod_slot::kBonus:
    return ustring_view::ascii("Bonus");
  case mod_slot::kWeaponCombo:
  case mod_slot::kSuperCombo:
  case mod_slot::kBombCombo:
  case mod_slot::kShieldCombo:
  case mod_slot::kBonusCombo:
    return {};
  }
  return {};
}

ustring_view mod_category_name(mod_category category) {
  switch (category) {
  case mod_category::kGeneral:
    return ustring_view::ascii("General");
  case mod_category::kCorruption:
    return ustring_view::ascii("Corruption");
  case mod_category::kCloseCombat:
    return ustring_view::ascii("Proximity");
  case mod_category::kLightning:
    return ustring_view::ascii("Lightning");
  case mod_category::kSniper:
    return ustring_view::ascii("Sniper");
  case mod_category::kGravity:
    return ustring_view::ascii("Gravity");
  case mod_category::kLaser:
    return ustring_view::ascii("Laser");
  case mod_category::kPummel:
    return ustring_view::ascii("Punisher");
  case mod_category::kRemote:
    return ustring_view::ascii("Remote");
  case mod_category::kUnknown0:
    return ustring_view::ascii("Unknown");
  case mod_category::kUnknown1:
    return ustring_view::ascii("Unknown");
  case mod_category::kDefender:
    return ustring_view::ascii("Defender");
  case mod_category::kCluster:
    return ustring_view::ascii("Cluster");
  }
  return {};
}

cvec4 mod_category_colour(mod_category category) {
  switch (category) {
  case mod_category::kGeneral:
    return colour::kCategoryGeneral;
  case mod_category::kCorruption:
    return colour::kCategoryCorruption;
  case mod_category::kCloseCombat:
    return colour::kCategoryCloseCombat;
  case mod_category::kLightning:
    return colour::kCategoryLightning;
  case mod_category::kSniper:
    return colour::kCategorySniper;
  case mod_category::kGravity:
    return colour::kCategoryGravity;
  case mod_category::kLaser:
    return colour::kCategoryLaser;
  case mod_category::kPummel:
    return colour::kCategoryPummel;
  case mod_category::kRemote:
    return colour::kCategoryRemote;
  case mod_category::kUnknown0:
    return colour::kCategoryUnknown0;
  case mod_category::kUnknown1:
    return colour::kCategoryUnknown1;
  case mod_category::kDefender:
    return colour::kCategoryDefender;
  case mod_category::kCluster:
    return colour::kCategoryCluster;
  }
  return colour::kWhite0;
}

bool is_mod_slot_single_limit(mod_slot slot) {
  switch (slot) {
  case mod_slot::kWeapon:
  case mod_slot::kSuper:
  case mod_slot::kBomb:
  case mod_slot::kShield:
  case mod_slot::kBonus:
    return true;
  case mod_slot::kWeaponCombo:
  case mod_slot::kSuperCombo:
  case mod_slot::kBombCombo:
  case mod_slot::kShieldCombo:
  case mod_slot::kBonusCombo:
    return false;
  }
  return false;
}

}  // namespace ii::v0