#include "game/logic/v0/lib/particles.h"

namespace ii::v0 {

void add_line_particle(EmitHandle& e, const fvec2& source, const fvec2& a, const fvec2& b,
                       const cvec4& c, float w, float z, std::uint32_t time) {
  auto& r = e.random();
  auto position = (a + b) / 2.f;
  auto velocity = (1.75f + .6f * r.fixed().to_float()) * normalise(position - source) +
      from_polar((2 * pi<fixed> * r.fixed()).to_float(), (r.fixed() / 10).to_float());
  auto diameter = distance(a, b);
  auto d = distance(source, a) - distance(source, b);
  if (angle_diff(angle(a - source), angle(b - source)) > 0) {
    d = -d;
  }
  auto angular_velocity = (d / (30.f * diameter)) + r.fixed().to_float() / 60.f;
  e.add(particle{
      .position = position - velocity,
      .velocity = velocity,
      .end_velocity = velocity / 2.f,
      .colour = c,
      .data =
          line_particle{
              .radius = diameter / 2.f,
              .rotation = angle(b - a) - angular_velocity,
              .angular_velocity = angular_velocity,
              .width = w,
          },
      .end_time = time + r.uint(time),
      .flash_time = z > colour::kZTrails ? 16u : 0u,
      .fade = true,
  });
}

void add_explode_particle(EmitHandle& e, const fvec2& source, const fvec2& position, float radius,
                          const cvec4& c, std::uint32_t time) {
  auto& r = e.random();
  auto base_velocity = (1.f + 1.5f * r.fixed().to_float()) * normalise(position - source) +
      from_polar((2 * pi<fixed> * r.fixed()).to_float(), r.fixed().to_float());
  auto seed = 1024.f * fvec2{r.fixed().to_float(), r.fixed().to_float()};
  for (std::uint32_t i = 0; i < 3; ++i) {
    auto velocity = base_velocity + r.fixed().to_float() * normalise(position - source) / 32.f +
        from_polar((2 * pi<fixed> * r.fixed()).to_float(), r.fixed().to_float() / 32.f);
    auto offset =
        from_polar((2 * pi<fixed> * r.fixed()).to_float(), radius * r.fixed().to_float() / 64.f);
    ball_fx_particle data{
        .style = render::fx_style::kExplosion,
        .seed = seed + 64.f * fvec2{r.fixed().to_float() - .5f, r.fixed().to_float() - .5f},
        .anim_speed = 1.5f / 2};
    auto cv = c;
    if (i == 0) {
      data.radius = radius / 2.f;
      data.inner_radius = -radius / 2.f;
      data.end_radius = radius;
      data.end_inner_radius = -radius;
      data.value = 2.f;
      data.end_value = 0.f;
    } else if (i == 1) {
      data.inner_radius = data.end_inner_radius = -.75f * radius;
      data.radius = radius;
      data.end_radius = 0.f;
      data.value = data.end_value = 1.f;
      cv = colour::perceptual_mix(c, colour::kWhite0, .5f);
    } else {
      data.radius = .325f * radius;
      data.inner_radius = -.75f * radius;
      data.end_radius = data.end_inner_radius = .75f * radius;
      data.value = 1.f;
      data.end_value = 0.f;
      cv = colour::kWhite0;
    }
    auto tv = time + static_cast<int>(radius / 4.f);
    e.add(particle{
        .position = offset + position - velocity,
        .velocity = velocity,
        .end_velocity = velocity / 2.f,
        .colour = cv,
        .z_index = static_cast<float>(i),
        .data = data,
        .end_time = tv + r.uint(tv),
    });
  }
}

}  // namespace ii::v0