#include "game/logic/v0/lib/ship_template.h"

namespace ii::v0 {

std::optional<cvec4> get_shape_colour(const geom2::resolve_result& r) {
  for (const auto& e : r.entries) {
    switch (e.data.index()) {
      VARIANT_CASE_GET(geom2::resolve_result::ball, e.data, d) {
        return d.line.colour0;
      }

      VARIANT_CASE_GET(geom2::resolve_result::box, e.data, d) {
        return d.line.colour0;
      }

      VARIANT_CASE_GET(geom2::resolve_result::ngon, e.data, d) {
        return d.line.colour0;
      }
    }
  }
  return std::nullopt;
}

void render_shape(std::vector<render::shape>& output, const geom2::resolve_result& r,
                  const std::optional<float>& hit_alpha, const std::optional<float>& shield_alpha) {
  auto handle_shape = [&](const render::shape& shape) {
    render::shape shape_copy = shape;
    if (shape.z > colour::kZTrails && hit_alpha) {
      shape_copy.apply_hit_flash(*hit_alpha);
    }
    if (shape.z <= colour::kZTrails && shield_alpha) {
      auto shield_copy = shape_copy;
      if (shape_copy.apply_shield(shield_copy, *shield_alpha)) {
        output.emplace_back(shield_copy);
      }
    }
    output.emplace_back(shape_copy);
  };

  for (const auto& e : r.entries) {
    switch (e.data.index()) {
      VARIANT_CASE_GET(geom2::resolve_result::ball, e.data, d) {
        if (d.line.colour0.a || d.line.colour1.a) {
          handle_shape({
              .origin = to_float(*e.t),
              .rotation = e.t.rotation().to_float(),
              .colour0 = d.line.colour0,
              .colour1 = d.line.colour1,
              .z = d.line.z,
              .tag = d.tag,
              .flags = d.flags,
              .data = render::ball{.radius = d.dimensions.radius.to_float(),
                                   .inner_radius = d.dimensions.inner_radius.to_float(),
                                   .line_width = d.line.width},
          });
        }
        if (d.fill.colour0.a || d.fill.colour1.a) {
          handle_shape({
              .origin = to_float(*e.t),
              .rotation = e.t.rotation().to_float(),
              .colour0 = d.fill.colour0,
              .colour1 = d.fill.colour1,
              .z = d.fill.z,
              .tag = d.tag,
              .flags = d.flags,
              .data = render::ball_fill{.radius = d.dimensions.radius.to_float(),
                                        .inner_radius = d.dimensions.inner_radius.to_float()},
          });
        }
        break;
      }

      VARIANT_CASE_GET(geom2::resolve_result::box, e.data, d) {
        if (d.line.colour0.a || d.line.colour1.a) {
          handle_shape({
              .origin = to_float(*e.t),
              .rotation = e.t.rotation().to_float(),
              .colour0 = d.line.colour0,
              .colour1 = d.line.colour1,
              .z = d.line.z,
              .tag = d.tag,
              .flags = d.flags,
              .data = render::box{.dimensions = to_float(d.dimensions), .line_width = d.line.width},
          });
        }
        if (d.fill.colour0.a || d.fill.colour1.a) {
          handle_shape({
              .origin = to_float(*e.t),
              .rotation = e.t.rotation().to_float(),
              .colour0 = d.fill.colour0,
              .colour1 = d.fill.colour1,
              .z = d.fill.z,
              .tag = d.tag,
              .flags = d.flags,
              .data = render::box_fill{.dimensions = to_float(d.dimensions)},
          });
        }
        break;
      }

      VARIANT_CASE_GET(geom2::resolve_result::line, e.data, d) {
        if (d.style.colour0.a || d.style.colour1.a) {
          auto s = render::shape::line(to_float(*e.t.translate(d.a)), to_float(*e.t.translate(d.b)),
                                       d.style.colour0, d.style.colour1, d.style.z, d.style.width,
                                       d.tag);
          s.flags = d.flags;
          handle_shape(s);
        }
        break;
      }

      VARIANT_CASE_GET(geom2::resolve_result::ngon, e.data, d) {
        if (d.line.colour0.a || d.line.colour1.a) {
          handle_shape({
              .origin = to_float(*e.t),
              .rotation = e.t.rotation().to_float(),
              .colour0 = d.line.colour0,
              .colour1 = d.line.colour1,
              .z = d.line.z,
              .tag = d.tag,
              .flags = d.flags,
              .data = render::ngon{.radius = d.dimensions.radius.to_float(),
                                   .inner_radius = d.dimensions.inner_radius.to_float(),
                                   .sides = d.dimensions.sides,
                                   .segments = d.dimensions.segments,
                                   .style = d.style,
                                   .line_width = d.line.width},
          });
        }
        if (d.fill.colour0.a || d.fill.colour1.a) {
          handle_shape({
              .origin = to_float(*e.t),
              .rotation = e.t.rotation().to_float(),
              .colour0 = d.fill.colour0,
              .colour1 = d.fill.colour1,
              .z = d.fill.z,
              .tag = d.tag,
              .flags = d.flags,
              .data = render::ngon_fill{.radius = d.dimensions.radius.to_float(),
                                        .inner_radius = d.dimensions.inner_radius.to_float(),
                                        .sides = d.dimensions.sides,
                                        .segments = d.dimensions.segments},
          });
        }
        break;
      }
    }
  }
}

}  // namespace ii::v0