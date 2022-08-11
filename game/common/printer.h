#ifndef II_GAME_COMMON_PRINTER_H
#define II_GAME_COMMON_PRINTER_H
#include "game/common/enum.h"
#include "game/common/math.h"
#include <concepts>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>
#include <utility>
#include <vector>

namespace ii {

class Printer {
public:
  bool print_pointers = false;

  std::string extract() {
    return std::move(contents_);
  }

  Printer& indent() {
    ++indent_;
    return *this;
  }

  Printer& undent() {
    --indent_;
    return *this;
  }

  Printer& begin() {
    for (std::uint32_t i = 0; i < indent_ * 2; ++i) {
      contents_ += ' ';
    }
    return *this;
  }

  Printer& end() {
    contents_ += '\n';
    return *this;
  }

  Printer& put(bool b) {
    contents_ += b ? "true" : "false";
    return *this;
  }

  Printer& put(char c) {
    contents_ += c;
    return *this;
  }

  Printer& put(const char* s) {
    contents_ += s;
    return *this;
  }

  Printer& put(std::string_view s) {
    contents_ += s;
    return *this;
  }

  Printer& put(std::integral auto v) {
    contents_ += std::to_string(v);
    return *this;
  }

  Printer& put(Enum auto v) {
    return put(to_underlying(v));
  }

  template <typename R, typename... Args>
  Printer& put(R (*const v)(Args...)) {
    if (print_pointers) {
      std::stringstream ss;
      ss << reinterpret_cast<const void*>(v);
      return put(ss.str());
    } else {
      return put("(function pointer)");
    }
  }

  Printer& put(fixed v) {
    std::stringstream ss;
    ss << v;
    return put(ss.str()).put(" (").put(v.to_internal()).put(')');
  }

  Printer& put(const vec2& v) {
    std::stringstream ss;
    ss << v.x << ", " << v.y;
    return put('{')
        .put(ss.str())
        .put("} (")
        .put(v.x.to_internal())
        .put(", ")
        .put(v.y.to_internal())
        .put(')');
  }

  template <typename T>
  Printer& put(const std::optional<T>& v) {
    return v ? put('<').put(*v).put('>') : put("nullopt");
  }

  template <typename T>
  Printer& put(const std::vector<T>& v) {
    put('[');
    bool first = true;
    for (const auto& e : v) {
      if (!first) {
        put(", ");
      }
      first = false;
      put(e);
    }
    return put(']');
  }

  template <typename T, std::size_t N>
  Printer& put(const std::array<T, N>& v) {
    put('[');
    bool first = true;
    for (const auto& e : v) {
      if (!first) {
        put(", ");
      }
      first = false;
      put(e);
    }
    return put(']');
  }

private:
  std::uint32_t indent_ = 0;
  std::string contents_;
};

}  // namespace ii

#endif
