#ifndef II_GAME_CORE_ICONS_MOD_ICONS_H
#define II_GAME_CORE_ICONS_MOD_ICONS_H
#include "game/common/colour.h"
#include "game/geometry/node_transform.h"
#include "game/geometry/shapes/ngon.h"

namespace ii::icons {
template <geom::ShapeNode... S>
using rotate = geom::rotate_p<0, S...>;
using mod_unknown = rotate<geom::ngon<geom::nd(12_fx, 3), geom::nline(colour::kWhite1),
                                      geom::sfill(colour::alpha(colour::kWhite1, .5f))>>;

template <cvec4 C>
using generic_weapon = rotate<geom::ngon<geom::nd(12_fx, 3), geom::nline(C),
                                         geom::sfill(colour::alpha(C, colour::kFillAlpha0))>>;
template <cvec4 C>
using generic_super =
    rotate<geom::translate<-4, 0,
                           geom::ngon<geom::nd(10_fx, 3), geom::nline(C),
                                      geom::sfill(colour::alpha(C, colour::kFillAlpha0))>>,
           geom::translate<4, 0, geom::ngon<geom::nd(10_fx, 3), geom::nline(C)>>>;
template <cvec4 C>
using generic_bomb = rotate<geom::ngon<geom::nd(14, 6), geom::nline(C)>,
                            rotate<geom::ngon<geom::nd(5, 6), geom::nline(C, 0.f, 1.5f),
                                              geom::sfill(colour::alpha(C, colour::kFillAlpha0))>>>;
template <cvec4 C>
using generic_shield = rotate<geom::ngon<geom::nd(14, 6), geom::nline(C)>,
                              geom::ngon<geom::nd(11, 6), geom::nline(C, 0.f, 1.5f)>>;
template <cvec4 C>
using generic_bonus = rotate<geom::ngon<geom::nd(12, 4), geom::nline(C)>,
                             geom::ngon<geom::nd(9, 4), geom::nline(C, 0.f, 1.5f),
                                        geom::sfill(colour::alpha(C, colour::kFillAlpha0))>>;
}  // namespace ii::icons

#endif