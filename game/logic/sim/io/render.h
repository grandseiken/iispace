#ifndef II_GAME_LOGIC_SIM_IO_RENDER_H
#define II_GAME_LOGIC_SIM_IO_RENDER_H
#include "game/render/data/shapes.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

namespace ii::render {

struct player_info {
  glm::vec4 colour{0.f};
  std::uint64_t score = 0;
  std::uint32_t multiplier = 0;
  float timer = 0.f;
};

struct entity_state {
  std::unordered_map<unsigned char, std::vector<std::optional<motion_trail>>> trails;
};

}  // namespace ii::render

#endif
