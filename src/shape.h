#ifndef IISPACE_SRC_SHAPE_H
#define IISPACE_SRC_SHAPE_H

#include "z.h"
#include "lib.h"

// Basic shape
//------------------------------
class Shape {
public:

  Shape(const vec2& centre, fixed rotation, colour_t colour,
        int32_t category, bool can_rotate = true)
    : centre(centre)
    , colour(colour)
    , category(category)
    , _rotation(can_rotate ? rotation : 0)
    , _can_rotate(can_rotate) {}
  virtual ~Shape() {}

  bool check_point(const vec2& v) const
  {
    vec2 a = v - centre;
    if (_can_rotate) {
      a.rotate(-_rotation);
    }
    return check_local_point(a);
  }

  vec2 convert_point(
      const vec2& position, fixed rotation, const vec2& v) const
  {
    vec2 a = v;
    if (_can_rotate) {
      a.rotate(_rotation);
    }
    a += centre;
    a.rotate(rotation);
    return position + a;
  }

  flvec2 convert_fl_point(
      const flvec2& position, float rotation, const flvec2& v) const
  {
    flvec2 a = v;
    if (_can_rotate) {
      a.rotate(_rotation.to_float());
    }
    a += to_float(centre);
    a.rotate(rotation);
    return position + a;
  }

  fixed rotation() const
  {
    return _can_rotate ? _rotation : 0;
  }

  void set_rotation(fixed rotation)
  {
    if (_can_rotate) {
      _rotation =
          rotation > 2 * fixed::pi ? rotation - 2 * fixed::pi :
          rotation < 0 ? rotation + 2 * fixed::pi : rotation;
    }
  }

  void rotate(fixed rotation_amount)
  {
    set_rotation(_rotation + rotation_amount);
  }

  virtual void render(Lib& lib, const flvec2& position, float rotation,
                      colour_t colour_override = 0) const = 0;

  vec2 centre;
  colour_t colour;
  int32_t category;

private:

  virtual bool check_local_point(const vec2& v) const = 0;
  
  fixed _rotation;
  bool _can_rotate;

};

// Filled rectangle
//------------------------------
class Fill : public Shape {
public:

  Fill(const vec2& centre, fixed width, fixed height,
       colour_t colour, int32_t category = 0)
    : Shape(centre, 0, colour, category, false)
    , width(width)
    , height(height) {}

  void render(Lib& lib, const flvec2& position, float rotation,
              colour_t colour_override = 0) const override
  {
    flvec2 c = convert_fl_point(position, rotation, flvec2());
    flvec2 wh = flvec2(width.to_float(), height.to_float());
    flvec2 a = c + wh;
    flvec2 b = c - wh;
    lib.RenderRect(a, b, colour_override ? colour_override : colour);
  }

  fixed width;
  fixed height;

private:

  bool check_local_point(const vec2& v) const override
  {
    return v.x.abs() < width && v.y.abs() < height;
  }

};

// Line between two points
//------------------------------
class Line : public Shape {
public:

  Line(const vec2& centre, const vec2& a, const vec2& b,
       colour_t colour, fixed rotation = 0)
    : Shape(centre, rotation, colour, 0)
    , a(a)
    , b(b) {}

  void render(Lib& lib, const flvec2& position, float rotation,
              colour_t colour_override = 0) const override
  {
    flvec2 aa = convert_fl_point(position, rotation, to_float(a));
    flvec2 bb = convert_fl_point(position, rotation, to_float(b));
    lib.RenderLine(aa, bb, colour_override ? colour_override : colour);
  }

  vec2 a;
  vec2 b;

private:

  bool check_local_point(const vec2& v) const override
  {
    return false;
  }

};

// Box with width and height
//------------------------------
class Box : public Shape {
public:

  Box(const vec2& centre, fixed width, fixed height,
      colour_t colour, fixed rotation = 0, int32_t category = 0)
    : Shape(centre, rotation, colour, category)
    , width(width)
    , height(height) {}

  void render(Lib& lib, const flvec2& position, float rotation,
              colour_t colour_override = 0) const override
  {
    float w = width.to_float();
    float h = height.to_float();

    flvec2 a = convert_fl_point(position, rotation, flvec2(w, h));
    flvec2 b = convert_fl_point(position, rotation, flvec2(-w, h));
    flvec2 c = convert_fl_point(position, rotation, flvec2(-w, -h));
    flvec2 d = convert_fl_point(position, rotation, flvec2(w, -h));

    lib.RenderLine(a, b, colour_override ? colour_override : colour);
    lib.RenderLine(b, c, colour_override ? colour_override : colour);
    lib.RenderLine(c, d, colour_override ? colour_override : colour);
    lib.RenderLine(d, a, colour_override ? colour_override : colour);
  }

  fixed width;
  fixed height;

private:

  bool check_local_point(const vec2& v) const override
  {
    return v.x.abs() < width && v.y.abs() < height;
  }

};

// Outlined/spoked/complete regular polygon
//------------------------------
class Polygon : public Shape {
public:

  enum class T {
    POLYGON,
    POLYSTAR,
    POLYGRAM,
  };

  Polygon(const vec2& centre, fixed radius, int32_t sides, colour_t colour,
          fixed rotation = 0, int32_t category = 0, T type = T::POLYGON)
    : Shape(centre, rotation, colour, category)
    , radius(radius)
    , sides(sides)
    , type(type) {}

  void render(Lib& lib, const flvec2& position, float rotation,
              colour_t colour_override = 0) const override
  {
    if (sides < 2) {
      return;
    }

    float r = radius.to_float();
    std::vector<flvec2> lines;
    if (type == T::POLYGON) {
      for (int32_t i = 0; i < sides; i++) {
        flvec2 a, b;
        a.set_polar(i * 2 * M_PIf / float(sides), r);
        b.set_polar((i + 1) * 2 * M_PIf / float(sides), r);
        lines.push_back(a);
        lines.push_back(b);
      }
    }
    else if (type == T::POLYGRAM) {
      std::vector<flvec2> list;
      for (int32_t i = 0; i < sides; i++) {
        flvec2 v;
        v.set_polar(i * 2 * M_PIf / float(sides), r);
        list.push_back(v);
      }

      for (std::size_t i = 0; i < list.size(); i++) {
        for (std::size_t j = i + 1; j < list.size(); j++) {
          lines.push_back(list[i]);
          lines.push_back(list[j]);
        }
      }
    } else if (type == T::POLYSTAR) {
      for (int32_t i = 0; i < sides; i++) {
        flvec2 v;
        v.set_polar(i * 2 * M_PIf / float(sides), r);
        lines.push_back(flvec2());
        lines.push_back(v);
      }
    }
    for (std::size_t i = 0; i < lines.size(); ++i) {
      lib.RenderLine(convert_fl_point(position, rotation, lines[i]),
                     convert_fl_point(position, rotation, lines[i + 1]),
                     colour_override ? colour_override : colour);
    }
  }

  fixed radius;
  int32_t sides;
  T type;

private:

  bool check_local_point(const vec2& v) const override
  {
    return v.length() < radius;
  }

};

// Polygon segment
//------------------------------
class PolyArc : public Shape {
public:

  PolyArc(const vec2& centre, fixed radius, int32_t sides, int32_t segments,
          colour_t colour, fixed rotation = 0, int32_t category = 0)
    : Shape(centre, rotation, colour, category)
    , radius(radius)
    , sides(sides)
    , segments(segments) {}

  void render(Lib& lib, const flvec2& position, float rotation,
              colour_t colour_override = 0) const override
  {
    if (sides < 2) {
      return;
    }
    float r = radius.to_float();

    for (int32_t i = 0; i < sides && i < segments; i++) {
      flvec2 a, b;
      a.set_polar(i * 2 * M_PIf / float(sides), r);
      b.set_polar((i + 1) * 2 * M_PIf / float(sides), r);
      lib.RenderLine(convert_fl_point(position, rotation, a),
                     convert_fl_point(position, rotation, b),
                     colour_override ? colour_override : colour);
    }
  }

  fixed radius;
  int32_t sides;
  int32_t segments;

private:

  bool check_local_point(const vec2& v) const override
  {
    fixed angle = v.angle();
    fixed len = v.length();
    bool b = 0 <= angle && v.angle() <= (2 * fixed::pi * segments) / sides;
    return b && len >= radius - 10 && len < radius;
  }

};

// Group of lesser shapes
//------------------------------
class CompoundShape : public Shape {
public:

  // Child shapes take the top-level category
  CompoundShape(const vec2& centre, fixed rotation = 0,
                int32_t category = 0)
    : Shape(centre, rotation, 0, category)
  {
  }

  typedef std::vector<std::unique_ptr<Shape>> shape_list;
  const shape_list& shapes() const
  {
    return _children;
  }

  void add_shape(Shape* shape)
  {
    _children.emplace_back(shape);
  }

  void clear_shapes()
  {
    _children.clear();
  }

  void render(Lib& lib, const flvec2& position, float rot,
              colour_t colour = 0) const override
  {
    flvec2 c = convert_fl_point(position, rot, flvec2());
    for (const auto& child : _children) {
      child->render(lib, c, rotation().to_float() + rot, colour);
    }
  }

private:

  bool check_local_point(const vec2& v) const override
  {
    for (const auto& child : _children) {
      if (child->check_point(v)) {
        return true;
      }
    }
    return false;
  }

  shape_list _children;

};

#endif
