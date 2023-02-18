#ifndef II_GAME_RENDER_DATA_FX_H
#define II_GAME_RENDER_DATA_FX_H
#include "game/common/colour.h"
#include "game/common/math.h"
#include <variant>

namespace ii::render {

enum class fx_style {
  kNone,
  kExplosion,
};

struct ball_fx {
  fvec2 position{0.f};
  float radius = 0.f;
  float inner_radius = 0.f;
};

struct fx {
  fx_style style = fx_style::kNone;
  float time = 0.f;
  float z = 0.f;
  cvec4 colour = colour::kWhite0;
  fvec2 seed{0.f};
  std::variant<ball_fx> data;
};

}  // namespace ii::render

#endif