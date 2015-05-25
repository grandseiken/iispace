#ifndef IISPACE_SRC_Z0_H
#define IISPACE_SRC_Z0_H

#include <cmath>
#include <string>
#include "fix32.h"

typedef uint32_t colour_t;
static const int32_t PLAYERS = 4;
struct Mode {
  enum mode {
    NORMAL,
    BOSS,
    HARD,
    FAST,
    WHAT,
    END_MODES,
  };
};

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

colour_t colour_cycle(colour_t rgb, int32_t cycle);

// End namespace z.
}

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

  static vec2_t from_polar(const T& angle, const T& length)
  {
    return length * vec2_t(cos(angle), sin(angle));
  }

  vec2_t normalised() const
  {
    T len = length();
    return len <= 0 ? *this : *this / len;
  }

  vec2_t rotated(const T& r_angle) const
  {
    return r_angle ? from_polar(angle() + r_angle, length()) : *this;
  }

  T dot(const vec2_t& v) const
  {
    return x * v.x + y * v.y;
  }

  vec2_t operator-() const
  {
    return vec2_t() - *this;
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

  bool operator<(const vec2_t& v) const
  {
    return x < v.x && y < v.y;
  }

  bool operator>(const vec2_t& v) const
  {
    return x > v.x && y > v.y;
  }

  bool operator<=(const vec2_t& v) const
  {
    return x <= v.x && y <= v.y;
  }

  bool operator>=(const vec2_t& v) const
  {
    return x >= v.x && y >= v.y;
  }

  vec2_t& operator=(const vec2_t& v)
  {
    x = v.x;
    y = v.y;
    return *this;
  }

};

namespace std {
  template<typename T>
  vec2_t<T> max(const vec2_t<T>& a, const vec2_t<T>& b)
  {
    return vec2_t<T>(std::max(a.x, b.x), std::max(a.y, b.y));
  }

  template<typename T>
  vec2_t<T> min(const vec2_t<T>& a, const vec2_t<T>& b)
  {
    return vec2_t<T>(std::min(a.x, b.x), std::min(a.y, b.y));
  }
}

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
typedef vec2_t<int32_t> ivec2;
typedef vec2_t<float> flvec2;

inline flvec2 to_float(const vec2& a)
{
  return flvec2(a.x.to_float(), a.y.to_float());
}

#endif
