#ifndef II_GAME_LOGIC_SIM_IO_RENDER_H
#define II_GAME_LOGIC_SIM_IO_RENDER_H
#include <glm/glm.hpp>
#include <cstdint>
#include <optional>
#include <variant>

namespace ii::render {

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

using shape_data = std::variant<line_t>;

struct shape {
  std::optional<glm::vec4> colour_override;
  shape_data data;

  static shape line(const glm::vec2& a, const glm::vec2& b, const glm::vec4& c) {
    shape s;
    s.data = line_t{a, b, c};
    return s;
  }
};

}  // namespace ii::render

#endif
