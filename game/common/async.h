#ifndef II_GAME_COMMON_ASYNC_H
#define II_GAME_COMMON_ASYNC_H
#include "game/common/result.h"
#include <memory>
#include <optional>
#include <type_traits>

namespace ii {
namespace detail {
struct void_t {};
template <typename T>
using swap_void = std::conditional_t<std::is_void_v<T>, detail::void_t, T>;
}  // namespace detail

template <typename T>
class async_promise;

template <typename T>
class async_future {
public:
  using value_type = detail::swap_void<T>;

  ~async_future() = default;
  async_future(async_future&&) noexcept = default;
  async_future(const async_future&) = delete;
  async_future& operator=(async_future&&) noexcept = default;
  async_future& operator=(const async_future&) = delete;

  async_future() = default;
  async_future(value_type&& value)
  : value_{std::make_shared<std::optional<value_type>>(std::move(value))} {}
  async_future(const value_type& value)
  : value_{std::make_shared<std::optional<value_type>>(value)} {}
  template <typename... Args>
  async_future(Args&&... args) : value_{std::make_shared<std::optional<value_type>>()} {
    value_->emplace(std::forward<Args>(args)...);
  }

  explicit operator bool() const { return has_value(); }
  bool has_value() const { return value_->has_value(); }
  bool is_valid() const { return value_; }

  value_type& operator*() { return **value_; }
  value_type* operator->() { return &**value_; }
  const value_type& operator*() const { return **value_; }
  const value_type* operator->() const { return &**value_; }

private:
  friend class async_promise<T>;
  std::shared_ptr<std::optional<value_type>> value_;
};

template <typename T>
class async_promise {
public:
  using value_type = detail::swap_void<T>;

  ~async_promise() = default;
  async_promise(async_promise&&) noexcept = default;
  async_promise(const async_promise&) = delete;
  async_promise& operator=(async_promise&&) noexcept = default;
  async_promise& operator=(const async_promise&) = delete;
  async_promise() : value_{std::make_shared<std::optional<T>>()} {}

  explicit operator bool() const { return has_value(); }
  bool has_value() const { return value_->has_value(); }
  bool is_valid() const { return value_; }

  async_future<T> future() {
    async_future<T> f;
    f.value_ = value_;
    return f;
  }

  void set(value_type&& value) {
    if (!*value_) {
      *value_ = std::move(value);
    }
  }

  void set(const value_type& value) {
    if (!*value_) {
      *value_ = value;
    }
  }

  template <typename... Args>
  void emplace(Args&&... args) {
    if (!*value_) {
      value_->emplace(std::forward<Args>(args)...);
    }
  }

private:
  std::shared_ptr<std::optional<value_type>> value_;
};

template <typename T>
using async_result = async_future<result<T>>;
template <typename T>
using promise_result = async_promise<result<T>>;

}  // namespace ii

#endif