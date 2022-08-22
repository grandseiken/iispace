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

enum class ngon_style {
  kPolygon,
  kPolystar,
  kPolygram,
};

struct line {
  glm::vec2 a{0.f};
  glm::vec2 b{0.f};
  glm::vec4 colour{0.f};
  float line_width = 1.f;
  std::uint32_t sides = 4;
};

struct ngon {
  glm::vec2 origin{0.f};
  float rotation = 0.f;
  float radius = 0.f;
  std::uint32_t sides = 0;
  glm::vec4 colour{0.f};
  float line_width = 1.f;
  ngon_style style = ngon_style::kPolygon;
};

struct box {
  glm::vec2 origin{0.f};
  float rotation = 0.f;
  glm::vec2 dimensions{0.f};
  glm::vec4 colour{0.f};
  float line_width = 1.f;
};

struct polyarc {
  glm::vec2 origin{0.f};
  float rotation = 0.f;
  float radius = 0;
  std::uint32_t sides = 0;
  std::uint32_t segments = 0;
  glm::vec4 colour{0.f};
  float line_width = 1.f;
};

using shape_data = std::variant<line, ngon, box, polyarc>;

struct shape {
  std::optional<glm::vec4> colour_override;
  shape_data data;

  static shape line(const glm::vec2& a, const glm::vec2& b, const glm::vec4& c) {
    shape s;
    s.data = render::line{a, b, c};
    return s;
  }
};

}  // namespace ii::render

#endif
