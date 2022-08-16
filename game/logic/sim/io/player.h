#ifndef II_GAME_LOGIC_SIM_IO_PLAYER_H
#define II_GAME_LOGIC_SIM_IO_PLAYER_H
#include "game/common/math.h"
#include <glm/glm.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace ii {
static constexpr std::uint32_t kMaxPlayers = 4;

inline glm::vec4 player_colour(std::size_t player_number) {
  return colour_hue360((20 * player_number) % 360);
}

struct input_frame {
  enum key : std::uint32_t {
    kFire = 1,
    kBomb = 2,
  };
  vec2 velocity{0};
  std::optional<vec2> target_absolute;
  std::optional<vec2> target_relative;
  std::uint32_t keys = 0;
};

}  // namespace ii

#endif
