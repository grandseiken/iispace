#ifndef II_GAME_LOGIC_V0_OVERMIND_WAVE_DATA_H
#define II_GAME_LOGIC_V0_OVERMIND_WAVE_DATA_H
#include "game/logic/sim/io/render.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <optional>

namespace ii::v0 {

enum class wave_type {
  kEnemy,
  kBoss,
  kUpgrade,
};

struct background_input {
  bool initialise = false;
  std::uint64_t tick_count = 0;
  std::uint32_t biome_index = 0;
  std::optional<std::uint32_t> wave_number;
};

struct background_update {
  render::background::type type = render::background::type::kNone;
  glm::vec4 colour{0.f};
  glm::vec2 parameters{0.f};
  glm::vec4 position_delta{0.f};
  float rotation_delta{0.f};
};

struct wave_id {
  std::uint32_t biome_index = 0;
  std::uint32_t wave_number = 0;
};

struct wave_data {
  wave_type type = wave_type::kEnemy;
  std::uint32_t biome_index = 0;
  std::uint32_t power = 0;
  std::uint32_t upgrade_budget = 0;
  std::uint32_t threat_trigger = 0;
};

}  // namespace ii::v0

#endif
