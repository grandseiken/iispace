#ifndef II_GAME_FLAGS_H
#define II_GAME_FLAGS_H
#include "game/common/result.h"
#include <algorithm>
#include <charconv>
#include <cstdint>
#include <iostream>
#include <optional>
#include <ranges>
#include <string>
#include <system_error>
#include <unordered_set>

namespace ii {

inline bool& has_help_flag() {
  static bool value;
  return value;
}

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
}

template <typename T>
result<void>
flag_parse(std::vector<std::string>& args, const std::string& name, std::optional<T>& out_value) {
  if (has_help_flag()) {
    std::cout << "\t--" << name << " (optional)\n";
    return {};
  }
  if constexpr (std::is_same_v<T, bool>) {
    for (auto it = args.begin(); it != args.end(); ++it) {
      if (*it == "--" + name) {
        args.erase(it);
        out_value = true;
        return {};
      }
      if (*it == "--no-" + name) {
        args.erase(it);
        out_value = false;
        return {};
      }
      if (it->starts_with("--" + name + "=")) {
        return unexpected("error: flag --" + name + " does not take a value");
      }
    }
    return {};
  } else {
    for (auto it = args.begin(); it != args.end(); ++it) {
      if (*it == "--" + name) {
        auto jt = it;
        if (++jt == args.end()) {
          return unexpected("error: flag --" + name + " requires a value");
        }
        auto parse_value = flag_parse_value<T>(*jt);
        if (!parse_value) {
          return unexpected("error: couldn't parse value " + *jt + " for flag --" + name);
        }
        ++jt;
        args.erase(it, jt);
        out_value = std::move(parse_value);
        return {};
      }
      if (it->starts_with("--" + name + "=")) {
        auto value = it->substr(name.size() + 3);
        auto parse_value = flag_parse_value<T>(value);
        if (!parse_value) {
          return unexpected("error: couldn't parse value " + value + " for flag --" + name);
        }
        args.erase(it);
        out_value = std::move(parse_value);
        return {};
      }
    }
    return {};
  }
}

template <typename T>
result<void> flag_parse(std::vector<std::string>& args, const std::string& name, T& out_value,
                        const std::optional<T>& default_value = std::nullopt) {
  if (has_help_flag()) {
    std::cout << "\t--" << name << (default_value ? " (optional)\n" : " (required)\n");
    return {};
  }
  std::optional<T> optional_value;
  if (auto result = flag_parse(args, name, optional_value); !result) {
    return result;
  }
  if (!optional_value) {
    if (default_value) {
      optional_value = default_value;
    } else if constexpr (std::is_same_v<T, bool>) {
      return unexpected("error: flag --" + name + " (or --no-" + name + ") must be specified");
    } else {
      return unexpected("error: flag --" + name + " must be specified");
    }
  }
  out_value = std::move(*optional_value);
  return {};
}

template <typename T>
result<void> flag_parse(std::vector<std::string>& args, const std::string& name,
                        std::unordered_set<T>& out_value) {
  if (has_help_flag()) {
    std::cout << "\t--" << name << " (list)\n";
    return {};
  }
  while (true) {
    std::optional<std::string> list_value;
    if (auto result = flag_parse(args, name, list_value); !result) {
      return result;
    }
    if (!list_value) {
      break;
    }
    for (auto sr : *list_value | std::ranges::views::split(',')) {
      std::string s{&*sr.begin(), static_cast<std::size_t>(std::ranges::distance(sr))};
      auto value = flag_parse_value<T>(s);
      if (!value) {
        return unexpected("error: couldn't parse value " + s + " for flag --" + name);
      }
      out_value.emplace(std::move(*value));
    }
  }
  return {};
}

inline std::vector<std::string> args_init(int argc, char** argv) {
  std::vector<std::string> result;
  for (int i = 1; i < argc; ++i) {
    result.emplace_back(argv[i]);
    if (result.back() == "--help") {
      has_help_flag() = true;
    }
  }
  if (has_help_flag()) {
    std::cout << argv[0] << " flags:\n";
  }
  return result;
}

inline result<void> args_finish(const std::vector<std::string>& args) {
  if (has_help_flag()) {
    std::cout << std::endl;
    return unexpected("--help specified, exiting");
  }
  for (const auto& arg : args) {
    if (!arg.empty() && arg.front() == '-') {
      return unexpected("error: unknown flag " + arg);
    }
  }
  return {};
}

}  // namespace ii

#endif
