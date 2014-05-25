#ifndef SHAPE_H
#define SHAPE_H

#include "z0.h"
#include "lib.h"

// Basic shape
//------------------------------
class Shape {
public:

  Shape(const Vec2& centre, Colour colour, int category)
    : _centre(centre)
    , _colour(colour)
    , _category(category) {}

  virtual ~Shape() {}

  const Vec2& GetCentre() const
  {
    return _centre;
  }

  void SetCentre(const Vec2& centre)
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

  Colour GetColour(Colour colour = 0) const
  {
    return colour ? colour : _colour;
  }

  void SetColour(Colour colour)
  {
    _colour = colour;
  }

  bool CheckPoint(const Vec2& v) const
  {
    return CheckLocalPoint(v - GetCentre());
  }

  virtual Vec2 ConvertPoint(
      const Vec2& position, fixed rotation, const Vec2& v) const
  {
    Vec2 a = GetCentre() + v;
    a.Rotate(rotation);
    return position + a;
  }

  virtual Vec2f ConvertPointf(
      const Vec2f& position, float rotation, const Vec2f& v) const
  {
    Vec2f a = Vec2f(GetCentre()) + v;
    a.Rotate(rotation);
    return position + a;
  }

  virtual void Render(
      Lib& lib,
      const Vec2f& position, float rotation, Colour colour = 0) const = 0;

  virtual fixed GetRotation() const
  {
    return 0;
  }

  virtual void SetRotation(fixed rotation) {}

  void Rotate(fixed rotation)
  {
    SetRotation(GetRotation() + rotation);
  }

private:

  virtual bool CheckLocalPoint(const Vec2& v) const = 0;

  Vec2 _centre;
  Colour _colour;
  int _category;

};

// Shape with independent rotation
//------------------------------
class RotateShape : public Shape {
public:

  RotateShape(const Vec2& centre, fixed rotation, Colour colour, int category)
    : Shape(centre, colour, category)
    , _rotation(rotation) {}

  virtual ~RotateShape() {}

  virtual fixed GetRotation() const
  {
    return _rotation;
  }

  virtual void SetRotation(fixed rotation)
  {
    _rotation =
        rotation > M_2PI ? rotation - M_2PI :
        rotation < 0 ? rotation + M_2PI : rotation;
  }

  virtual Vec2 ConvertPoint(
      const Vec2& position, fixed rotation, const Vec2& v) const
  {
    Vec2 a = v; a.Rotate(GetRotation());
    return Shape::ConvertPoint(position, rotation, a);
  }

  virtual Vec2f ConvertPointf(
      const Vec2f& position, float rotation, const Vec2f& v) const
  {
    Vec2f a = v; a.Rotate(z_float(GetRotation()));
    return Shape::ConvertPointf(position, rotation, a);
  }

private:

  virtual bool CheckLocalPoint(const Vec2& v) const
  {
    Vec2 a = v; a.Rotate(-GetRotation());
    return CheckRotatedPoint(a);
  }

  virtual bool CheckRotatedPoint(const Vec2& v) const = 0;

  fixed _rotation;

};

// Filled rectangle
//------------------------------
class Fill : public Shape {
public:

  Fill(const Vec2& centre, fixed width, fixed height,
       Colour colour, int category = 0)
    : Shape(centre, colour, category)
    , _width(width)
    ,_height(height) {}

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
      Lib& lib, const Vec2f& position, float rotation, Colour colour = 0) const
  {
    Vec2f c = ConvertPointf(position, rotation, Vec2f());
    Vec2f wh = Vec2f(z_float(GetWidth()), z_float(GetHeight()));
    Vec2f a = c + wh;
    Vec2f b = c - wh;
    lib.RenderRect(a, b, GetColour(colour));
  }

private:

  virtual bool CheckLocalPoint(const Vec2& v) const
  {
    return z_abs(v._x) < _width && z_abs(v._y) < _height;
  }

  fixed _width;
  fixed _height;

};

// Line between two points
//------------------------------
class Line : public RotateShape {
public:

  Line(const Vec2& centre, const Vec2& a, const Vec2& b,
       Colour colour, fixed rotation = 0)
    : RotateShape(centre, rotation, colour, 0)
    , _a(a)
    , _b(b) {}

  virtual ~Line() {}

  Vec2 GetA() const
  {
    return _a;
  }

  Vec2 GetB() const
  {
    return _b;
  }

  void SetA(const Vec2& a)
  {
    _a = a;
  }

  void SetB(const Vec2& b)
  {
    _b = b;
  }

  virtual void Render(Lib& lib, const Vec2f& position, float rotation,
                      Colour colour = 0) const
  {
    Vec2f a = ConvertPointf(position, rotation, Vec2f(_a));
    Vec2f b = ConvertPointf(position, rotation, Vec2f(_b));
    lib.RenderLine(a, b, GetColour(colour));
  }

private:

  virtual bool CheckRotatedPoint(const Vec2& v) const
  {
    return false;
  }

  Vec2 _a;
  Vec2 _b;

};

// Box with width and height
//------------------------------
class Box : public RotateShape {
public:

  Box(const Vec2& centre, fixed width, fixed height,
      Colour colour, fixed rotation = 0, int category = 0)
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
      Lib& lib, const Vec2f& position, float rotation, Colour colour = 0) const
  {
    float w = z_float(GetWidth());
    float h = z_float(GetHeight());

    Vec2f a = ConvertPointf(position, rotation, Vec2f(w, h));
    Vec2f b = ConvertPointf(position, rotation, Vec2f(-w, h));
    Vec2f c = ConvertPointf(position, rotation, Vec2f(-w, -h));
    Vec2f d = ConvertPointf(position, rotation, Vec2f(w, -h));

    lib.RenderLine(a, b, GetColour(colour));
    lib.RenderLine(b, c, GetColour(colour));
    lib.RenderLine(c, d, GetColour(colour));
    lib.RenderLine(d, a, GetColour(colour));
  }

private:

  virtual bool CheckRotatedPoint(const Vec2& v) const
  {
    return z_abs(v._x) < _width && z_abs(v._y) < _height;
  }

  fixed _width;
  fixed _height;

};

// Outlined regular polygon
//------------------------------
class Polygon : public RotateShape {
public:

  Polygon(const Vec2& centre, fixed radius, int sides,
          Colour colour, fixed rotation = 0, int category = 0)
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
      Lib& lib, const Vec2f& position, float rotation, Colour colour = 0) const
  {
    if (GetSides() < 2) {
      return;
    }

    float r = z_float(GetRadius());
    for (int i = 0; i < GetSides(); i++) {
      Vec2f a, b;
      a.SetPolar(i * 2 * M_PIf / float(GetSides()), r);
      b.SetPolar((i + 1) * 2 * M_PIf / float(GetSides()), r);
      lib.RenderLine(ConvertPointf(position, rotation, a),
                     ConvertPointf(position, rotation, b), GetColour(colour));
    }
  }

private:

  virtual bool CheckRotatedPoint(const Vec2& v) const
  {
    return v.Length() < GetRadius();
  }

  fixed _radius;
  int _sides;

};

// Polygon segment
//------------------------------
class PolyArc : public RotateShape {
public:

  PolyArc(const Vec2& centre, fixed radius, int sides, int segments,
          Colour colour, fixed rotation = 0, int category = 0)
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
      Lib& lib, const Vec2f& position, float rotation, Colour colour = 0) const
  {
    if (GetSides() < 2) return;
    float r = z_float(GetRadius());

    for (int i = 0; i < GetSides() && i < _segments; i++) {
      Vec2f a, b;
      a.SetPolar(i * 2 * M_PIf / float(GetSides()), r);
      b.SetPolar((i + 1) * 2 * M_PIf / float(GetSides()), r);
      lib.RenderLine(ConvertPointf(position, rotation, a),
                     ConvertPointf(position, rotation, b), GetColour(colour));
    }
  }

private:

  virtual bool CheckRotatedPoint(const Vec2& v) const
  {
    fixed angle = v.Angle();
    fixed len = v.Length();
    bool b = 0 <= angle && v.Angle() <= (M_2PI * _segments) / _sides;
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

  Polygram(const Vec2& centre, fixed radius, int sides,
           Colour colour, fixed rotation = 0, int category = 0)
    : Polygon(centre, radius, sides, colour, rotation, category) {}

  virtual ~Polygram() {}

  virtual void Render(
      Lib& lib, const Vec2f& position, float rotation, Colour colour = 0) const
  {
    if (GetSides() < 2) {
      return;
    }

    float r = z_float(GetRadius());
    std::vector<Vec2f> list;
    for (int i = 0; i < GetSides(); i++) {
      Vec2f v;
      v.SetPolar(i * 2 * M_PIf / float(GetSides()), r);
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

  Polystar(const Vec2& centre, fixed radius, int sides,
           Colour colour, fixed rotation = 0, int category = 0)
    : Polygon(centre, radius, sides, colour, rotation, category) {}

  virtual ~Polystar() {}

  virtual void Render(
      Lib& lib, const Vec2f& position, float rotation, Colour colour = 0) const
  {
    if (GetSides() < 2) {
      return;
    }

    float r = z_float(GetRadius());
    for (int i = 0; i < GetSides(); i++) {
      Vec2f v;
      v.SetPolar(i * 2 * M_PIf / float(GetSides()), r);
      lib.RenderLine(ConvertPointf(position, rotation, Vec2f()),
                     ConvertPointf(position, rotation, v), GetColour(colour));
    }
  }

};

// Group of lesser shapes
//------------------------------
class CompoundShape : public RotateShape {
public:

  // Child shapes take the top-level category
  CompoundShape(const Vec2& centre, fixed rotation = 0,
                int category = 0, Colour colour = 0)
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

  void AddShape(Shape* shape)
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
      Lib& lib, const Vec2f& position, float rotation, Colour colour = 0) const
  {
    Vec2f c = ConvertPointf(position, rotation, Vec2f());
    for (unsigned int i = 0; i < _children.size(); i++) {
      _children[i]->Render(lib, c, z_float(GetRotation()) + rotation, colour);
    }
  }

  std::size_t CountShapes() const
  {
    return _children.size();
  }

  Shape& GetShape(std::size_t i) const
  {
    return *_children[i];
  }

private:

  virtual bool CheckRotatedPoint(const Vec2& v) const
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
