#include "game/geom2/resolve.h"
#include "game/geometry/shapes/ball.h"
#include "game/geometry/shapes/box.h"
#include "game/geometry/shapes/legacy.h"
#include "game/geometry/shapes/line.h"
#include "game/geometry/shapes/ngon.h"

namespace ii::geom {

void legacy_dummy_data::iterate(iterate_resolve_t, const transform& t, resolve_result& r) const {
  r.add(t, geom2::resolve_result::ball{});
}

void ball_data::iterate(iterate_resolve_t, const transform& t, resolve_result& r) const {
  r.add(t,
        geom2::resolve_result::ball{
            .dimensions = dimensions,
            .line = line,
            .fill = fill,
            .flags = flags,
        });
}

void box_data::iterate(iterate_resolve_t, const transform& t, resolve_result& r) const {
  r.add(t,
        geom2::resolve_result::box{
            .dimensions = dimensions,
            .line = line,
            .fill = fill,
            .flags = flags,
        });
}

void line_data::iterate(iterate_resolve_t, const transform& t, resolve_result& r) const {
  r.add(t,
        geom2::resolve_result::line{
            .a = a,
            .b = b,
            .style = style,
            .flags = flags,
        });
}

void ngon_data::iterate(iterate_resolve_t, const transform& t, resolve_result& r) const {
  r.add(t,
        geom2::resolve_result::ngon{
            .dimensions = dimensions,
            .style = line.style,
            .line = line,
            .fill = fill,
            .tag = tag,
            .flags = flags,
        });
}

}  // namespace ii::geom