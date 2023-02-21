#include "game/geom2/shape_bank.h"
#include "game/common/collision.h"
#include "game/common/variant_switch.h"

namespace ii::geom2 {
namespace {

resolve_result::fill_style resolve(const fill_style& v, const parameter_set& parameters) {
  return {
      .colour0 = v.colour0(parameters),
      .colour1 = v.colour1(parameters),
      .z = v.z(parameters),
  };
}

resolve_result::line_style resolve(const line_style& v, const parameter_set& parameters) {
  return {
      .colour0 = v.colour0(parameters),
      .colour1 = v.colour1(parameters),
      .z = v.z(parameters),
      .width = v.width(parameters),
  };
}

resolve_result::ngon_dimensions resolve(const ngon_dimensions& v, const parameter_set& parameters) {
  return {
      .radius = v.radius(parameters),
      .inner_radius = v.inner_radius(parameters),
      .sides = v.sides(parameters),
      .segments = v.segments(parameters),
  };
}

resolve_result::ball_dimensions resolve(const ball_dimensions& v, const parameter_set& parameters) {
  return {
      .radius = v.radius(parameters),
      .inner_radius = v.inner_radius(parameters),
  };
}

resolve_result::ball resolve(const ball& v, const parameter_set& parameters) {
  return {
      .dimensions = resolve(v.dimensions, parameters),
      .line = resolve(v.line, parameters),
      .fill = resolve(v.fill, parameters),
      .tag = v.tag(parameters),
      .flags = v.flags(parameters),
  };
}

resolve_result::box resolve(const box& v, const parameter_set& parameters) {
  return {
      .dimensions = v.dimensions(parameters),
      .line = resolve(v.line, parameters),
      .fill = resolve(v.fill, parameters),
      .tag = v.tag(parameters),
      .flags = v.flags(parameters),
  };
}

resolve_result::line resolve(const line& v, const parameter_set& parameters) {
  return {
      .a = v.a(parameters),
      .b = v.b(parameters),
      .style = resolve(v.style, parameters),
      .tag = v.tag(parameters),
      .flags = v.flags(parameters),
  };
}

resolve_result::ngon resolve(const ngon& v, const parameter_set& parameters) {
  return {
      .dimensions = resolve(v.dimensions, parameters),
      .style = v.style(parameters),
      .line = resolve(v.line, parameters),
      .fill = resolve(v.fill, parameters),
      .tag = v.tag(parameters),
      .flags = v.flags(parameters),
  };
}

std::span<const vec2> ngon_convex_segment(const resolve_result::ngon_dimensions dimensions,
                                          std::array<vec2, 4>& va, std::uint32_t i) {
  if (dimensions.inner_radius) {
    vec2 a = ::rotate(vec2{1, 0}, 2 * i * pi<fixed> / dimensions.sides);
    vec2 b = ::rotate(vec2{1, 0}, 2 * (i + 1) * pi<fixed> / dimensions.sides);
    va[0] = dimensions.inner_radius * a;
    va[1] = dimensions.inner_radius * b;
    va[2] = dimensions.radius * b;
    va[3] = dimensions.radius * a;
    return {va.begin(), va.begin() + 4};
  }
  va[0] = vec2{0};
  va[1] = ::rotate(vec2{dimensions.radius, 0}, 2 * i * pi<fixed> / dimensions.sides);
  va[2] = ::rotate(vec2{dimensions.radius, 0}, 2 * (i + 1) * pi<fixed> / dimensions.sides);
  return {va.begin(), va.begin() + 3};
}

void check_collision(hit_result& hit, const convert_local_transform& t, const ball_collider& c,
                     const parameter_set& parameters, const check_t& check) {
  auto flags = c.flags(parameters) & check.mask;
  if (!flags) {
    return;
  }
  auto dimensions = resolve(c.dimensions, parameters);

  switch (check.extent.index()) {
    VARIANT_CASE_GET(check_point_t, check.extent, cx) {
      auto d_sq = length_squared(t.transform_ignore_rotation(cx.v));
      if (d_sq <= square(dimensions.radius) && d_sq >= square(dimensions.inner_radius)) {
        hit.add(flags, t.inverse_transform(vec2{0}));
      }
      break;
    }

    VARIANT_CASE_GET(check_line_t, check.extent, cx) {
      auto a = t.transform(cx.a);
      auto b = t.transform(cx.b);
      auto ir_sq = square(dimensions.inner_radius);
      if (intersect_line_ball(a, b, vec2{0}, dimensions.radius) &&
          (length_squared(a) >= ir_sq || length_squared(b) >= ir_sq)) {
        hit.add(flags, t.inverse_transform(vec2{0}));
      }
      break;
    }

    VARIANT_CASE_GET(check_ball_t, check.extent, cx) {
      auto d_sq = length_squared(t.transform_ignore_rotation(cx.c));
      auto r = dimensions.radius + cx.r;
      if (d_sq <= r * r &&
          (!dimensions.inner_radius || sqrt(d_sq) + cx.r >= dimensions.inner_radius)) {
        hit.add(flags, t.inverse_transform(vec2{0}));
      }
      break;
    }

    VARIANT_CASE_GET(check_convex_t, check.extent, cx) {
      auto va = t.transform(cx.vs);
      auto ir_sq = square(dimensions.inner_radius);
      if (intersect_convex_ball(va, vec2{0}, dimensions.radius) &&
          (!dimensions.inner_radius || std::any_of(va.begin(), va.end(), [&](const vec2& v) {
            return length_squared(v) >= ir_sq;
          }))) {
        hit.add(flags, t.inverse_transform(vec2{0}));
      }
      break;
    }
  }
}

void check_collision(hit_result& hit, const convert_local_transform& t, const box_collider& c,
                     const parameter_set& parameters, const check_t& check) {
  auto flags = c.flags(parameters) & check.mask;
  if (!flags) {
    return;
  }
  auto dimensions = c.dimensions(parameters);

  switch (check.extent.index()) {
    VARIANT_CASE_GET(check_point_t, check.extent, cx) {
      auto v = t.transform(cx.v);
      if (abs(v.x) <= dimensions.x && abs(v.y) <= dimensions.y) {
        hit.add(flags, t.inverse_transform(vec2{0}));
      }
      break;
    }

    VARIANT_CASE_GET(check_line_t, check.extent, cx) {
      if (intersect_aabb_line(-dimensions, dimensions, t.transform(cx.a), t.transform(cx.b))) {
        hit.add(flags, t.inverse_transform(vec2{0}));
      }
      break;
    }

    VARIANT_CASE_GET(check_ball_t, check.extent, cx) {
      if (intersect_aabb_ball(-dimensions, dimensions, t.transform(cx.c), cx.r)) {
        hit.add(flags, t.inverse_transform(vec2{0}));
      }
      break;
    }

    VARIANT_CASE_GET(check_convex_t, check.extent, cx) {
      auto va = t.transform(cx.vs);
      if (intersect_aabb_convex(-dimensions, dimensions, va)) {
        hit.add(flags, t.inverse_transform(vec2{0}));
      }
      break;
    }
  }
}

void check_collision(hit_result& hit, const convert_local_transform& t, const ngon_collider& c,
                     const parameter_set& parameters, const check_t& check) {
  auto flags = c.flags(parameters) & check.mask;
  if (!flags) {
    return;
  }
  auto dimensions = resolve(c.dimensions, parameters);

  std::array<vec2, 4> va{};
  switch (check.extent.index()) {
    VARIANT_CASE_GET(check_point_t, check.extent, cx) {
      auto v = t.transform(cx.v);
      auto theta = angle(v);
      if (theta < 0) {
        theta += 2 * pi<fixed>;
      }
      auto a = 2 * pi<fixed> / dimensions.sides;
      auto at = (theta % a) / a;
      auto dt = (1 + 2 * at * (at - 1) * (1 - cos(a)));
      auto d_sq = dt * square(dimensions.radius);
      auto i_sq = dt * square(dimensions.inner_radius);
      if (theta <= a * dimensions.segments && length_squared(v) <= d_sq &&
          length_squared(v) >= i_sq) {
        hit.add(flags, t.inverse_transform(vec2{0}));
      }
      break;
    }

    VARIANT_CASE_GET(check_line_t, check.extent, cx) {
      auto line_a = t.transform(cx.a);
      auto line_b = t.transform(cx.b);
      for (std::uint32_t i = 0; i < dimensions.segments; ++i) {
        auto convex = ngon_convex_segment(dimensions, va, i);
        if (intersect_convex_line(convex, line_a, line_b)) {
          hit.add(flags, t.inverse_transform(vec2{0}));
          break;
        }
      }
      break;
    }

    VARIANT_CASE_GET(check_ball_t, check.extent, cx) {
      auto v = t.transform(cx.c);
      for (std::uint32_t i = 0; i < dimensions.segments; ++i) {
        auto convex = ngon_convex_segment(dimensions, va, i);
        if (intersect_convex_ball(convex, v, cx.r)) {
          hit.add(flags, t.inverse_transform(vec2{0}));
          break;
        }
      }
      break;
    }

    VARIANT_CASE_GET(check_convex_t, check.extent, cx) {
      auto va0 = t.transform(cx.vs);
      for (std::uint32_t i = 0; i < dimensions.segments; ++i) {
        auto convex = ngon_convex_segment(dimensions, va, i);
        if (intersect_convex_convex(convex, va0)) {
          hit.add(flags, t.inverse_transform(vec2{0}));
          break;
        }
      }
      break;
    }
  }
}

void resolve_internal(resolve_result& result, const transform& t, const node& n,
                      const parameter_set& parameters) {
  auto recurse = [&](const transform& st) {
    for (std::size_t i = 0; i < n.size(); ++i) {
      if (+(n[i].type() & node_type::kResolve)) {
        resolve_internal(result, st, n[i], parameters);
      }
    }
  };

  switch (n->index()) {
    VARIANT_CASE_GET(ball, *n, x) {
      result.add(t, resolve(x, parameters));
      break;
    }

    VARIANT_CASE_GET(box, *n, x) {
      result.add(t, resolve(x, parameters));
      break;
    }

    VARIANT_CASE_GET(line, *n, x) {
      result.add(t, resolve(x, parameters));
      break;
    }

    VARIANT_CASE_GET(ngon, *n, x) {
      result.add(t, resolve(x, parameters));
      break;
    }

    VARIANT_CASE(compound, *n) {
      recurse(t);
      break;
    }

    VARIANT_CASE_GET(enable, *n, x) {
      if (x.x(parameters)) {
        recurse(t);
      }
      break;
    }

    VARIANT_CASE_GET(translate, *n, x) {
      recurse(t.translate(x.x(parameters)));
      break;
    }

    VARIANT_CASE_GET(rotate, *n, x) {
      recurse(t.rotate(x.x(parameters)));
      break;
    }

    VARIANT_CASE_GET(translate_rotate, *n, x) {
      recurse(t.translate(x.v(parameters)).rotate(x.r(parameters)));
      break;
    }
  }
}

void check_collision_internal(hit_result& result, const convert_local_transform& t, const node& n,
                              const parameter_set& parameters, const check_t& check) {
  auto recurse = [&](const convert_local_transform& st) {
    for (std::size_t i = 0; i < n.size(); ++i) {
      if (+(n[i].type() & node_type::kCollision)) {
        check_collision_internal(result, st, n[i], parameters, check);
      }
    }
  };

  switch (n->index()) {
    VARIANT_CASE_GET(ball_collider, *n, x) {
      check_collision(result, t, x, parameters, check);
      break;
    }

    VARIANT_CASE_GET(box_collider, *n, x) {
      check_collision(result, t, x, parameters, check);
      break;
    }

    VARIANT_CASE_GET(ngon_collider, *n, x) {
      check_collision(result, t, x, parameters, check);
      break;
    }

    VARIANT_CASE(compound, *n) {
      recurse(t);
      break;
    }

    VARIANT_CASE_GET(enable, *n, x) {
      if (x.x(parameters)) {
        recurse(t);
      }
      break;
    }

    VARIANT_CASE_GET(translate, *n, x) {
      recurse(t.translate(x.x(parameters)));
      break;
    }

    VARIANT_CASE_GET(rotate, *n, x) {
      recurse(t.rotate(x.x(parameters)));
      break;
    }

    VARIANT_CASE_GET(translate_rotate, *n, x) {
      recurse(t.translate(x.v(parameters)).rotate(x.r(parameters)));
      break;
    }
  }
}

}  // namespace

// TODO: optimize further. For example:
// - can also inline any compound node into its parent.
// - instead of flagging nodes with kCollision or kResolve, stable_partion or duplicate
//   the whole thing?
node_type ShapeBank::optimize(node& n) {
  if (std::holds_alternative<compound>(n.data_) && n.size() == 1u) {
    const auto& c = *n.children_.front();
    n.data_ = c.data_;
    n.bank_ = c.bank_;
    n.children_ = c.children_;
  }
  node_type type = node_type::kNone;
  for (auto* c : n.children_) {
    type |= optimize(*c);
  }

  switch (n->index()) {
    VARIANT_CASE(ball_collider, *n)
    VARIANT_CASE(box_collider, *n)
    VARIANT_CASE(ngon_collider, *n) {
      type |= node_type::kCollision;
      break;
    }

    VARIANT_CASE(ball, *n)
    VARIANT_CASE(box, *n)
    VARIANT_CASE(line, *n)
    VARIANT_CASE(ngon, *n) {
      type |= node_type::kResolve;
      break;
    }
  }
  return n.type_ = type;
}

void resolve(resolve_result& result, const node& n, const parameter_set& parameters) {
  resolve_internal(result, {}, n, parameters);
}

void check_collision(hit_result& result, const node& n, const parameter_set& parameters,
                     const check_t& check) {
  check_collision_internal(result, {}, n, parameters, check);
}

}  // namespace ii::geom2
