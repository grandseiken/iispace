#ifndef II_GAME_LOGIC_SIM_IO_RENDER_H
#define II_GAME_LOGIC_SIM_IO_RENDER_H
#include <glm/glm.hpp>
#include <cstdint>
#include <optional>
#include <vector>

namespace ii {

struct render_output {
  struct player_info {
    glm::vec4 colour{0.f};
    std::uint64_t score = 0;
    std::uint32_t multiplier = 0;
    float timer = 0.f;
  };
  struct line_t {
    glm::vec2 a{0.f};
    glm::vec2 b{0.f};
    glm::vec4 c{0.f};
  };
  std::vector<player_info> players;
  std::vector<line_t> lines;
  std::uint64_t tick_count = 0;
  std::uint32_t lives_remaining = 0;
  std::optional<std::uint32_t> overmind_timer;
  std::optional<float> boss_hp_bar;
  std::uint32_t colour_cycle = 0;
};

}  // namespace ii

#endif
