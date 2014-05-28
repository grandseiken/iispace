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

std::string crypt(const std::string& text, const std::string& key);
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

  void rotate(const T& r_angle)
  {
    if (r_angle != 0) {
      set_polar(angle() + r_angle, length());
    }
  }

  T dot(const vec2_t& v) const
  {
    return x * v.x + y * v.y;
  }

  vec2_t operator+(const vec2_t& v) const
  {
    return vec2_t(x + v.x, y + v.y);
  }

  vec2_t operator-(const vec2_t& v) const
  {
    return vec2_t(x - v.x, y - v.y);
  }

  vec2_t operator+=(const vec2_t& v)
  {
    return *this = *this + v;
  }

  vec2_t operator-=(const vec2_t& v)
  {
    return *this = *this - v;
  }

  vec2_t operator*(const T& t) const
  {
    return vec2_t(x * t, y * t);
  }

  vec2_t operator/(const T& t) const
  {
    return vec2_t(x / t, y / t);
  }

  vec2_t operator*=(const T& t)
  {
    return *this = *this * t;
  }

  vec2_t operator/=(const T& t)
  {
    return *this = *this / t;
  }

  bool operator==(const vec2_t& v) const
  {
    return x == v.x && y == v.y;
  }

  bool operator!=(const vec2_t& v) const
  {
    return !(*this == v);
  }

  vec2_t& operator=(const vec2_t& v)
  {
    x = v.x;
    y = v.y;
    return *this;
  }

};

template<typename T>
vec2_t<T> operator*(const T& t, const vec2_t<T>& v)
{
  return vec2_t<T>(t * v.x, t * v.y);
}

template<typename T>
vec2_t<T> operator/(const T& t, const vec2_t<T>& v)
{
  return vec2_t<T>(t / v.x, t / v.y);
}

typedef vec2_t<fixed> vec2;
typedef vec2_t<float> flvec2;

inline flvec2 to_float(const vec2& a)
{
  return flvec2(a.x.to_float(), a.y.to_float());
}

#endif
