#ifndef II_GAME_RENDER_DATA_SHAPES_H
#define II_GAME_RENDER_DATA_SHAPES_H
#include "game/common/colour.h"
#include "game/common/math.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <optional>
#include <variant>

namespace ii::render {

enum class shape_style {
  kNone,
  kStandard,
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
  float inner_radius = 0.f;
  std::uint32_t sides = 0;
  std::uint32_t segments = sides;
  ngon_style style = ngon_style::kPolygon;
  float line_width = 1.f;
};

struct box {
  glm::vec2 dimensions{0.f};
  float line_width = 1.f;
};

struct ball {
  float radius = 0.f;
  float inner_radius = 0.f;
  float line_width = 1.f;
};

struct ngon_fill {
  float radius = 0.f;
  float inner_radius = 0.f;
  std::uint32_t sides = 0;
  std::uint32_t segments = sides;
};

struct box_fill {
  glm::vec2 dimensions{0.f};
};

struct ball_fill {
  float radius = 0.f;
  float inner_radius = 0.f;
};

using shape_data = std::variant<line, ngon, box, ball, ngon_fill, box_fill, ball_fill>;

struct motion_trail {
  glm::vec2 prev_origin{0.f};
  float prev_rotation = 0.f;
  glm::vec4 prev_colour{0.f};
};

struct shape {
  glm::vec2 origin{0.f};
  float rotation = 0.f;
  glm::vec4 colour{0.f};
  float z_index = 0.f;
  unsigned char s_index = 0;
  std::optional<motion_trail> trail;
  shape_data data;

  static shape line(const glm::vec2& a, const glm::vec2& b, const glm::vec4& c, float z = 0.f,
                    float width = 1.f, unsigned char index = 0) {
    return shape{
        .origin = (a + b) / 2.f,
        .rotation = angle(b - a),
        .colour = c,
        .z_index = z,
        .s_index = index,
        .data = render::line{.radius = distance(a, b) / 2.f, .line_width = width},
    };
  }

  void apply_hit_flash(float a) {
    z_index += a;
    auto w = std::max(0.f, 3.f * (a - .5f));
    auto l = a / 4.f;
    if (auto* p = std::get_if<render::ngon>(&data); p) {
      p->line_width += w;
    } else if (auto* p = std::get_if<render::box>(&data); p) {
      p->line_width += w;
    } else if (auto* p = std::get_if<render::ball>(&data); p) {
      p->line_width += w;
    } else if (auto* p = std::get_if<render::line>(&data); p) {
      p->line_width += w;
    }
    colour.z = std::min(1.f, colour.z + l);
  }

  bool apply_shield(shape& copy, float a) {
    if (auto* p = std::get_if<render::ngon>(&copy.data); p) {
      if (p->style == ngon_style::kPolystar) {
        return false;
      }
      p->line_width *= -.75f;
    } else if (auto* p = std::get_if<render::box>(&copy.data); p) {
      p->line_width *= -.75f;
    } else if (auto* p = std::get_if<render::ball>(&copy.data); p) {
      p->line_width *= -.75f;
    } else if (auto* p = std::get_if<render::line>(&copy.data); p) {
      p->line_width *= -.75f;
    } else {
      return false;
    }
    copy.colour = colour::alpha(copy.colour, a);
    copy.s_index += 'S';
    z_index += a;
    colour = colour::hsl_mix(colour, colour::kWhite1, a * a);
    return true;
  }
};

}  // namespace ii::render

#endif