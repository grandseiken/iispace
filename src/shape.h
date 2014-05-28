#ifndef IISPACE_SRC_SHAPE_H
#define IISPACE_SRC_SHAPE_H

#include "z.h"
#include "lib.h"

// Basic shape
//------------------------------
class Shape {
public:

  Shape(const vec2& centre, colour colour, int category)
    : _centre(centre)
    , _colour(colour)
    , _category(category) {}

  virtual ~Shape() {}

  const vec2& GetCentre() const
  {
    return _centre;
  }

  void SetCentre(const vec2& centre)
  {
    _centre = centre;
  }

  int GetCategory() const
  {
    return _category;
  }

  void SetCategory(int category)
  {
    _category = category;
  }

  colour GetColour(colour colour = 0) const
  {
    return colour ? colour : _colour;
  }

  void SetColour(colour colour)
  {
    _colour = colour;
  }

  bool CheckPoint(const vec2& v) const
  {
    return CheckLocalPoint(v - GetCentre());
  }

  virtual vec2 ConvertPoint(
      const vec2& position, fixed rotation, const vec2& v) const
  {
    vec2 a = GetCentre() + v;
    a.rotate(rotation);
    return position + a;
  }

  virtual flvec2 ConvertPointf(
      const flvec2& position, float rotation, const flvec2& v) const
  {
    flvec2 a = to_float(GetCentre()) + v;
    a.rotate(rotation);
    return position + a;
  }

  virtual void Render(
      Lib& lib,
      const flvec2& position, float rotation, colour colour = 0) const = 0;

  virtual fixed rotation() const
  {
    return 0;
  }

  virtual void set_rotation(fixed rot) {}

  void rotate(fixed rotation_amount)
  {
    set_rotation(rotation() + rotation_amount);
  }

private:

  virtual bool CheckLocalPoint(const vec2& v) const = 0;

  vec2 _centre;
  colour _colour;
  int _category;

};

// Shape with independent rotation
//------------------------------
class RotateShape : public Shape {
public:

  RotateShape(const vec2& centre, fixed rotation, colour colour, int category)
    : Shape(centre, colour, category)
    , _rotation(rotation) {}

  virtual ~RotateShape() {}

  virtual fixed rotation() const
  {
    return _rotation;
  }

  virtual void set_rotation(fixed rotation)
  {
    _rotation =
        rotation > 2 * fixed::pi ? rotation - 2 * fixed::pi :
        rotation < 0 ? rotation + 2 * fixed::pi : rotation;
  }

  virtual vec2 ConvertPoint(
      const vec2& position, fixed rot, const vec2& v) const
  {
    vec2 a = v; a.rotate(rotation());
    return Shape::ConvertPoint(position, rot, a);
  }

  virtual flvec2 ConvertPointf(
      const flvec2& position, float rot, const flvec2& v) const
  {
    flvec2 a = v; a.rotate(rotation().to_float());
    return Shape::ConvertPointf(position, rot, a);
  }

private:

  virtual bool CheckLocalPoint(const vec2& v) const
  {
    vec2 a = v; a.rotate(-rotation());
    return CheckRotatedPoint(a);
  }

  virtual bool CheckRotatedPoint(const vec2& v) const = 0;

  fixed _rotation;

};

// Filled rectangle
//------------------------------
class Fill : public Shape {
public:

  Fill(const vec2& centre, fixed width, fixed height,
       colour colour, int category = 0)
    : Shape(centre, colour, category)
    , _width(width)
    , _height(height) {}

  virtual ~Fill() {}

  fixed GetWidth() const
  {
    return _width;
  }

  fixed GetHeight() const
  {
    return _height;
  }

  void SetWidth(fixed width)
  {
    _width = width;
  }

  void SetHeight(fixed height)
  {
    _height = height;
  }

  virtual void Render(
      Lib& lib, const flvec2& position, float rotation, colour colour = 0) const
  {
    flvec2 c = ConvertPointf(position, rotation, flvec2());
    flvec2 wh = flvec2(GetWidth().to_float(), GetHeight().to_float());
    flvec2 a = c + wh;
    flvec2 b = c - wh;
    lib.RenderRect(a, b, GetColour(colour));
  }

private:

  virtual bool CheckLocalPoint(const vec2& v) const
  {
    return v.x.abs() < _width && v.y.abs() < _height;
  }

  fixed _width;
  fixed _height;

};

// Line between two points
//------------------------------
class Line : public RotateShape {
public:

  Line(const vec2& centre, const vec2& a, const vec2& b,
       colour colour, fixed rotation = 0)
    : RotateShape(centre, rotation, colour, 0)
    , _a(a)
    , _b(b) {}

  virtual ~Line() {}

  vec2 GetA() const
  {
    return _a;
  }

  vec2 GetB() const
  {
    return _b;
  }

  void SetA(const vec2& a)
  {
    _a = a;
  }

  void SetB(const vec2& b)
  {
    _b = b;
  }

  virtual void Render(Lib& lib, const flvec2& position, float rotation,
                      colour colour = 0) const
  {
    flvec2 a = ConvertPointf(position, rotation, to_float(_a));
    flvec2 b = ConvertPointf(position, rotation, to_float(_b));
    lib.RenderLine(a, b, GetColour(colour));
  }

private:

  virtual bool CheckRotatedPoint(const vec2& v) const
  {
    return false;
  }

  vec2 _a;
  vec2 _b;

};

// Box with width and height
//------------------------------
class Box : public RotateShape {
public:

  Box(const vec2& centre, fixed width, fixed height,
      colour colour, fixed rotation = 0, int category = 0)
    : RotateShape(centre, rotation, colour, category)
    , _width(width)
    , _height(height) {}

  virtual ~Box() {}

  fixed GetWidth() const
  {
    return _width;
  }

  fixed GetHeight() const
  {
    return _height;
  }

  void SetWidth(fixed width)
  {
    _width = width;
  }

  void SetHeight(fixed height)
  {
    _height = height;
  }

  virtual void Render(
      Lib& lib, const flvec2& position, float rotation, colour colour = 0) const
  {
    float w = GetWidth().to_float();
    float h = GetHeight().to_float();

    flvec2 a = ConvertPointf(position, rotation, flvec2(w, h));
    flvec2 b = ConvertPointf(position, rotation, flvec2(-w, h));
    flvec2 c = ConvertPointf(position, rotation, flvec2(-w, -h));
    flvec2 d = ConvertPointf(position, rotation, flvec2(w, -h));

    lib.RenderLine(a, b, GetColour(colour));
    lib.RenderLine(b, c, GetColour(colour));
    lib.RenderLine(c, d, GetColour(colour));
    lib.RenderLine(d, a, GetColour(colour));
  }

private:

  virtual bool CheckRotatedPoint(const vec2& v) const
  {
    return v.x.abs() < _width && v.y.abs() < _height;
  }

  fixed _width;
  fixed _height;

};

// Outlined regular polygon
//------------------------------
class Polygon : public RotateShape {
public:

  Polygon(const vec2& centre, fixed radius, int sides,
          colour colour, fixed rotation = 0, int category = 0)
    : RotateShape(centre, rotation, colour, category)
    , _radius(radius)
    , _sides(sides) {}

  virtual ~Polygon() {}

  fixed GetRadius() const
  {
    return _radius;
  }

  void SetRadius(fixed radius)
  {
    _radius = radius;
  }

  int GetSides() const
  {
    return _sides;
  }

  void SetSides(int sides)
  {
    _sides = sides;
  }

  virtual void Render(
      Lib& lib, const flvec2& position, float rotation, colour colour = 0) const
  {
    if (GetSides() < 2) {
      return;
    }

    float r = GetRadius().to_float();
    for (int i = 0; i < GetSides(); i++) {
      flvec2 a, b;
      a.set_polar(i * 2 * M_PIf / float(GetSides()), r);
      b.set_polar((i + 1) * 2 * M_PIf / float(GetSides()), r);
      lib.RenderLine(ConvertPointf(position, rotation, a),
                     ConvertPointf(position, rotation, b), GetColour(colour));
    }
  }

private:

  virtual bool CheckRotatedPoint(const vec2& v) const
  {
    return v.length() < GetRadius();
  }

  fixed _radius;
  int _sides;

};

// Polygon segment
//------------------------------
class PolyArc : public RotateShape {
public:

  PolyArc(const vec2& centre, fixed radius, int sides, int segments,
          colour colour, fixed rotation = 0, int category = 0)
    : RotateShape(centre, rotation, colour, category)
    , _radius(radius)
    , _sides(sides)
    , _segments(segments) {}

  virtual ~PolyArc() {}

  fixed GetRadius() const
  {
    return _radius;
  }

  void SetRadius(fixed radius)
  {
    _radius = radius;
  }

  int GetSides() const
  {
    return _sides;
  }

  void SetSides(int sides)
  {
    _sides = sides;
  }

  int GetSegments() const
  {
    return _segments;
  }

  void SetSegments(int segments)
  {
    _segments = segments;
  }

  virtual void Render(
      Lib& lib, const flvec2& position, float rotation, colour colour = 0) const
  {
    if (GetSides() < 2) {
      return;
    }
    float r = GetRadius().to_float();

    for (int i = 0; i < GetSides() && i < _segments; i++) {
      flvec2 a, b;
      a.set_polar(i * 2 * M_PIf / float(GetSides()), r);
      b.set_polar((i + 1) * 2 * M_PIf / float(GetSides()), r);
      lib.RenderLine(ConvertPointf(position, rotation, a),
          ConvertPointf(position, rotation, b), GetColour(colour));
    }
  }

private:

  virtual bool CheckRotatedPoint(const vec2& v) const
  {
    fixed angle = v.angle();
    fixed len = v.length();
    bool b = 0 <= angle && v.angle() <= (2 * fixed::pi * _segments) / _sides;
    return b && len >= GetRadius() - 10 && len < GetRadius();
  }

  fixed _radius;
  int _sides;
  int _segments;

};

// Complete regular polygon
//------------------------------
class Polygram : public Polygon {
public:

  Polygram(const vec2& centre, fixed radius, int sides,
           colour colour, fixed rotation = 0, int category = 0)
    : Polygon(centre, radius, sides, colour, rotation, category) {}

  virtual ~Polygram() {}

  virtual void Render(
      Lib& lib, const flvec2& position, float rotation, colour colour = 0) const
  {
    if (GetSides() < 2) {
      return;
    }

    float r = GetRadius().to_float();
    std::vector<flvec2> list;
    for (int i = 0; i < GetSides(); i++) {
      flvec2 v;
      v.set_polar(i * 2 * M_PIf / float(GetSides()), r);
      list.push_back(v);
    }

    for (unsigned int i = 0; i < list.size(); i++) {
      for (unsigned int j = i + 1; j < list.size(); j++) {
        lib.RenderLine(
            ConvertPointf(position, rotation, list[i]),
            ConvertPointf(position, rotation, list[j]), GetColour(colour));
      }
    }
  }

};

// Spoked regular polygon
//------------------------------
class Polystar : public Polygon {
public:

  Polystar(const vec2& centre, fixed radius, int sides,
           colour colour, fixed rotation = 0, int category = 0)
    : Polygon(centre, radius, sides, colour, rotation, category) {}

  virtual ~Polystar() {}

  virtual void Render(
      Lib& lib, const flvec2& position, float rotation, colour colour = 0) const
  {
    if (GetSides() < 2) {
      return;
    }

    float r = GetRadius().to_float();
    for (int i = 0; i < GetSides(); i++) {
      flvec2 v;
      v.set_polar(i * 2 * M_PIf / float(GetSides()), r);
      lib.RenderLine(ConvertPointf(position, rotation, flvec2()),
                     ConvertPointf(position, rotation, v), GetColour(colour));
    }
  }

};

// Group of lesser shapes
//------------------------------
class CompoundShape : public RotateShape {
public:

  // Child shapes take the top-level category
  CompoundShape(const vec2& centre, fixed rotation = 0,
                int category = 0, colour colour = 0)
    : RotateShape(centre, rotation, 0, category)
  {
    SetColour(colour);
  }

  virtual ~CompoundShape()
  {
    for (unsigned int i = 0; i < _children.size(); i++) {
      delete _children[i];
    }
  }

  void add_shape(Shape* shape)
  {
    _children.push_back(shape);
  }

  void ClearShapes()
  {
    for (unsigned int i = 0; i < _children.size(); i++) {
      delete _children[i];
    }
    _children.clear();
  }

  virtual void Render(
      Lib& lib, const flvec2& position, float rot, colour colour = 0) const
  {
    flvec2 c = ConvertPointf(position, rot, flvec2());
    for (unsigned int i = 0; i < _children.size(); i++) {
      _children[i]->Render(lib, c, rotation().to_float() + rot, colour);
    }
  }

  std::size_t CountShapes() const
  {
    return _children.size();
  }

  Shape& get_shape(std::size_t i) const
  {
    return *_children[i];
  }

private:

  virtual bool CheckRotatedPoint(const vec2& v) const
  {
    for (unsigned int i = 0; i < _children.size(); i++) {
      if (_children[i]->CheckPoint(v)) {
        return true;
      }
    }
    return false;
  }

  typedef std::vector<Shape*> ShapeList;
  ShapeList _children;

};

#endif
