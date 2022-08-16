#include "game/logic/boss/boss_internal.h"
#include "game/logic/ecs/call.h"
#include "game/logic/sim/io/result_events.h"
#include <algorithm>

namespace {
constexpr fixed kHpPerExtraPlayer = 1_fx / 10;
constexpr fixed kHpPerExtraCycle = 3 * 1_fx / 10;
}  // namespace

namespace ii {

std::uint32_t calculate_boss_score(boss_flag boss, std::uint32_t players, std::uint32_t cycle) {
  std::uint32_t s =
      5000 * (cycle + 1) + 2500 * (boss > boss_flag::kBoss1C) + 2500 * (boss > boss_flag::kBoss2C);
  std::uint32_t r = s;
  for (std::uint32_t i = 0; i < players - 1; ++i) {
    r += (s * kHpPerExtraPlayer).to_int();
  }
  return r;
}

std::uint32_t calculate_boss_hp(std::uint32_t base, std::uint32_t players, std::uint32_t cycle) {
  auto hp = base * 30;
  auto r = hp;
  for (std::uint32_t i = 0; i < cycle; ++i) {
    r += (hp * kHpPerExtraCycle).to_int();
  }
  for (std::uint32_t i = 0; i < players - 1; ++i) {
    r += (hp * kHpPerExtraPlayer).to_int();
  }
  for (std::uint32_t i = 0; i < cycle * (players - 1); ++i) {
    r += (hp * kHpPerExtraCycle + kHpPerExtraPlayer).to_int();
  }
  return r;
}

std::uint32_t scale_boss_damage(ecs::handle, SimInterface& sim, damage_type, std::uint32_t damage) {
  return damage * (60 / (1 + (sim.get_lives() ? sim.player_count() : sim.alive_players())));
}

}  // namespace ii
