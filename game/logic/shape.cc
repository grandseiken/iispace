#include "game/logic/shape.h"
#include "game/core/lib.h"

Shape::Shape(const vec2& centre, fixed rotation, colour_t colour, std::int32_t category,
             bool can_rotate)
: centre(centre)
, colour(colour)
, category(category)
, _rotation(can_rotate ? rotation : 0)
, _can_rotate(can_rotate) {}

bool Shape::check_point(const vec2& v) const {
  vec2 a = v - centre;
  if (_can_rotate) {
    a = a.rotated(-_rotation);
  }
  return check_local_point(a);
}

vec2 Shape::convert_point(const vec2& position, fixed rotation, const vec2& v) const {
  vec2 a = v;
  if (_can_rotate) {
    a = a.rotated(_rotation);
  }
  a = (a + centre).rotated(rotation);
  return position + a;
}

fvec2 Shape::convert_fl_point(const fvec2& position, float rotation, const fvec2& v) const {
  fvec2 a = v;
  if (_can_rotate) {
    a = a.rotated(_rotation.to_float());
  }
  a = (a + to_float(centre)).rotated(rotation);
  return position + a;
}

fixed Shape::rotation() const {
  return _can_rotate ? _rotation : 0;
}

void Shape::set_rotation(fixed rotation) {
  if (_can_rotate) {
    _rotation = rotation > 2 * fixed_c::pi ? rotation - 2 * fixed_c::pi
        : rotation < 0                     ? rotation + 2 * fixed_c::pi
                                           : rotation;
  }
}

void Shape::rotate(fixed rotation_amount) {
  set_rotation(_rotation + rotation_amount);
}

Fill::Fill(const vec2& centre, fixed width, fixed height, colour_t colour, std::int32_t category)
: Shape(centre, 0, colour, category, false), width(width), height(height) {}

void Fill::render(Lib& lib, const fvec2& position, float rotation, colour_t colour_override) const {
  fvec2 c = convert_fl_point(position, rotation, fvec2());
  fvec2 wh = fvec2(width.to_float(), height.to_float());
  fvec2 a = c + wh;
  fvec2 b = c - wh;
  lib.render_rect(a, b, colour_override ? colour_override : colour);
}

bool Fill::check_local_point(const vec2& v) const {
  return abs(v.x) < width && abs(v.y) < height;
}

Line::Line(const vec2& centre, const vec2& a, const vec2& b, colour_t colour, fixed rotation)
: Shape(centre, rotation, colour, 0), a(a), b(b) {}

void Line::render(Lib& lib, const fvec2& position, float rotation, colour_t colour_override) const {
  fvec2 aa = convert_fl_point(position, rotation, to_float(a));
  fvec2 bb = convert_fl_point(position, rotation, to_float(b));
  lib.render_line(aa, bb, colour_override ? colour_override : colour);
}

bool Line::check_local_point(const vec2& v) const {
  return false;
}

Box::Box(const vec2& centre, fixed width, fixed height, colour_t colour, fixed rotation,
         std::int32_t category)
: Shape(centre, rotation, colour, category), width(width), height(height) {}

void Box::render(Lib& lib, const fvec2& position, float rotation, colour_t colour_override) const {
  float w = width.to_float();
  float h = height.to_float();

  fvec2 a = convert_fl_point(position, rotation, fvec2(w, h));
  fvec2 b = convert_fl_point(position, rotation, fvec2(-w, h));
  fvec2 c = convert_fl_point(position, rotation, fvec2(-w, -h));
  fvec2 d = convert_fl_point(position, rotation, fvec2(w, -h));

  lib.render_line(a, b, colour_override ? colour_override : colour);
  lib.render_line(b, c, colour_override ? colour_override : colour);
  lib.render_line(c, d, colour_override ? colour_override : colour);
  lib.render_line(d, a, colour_override ? colour_override : colour);
}

bool Box::check_local_point(const vec2& v) const {
  return abs(v.x) < width && abs(v.y) < height;
}

Polygon::Polygon(const vec2& centre, fixed radius, std::int32_t sides, colour_t colour,
                 fixed rotation, std::int32_t category, T type)
: Shape(centre, rotation, colour, category), radius(radius), sides(sides), type(type) {}

void Polygon::render(Lib& lib, const fvec2& position, float rotation,
                     colour_t colour_override) const {
  if (sides < 2) {
    return;
  }

  float r = radius.to_float();
  std::vector<fvec2> lines;
  if (type == T::kPolygram) {
    std::vector<fvec2> list;
    for (std::int32_t i = 0; i < sides; i++) {
      fvec2 v = fvec2::from_polar(i * 2 * M_PIf / float(sides), r);
      list.push_back(v);
    }

    for (std::size_t i = 0; i < list.size(); i++) {
      for (std::size_t j = i + 1; j < list.size(); j++) {
        lines.push_back(list[i]);
        lines.push_back(list[j]);
      }
    }
  } else {
    for (std::int32_t i = 0; i < sides; i++) {
      fvec2 a = fvec2::from_polar(i * 2 * M_PIf / float(sides), r);
      fvec2 b = fvec2::from_polar((i + 1) * 2 * M_PIf / float(sides), r);
      lines.push_back(a);
      lines.push_back(type == T::kPolygon ? b : fvec2());
    }
  }
  for (std::size_t i = 0; i < lines.size(); i += 2) {
    lib.render_line(convert_fl_point(position, rotation, lines[i]),
                    convert_fl_point(position, rotation, lines[i + 1]),
                    colour_override ? colour_override : colour);
  }
}

bool Polygon::check_local_point(const vec2& v) const {
  return v.length() < radius;
}

PolyArc::PolyArc(const vec2& centre, fixed radius, std::int32_t sides, std::int32_t segments,
                 colour_t colour, fixed rotation, std::int32_t category)
: Shape(centre, rotation, colour, category), radius(radius), sides(sides), segments(segments) {}

void PolyArc::render(Lib& lib, const fvec2& position, float rotation,
                     colour_t colour_override) const {
  if (sides < 2) {
    return;
  }
  float r = radius.to_float();

  for (std::int32_t i = 0; i < sides && i < segments; i++) {
    fvec2 a = fvec2::from_polar(i * 2 * M_PIf / float(sides), r);
    fvec2 b = fvec2::from_polar((i + 1) * 2 * M_PIf / float(sides), r);
    lib.render_line(convert_fl_point(position, rotation, a),
                    convert_fl_point(position, rotation, b),
                    colour_override ? colour_override : colour);
  }
}

bool PolyArc::check_local_point(const vec2& v) const {
  fixed angle = v.angle();
  fixed len = v.length();
  bool b = 0 <= angle && v.angle() <= (2 * fixed_c::pi * segments) / sides;
  return b && len >= radius - 10 && len < radius;
}

CompoundShape::CompoundShape(const vec2& centre, fixed rotation, std::int32_t category)
: Shape(centre, rotation, 0, category) {}

const CompoundShape::shape_list& CompoundShape::shapes() const {
  return _children;
}

void CompoundShape::add_shape(Shape* shape) {
  _children.emplace_back(shape);
}

void CompoundShape::destroy_shape(std::size_t index) {
  if (index < _children.size()) {
    _children.erase(index + _children.begin());
  }
}

void CompoundShape::clear_shapes() {
  _children.clear();
}

void CompoundShape::render(Lib& lib, const fvec2& position, float rot, colour_t colour) const {
  fvec2 c = convert_fl_point(position, rot, fvec2());
  for (const auto& child : _children) {
    child->render(lib, c, rotation().to_float() + rot, colour);
  }
}

bool CompoundShape::check_local_point(const vec2& v) const {
  for (const auto& child : _children) {
    if (child->check_point(v)) {
      return true;
    }
  }
  return false;
}
