#include "game/logic/v0/player/loadout.h"
#include "game/common/random.h"
#include "game/logic/sim/components.h"
#include "game/logic/sim/io/conditions.h"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace ii::v0 {
namespace {

std::vector<mod_id> make_mod_list() {
  std::vector<mod_id> v = {
      mod_id::kBackShots, mod_id::kFrontShots,
      // mod_id::kBounceShots,
      mod_id::kHomingShots,
      // mod_id::kSuperCapacity, mod_id::kSuperRefill,
      mod_id::kBombCapacity, mod_id::kBombRadius,
      // mod_id::kBombSpeedClearCharge,
      mod_id::kBombDoubleTrigger, mod_id::kShieldCapacity, mod_id::kShieldRefill,
      mod_id::kShieldRespawn,
      // mod_id::kPowerupDrops, mod_id::kCurrencyDrops,
      // mod_id::kCorruptionWeapon, mod_id::kCorruptionSuper, mod_id::kCorruptionBomb,
      // mod_id::kCorruptionShield, mod_id::kCorruptionBonus,
      mod_id::kCloseCombatWeapon,
      // mod_id::kCloseCombatSuper, mod_id::kCloseCombatBomb,
      mod_id::kCloseCombatShield,
      // mod_id::kCloseCombatBonus,
      mod_id::kLightningWeapon,
      // mod_id::kLightningSuper, mod_id::kLightningBomb,
      // mod_id::kLightningShield, mod_id::kLightningBonus,
      mod_id::kSniperWeapon,
      // mod_id::kSniperSuper, mod_id::kSniperBomb,
      // mod_id::kSniperShield, mod_id::kSniperBonus,
      // mod_id::kLaserWeapon, mod_id::kLaserSuper, mod_id::kLaserBomb,
      // mod_id::kLaserShield, mod_id::kLaserBonus,
      // mod_id::kClusterWeapon, mod_id::kClusterSuper, mod_id::kClusterBomb,
      // mod_id::kClusterShield, mod_id::kClusterBonus
  };
  return v;
};

}  // namespace

std::vector<mod_id> mod_selection(const initial_conditions& conditions, RandomEngine& random,
                                  const player_loadout& combined_loadout) {
  std::unordered_map<mod_category, std::uint32_t> loadout_category_counts;
  std::unordered_map<mod_slot, std::uint32_t> loadout_slot_counts;
  std::unordered_map<mod_id, std::uint32_t> loadout_mod_counts;
  for (const auto& pair : combined_loadout) {
    const auto& data = mod_lookup(pair.first);
    ++loadout_category_counts[data.category];
    ++loadout_slot_counts[data.slot];
    ++loadout_mod_counts[pair.first];
  }

  auto is_mod_allowed = [&](const mod_data& data) {
    return (data.options.allow_multiple || data.options.allow_multiple_per_player ||
            !loadout_mod_counts[data.id]) &&
        (data.options.allow_multiple_per_player ||
         loadout_mod_counts[data.id] < conditions.player_count) &&
        (!data.options.multiplayer_only || conditions.player_count > 1u) &&
        (!data.options.category_dependency ||
         loadout_category_counts[*data.options.category_dependency]) &&
        (!data.options.slot_dependency || loadout_slot_counts[*data.options.slot_dependency]);
  };

  static const auto mod_list = make_mod_list();
  std::unordered_set<mod_slot> selected_slots;
  std::unordered_set<mod_category> selected_categories;
  using mod_slot_data = std::vector<const mod_data*>;
  using mod_slot_map = std::unordered_map<mod_slot, mod_slot_data>;

  auto slot_weight = [&](mod_slot slot, std::size_t available_count) -> std::uint32_t {
    if (available_count <= 1u) {
      return static_cast<std::uint32_t>(available_count);
    }
    std::uint32_t loadout_slot_count = 0u;
    if (auto it = loadout_slot_counts.find(slot); it != loadout_slot_counts.end()) {
      loadout_slot_count = it->second;
    }

    switch (slot) {
    case mod_slot::kWeapon:
    case mod_slot::kSuper:
    case mod_slot::kBomb:
    case mod_slot::kShield:
      if (conditions.player_count == 1u) {
        return loadout_slot_count ? 1u : 5u;
      } else if (conditions.player_count == 2u) {
        return loadout_slot_count >= 2u ? 1u : loadout_slot_count >= 1u ? 4u : 5u;
      } else if (conditions.player_count == 3u) {
        return loadout_slot_count >= 3u ? 1u : loadout_slot_count >= 2u ? 3u : 5u;
      } else {
        return loadout_slot_count >= 4u ? 1u
            : loadout_slot_count >= 3u  ? 2u
            : loadout_slot_count >= 2u  ? 3u
                                        : 5u;
      }
    case mod_slot::kBonus:
    case mod_slot::kWeaponCombo:
    case mod_slot::kSuperCombo:
    case mod_slot::kBombCombo:
    case mod_slot::kShieldCombo:
    case mod_slot::kBonusCombo:
      return 2u;
    }
    return 0u;
  };

  auto mod_weight = [&](const mod_data& data) -> std::uint32_t {
    auto count = data.options.allow_multiple_per_player ? conditions.player_count : 1u;
    auto it = loadout_mod_counts.find(data.id);
    std::uint32_t weight = it != loadout_mod_counts.end() && it->second >= count ? 1u : 2u;
    if (data.category != mod_category::kGeneral) {
      auto it = loadout_category_counts.find(data.category);
      if (it != loadout_category_counts.end() && it->second > 0u) {
        weight *= 2u;
      }
    }
    return weight;
  };

  auto select_mod_from_map = [&](const mod_slot_map& slot_mods) -> const mod_data* {
    std::uint32_t total_weight = 0;
    for (const auto& pair : slot_mods) {
      total_weight += slot_weight(pair.first, pair.second.size());
    }
    if (!total_weight) {
      return nullptr;
    }
    auto r = random.uint(total_weight);
    auto slot = mod_slot::kBonusCombo;
    total_weight = 0;
    for (const auto& pair : slot_mods) {
      total_weight += slot_weight(pair.first, pair.second.size());
      if (r < total_weight) {
        slot = pair.first;
        break;
      }
    }

    const auto& mods = slot_mods.find(slot)->second;
    total_weight = 0;
    for (const auto* data : mods) {
      total_weight += mod_weight(*data);
    }
    if (!total_weight) {
      return nullptr;
    }
    r = random.uint(total_weight);
    total_weight = 0;
    for (const auto* data : mods) {
      total_weight += mod_weight(*data);
      if (r < total_weight) {
        return data;
      }
    }
    return nullptr;
  };

  auto select_mod = [&](std::optional<mod_category> required_category) -> const mod_data* {
    mod_slot_map slot_mods;
    for (auto id : mod_list) {
      const auto& data = mod_lookup(id);
      bool selected_slot = selected_slots.contains(data.slot);
      bool selected_category =
          data.category != mod_category::kGeneral && selected_categories.contains(data.category);
      bool already_selected = selected_slot || selected_category;
      bool is_required = required_category && data.category == required_category;
      if (is_mod_allowed(data) && (!required_category || is_required) &&
          (!already_selected || (is_required && !selected_slot))) {
        slot_mods[data.slot].emplace_back(&data);
      }
    }
    return select_mod_from_map(slot_mods);
  };

  std::vector<mod_id> result;
  auto add_result = [&](const mod_data& data) {
    result.emplace_back(data.id);
    selected_slots.emplace(data.slot);
    selected_categories.emplace(data.category);
  };

  auto r = random.uint(1 + static_cast<std::uint32_t>(loadout_category_counts.size()));
  auto it = loadout_category_counts.begin();
  for (std::uint32_t i = 0; i < r && it != loadout_category_counts.end(); ++i) {
    ++it;
  }
  if (it != loadout_category_counts.end()) {
    if (const auto* mod_data = select_mod(it->first); mod_data) {
      add_result(*mod_data);
    }
  }
  while (true) {
    while (result.size() < 4u) {
      const auto* mod_data = select_mod(std::nullopt);
      if (!mod_data) {
        break;
      }
      add_result(*mod_data);
    }
    if (!selected_slots.empty()) {
      selected_slots.clear();
    } else if (!selected_categories.empty()) {
      selected_categories.clear();
    } else {
      break;
    }
  }

  for (std::uint32_t i = 0; i + 1 < result.size(); ++i) {
    auto j = i + random.uint(result.size() - i);
    std::swap(result[i], result[j]);
  }
  return result;
}

void PlayerLoadout::add(ecs::handle h, mod_id id, const SimInterface& sim) {
  const auto& data = mod_lookup(id);
  if (is_mod_slot_single_limit(data.slot)) {
    for (auto it = loadout.begin(); it != loadout.end();) {
      if (mod_lookup(it->first).slot == data.slot) {
        it = loadout.erase(it);
      } else {
        ++it;
      }
    }
  }
  if (data.options.allow_multiple_per_player) {
    ++loadout[id];
  } else {
    loadout[id] = 1u;
  }

  auto& pc = *h.get<Player>();
  if (id == mod_id::kBombCapacity) {
    ++pc.bomb_count;
  } else if (id == mod_id::kShieldCapacity) {
    ++pc.shield_count;
  }
  pc.bomb_count = std::min(pc.bomb_count, max_bomb_capacity(sim));
  pc.shield_count = std::min(pc.shield_count, max_shield_capacity(sim));
}

bool PlayerLoadout::has(mod_id id) const {
  return count(id);
}

std::uint32_t PlayerLoadout::count(mod_id id) const {
  auto it = loadout.find(id);
  return it == loadout.end() ? 0u : it->second;
}

std::uint32_t PlayerLoadout::max_shield_capacity(const SimInterface&) const {
  return has(mod_id::kShieldRefill) ? 1u : 2u + count(mod_id::kShieldCapacity);
}

std::uint32_t PlayerLoadout::max_bomb_capacity(const SimInterface&) const {
  return 2u + count(mod_id::kBombCapacity);
}

std::uint32_t PlayerLoadout::shot_fire_timer() const {
  if (has(mod_id::kCloseCombatWeapon)) {
    return has(mod_id::kFrontShots) ? 48u : 30u;
  }
  return has(mod_id::kFrontShots) ? 8u : 5u;
}

fixed PlayerLoadout::bomb_radius_multiplier() const {
  auto r = 1_fx;
  auto c = count(mod_id::kBombRadius);
  switch (c) {
  default:
    r += (c - 3) / 20_fx;
  case 3:
    r += 2 / 20_fx;
  case 2:
    r += 3 / 20_fx;
  case 1:
    r += 4 / 20_fx;
  case 0:
    break;
  }
  return r;
}

auto PlayerLoadout::check_viability(mod_id id) const -> std::pair<viability, mod_id> {
  const auto& data = mod_lookup(id);
  if (!data.options.allow_multiple_per_player && has(id)) {
    return {viability::kAlreadyHave, mod_id::kNone};
  }

  if (is_mod_slot_single_limit(data.slot)) {
    for (const auto& pair : loadout) {
      if (mod_lookup(pair.first).slot == data.slot) {
        return {viability::kReplacesMod, pair.first};
      }
    }
  }
  // TODO: implement kNoEffectYet.
  return {viability::kOk, mod_id::kNone};
}

}  // namespace ii::v0