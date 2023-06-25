#ifndef II_GAME_RENDER_DATA_BACKGROUND_H
#define II_GAME_RENDER_DATA_BACKGROUND_H
#include "game/common/math.h"
#include <cstdint>

namespace ii::render {

struct background {
  enum class type : std::uint32_t {
    kNone = 0,
    kBiome0 = 1,
    kBiome0_Polar = 2,
  };

  struct data {
    background::type type = background::type::kNone;
    cvec4 colour{0.f};
    fvec2 parameters{0.f};
  };

  struct update {
    background::data data;
    fvec4 position_delta{0.f};
    float rotation_delta{0.f};
    bool begin_transition = false;
  };

  fvec4 position{0.f};
  float rotation = 0.f;

  background::data data0;
  background::data data1;
  float interpolate = 0.f;
};

}  // namespace ii::render

#endif