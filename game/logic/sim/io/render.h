#ifndef II_GAME_LOGIC_SIM_IO_RENDER_H
#define II_GAME_LOGIC_SIM_IO_RENDER_H
#include "game/common/math.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <variant>
#include <vector>

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
  std::uint32_t segments = sides;
  ngon_style style = ngon_style::kPolygon;
  float line_width = 1.f;
};

struct box {
  glm::vec2 dimensions{0.f};
  float line_width = 1.f;
};

struct ngon_fill {
  float radius = 0.f;
  std::uint32_t sides = 0;
  std::uint32_t segments = sides;
};

struct box_fill {
  glm::vec2 dimensions{0.f};
};

using shape_data = std::variant<line, ngon, box, ngon_fill, box_fill>;

struct motion_trail {
  glm::vec2 prev_origin{0.f};
  float prev_rotation = 0.f;
  glm::vec4 prev_colour{0.f};
};

struct shape {
  glm::vec2 origin{0.f};
  float rotation = 0.f;
  glm::vec4 colour{0.f};
  std::optional<float> z_index;
  std::optional<motion_trail> trail;
  bool disable_trail = false;
  shape_data data;

  static shape line(const glm::vec2& a, const glm::vec2& b, const glm::vec4& c, float z = 0.f,
                    float width = 1.f) {
    return shape{
        .origin = (a + b) / 2.f,
        .rotation = angle(b - a),
        .colour = c,
        .z_index = z,
        .data = render::line{.radius = distance(a, b) / 2.f, .line_width = width},
    };
  }

  void apply_hit_flash(float a) {
    z_index = a + z_index.value_or(0.f);
    auto w = std::max(0.f, 3.f * (a - .5f));
    auto l = a / 4.f;
    if (auto* p = std::get_if<render::ngon>(&data); p) {
      p->line_width += w;
    } else if (auto* p = std::get_if<render::box>(&data); p) {
      p->line_width += w;
    } else if (auto* p = std::get_if<render::line>(&data); p) {
      p->line_width += w;
    }
    colour.z = std::min(1.f, colour.z + l);
  }
};

struct entity_state {
  std::vector<std::optional<motion_trail>> trails;
};

}  // namespace ii::render

#endif
