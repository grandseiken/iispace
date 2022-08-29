#ifndef II_GAME_LOGIC_SIM_IO_RENDER_H
#define II_GAME_LOGIC_SIM_IO_RENDER_H
#include "game/common/math.h"
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
  float radius = 0.f;
  float line_width = 1.f;
  std::uint32_t sides = 4;
};

struct ngon {
  float radius = 0.f;
  std::uint32_t sides = 0;
  ngon_style style = ngon_style::kPolygon;
  float line_width = 1.f;
};

struct box {
  glm::vec2 dimensions{0.f};
  float line_width = 1.f;
};

struct polyarc {
  float radius = 0.f;
  std::uint32_t sides = 0;
  std::uint32_t segments = 0;
  float line_width = 1.f;
};

using shape_data = std::variant<line, ngon, box, polyarc>;

struct motion_trail {
  glm::vec2 prev_origin{0.f};
  float prev_rotation = 0.f;
};

struct shape {
  glm::vec2 origin{0.f};
  float rotation = 0.f;
  glm::vec4 colour{0.f};
  std::optional<float> z_index;
  std::optional<motion_trail> trail;
  shape_data data;

  static shape line(const glm::vec2& a, const glm::vec2& b, const glm::vec4& c) {
    return shape{
        .origin = (a + b) / 2.f,
        .rotation = angle(b - a),
        .colour = c,
        .data = render::line{.radius = distance(a, b) / 2.f},
    };
  }
};

}  // namespace ii::render

#endif
