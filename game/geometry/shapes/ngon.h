#ifndef II_GAME_GEOMETRY_SHAPES_NGON_H
#define II_GAME_GEOMETRY_SHAPES_NGON_H
#include "game/common/collision.h"
#include "game/geometry/expressions.h"
#include "game/geometry/node.h"
#include "game/geometry/shapes/data.h"
#include "game/render/data/shapes.h"
#include <array>
#include <cstddef>
#include <cstdint>

namespace ii::geom {
inline namespace shapes {
using render::ngon_style;

struct ngon_dimensions {
  fixed radius = 0;
  fixed inner_radius = 0;
  std::uint32_t sides = 0;
  std::uint32_t segments = sides;
};

struct ngon_line_style : line_style {
  ngon_style style = ngon_style::kPolygon;
};

constexpr ngon_dimensions nd(fixed radius, std::uint32_t sides, std::uint32_t segments = 0) {
  return {radius, 0, sides, segments ? segments : sides};
}

constexpr ngon_dimensions
nd2(fixed radius, fixed inner_radius, std::uint32_t sides, std::uint32_t segments = 0) {
  return {radius, inner_radius, sides, segments ? segments : sides};
}

constexpr ngon_line_style
nline(const cvec4& colour = cvec4{0.f}, float z = 0.f, float width = 1.f, unsigned char index = 0) {
  return {{.colour0 = colour, .colour1 = colour, .z = z, .width = width, .index = index}};
}

constexpr ngon_line_style nline(const cvec4& colour0, const cvec4& colour1, float z = 0.f,
                                float width = 1.f, unsigned char index = 0) {
  return {{.colour0 = colour0, .colour1 = colour1, .z = z, .width = width, .index = index}};
}

constexpr ngon_line_style nline(ngon_style style, const cvec4& colour, float z = 0.f,
                                float width = 1.f, unsigned char index = 0) {
  return {{.colour0 = colour, .colour1 = colour, .z = z, .width = width, .index = index}, style};
}

constexpr ngon_line_style nline(ngon_style style, const cvec4& colour0, const cvec4& colour1,
                                float z = 0.f, float width = 1.f, unsigned char index = 0) {
  return {{.colour0 = colour0, .colour1 = colour1, .z = z, .width = width, .index = index}, style};
}

//////////////////////////////////////////////////////////////////////////////////
// Collider.
//////////////////////////////////////////////////////////////////////////////////
struct ngon_collider_data : shape_data_base {
  using shape_data_base::iterate;
  ngon_dimensions dimensions;
  shape_flag flags = shape_flag::kNone;

  constexpr void iterate(iterate_flags_t, const null_transform&, FlagFunction auto&& f) const {
    std::invoke(f, flags);
  }

  void
  iterate(iterate_check_collision_t it, const convert_local_transform& t, hit_result& hit) const;
};

constexpr ngon_collider_data
make_ngon_collider(ngon_dimensions dimensions, shape_flag flags = shape_flag::kNone) {
  return {{}, dimensions, flags};
}

template <Expression<ngon_dimensions>, Expression<shape_flag> Flags = constant<shape_flag::kNone>>
struct ngon_collider_eval {};

template <Expression<ngon_dimensions> Dimensions, Expression<shape_flag> Flags>
constexpr auto evaluate(ngon_collider_eval<Dimensions, Flags>, const auto& params) {
  return make_ngon_collider(evaluate(Dimensions{}, params), shape_flag{evaluate(Flags{}, params)});
}

//////////////////////////////////////////////////////////////////////////////////
// Render shape.
//////////////////////////////////////////////////////////////////////////////////
struct ngon_data : shape_data_base {
  using shape_data_base::iterate;
  ngon_dimensions dimensions;
  ngon_line_style line;
  fill_style fill;
  render::flag flags = render::flag::kNone;

  constexpr void iterate(iterate_lines_t, const Transform auto& t, LineFunction auto&& f) const {
    if (!line.colour0.a) {
      return;
    }
    auto vertex = [&](std::uint32_t i) {
      return *t.rotate(i * 2 * pi<fixed> / dimensions.sides).translate({dimensions.radius, 0});
    };
    auto ivertex = [&](std::uint32_t i) {
      return *t.rotate(i * 2 * pi<fixed> / dimensions.sides)
                  .translate({dimensions.inner_radius, 0});
    };

    switch (line.style) {
    case ngon_style::kPolystar:
      // TODO: need line gradients to match rendering, if we use them.
      for (std::uint32_t i = 0; i < dimensions.segments; ++i) {
        std::invoke(f, ivertex(i), vertex(i), line.colour0, line.width, line.z);
      }
      break;
    case ngon_style::kPolygram:
      for (std::size_t i = 0; i < dimensions.sides; ++i) {
        for (std::size_t j = i + 2; j < dimensions.sides && (j + 1) % dimensions.sides != i; ++j) {
          std::invoke(f, vertex(i), vertex(j), line.colour1, line.width, line.z);
        }
      }
      // Fallthrough.
    case ngon_style::kPolygon:
      for (std::uint32_t i = 0; i < dimensions.segments; ++i) {
        std::invoke(f, vertex(i), vertex(i + 1), line.colour0, line.width, line.z);
      }
      for (std::uint32_t i = 0; dimensions.inner_radius && i < dimensions.segments; ++i) {
        std::invoke(f, ivertex(i), ivertex(i + 1), line.colour1, line.width, line.z);
      }
      break;
    }
  }

  constexpr void iterate(iterate_shapes_t, const Transform auto& t, ShapeFunction auto&& f) const {
    if (line.colour0.a || line.colour1.a) {
      std::invoke(f,
                  render::shape{
                      .origin = to_float(*t),
                      .rotation = t.rotation().to_float(),
                      .colour0 = line.colour0,
                      .colour1 = line.colour1,
                      .z_index = line.z,
                      .s_index = line.index,
                      .flags = flags,
                      .data = render::ngon{.radius = dimensions.radius.to_float(),
                                           .inner_radius = dimensions.inner_radius.to_float(),
                                           .sides = dimensions.sides,
                                           .segments = dimensions.segments,
                                           .style = line.style,
                                           .line_width = line.width},
                  });
    }
    if (fill.colour0.a || fill.colour1.a) {
      std::invoke(f,
                  render::shape{
                      .origin = to_float(*t),
                      .rotation = t.rotation().to_float(),
                      .colour0 = fill.colour0,
                      .colour1 = fill.colour1,
                      .z_index = fill.z,
                      .s_index = fill.index,
                      .flags = flags,
                      .data = render::ngon_fill{.radius = dimensions.radius.to_float(),
                                                .inner_radius = dimensions.inner_radius.to_float(),
                                                .sides = dimensions.sides,
                                                .segments = dimensions.segments},
                  });
    }
  }

  constexpr void
  iterate(iterate_volumes_t, const Transform auto& t, VolumeFunction auto&& f) const {
    std::invoke(f, *t, dimensions.radius, line.colour0, fill.colour1);
  }
};

constexpr ngon_data make_ngon(ngon_dimensions dimensions, ngon_line_style line,
                              fill_style fill = sfill(), render::flag flags = render::flag::kNone) {
  return {{}, dimensions, line, fill, flags};
}

template <Expression<ngon_dimensions>, Expression<ngon_line_style>,
          Expression<fill_style> = constant<sfill()>,
          Expression<render::flag> = constant<render::flag::kNone>>
struct ngon_eval {};

template <Expression<ngon_dimensions> Dimensions, Expression<ngon_line_style> Line,
          Expression<fill_style> Fill, Expression<render::flag> RFlags>
constexpr auto evaluate(ngon_eval<Dimensions, Line, Fill, RFlags>, const auto& params) {
  return make_ngon(evaluate(Dimensions{}, params), evaluate(Line{}, params),
                   evaluate(Fill{}, params), render::flag{evaluate(RFlags{}, params)});
}

//////////////////////////////////////////////////////////////////////////////////
// Helper combinations.
//////////////////////////////////////////////////////////////////////////////////
template <ngon_dimensions Dimensions, shape_flag Flags = shape_flag::kNone>
using ngon_collider = constant<make_ngon_collider(Dimensions, Flags)>;
template <ngon_dimensions Dimensions, ngon_line_style Line, fill_style Fill = sfill(),
          render::flag RFlags = render::flag::kNone>
using ngon = constant<make_ngon(Dimensions, Line, Fill, RFlags)>;
template <ngon_dimensions Dimensions, ngon_line_style Line, fill_style Fill,
          shape_flag Flags = shape_flag::kNone, render::flag RFlags = render::flag::kNone>
using ngon_with_collider =
    compound<ngon<Dimensions, Line, Fill, RFlags>, ngon_collider<Dimensions, Flags>>;

template <ngon_dimensions Dimensions, std::size_t N, ngon_line_style Line = nline(),
          render::flag RFlags = render::flag::kNone>
using ngon_colour_p =
    ngon_eval<constant<Dimensions>, set_colour_p<Line, N>, constant<sfill()>, constant<RFlags>>;
template <ngon_dimensions Dimensions, std::size_t N0, std::size_t N1,
          ngon_line_style Line = nline(), fill_style Fill = sfill(),
          render::flag RFlags = render::flag::kNone>
using ngon_colour_p2 = ngon_eval<constant<Dimensions>, set_colour_p<Line, N0>,
                                 set_colour_p<Fill, N1>, constant<RFlags>>;

}  // namespace shapes
}  // namespace ii::geom

#endif