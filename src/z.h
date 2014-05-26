#ifndef IISPACE_SRC_Z0_H
#define IISPACE_SRC_Z0_H

#include <string>
#include <vector>
#include "fix32.h"

typedef uint32_t colour;

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

colour colour_cycle(colour rgb, int32_t cycle);

// End namespace z.
}

// Vector math
//------------------------------
#define M_PIf 3.14159265358979323846264338327f

inline fixed sqrt(fixed f)
{
  return f.sqrt();
}

inline fixed cos(fixed f)
{
  return f.cos();
}

inline fixed sin(fixed f)
{
  return f.sin();
}

inline fixed atan2(fixed y, fixed x)
{
  return y.atan2(x);
}

template<typename T>
class vec2_t {
public:

  T x;
  T y;

  vec2_t()
    : x(0)
    , y(0) {}

  vec2_t(const T& x, const T& y)
    : x(x)
    , y(y) {}

  T length_squared() const
  {
    return dot(*this);
  }

  T length() const
  {
    return sqrt(length_squared());
  }

  T angle() const
  {
    return atan2(y, x);
  }

  void set(const T& x, const T& y)
  {
    this->x = x;
    this->y = y;
  }

  void set_polar(const T& angle, const T& length)
  {
    x = length * cos(angle);
    y = length * sin(angle);
  }

  void normalise()
  {
    T len = length();
    if (len <= 0) {
      return;
    }
    x /= len;
    y /= len;
  }

  void rotate(const T& a)
  {
    if (a != 0) {
      set_polar(angle() + a, length());
    }
  }

  T dot(const vec2_t& a) const
  {
    return x * a.x + y * a.y;
  }

  vec2_t operator+(const vec2_t& a) const
  {
    return vec2_t(x + a.x, y + a.y);
  }

  vec2_t operator-(const vec2_t& a) const
  {
    return vec2_t(x - a.x, y - a.y);
  }

  vec2_t operator+=(const vec2_t& a)
  {
    return *this = *this + a;
  }

  vec2_t operator-=(const vec2_t& a)
  {
    return *this = *this - a;
  }

  vec2_t operator*(const T& a) const
  {
    return vec2_t(x * a, y * a);
  }

  vec2_t operator/(const T& a) const
  {
    return vec2_t(x / a, y / a);
  }

  vec2_t operator*=(const T& a)
  {
    return *this = *this * a;
  }

  vec2_t operator/=(const T& a)
  {
    return *this = *this / a;
  }

  bool operator==(const vec2_t& a) const
  {
    return x == a.x && y == a.y;
  }

  bool operator!=(const vec2_t& a) const
  {
    return !(*this == a);
  }

  vec2_t& operator=(const vec2_t& a)
  {
    x = a.x;
    y = a.y;
    return *this;
  }

};

typedef vec2_t<fixed> Vec2;
typedef vec2_t<float> Vec2f;

inline Vec2f to_float(const Vec2& a)
{
  return Vec2f(a.x.to_float(), a.y.to_float());
}

#endif
