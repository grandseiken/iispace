#ifndef II_GAME_RENDER_DATA_BACKGROUND_H
#define II_GAME_RENDER_DATA_BACKGROUND_H
#include "game/common/math.h"
#include <cstdint>
#include <optional>

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
    std::optional<background::type> type;
    std::optional<cvec4> colour;
    std::optional<fvec2> parameters;
    std::optional<fvec4> velocity;
    std::optional<float> angular_velocity;

    void fill_defaults() {
      type = type.value_or(background::type::kNone);
      colour = colour.value_or(cvec4{0.f});
      parameters = parameters.value_or(fvec2{0.f});
      velocity = velocity.value_or(fvec4{0.f});
      angular_velocity = angular_velocity.value_or(0.f);
    }

    void fill_from(const update& u) {
      auto v = u;
      v.fill_defaults();
      type = type.value_or(*v.type);
      colour = colour.value_or(*v.colour);
      parameters = parameters.value_or(*v.parameters);
      velocity = velocity.value_or(*v.velocity);
      angular_velocity = angular_velocity.value_or(*v.angular_velocity);
    }
  };

  fvec4 position{0.f};
  float rotation = 0.f;

  background::data data0;
  background::data data1;
  float interpolate = 0.f;
};

}  // namespace ii::render

#endif