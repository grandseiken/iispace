#ifndef IISPACE_GAME_LOGIC_SHAPE_H
#define IISPACE_GAME_LOGIC_SHAPE_H
#include "game/common/z.h"
#include <memory>
#include <vector>

class Lib;
class Shape {
public:
  Shape(const vec2& centre, fixed rotation, colour_t colour, std::int32_t category,
        bool can_rotate = true);
  virtual ~Shape() {}

  bool check_point(const vec2& v) const;
  vec2 convert_point(const vec2& position, fixed rotation, const vec2& v) const;
  fvec2 convert_fl_point(const fvec2& position, float rotation, const fvec2& v) const;

  fixed rotation() const;
  void set_rotation(fixed rotation);
  void rotate(fixed rotation_amount);

  virtual void
  render(Lib& lib, const fvec2& position, float rotation, colour_t colour_override = 0) const = 0;

  vec2 centre;
  colour_t colour;
  std::int32_t category;

private:
  virtual bool check_local_point(const vec2& v) const = 0;

  fixed _rotation;
  bool _can_rotate;
};

class Fill : public Shape {
public:
  Fill(const vec2& centre, fixed width, fixed height, colour_t colour, std::int32_t category = 0);

  void render(Lib& lib, const fvec2& position, float rotation,
              colour_t colour_override = 0) const override;

  fixed width;
  fixed height;

private:
  bool check_local_point(const vec2& v) const override;
};

class Line : public Shape {
public:
  Line(const vec2& centre, const vec2& a, const vec2& b, colour_t colour, fixed rotation = 0);

  void render(Lib& lib, const fvec2& position, float rotation,
              colour_t colour_override = 0) const override;

  vec2 a;
  vec2 b;

private:
  bool check_local_point(const vec2& v) const override;
};

class Box : public Shape {
public:
  Box(const vec2& centre, fixed width, fixed height, colour_t colour, fixed rotation = 0,
      std::int32_t category = 0);

  void render(Lib& lib, const fvec2& position, float rotation,
              colour_t colour_override = 0) const override;

  fixed width;
  fixed height;

private:
  bool check_local_point(const vec2& v) const override;
};

class Polygon : public Shape {
public:
  enum class T {
    kPolygon,
    kPolystar,
    kPolygram,
  };

  Polygon(const vec2& centre, fixed radius, std::int32_t sides, colour_t colour, fixed rotation = 0,
          std::int32_t category = 0, T type = T::kPolygon);

  void render(Lib& lib, const fvec2& position, float rotation,
              colour_t colour_override = 0) const override;

  fixed radius;
  std::int32_t sides;
  T type;

private:
  bool check_local_point(const vec2& v) const override;
};

class PolyArc : public Shape {
public:
  PolyArc(const vec2& centre, fixed radius, std::int32_t sides, std::int32_t segments,
          colour_t colour, fixed rotation = 0, std::int32_t category = 0);

  void render(Lib& lib, const fvec2& position, float rotation,
              colour_t colour_override = 0) const override;

  fixed radius;
  std::int32_t sides;
  std::int32_t segments;

private:
  bool check_local_point(const vec2& v) const override;
};

class CompoundShape : public Shape {
public:
  CompoundShape(const vec2& centre, fixed rotation = 0, std::int32_t category = 0);

  typedef std::vector<std::unique_ptr<Shape>> shape_list;
  const shape_list& shapes() const;
  void add_shape(Shape* shape);
  void destroy_shape(std::size_t index);
  void clear_shapes();

  void render(Lib& lib, const fvec2& position, float rot, colour_t colour = 0) const override;

private:
  bool check_local_point(const vec2& v) const override;

  shape_list _children;
};

#endif
