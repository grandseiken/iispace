#ifndef II_GAME_COMMON_RAW_PTR_H
#define II_GAME_COMMON_RAW_PTR_H
#include <memory>

namespace ii {
namespace detail {
template <typename T>
struct raw_deleter {
  using deleter_t = void (*)(T*);
  deleter_t deleter;

  raw_deleter() : deleter{nullptr} {}
  raw_deleter(deleter_t d) : deleter{d} {}
  void operator()(T* p) const {
    deleter(p);
  }
};
}  // namespace detail

template <typename T>
using raw_ptr = std::unique_ptr<T, detail::raw_deleter<T>>;

template <typename T>
raw_ptr<T> make_raw(T* p, typename detail::raw_deleter<T>::deleter_t d) {
  return raw_ptr<T>{p, {d}};
}
}  // namespace ii

#endif