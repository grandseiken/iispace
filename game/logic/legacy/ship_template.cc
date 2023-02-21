#include "game/logic/legacy/ship_template.h"
#include "game/common/variant_switch.h"

namespace ii::legacy {
namespace {

void add_line_particle(EmitHandle& e, const fvec2& source, const fvec2& a, const fvec2& b,
                       const cvec4& c, float w, float z, std::uint32_t time) {
  auto& r = e.random();
  auto position = (a + b) / 2.f;
  auto velocity = (2.f + .5f * r.fixed().to_float()) * normalise(position - source) +
      from_polar((2 * pi<fixed> * r.fixed()).to_float(), (r.fixed() / 8).to_float());
  auto diameter = distance(a, b);
  auto d = distance(source, a) - distance(source, b);
  if (angle_diff(angle(a - source), angle(b - source)) > 0) {
    d = -d;
  }
  auto angular_velocity = (d / (32.f * diameter)) + r.fixed().to_float() / 64.f;
  e.add(particle{
      .position = position - velocity,
      .velocity = velocity,
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

}  // namespace

void explode_shapes(EmitHandle& e, const geom::resolve_result& r,
                    const std::optional<cvec4>& colour_override, std::uint32_t time,
                    const std::optional<fvec2>& towards, std::optional<float> speed) {
  for (const auto& entry : r.entries) {
    std::optional<cvec4> c;
    render::flag flags = render::flag::kNone;

    switch (entry.data.index()) {
      VARIANT_CASE_GET(geom::resolve_result::ball, entry.data, d) {
        c = d.line.colour0;
        flags = d.flags;
        break;
      }

      VARIANT_CASE_GET(geom::resolve_result::box, entry.data, d) {
        c = d.line.colour0;
        flags = d.flags;
        break;
      }

      VARIANT_CASE_GET(geom::resolve_result::line, entry.data, d) {
        c = d.style.colour0;
        flags = d.flags;
        break;
      }

      VARIANT_CASE_GET(geom::resolve_result::ngon, entry.data, d) {
        c = d.line.colour0;
        flags = d.flags;
        break;
      }
    }
    if (c && !(flags & render::flag::kLegacy_NoExplode)) {
      e.explosion(to_float(*entry.t), colour_override.value_or(*c), time, towards, speed);
    }
  }
}

void destruct_lines(EmitHandle& e, const geom::resolve_result& r, const fvec2& source,
                    std::uint32_t time) {
  auto handle_line = [&](const vec2& a, const vec2& b, const cvec4& c) {
    add_line_particle(e, source, to_float(a), to_float(b), c, 1.f, 0.f, time);
  };

  for (const auto& entry : r.entries) {
    switch (entry.data.index()) {
      VARIANT_CASE_GET(geom::resolve_result::box, entry.data, d) {
        auto va = *entry.t.translate({d.dimensions.x, d.dimensions.y});
        auto vb = *entry.t.translate({-d.dimensions.x, d.dimensions.y});
        auto vc = *entry.t.translate({-d.dimensions.x, -d.dimensions.y});
        auto vd = *entry.t.translate({d.dimensions.x, -d.dimensions.y});

        handle_line(va, vb, d.line.colour0);
        handle_line(vb, vc, d.line.colour0);
        handle_line(vc, vd, d.line.colour0);
        handle_line(vd, va, d.line.colour0);
        break;
      }

      VARIANT_CASE_GET(geom::resolve_result::line, entry.data, d) {
        handle_line(*entry.t.translate(d.a), *entry.t.translate(d.b), d.style.colour0);
        break;
      }

      VARIANT_CASE_GET(geom::resolve_result::ngon, entry.data, d) {
        auto vertex = [&](std::uint32_t i) {
          return *entry.t.rotate(i * 2 * pi<fixed> / d.dimensions.sides)
                      .translate({d.dimensions.radius, 0});
        };

        if (d.style != render::ngon_style::kPolygram) {
          for (std::uint32_t i = 0;
               d.dimensions.sides >= 2 && i < d.dimensions.sides && i < d.dimensions.segments;
               ++i) {
            handle_line(vertex(i),
                        d.style == render::ngon_style::kPolygon ? vertex(i + 1) : entry.t.v,
                        d.line.colour0);
          }
        } else {
          for (std::size_t i = 0; i < d.dimensions.sides; ++i) {
            for (std::size_t j = i + 1; j < d.dimensions.sides; ++j) {
              handle_line(vertex(i), vertex(j), d.line.colour0);
            }
          }
        }
        break;
      }
    }
  }
}

void render_shape(std::vector<render::shape>& output, const geom::resolve_result& r, float z,
                  const std::optional<float>& hit_alpha, const std::optional<cvec4>& c_override,
                  const std::optional<std::size_t>& c_override_max_index) {
  std::size_t i = 0;
  auto handle_shape = [&](const render::shape& shape) {
    render::shape shape_copy = shape;
    shape_copy.z = z;
    if ((c_override || hit_alpha) && (!c_override_max_index || i < *c_override_max_index)) {
      if (c_override) {
        shape_copy.colour0 = cvec4{c_override->r, c_override->g, c_override->b, shape.colour0.a};
        if (shape_copy.colour1) {
          shape_copy.colour1 = cvec4{c_override->r, c_override->g, c_override->b, shape.colour1->a};
        }
      }
      if (hit_alpha) {
        shape_copy.apply_hit_flash(*hit_alpha);
      }
    }
    output.emplace_back(shape_copy);
  };

  for (const auto& entry : r.entries) {
    switch (entry.data.index()) {
      VARIANT_CASE_GET(geom::resolve_result::box, entry.data, d) {
        handle_shape(render::shape{
            .origin = to_float(*entry.t),
            .rotation = entry.t.rotation().to_float(),
            .colour0 = d.line.colour0,
            .tag = d.tag,
            .data = render::box{.dimensions = to_float(d.dimensions)},
        });
        break;
      }

      VARIANT_CASE_GET(geom::resolve_result::line, entry.data, d) {
        handle_shape(render::shape::line(to_float(*entry.t.translate(d.a)),
                                         to_float(*entry.t.translate(d.b)), d.style.colour0, 0.f,
                                         1.f, d.tag));
        break;
      }

      VARIANT_CASE_GET(geom::resolve_result::ngon, entry.data, d) {
        handle_shape(render::shape{
            .origin = to_float(*entry.t),
            .rotation = entry.t.rotation().to_float(),
            .colour0 = d.line.colour0,
            .tag = d.tag,
            .data = render::ngon{.radius = d.dimensions.radius.to_float(),
                                 .sides = d.dimensions.sides,
                                 .segments = d.dimensions.segments,
                                 .style = d.style},
        });
        break;
      }
    }
    ++i;
  }
}

}  // namespace ii::legacy