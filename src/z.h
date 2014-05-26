#ifndef IISPACE_SRC_Z0_H
#define IISPACE_SRC_Z0_H

#include <string>
#include <vector>
#include "fix32.h"

// Forward declarations
//------------------------------
typedef unsigned int Colour;
class z0Game;
class Shape;
class Ship;
class Enemy;
class Player;
class DeathRayBoss;
class DeathRayArm;

namespace z {

static const int32_t rand_max = 0x7fff;
void seed(int32_t seed);
int32_t rand_int();

inline int32_t rand_int(int32_t max)
{
  return rand_int() % max;
}

inline fixed rand_fixed()
{
  return fixed(rand_int()) / rand_max;
}

std::string compress_string(const std::string& str);
std::string decompress_string(const std::string& str);

Colour colour_cycle(Colour rgb, int cycle);

// End namespace z.
}

// Vector math
//------------------------------
#define M_PIf 3.14159265358979323846264338327f

class Vec2 {
public:

  fixed _x;
  fixed _y;

  Vec2()
    : _x(0)
    , _y(0) {}

  Vec2(fixed x, fixed y)
    : _x(x)
    , _y(y) {}

  Vec2(const Vec2& a)
    : _x(a._x)
    , _y(a._y) {}

  fixed Length() const
  {
    return (_x * _x + _y * _y).sqrt();
  }

  fixed Angle() const
  {
    return _y.atan2(_x);
  }

  void Set(fixed x, fixed y)
  {
    _x = x;
    _y = y;
  }

  void SetPolar(fixed angle, fixed length)
  {
    _x = length * angle.cos();
    _y = length * angle.sin();
  }

  void Normalise()
  {
    fixed l = Length();
    if (l <= 0) {
      return;
    }
    _x /= l;
    _y /= l;
  }

  void Rotate(fixed angle)
  {
    if (angle != 0) {
      SetPolar(Angle() + angle, Length());
    }
  }

  fixed DotProduct(const Vec2& a) const
  {
    return _x * a._x + _y * a._y;
  }

  Vec2 operator+(const Vec2& a) const
  {
    return Vec2(_x + a._x, _y + a._y);
  }

  Vec2 operator-(const Vec2& a) const
  {
    return Vec2(_x - a._x, _y - a._y);
  }

  Vec2 operator+=(const Vec2& a)
  {
    _x += a._x;
    _y += a._y;
    return *this;
  }

  Vec2 operator-=(const Vec2& a)
  {
    _x -= a._x;
    _y -= a._y;
    return *this;
  }

  Vec2 operator*(fixed a) const
  {
    return Vec2(_x * a, _y * a);
  }

  Vec2 operator/(fixed a) const
  {
    return Vec2(_x / a, _y / a);
  }

  Vec2 operator*=(fixed a)
  {
    _x *= a;
    _y *= a;
    return *this;
  }

  Vec2 operator/=(fixed a)
  {
    _x /= a;
    _y /= a;
    return *this;
  }

  bool operator==(const Vec2& a) const
  {
    return _x == a._x && _y == a._y;
  }

  bool operator!=(const Vec2& a) const
  {
    return _x != a._x || _y != a._y;
  }

  Vec2& operator=(const Vec2& a)
  {
    _x = a._x;
    _y = a._y;
    return *this;
  }

};

class Vec2f {
public:

  float _x;
  float _y;

  Vec2f()
    : _x(0)
    , _y(0) {}

  Vec2f(float x, float y)
    : _x(x)
    , _y(y) {}

  explicit Vec2f(const Vec2& a)
    : _x(a._x.to_float())
    , _y(a._y.to_float()) {}

  float Length() const
  {
    return sqrt(_x * _x + _y * _y);
  }

  float Angle() const
  {
    return atan2(_y, _x);
  }

  void Set(float x, float y)
  {
    _x = x;
    _y = y;
  }

  void SetPolar(float angle, float length)
  {
    _x = length * cos(angle);
    _y = length * sin(angle);
  }

  void Normalise()
  {
    float l = Length();
    if (l <= 0) {
      return;
    }
    _x /= l;
    _y /= l;
  }

  void Rotate(float angle)
  {
    SetPolar(Angle() + angle, Length());
  }

  float DotProduct(const Vec2f& a) const
  {
    return _x * a._x + _y * a._y;
  }

  Vec2f operator+(const Vec2f& a) const
  {
    return Vec2f(_x + a._x, _y + a._y);
  }

  Vec2f operator-(const Vec2f& a) const
  {
    return Vec2f(_x - a._x, _y - a._y);
  }

  Vec2f operator+=(const Vec2f& a)
  {
    _x += a._x;
    _y += a._y;
    return *this;
  }

  Vec2f operator-=(const Vec2f& a)
  {
    _x -= a._x;
    _y -= a._y;
    return *this;
  }

  Vec2f operator*(float a) const
  {
    return Vec2f(_x * a, _y * a);
  }

  Vec2f operator/(float a) const
  {
    return Vec2f(_x / a, _y / a);
  }

  Vec2f operator*=(float a)
  {
    _x *= a;
    _y *= a;
    return *this;
  }

  Vec2f operator/=(float a)
  {
    _x /= a;
    _y /= a;
    return *this;
  }

  bool operator==(const Vec2f& a) const
  {
    return _x == a._x && _y == a._y;
  }

  bool operator!=(const Vec2f& a) const
  {
    return _x != a._x || _y != a._y;
  }

  Vec2f& operator=(const Vec2f& a)
  {
    _x = a._x;
    _y = a._y;
    return *this;
  }

};

#endif
