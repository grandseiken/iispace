#ifndef II_GAME_COMMON_EASING_H
#define II_GAME_COMMON_EASING_H
#include "game/common/math.h"
#include <cmath>

namespace ii {

template <typename T>
T ease_clamp(T x) {
  return x < T{0} ? T{0} : x > T{1} ? T{1} : x;
}

template <typename T>
T ease_in_sine(T x) {
  using std::cos;
  return T{1} - cos(ease_clamp(x) * pi<T> / T{2});
}

template <typename T>
T ease_out_sine(T x) {
  using std::sin;
  return sin(ease_clamp(x) * pi<T> / T{2});
}

template <typename T>
T ease_in_out_sine(T x) {
  using std::cos;
  return -(cos(pi<T> * ease_clamp(x)) - T{1}) / T{2};
}

template <typename T>
T ease_in_quad(T x) {
  return square(ease_clamp(x));
}

template <typename T>
T ease_out_quad(T x) {
  return T{1} - square(T{1} - ease_clamp(x));
}

template <typename T>
T ease_in_out_quad(T x) {
  if (x < T{1} / T{2}) {
    return T{2} * square(ease_clamp(x));
  }
  return T{1} - square(T{2} - T{2} * ease_clamp(x)) / T{2};
}

template <typename T>
T ease_in_cubic(T x) {
  return cube(ease_clamp(x));
}

template <typename T>
T ease_out_cubic(T x) {
  return T{1} - cube(T{1} - ease_clamp(x));
}

template <typename T>
T ease_in_out_cubic(T x) {
  if (x < T{1} / T{2}) {
    return T{4} * cube(ease_clamp(x));
  }
  return T{1} - cube(T{2} - T{2} * ease_clamp(x)) / T{2};
}

template <typename T>
T ease_in_quart(T x) {
  return fourth_power(ease_clamp(x));
}

template <typename T>
T ease_out_quart(T x) {
  return T{1} - fourth_power(T{1} - ease_clamp(x));
}

template <typename T>
T ease_in_out_quart(T x) {
  if (x < T{1} / T{2}) {
    return T{8} * fourth_power(ease_clamp(x));
  }
  return T{1} - fourth_power(T{2} - T{2} * ease_clamp(x)) / T{2};
}

template <typename T>
T ease_in_quint(T x) {
  return fifth_power(ease_clamp(x));
}

template <typename T>
T ease_out_quint(T x) {
  return T{1} - fifth_power(T{1} - ease_clamp(x));
}

template <typename T>
T ease_in_out_quint(T x) {
  if (x < T{1} / T{2}) {
    return T{16} * fifth_power(ease_clamp(x));
  }
  return T{1} - fifth_power(T{2} - T{2} * ease_clamp(x)) / T{2};
}

template <typename T>
T ease_in_circ(T x) {
  using std::sqrt;
  return T{1} - sqrt(T{1} - square(ease_clamp(x)));
}

template <typename T>
T ease_out_circ(T x) {
  using std::sqrt;
  return sqrt(T{1} - square(ease_clamp(x) - T{1}));
}

template <typename T>
T ease_in_out_circ(T x) {
  using std::sqrt;
  if (x < T{1} / T{2}) {
    return (T{1} - sqrt(T{1} - square(T{2} * ease_clamp(x)))) / T{2};
  }
  return (T{1} + sqrt(T{1} - square(T{2} - T{2} * ease_clamp(x)))) / T{2};
}

}  // namespace ii

#endif
