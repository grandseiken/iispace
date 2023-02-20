#ifndef II_GAME_CORE_ICONS_MOD_ICONS_H
#define II_GAME_CORE_ICONS_MOD_ICONS_H
#include "game/common/colour.h"
#include "game/geom2/shape_bank.h"

namespace ii::icons {

inline void mod_unknown(geom2::node& root) {
  root.add(geom2::rotate{geom2::key{'r'}})
      .add(geom2::ngon{.dimensions = geom2::nd(12, 4),
                       .style = geom2::ngon_style::kPolystar,
                       .line = {.colour0 = geom2::key{'c'}},
                       .fill = {.colour0 = geom2::key{'f'}}});
}

inline void generic_weapon(geom2::node& root) {
  root.add(geom2::rotate{geom2::key{'r'}})
      .add(geom2::ngon{.dimensions = geom2::nd(12, 3),
                       .line = {.colour0 = geom2::key{'c'}},
                       .fill = {.colour0 = geom2::key{'f'}}});
}

inline void generic_super(geom2::node& root) {
  auto& r = root.add(geom2::rotate{geom2::key{'r'}});
  auto& t = root.create(geom2::ngon{.dimensions = geom2::nd(10, 3),
                                    .line = {.colour0 = geom2::key{'c'}},
                                    .fill = {.colour0 = geom2::key{'f'}}});
  r.add(geom2::translate{vec2{-4, 0}}).add(t);
  r.add(geom2::translate{vec2{4, 0}}).add(t);
}

inline void generic_bomb(geom2::node& root) {
  auto& n = root.add(geom2::rotate{geom2::key{'r'}});
  n.add(geom2::ngon{.dimensions = geom2::nd(14, 6), .line = {.colour0 = geom2::key{'c'}}});
  n.add(geom2::ngon{.dimensions = geom2::nd(5, 6),
                    .line = {.colour0 = geom2::key{'c'}, .width = 1.5f},
                    .fill = {.colour0 = geom2::key{'f'}}});
}

inline void generic_shield(geom2::node& root) {
  auto& n = root.add(geom2::rotate{geom2::key{'r'}});
  n.add(geom2::ngon{.dimensions = geom2::nd(14, 6), .line = {.colour0 = geom2::key{'c'}}});
  n.add(geom2::ngon{.dimensions = geom2::nd(11, 6),
                    .line = {.colour0 = geom2::key{'c'}, .width = 1.5f}});
}

inline void generic_bonus(geom2::node& root) {
  auto& n = root.add(geom2::rotate{geom2::key{'r'}});
  n.add(geom2::ngon{.dimensions = geom2::nd(12, 4), .line = {.colour0 = geom2::key{'c'}}});
  n.add(geom2::ngon{.dimensions = geom2::nd(9, 4),
                    .line = {.colour0 = geom2::key{'c'}, .width = 1.5f},
                    .fill = {.colour0 = geom2::key{'f'}}});
}

}  // namespace ii::icons

#endif