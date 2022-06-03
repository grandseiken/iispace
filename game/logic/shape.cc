#include "game/logic/shape.h"
#include "game/logic/sim/sim_interface.h"
#include <glm/gtc/constants.hpp>
#include <cmath>

Shape::Shape(const vec2& centre, fixed rotation, const glm::vec4& colour, std::int32_t category,
             bool can_rotate)
: centre{centre}
, colour{colour}
, category{category}
, rotation_{can_rotate ? rotation : 0}
, can_rotate_{can_rotate} {}

bool Shape::check_point(const vec2& v) const {
  auto a = v - centre;
  if (can_rotate_) {
    a = ::rotate(a, -rotation_);
  }
  return check_local_point(a);
}

vec2 Shape::convert_point(const vec2& position, fixed rotation, const vec2& v) const {
  auto a = v;
  if (can_rotate_) {
    a = ::rotate(a, rotation_);
  }
  a = ::rotate(a + centre, rotation);
  return position + a;
}

glm::vec2
Shape::convert_fl_point(const glm::vec2& position, float rotation, const glm::vec2& v) const {
  auto a = v;
  if (can_rotate_) {
    a = ::rotate(a, rotation_.to_float());
  }
  a = ::rotate(a + to_float(centre), rotation);
  return position + a;
}

fixed Shape::rotation() const {
  return can_rotate_ ? rotation_ : 0;
}

void Shape::set_rotation(fixed rotation) {
  if (can_rotate_) {
    rotation_ = rotation > 2 * fixed_c::pi ? rotation - 2 * fixed_c::pi
        : rotation < 0                     ? rotation + 2 * fixed_c::pi
                                           : rotation;
  }
}

void Shape::rotate(fixed rotation_amount) {
  set_rotation(rotation_ + rotation_amount);
}

Fill::Fill(const vec2& centre, fixed width, fixed height, const glm::vec4& colour,
           std::int32_t category)
: Shape{centre, 0, colour, category, false}, width{width}, height{height} {}

void Fill::render(ii::SimInterface& sim, const glm::vec2& position, float rotation,
                  const std::optional<glm::vec4>& colour_override) const {
  auto c = convert_fl_point(position, rotation, glm::vec2{0.f});
  auto wh = glm::vec2{width.to_float(), height.to_float()};
  auto a = c + wh;
  auto b = c - wh;
  sim.render_line_rect(a, b, colour_override ? *colour_override : colour);
}

bool Fill::check_local_point(const vec2& v) const {
  return abs(v.x) < width && abs(v.y) < height;
}

Line::Line(const vec2& centre, const vec2& a, const vec2& b, const glm::vec4& colour,
           fixed rotation)
: Shape{centre, rotation, colour, 0}, a{a}, b{b} {}

void Line::render(ii::SimInterface& sim, const glm::vec2& position, float rotation,
                  const std::optional<glm::vec4>& colour_override) const {
  auto aa = convert_fl_point(position, rotation, to_float(a));
  auto bb = convert_fl_point(position, rotation, to_float(b));
  sim.render_line(aa, bb, colour_override ? *colour_override : colour);
}

bool Line::check_local_point(const vec2& v) const {
  return false;
}

Box::Box(const vec2& centre, fixed width, fixed height, const glm::vec4& colour, fixed rotation,
         std::int32_t category)
: Shape{centre, rotation, colour, category}, width{width}, height{height} {}

void Box::render(ii::SimInterface& sim, const glm::vec2& position, float rotation,
                 const std::optional<glm::vec4>& colour_override) const {
  float w = width.to_float();
  float h = height.to_float();

  auto a = convert_fl_point(position, rotation, {w, h});
  auto b = convert_fl_point(position, rotation, {-w, h});
  auto c = convert_fl_point(position, rotation, {-w, -h});
  auto d = convert_fl_point(position, rotation, {w, -h});

  sim.render_line(a, b, colour_override ? *colour_override : colour);
  sim.render_line(b, c, colour_override ? *colour_override : colour);
  sim.render_line(c, d, colour_override ? *colour_override : colour);
  sim.render_line(d, a, colour_override ? *colour_override : colour);
}

bool Box::check_local_point(const vec2& v) const {
  return abs(v.x) < width && abs(v.y) < height;
}

Polygon::Polygon(const vec2& centre, fixed radius, std::int32_t sides, const glm::vec4& colour,
                 fixed rotation, std::int32_t category, T type)
: Shape{centre, rotation, colour, category}, radius{radius}, sides{sides}, type{type} {}

void Polygon::render(ii::SimInterface& sim, const glm::vec2& position, float rotation,
                     const std::optional<glm::vec4>& colour_override) const {
  if (sides < 2) {
    return;
  }

  float r = radius.to_float();
  std::vector<glm::vec2> lines;
  if (type == T::kPolygram) {
    std::vector<glm::vec2> list;
    for (std::int32_t i = 0; i < sides; ++i) {
      list.push_back(from_polar(i * 2 * glm::pi<float>() / sides, r));
    }

    for (std::size_t i = 0; i < list.size(); ++i) {
      for (std::size_t j = i + 1; j < list.size(); ++j) {
        lines.push_back(list[i]);
        lines.push_back(list[j]);
      }
    }
  } else {
    for (std::int32_t i = 0; i < sides; ++i) {
      auto a = from_polar(i * 2 * glm::pi<float>() / sides, r);
      auto b = from_polar((i + 1) * 2 * glm::pi<float>() / sides, r);
      lines.push_back(a);
      lines.push_back(type == T::kPolygon ? b : glm::vec2{0.f});
    }
  }
  for (std::size_t i = 0; i < lines.size(); i += 2) {
    sim.render_line(convert_fl_point(position, rotation, lines[i]),
                    convert_fl_point(position, rotation, lines[i + 1]),
                    colour_override ? *colour_override : colour);
  }
}

bool Polygon::check_local_point(const vec2& v) const {
  return length(v) < radius;
}

PolyArc::PolyArc(const vec2& centre, fixed radius, std::int32_t sides, std::int32_t segments,
                 const glm::vec4& colour, fixed rotation, std::int32_t category)
: Shape{centre, rotation, colour, category}, radius{radius}, sides{sides}, segments{segments} {}

void PolyArc::render(ii::SimInterface& sim, const glm::vec2& position, float rotation,
                     const std::optional<glm::vec4>& colour_override) const {
  if (sides < 2) {
    return;
  }
  float r = radius.to_float();

  for (std::int32_t i = 0; i < sides && i < segments; ++i) {
    auto a = from_polar(i * 2 * glm::pi<float>() / sides, r);
    auto b = from_polar((i + 1) * 2 * glm::pi<float>() / sides, r);
    sim.render_line(convert_fl_point(position, rotation, a),
                    convert_fl_point(position, rotation, b),
                    colour_override ? *colour_override : colour);
  }
}

bool PolyArc::check_local_point(const vec2& v) const {
  auto theta = angle(v);
  auto r = length(v);
  bool b = 0 <= theta && theta <= (2 * fixed_c::pi * segments) / sides;
  return b && r >= radius - 10 && r < radius;
}

CompoundShape::CompoundShape(const vec2& centre, fixed rotation, std::int32_t category)
: Shape{centre, rotation, glm::vec4{0.f}, category} {}

const CompoundShape::shape_list& CompoundShape::shapes() const {
  return children_;
}

Shape* CompoundShape::add_shape(std::unique_ptr<Shape> shape) {
  auto p = shape.get();
  children_.emplace_back(std::move(shape));
  return p;
}

void CompoundShape::destroy_shape(std::size_t index) {
  if (index < children_.size()) {
    children_.erase(index + children_.begin());
  }
}

void CompoundShape::clear_shapes() {
  children_.clear();
}

void CompoundShape::render(ii::SimInterface& sim, const glm::vec2& position, float rot,
                           const std::optional<glm::vec4>& colour_override) const {
  auto c = convert_fl_point(position, rot, glm::vec2{0.f});
  for (const auto& child : children_) {
    child->render(sim, c, rotation().to_float() + rot, colour_override);
  }
}

bool CompoundShape::check_local_point(const vec2& v) const {
  for (const auto& child : children_) {
    if (child->check_point(v)) {
      return true;
    }
  }
  return false;
}
