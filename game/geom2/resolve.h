#ifndef II_GAME_GEOM2_RESOLVE_H
#define II_GAME_GEOM2_RESOLVE_H
#include "game/geom2/enums.h"
#include "game/geom2/transform.h"
#include <variant>
#include <vector>

namespace ii::geom2 {

struct resolve_result {
  struct line_style {
    cvec4 colour0{0.f};
    cvec4 colour1{0.f};
    float z = 0.f;
    float width = 1.f;
  };

  struct fill_style {
    cvec4 colour0{0.f};
    cvec4 colour1{0.f};
    float z = 0.f;
  };

  struct ball_dimensions {
    fixed radius = 0_fx;
    fixed inner_radius = 0_fx;
  };

  struct ngon_dimensions {
    fixed radius = 0_fx;
    fixed inner_radius = 0_fx;
    std::uint32_t sides = 0u;
    std::uint32_t segments = sides;
  };

  struct ball {
    ball_dimensions dimensions;
    line_style line;
    fill_style fill;
    tag_t tag{'0'};
    render::flag flags = render::flag::kNone;
  };

  struct box {
    vec2 dimensions{0};
    line_style line;
    fill_style fill;
    tag_t tag{'0'};
    render::flag flags = render::flag::kNone;
  };

  struct line {
    vec2 a{0};
    vec2 b{0};
    line_style style;
    tag_t tag{'0'};
    render::flag flags = render::flag::kNone;
  };

  struct ngon {
    ngon_dimensions dimensions;
    ngon_style style = ngon_style::kPolygon;
    line_style line;
    fill_style fill;
    tag_t tag{'0'};
    render::flag flags = render::flag::kNone;
  };

  using shape_data = std::variant<ball, box, line, ngon>;
  struct entry {
    transform t;
    shape_data data;
  };
  std::vector<entry> entries;

  template <typename T>
  void add(const transform& t, T&& data) {
    entries.emplace_back(entry{t, data});
  }
};

}  // namespace ii::geom2

#endif
