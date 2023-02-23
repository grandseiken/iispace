#ifndef II_GAME_GEOMETRY_RESOLVE_H
#define II_GAME_GEOMETRY_RESOLVE_H
#include "game/geometry/types.h"
#include <variant>
#include <vector>

namespace ii::geom {
namespace detail {

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
  tag_t tag = tag_t{'0'};
  render_flag flags = render_flag::kNone;
};

struct box {
  vec2 dimensions{0};
  line_style line;
  fill_style fill;
  tag_t tag = tag_t{'0'};
  render_flag flags = render_flag::kNone;
};

struct line {
  vec2 a{0};
  vec2 b{0};
  line_style style;
  tag_t tag = tag_t{'0'};
  render_flag flags = render_flag::kNone;
};

struct ngon {
  ngon_dimensions dimensions;
  ngon_style style = ngon_style::kPolygon;
  line_style line;
  fill_style fill;
  tag_t tag = tag_t{'0'};
  render_flag flags = render_flag::kNone;
};

}  // namespace detail

struct resolve_result {
  using line_style = detail::line_style;
  using fill_style = detail::fill_style;
  using ball_dimensions = detail::ball_dimensions;
  using ngon_dimensions = detail::ngon_dimensions;

  using ball = detail::ball;
  using box = detail::box;
  using line = detail::line;
  using ngon = detail::ngon;

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

}  // namespace ii::geom

#endif
