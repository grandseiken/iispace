#ifndef II_GAME_RENDER_DATA_BACKGROUND_H
#define II_GAME_RENDER_DATA_BACKGROUND_H
#include "game/common/math.h"
#include <cstdint>
#include <optional>

namespace ii::render {

struct background {
  enum class height_function {
    kZero = 0,
    kTurbulent0 = 1,
  };

  enum class combinator {
    kOctave0 = 0,
    kAbs0 = 1,
  };

  enum class tonemap {
    kPassthrough = 0,
    kTendrils = 1,
    kSinCombo = 2,
  };

  enum class parallax {
    kNone = 0,
    kLegacy_Stars = 1,
  };

  struct data {
    background::height_function height_function = background::height_function::kZero;
    background::combinator combinator = background::combinator::kOctave0;
    background::tonemap tonemap = background::tonemap::kPassthrough;

    std::optional<float> polar_period;
    float scale{1.f};
    float persistence{.5f};
    fvec2 parameters{0.f};
    cvec4 colour{0.f};
  };

  struct update {
    std::optional<background::height_function> height_function;
    std::optional<background::combinator> combinator;
    std::optional<background::tonemap> tonemap;
    std::optional<background::parallax> parallax;

    std::optional<std::optional<float>> polar_period;
    std::optional<float> scale;
    std::optional<float> persistence;

    std::optional<fvec2> parameters;
    std::optional<cvec4> colour;
    std::optional<fvec4> velocity;
    std::optional<float> angular_velocity;

    void fill_defaults() {
      height_function = height_function.value_or(background::height_function::kZero);
      combinator = combinator.value_or(background::combinator::kOctave0);
      tonemap = tonemap.value_or(background::tonemap::kPassthrough);

      polar_period = polar_period.value_or(std::optional<float>{});
      scale = scale.value_or(1.f);
      persistence = persistence.value_or(.5f);

      parameters = parameters.value_or(fvec2{0.f});
      colour = colour.value_or(cvec4{0.f});
      velocity = velocity.value_or(fvec4{0.f});
      angular_velocity = angular_velocity.value_or(0.f);
    }

    void fill_from(const update& u) {
      auto v = u;
      v.fill_defaults();

      height_function = height_function.value_or(*v.height_function);
      combinator = combinator.value_or(*v.combinator);
      tonemap = tonemap.value_or(*v.tonemap);

      polar_period = polar_period.value_or(*v.polar_period);
      scale = scale.value_or(*v.scale);
      persistence = persistence.value_or(*v.persistence);

      parameters = parameters.value_or(*v.parameters);
      colour = colour.value_or(*v.colour);
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