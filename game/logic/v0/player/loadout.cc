#include "game/logic/v0/player/loadout.h"
#include "game/common/random.h"
#include "game/logic/sim/io/conditions.h"
#include <algorithm>
#include <unordered_map>
#include <unordered_set>

namespace ii::v0 {
namespace {

std::vector<mod_id> make_mod_list() {
  std::vector<mod_id> v = {
      mod_id::kBackShots,         mod_id::kFrontShots,       mod_id::kBombCapacity,
      mod_id::kBombRadius,        mod_id::kShieldCapacity,   mod_id::kShieldRefill,
      mod_id::kCloseCombatWeapon, mod_id::kCloseCombatSuper, mod_id::kCloseCombatBomb,
      mod_id::kCloseCombatShield, mod_id::kCloseCombatBonus, mod_id::kSniperWeapon,
      mod_id::kSniperSuper,       mod_id::kSniperBomb,       mod_id::kSniperShield,
      mod_id::kSniperBonus,       mod_id::kClusterWeapon,    mod_id::kClusterSuper,
      mod_id::kClusterBomb,       mod_id::kClusterShield,    mod_id::kClusterBonus};
  return v;
};

}  // namespace

std::vector<mod_id> mod_selection(const initial_conditions& conditions, RandomEngine& random,
                                  std::span<const player_loadout> loadouts) {
  std::unordered_map<mod_category, std::uint32_t> loadout_category_counts;
  std::unordered_map<mod_slot, std::uint32_t> loadout_slot_counts;
  std::unordered_map<mod_id, std::uint32_t> loadout_mod_counts;
  for (const auto& loadout : loadouts) {
    for (auto id : loadout) {
      const auto& data = mod_lookup(id);
      ++loadout_category_counts[data.category];
      ++loadout_slot_counts[data.slot];
      ++loadout_mod_counts[id];
    }
  }

  auto is_mod_allowed = [&](const mod_data& data) {
    return (data.options.allow_multiple || data.options.allow_multiple_per_player ||
            !loadout_mod_counts[data.id]) &&
        (data.options.allow_multiple_per_player || loadout_mod_counts[data.id] < loadouts.size()) &&
        (!data.options.multiplayer_only || loadouts.size() > 1u) &&
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
    auto it = loadout_category_counts.find(data.category);
    return it != loadout_category_counts.end() && it->second > 0 ? 2u : 1u;
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

  auto select_mod = [&](std::optional<mod_category> required_category,
                        bool ignore_selected) -> const mod_data* {
    mod_slot_map slot_mods;
    for (auto id : mod_list) {
      const auto& data = mod_lookup(id);
      bool selected_slot = selected_slots.contains(data.slot);
      bool selected_category = selected_categories.contains(data.category);
      bool already_selected = selected_slot || selected_category;
      bool is_required = required_category && data.category == required_category;
      if (is_mod_allowed(data) && (!required_category || is_required) &&
          (ignore_selected || !already_selected || (is_required && !selected_slot))) {
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
    if (const auto* mod_data = select_mod(it->first, false); mod_data) {
      add_result(*mod_data);
    }
  }
  while (result.size() < 4u) {
    const auto* mod_data = select_mod(std::nullopt, false);
    if (!mod_data) {
      break;
    }
    add_result(*mod_data);
  }
  while (result.size() < 4u) {
    const auto* mod_data = select_mod(std::nullopt, /* ignore slots + categories */ true);
    if (!mod_data) {
      break;
    }
    add_result(*mod_data);
  }
  return result;
}

void PlayerLoadout::add_mod(mod_id id) {
  loadout.emplace_back(id);
}

std::uint32_t PlayerLoadout::max_shield_capacity(const SimInterface&) const {
  return 3u;  // TODO
}

std::uint32_t PlayerLoadout::max_bomb_capacity(const SimInterface&) const {
  return 3u;  // TODO
}

}  // namespace ii::v0