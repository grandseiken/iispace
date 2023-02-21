#include "game/logic/v0/lib/particles.h"

namespace ii::v0 {
namespace {

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
        .z = static_cast<float>(i),
        .data = data,
        .end_time = tv + r.uint(tv),
    });
  }
}

}  // namespace

void explode_shapes(EmitHandle& e, const geom2::resolve_result& r,
                    const std::optional<cvec4>& colour_override, std::uint32_t time,
                    const std::optional<fvec2>& towards, std::optional<float> speed) {
  for (const auto& entry : r.entries) {
    std::optional<cvec4> c;
    if (const auto* d = std::get_if<geom2::resolve_result::ball>(&entry.data)) {
      c = d->line.colour0.a ? d->line.colour0 : d->fill.colour0;
    } else if (const auto* d = std::get_if<geom2::resolve_result::box>(&entry.data)) {
      c = d->line.colour0.a ? d->line.colour0 : d->fill.colour0;
    } else if (const auto* d = std::get_if<geom2::resolve_result::ngon>(&entry.data)) {
      c = d->line.colour0.a ? d->line.colour0 : d->fill.colour1;
    }
    if (c) {
      e.explosion(to_float(*entry.t), colour_override.value_or(*c), time, towards, speed);
    }
  }
}

void destruct_lines(EmitHandle& e, const geom2::resolve_result& r, const fvec2& source,
                    std::uint32_t time) {
  auto handle_line = [&](const vec2& a, const vec2& b, const cvec4& c, float w, float z) {
    if (z > colour::kZOutline) {
      add_line_particle(e, source, to_float(a), to_float(b), c, w, z, time);
    }
  };

  for (const auto& entry : r.entries) {
    if (const auto* d = std::get_if<geom2::resolve_result::ball>(&entry.data)) {
      if (!d->line.colour0.a) {
        continue;
      }
      std::uint32_t n = std::max(3, d->dimensions.radius.to_int() / 2);
      std::uint32_t in = std::max(3, d->dimensions.inner_radius.to_int() / 2);
      auto vertex = [&](fixed r, std::uint32_t i, std::uint32_t n) {
        return *entry.t.rotate(i * 2 * pi<fixed> / n).translate({r, 0});
      };

      for (std::uint32_t i = 0; i < n; ++i) {
        handle_line(vertex(d->dimensions.radius, i, n), vertex(d->dimensions.radius, i + 1, n),
                    d->line.colour0, d->line.width, d->line.z);
      }
      for (std::uint32_t i = 0; d->dimensions.inner_radius && i < in; ++i) {
        handle_line(vertex(d->dimensions.inner_radius, i, in),
                    vertex(d->dimensions.inner_radius, i + 1, in), d->line.colour0, d->line.width,
                    d->line.z);
      }
    } else if (const auto* d = std::get_if<geom2::resolve_result::box>(&entry.data)) {
      if (!d->line.colour0.a && !d->line.colour1.a) {
        continue;
      }
      auto va = *entry.t.translate({d->dimensions.x, d->dimensions.y});
      auto vb = *entry.t.translate({-d->dimensions.x, d->dimensions.y});
      auto vc = *entry.t.translate({-d->dimensions.x, -d->dimensions.y});
      auto vd = *entry.t.translate({d->dimensions.x, -d->dimensions.y});

      // TODO: need line gradients to match rendering, if we use them.
      handle_line(va, vb, d->line.colour0, d->line.width, d->line.z);
      handle_line(vb, vc, d->line.colour0, d->line.width, d->line.z);
      handle_line(vc, vd, d->line.colour1, d->line.width, d->line.z);
      handle_line(vd, va, d->line.colour1, d->line.width, d->line.z);
    } else if (const auto* d = std::get_if<geom2::resolve_result::line>(&entry.data)) {
      // TODO: need line gradients to match rendering, if we use them.
      if (d->style.colour0.a) {
        handle_line(*entry.t.translate(d->a), *entry.t.translate(d->b), d->style.colour0,
                    d->style.width, d->style.z);
      }
    } else if (const auto* d = std::get_if<geom2::resolve_result::ngon>(&entry.data)) {
      if (!d->line.colour0.a) {
        continue;
      }
      auto vertex = [&](std::uint32_t i) {
        return *entry.t.rotate(i * 2 * pi<fixed> / d->dimensions.sides)
                    .translate({d->dimensions.radius, 0});
      };
      auto ivertex = [&](std::uint32_t i) {
        return *entry.t.rotate(i * 2 * pi<fixed> / d->dimensions.sides)
                    .translate({d->dimensions.inner_radius, 0});
      };

      switch (d->style) {
      case render::ngon_style::kPolystar:
        // TODO: need line gradients to match rendering, if we use them.
        for (std::uint32_t i = 0; i < d->dimensions.segments; ++i) {
          handle_line(ivertex(i), vertex(i), d->line.colour0, d->line.width, d->line.z);
        }
        break;
      case render::ngon_style::kPolygram:
        for (std::size_t i = 0; i < d->dimensions.sides; ++i) {
          for (std::size_t j = i + 2; j < d->dimensions.sides && (j + 1) % d->dimensions.sides != i;
               ++j) {
            handle_line(vertex(i), vertex(j), d->line.colour1, d->line.width, d->line.z);
          }
        }
        // Fallthrough.
      case render::ngon_style::kPolygon:
        for (std::uint32_t i = 0; i < d->dimensions.segments; ++i) {
          handle_line(vertex(i), vertex(i + 1), d->line.colour0, d->line.width, d->line.z);
        }
        for (std::uint32_t i = 0; d->dimensions.inner_radius && i < d->dimensions.segments; ++i) {
          handle_line(ivertex(i), ivertex(i + 1), d->line.colour1, d->line.width, d->line.z);
        }
        break;
      }
    }
  }
}

void explode_volumes(EmitHandle& e, const geom2::resolve_result& r, const fvec2& source,
                     std::uint32_t time) {
  auto handle_shape = [&](const vec2& v, fixed r, const cvec4& c) {
    if (c.a) {
      add_explode_particle(e, source, to_float(v), 2.f * r.to_float(), c, time);
    }
  };

  for (const auto& entry : r.entries) {
    if (const auto* d = std::get_if<geom2::resolve_result::ball>(&entry.data)) {
      handle_shape(*entry.t, d->dimensions.radius, d->fill.colour0);
    } else if (const auto* d = std::get_if<geom2::resolve_result::box>(&entry.data)) {
      handle_shape(*entry.t, std::min(d->dimensions.x, d->dimensions.y), d->fill.colour0);
    } else if (const auto* d = std::get_if<geom2::resolve_result::ngon>(&entry.data)) {
      handle_shape(*entry.t, d->dimensions.radius, d->fill.colour1);
    }
  }
}

}  // namespace ii::v0