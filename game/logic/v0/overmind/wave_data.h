#ifndef II_GAME_LOGIC_V0_OVERMIND_WAVE_DATA_H
#define II_GAME_LOGIC_V0_OVERMIND_WAVE_DATA_H
#include "game/common/math.h"
#include "game/common/struct_tuple.h"
#include "game/render/data/background.h"
#include <cstdint>
#include <optional>

namespace ii::v0 {

enum class wave_type {
  kEnemy,
  kBoss,
  kUpgrade,
  kRunComplete,
};

struct background_input {
  bool initialise = false;
  std::uint64_t tick_count = 0;
  std::uint32_t biome_index = 0;
  std::optional<std::uint32_t> wave_number;
};

struct background_update {
  render::background::type type = render::background::type::kNone;
  cvec4 colour{0.f};
  fvec2 parameters{0.f};
  fvec4 position_delta{0.f};
  float rotation_delta{0.f};
};
DEBUG_STRUCT_TUPLE(background_update, type);

struct wave_id {
  std::uint32_t biome_index = 0;
  std::uint32_t wave_number = 0;
};
DEBUG_STRUCT_TUPLE(wave_id, biome_index, wave_number);

struct wave_data {
  wave_type type = wave_type::kEnemy;
  std::uint32_t biome_index = 0;
  std::uint32_t power = 0;
  std::uint32_t upgrade_budget = 0;
  std::uint32_t threat_trigger = 0;
};
DEBUG_STRUCT_TUPLE(wave_data, type, biome_index, power, upgrade_budget, threat_trigger);

}  // namespace ii::v0

#endif
