#ifndef II_GAME_GEOMETRY_RESOLVE_H
#include "game/geometry/iteration.h"
#include "game/geometry/shapes/ball.h"
#include "game/geometry/shapes/box.h"
#include "game/geometry/shapes/line.h"
#include "game/geometry/shapes/ngon.h"
#include <variant>
#include <vector>

namespace ii::geom {

struct resolve_result {
  using shape_data = std::variant<ball_data, box_data, line_data, ngon_data>;
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