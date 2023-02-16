#ifndef II_GAME_GEOMETRY_LEGACY_POLYARC_H
#define II_GAME_GEOMETRY_LEGACY_POLYARC_H
#include "game/geometry/expressions.h"
#include "game/render/data/shapes.h"
#include <cstddef>
#include <cstdint>

namespace ii::geom::legacy {

struct polyarc_data : shape_data_base {
  using shape_data_base::iterate;
  fixed radius = 0;
  std::uint32_t sides = 0;
  std::uint32_t segments = 0;
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
      auto theta = angle(v);
      auto r = length(v);
      if (0 <= theta && theta <= (2 * pi<fixed> * segments) / sides && r >= radius - 10 &&
          r < radius) {
        hit.add(flags & it.mask);
      }
    }
  }

  void iterate(iterate_resolve_t, const transform& t, resolve_result& r) const;
};

constexpr polyarc_data make_polyarc(fixed radius, std::uint32_t sides, std::uint32_t segments,
                                    const cvec4& colour, unsigned char index = 0,
                                    shape_flag flags = shape_flag::kNone) {
  return {{}, radius, sides, segments, colour, index, flags};
}

template <Expression<fixed> Radius, Expression<std::uint32_t> Sides,
          Expression<std::uint32_t> Segments, Expression<cvec4> Colour,
          Expression<unsigned char> Index = constant<0>,
          Expression<shape_flag> Flags = constant<shape_flag::kNone>>
struct polyarc_eval {};

template <Expression<fixed> Radius, Expression<std::uint32_t> Sides,
          Expression<std::uint32_t> Segments, Expression<cvec4> Colour,
          Expression<unsigned char> Index, Expression<shape_flag> Flags>
constexpr auto
evaluate(polyarc_eval<Radius, Sides, Segments, Colour, Index, Flags>, const auto& params) {
  return make_polyarc(
      fixed{evaluate(Radius{}, params)}, std::uint32_t{evaluate(Sides{}, params)},
      std::uint32_t{evaluate(Segments{}, params)}, cvec4{evaluate(Colour{}, params)},
      static_cast<unsigned char>(evaluate(Index{}, params)), shape_flag{evaluate(Flags{}, params)});
}

//////////////////////////////////////////////////////////////////////////////////
// Helper combinations.
//////////////////////////////////////////////////////////////////////////////////
template <fixed Radius, std::uint32_t Sides, std::uint32_t Segments, cvec4 Colour,
          unsigned char Index = 0, shape_flag Flags = shape_flag::kNone>
using polyarc = constant<make_polyarc(Radius, Sides, Segments, Colour, Index, Flags)>;

template <fixed Radius, std::uint32_t Sides, std::uint32_t Segments, std::size_t ParameterIndex,
          unsigned char Index = 0, shape_flag Flags = shape_flag::kNone>
using polyarc_colour_p = polyarc_eval<constant<Radius>, constant<Sides>, constant<Segments>,
                                      parameter<ParameterIndex>, constant<Index>, constant<Flags>>;

}  // namespace ii::geom::legacy

#endif