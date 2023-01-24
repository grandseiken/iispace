#ifndef II_GAME_GEOMETRY_LEGACY_BOX_H
#define II_GAME_GEOMETRY_LEGACY_BOX_H
#include "game/geometry/expressions.h"
#include "game/render/data/shapes.h"
#include <cstddef>
#include <cstdint>

namespace ii::geom {
inline namespace legacy {

struct box_data : shape_data_base {
  using shape_data_base::iterate;
  vec2 dimensions{0};
  cvec4 colour{0.f};
  unsigned char index = 0;
  shape_flag flags = shape_flag::kNone;

  constexpr void iterate(iterate_flags_t, const null_transform&, shape_flag& mask) const {
    mask |= flags;
  }

  constexpr void
  iterate(iterate_check_collision_t it, const Transform auto& t, hit_result& hit) const {
    if (!(flags & it.mask)) {
      return;
    }
    if (const auto* c = std::get_if<check_point>(&it.check)) {
      auto v = t.transform(c->v);
      if (abs(v.x) < dimensions.x && abs(v.y) < dimensions.y) {
        hit.add(flags & it.mask);
      }
    }
  }

  constexpr void iterate(iterate_lines_t, const Transform auto& t, LineFunction auto&& f) const {
    auto a = *t.translate({dimensions.x, dimensions.y});
    auto b = *t.translate({-dimensions.x, dimensions.y});
    auto c = *t.translate({-dimensions.x, -dimensions.y});
    auto d = *t.translate({dimensions.x, -dimensions.y});

    std::invoke(f, a, b, colour, 1.f, 0.f);
    std::invoke(f, b, c, colour, 1.f, 0.f);
    std::invoke(f, c, d, colour, 1.f, 0.f);
    std::invoke(f, d, a, colour, 1.f, 0.f);
  }

  constexpr void iterate(iterate_shapes_t, const Transform auto& t, ShapeFunction auto&& f) const {
    std::invoke(f,
                render::shape{
                    .origin = to_float(*t),
                    .rotation = t.rotation().to_float(),
                    .colour0 = colour,
                    .s_index = index,
                    .data = render::box{.dimensions = to_float(dimensions)},
                });
  }

  constexpr void
  iterate(iterate_volumes_t, const Transform auto& t, VolumeFunction auto&& f) const {
    std::invoke(f, *t, 0_fx, colour, colour::kZero);
  }
};

constexpr box_data make_box(const vec2& dimensions, const cvec4& colour, unsigned char index = 0,
                            shape_flag flags = shape_flag::kNone) {
  return {{}, dimensions, colour, index, flags};
}

template <Expression<vec2> Dimensions, Expression<cvec4> Colour,
          Expression<unsigned char> Index = constant<0>,
          Expression<shape_flag> Flags = constant<shape_flag::kNone>>
struct box_eval {};

template <Expression<vec2> Dimensions, Expression<cvec4> Colour, Expression<unsigned char> Index,
          Expression<shape_flag> Flags>
constexpr auto evaluate(box_eval<Dimensions, Colour, Index, Flags>, const auto& params) {
  return make_box(vec2{evaluate(Dimensions{}, params)}, cvec4{evaluate(Colour{}, params)},
                  static_cast<unsigned char>(evaluate(Index{}, params)),
                  shape_flag{evaluate(Flags{}, params)});
}

//////////////////////////////////////////////////////////////////////////////////
// Helper combinations.
//////////////////////////////////////////////////////////////////////////////////
template <fixed W, fixed H, cvec4 Colour, unsigned char Index = 0,
          shape_flag Flags = shape_flag::kNone>
using box = constant<make_box(vec2{W, H}, Colour, Index, Flags)>;
template <fixed W, fixed H, std::size_t ParameterIndex, unsigned char Index = 0,
          shape_flag Flags = shape_flag::kNone>
using box_colour_p =
    box_eval<constant_vec2<W, H>, parameter<ParameterIndex>, constant<Index>, constant<Flags>>;

}  // namespace legacy
}  // namespace ii::geom

#endif