#ifndef II_GAME_COMMON_EASING_H
#define II_GAME_COMMON_EASING_H
#include <algorithm>
#include <cmath>

namespace ii {

template <typename T>
T ease_clamp(T x) {
  return std::clamp(x, T{0}, T{1});
}

template <typename T>
T ease_in_sine(T x) {
  using namespace std;
  return T{1} - cos(ease_clamp(x) * pi<T> / T{2});
}

template <typename T>
T ease_out_sine(T x) {
  using namespace std;
  return sin(ease_clamp(x) * pi<T> / T{2});
}

template <typename T>
T ease_in_out_sine(T x) {
  using namespace std;
  return -(cos(pi<T> * ease_clamp(x)) - T{1}) / T{2};
}

template <typename T>
T ease_in_quad(T x) {
  auto x0 = ease_clamp(x);
  return x0 * x0;
}

template <typename T>
T ease_out_quad(T x) {
  auto x0 = T{1} - ease_clamp(x);
  return T{1} - x0 * x0;
}

template <typename T>
T ease_in_out_quad(T x) {
  if (x < T{1} / T{2}) {
    auto x0 = ease_clamp(x);
    return T{2} * x0 * x0 * x0;
  }
  auto x0 = T{2} - T{2} * ease_clamp(x);
  return T{1} - x0 * x0 / T{2};
}

template <typename T>
T ease_in_cubic(T x) {
  auto x0 = ease_clamp(x);
  return x0 * x0 * x0;
}

template <typename T>
T ease_out_cubic(T x) {
  auto x0 = T{1} - ease_clamp(x);
  return T{1} - x0 * x0 * x0;
}

template <typename T>
T ease_in_out_cubic(T x) {
  if (x < T{1} / T{2}) {
    auto x0 = ease_clamp(x);
    return T{4} * x0 * x0 * x0;
  }
  auto x0 = T{2} - T{2} * ease_clamp(x);
  return T{1} - x0 * x0 * x0 / T{2};
}

template <typename T>
T ease_in_quart(T x) {
  auto x0 = ease_clamp(x);
  return x0 * x0 * x0 * x0;
}

template <typename T>
T ease_out_quart(T x) {
  auto x0 = T{1} - ease_clamp(x);
  return T{1} - x0 * x0 * x0 * x0;
}

template <typename T>
T ease_in_out_quart(T x) {
  if (x < T{1} / T{2}) {
    auto x0 = ease_clamp(x);
    return T{8} * x0 * x0 * x0 * x0;
  }
  auto x0 = T{2} - T{2} * ease_clamp(x);
  return T{1} - x0 * x0 * x0 * x0 / T{2};
}

template <typename T>
T ease_in_quint(T x) {
  auto x0 = ease_clamp(x);
  return x0 * x0 * x0 * x0 * x0;
}

template <typename T>
T ease_out_quint(T x) {
  auto x0 = T{1} - ease_clamp(x);
  return T{1} - x0 * x0 * x0 * x0 * x0;
}

template <typename T>
T ease_in_out_quint(T x) {
  if (x < T{1} / T{2}) {
    auto x0 = ease_clamp(x);
    return T{16} * x0 * x0 * x0 * x0 * x0;
  }
  auto x0 = T{2} - T{2} * ease_clamp(x);
  return T{1} - x0 * x0 * x0 * x0 * x0 / T{2};
}

template <typename T>
T ease_in_circ(T x) {
  using namespace std;
  auto x0 = ease_clamp(x);
  return T{1} - sqrt(T{1} - x0 * x0);
}

template <typename T>
T ease_out_circ(T x) {
  using namespace std;
  auto x0 = ease_clamp(x) - T{1};
  return sqrt(T{1} - x0 * x0);
}

template <typename T>
T ease_in_out_circ(T x) {
  using namespace std;
  if (x < T{1} / T{2}) {
    auto x0 = T{2} * ease_clamp(x);
    return (T{1} - sqrt(T{1} - x0 * x0)) / T{2};
  }
  auto x0 = T{2} - T{2} * ease_clamp(x);
  return (T{1} + sqrt(T{1} - x0 * x0)) / T{2};
}

}  // namespace ii

#endif
