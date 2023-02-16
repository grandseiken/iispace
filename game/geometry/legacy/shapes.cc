#include "game/geometry/legacy/ball_collider.h"
#include "game/geometry/legacy/box.h"
#include "game/geometry/legacy/line.h"
#include "game/geometry/legacy/ngon.h"
#include "game/geometry/legacy/polyarc.h"
#include "game/geometry/resolve.h"
#include "game/geometry/shapes/box.h"
#include "game/geometry/shapes/line.h"
#include "game/geometry/shapes/ngon.h"

namespace ii::geom::legacy {

void ball_collider_data::iterate(iterate_resolve_t, const transform& t, resolve_result& r) const {
  r.add(t, geom::ball_data{.dimensions = {.radius = radius}});
}

void box_data::iterate(iterate_resolve_t, const transform& t, resolve_result& r) const {
  r.add(
      t,
      geom::box_data{
          .dimensions = dimensions, .line = {.colour0 = colour, .index = index}, .flags = r_flags});
}

void line_data::iterate(iterate_resolve_t, const transform& t, resolve_result& r) const {
  r.add(t,
        geom::line_data{
            .a = a, .b = b, .style = {.colour0 = colour, .index = index}, .flags = r_flags});
}

void ngon_data::iterate(iterate_resolve_t, const transform& t, resolve_result& r) const {
  geom::ngon_line_style line;
  line.style = style;
  line.colour0 = colour;
  line.index = index;
  r.add(t,
        geom::ngon_data{
            .dimensions = {.radius = radius, .sides = sides}, .line = line, .flags = r_flags});
}

void polyarc_data::iterate(iterate_resolve_t, const transform& t, resolve_result& r) const {
  geom::ngon_line_style line;
  line.colour0 = colour;
  line.index = index;
  r.add(t,
        geom::ngon_data{.dimensions = {.radius = radius, .sides = sides, .segments = segments},
                        .line = line});
}

}  // namespace ii::geom::legacy