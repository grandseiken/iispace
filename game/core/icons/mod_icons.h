#ifndef II_GAME_CORE_ICONS_MOD_ICONS_H
#define II_GAME_CORE_ICONS_MOD_ICONS_H
#include "game/common/colour.h"
#include "game/geometry/shape_bank.h"

namespace ii::icons {

inline void mod_unknown(geom::node& root) {
  root.add(geom::rotate{geom::key{'r'}})
      .add(geom::ngon{.dimensions = geom::nd(12, 4),
                      .style = geom::ngon_style::kPolystar,
                      .line = {.colour0 = geom::key{'c'}},
                      .fill = {.colour0 = geom::key{'f'}}});
}

inline void generic_weapon(geom::node& root) {
  root.add(geom::rotate{geom::key{'r'}})
      .add(geom::ngon{.dimensions = geom::nd(12, 3),
                      .line = {.colour0 = geom::key{'c'}},
                      .fill = {.colour0 = geom::key{'f'}}});
}

inline void generic_super(geom::node& root) {
  auto& r = root.add(geom::rotate{geom::key{'r'}});
  auto& t = root.create(geom::ngon{.dimensions = geom::nd(10, 3),
                                   .line = {.colour0 = geom::key{'c'}},
                                   .fill = {.colour0 = geom::key{'f'}}});
  r.add(geom::translate{vec2{-4, 0}}).add(t);
  r.add(geom::translate{vec2{4, 0}}).add(t);
}

inline void generic_bomb(geom::node& root) {
  auto& n = root.add(geom::rotate{geom::key{'r'}});
  n.add(geom::ngon{.dimensions = geom::nd(14, 6), .line = {.colour0 = geom::key{'c'}}});
  n.add(geom::ngon{.dimensions = geom::nd(5, 6),
                   .line = {.colour0 = geom::key{'c'}, .width = 1.5f},
                   .fill = {.colour0 = geom::key{'f'}}});
}

inline void generic_shield(geom::node& root) {
  auto& n = root.add(geom::rotate{geom::key{'r'}});
  n.add(geom::ngon{.dimensions = geom::nd(14, 6), .line = {.colour0 = geom::key{'c'}}});
  n.add(geom::ngon{.dimensions = geom::nd(11, 6),
                   .line = {.colour0 = geom::key{'c'}, .width = 1.5f}});
}

inline void generic_bonus(geom::node& root) {
  auto& n = root.add(geom::rotate{geom::key{'r'}});
  n.add(geom::ngon{.dimensions = geom::nd(12, 4), .line = {.colour0 = geom::key{'c'}}});
  n.add(geom::ngon{.dimensions = geom::nd(9, 4),
                   .line = {.colour0 = geom::key{'c'}, .width = 1.5f},
                   .fill = {.colour0 = geom::key{'f'}}});
}

}  // namespace ii::icons

#endif