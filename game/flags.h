#ifndef II_GAME_FLAGS_H
#define II_GAME_FLAGS_H
#include "game/common/result.h"
#include <charconv>
#include <cstdint>
#include <optional>
#include <string>
#include <system_error>

namespace ii {

template <typename T>
std::optional<T> flag_parse_value(const std::string& arg) {
  if constexpr (std::is_same_v<T, std::string>) {
    return arg;
  } else {
    T value = 0;
    auto result = std::from_chars(arg.data(), arg.data() + arg.size(), value);
    if (result.ec != std::error_code{} || result.ptr != arg.data() + arg.size()) {
      return std::nullopt;
    }
    return value;
  }
};

inline std::vector<std::string> args_init(int argc, char** argv) {
  std::vector<std::string> result;
  for (int i = 1; i < argc; ++i) {
    result.emplace_back(argv[i]);
  }
  return result;
}

template <typename T>
result<std::optional<T>>
flag_parse(std::vector<std::string>& args, const std::string& name, bool required = false) {
  if constexpr (std::is_same_v<T, bool>) {
    for (auto it = args.begin(); it != args.end(); ++it) {
      if (*it == "--" + name) {
        args.erase(it);
        return std::make_optional(true);
      }
      if (*it == "--no-" + name) {
        args.erase(it);
        return std::make_optional(false);
      }
    }
    if (required) {
      return unexpected("error: flag --" + name + " (or --no-" + name + ") must be specified");
    }
    return std::nullopt;
  } else {
    for (auto it = args.begin(); it != args.end(); ++it) {
      if (*it != "--" + name) {
        continue;
      }
      auto jt = it;
      if (++jt == args.end()) {
        return unexpected("error: flag --" + name + " requires a value");
      }
      auto value = flag_parse_value<T>(*jt);
      if (!value) {
        return unexpected("error: couldn't parse value " + *jt + " for flag --" + name);
      }
      ++jt;
      args.erase(it, jt);
      return value;
    }
    if (required) {
      return unexpected("error: flag --" + name + " must be specified");
    }
    return std::nullopt;
  }
}

inline result<void> args_finish(const std::vector<std::string>& args) {
  for (const auto& arg : args) {
    if (!arg.empty() && arg.front() == '-') {
      return unexpected("error: unknown flag " + arg);
    }
  }
  return {};
}

}  // namespace ii

#endif