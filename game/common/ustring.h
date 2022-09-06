#ifndef II_GAME_COMMON_USTRING_H
#define II_GAME_COMMON_USTRING_H
#include <string>
#include <string_view>
#include <variant>

namespace ii {

enum ustring_encoding {
  kAscii,
  kUtf8,
  kUtf16,
  kUtf32,
};

class ustring {
public:
  static ustring ascii(const std::string& s) {
    return {ustring_encoding::kAscii, s};
  }

  static ustring ascii(std::string&& s) {
    return {ustring_encoding::kAscii, std::move(s)};
  }

  static ustring utf8(const std::string& s) {
    return {ustring_encoding::kUtf8, s};
  }

  static ustring utf8(std::string&& s) {
    return {ustring_encoding::kUtf8, std::move(s)};
  }

  static ustring utf16(const std::u16string& s) {
    return {ustring_encoding::kUtf16, s};
  }

  static ustring utf16(std::u16string&& s) {
    return {ustring_encoding::kUtf16, std::move(s)};
  }

  static ustring utf32(const std::u32string& s) {
    return {ustring_encoding::kUtf32, s};
  }

  static ustring utf32(std::u32string&& s) {
    return {ustring_encoding::kUtf32, std::move(s)};
  }

  ustring_encoding encoding() const {
    return e_;
  }

  const std::string& ascii() const {
    return std::get<std::string>(s_);
  }

  const std::string& utf8() const {
    return std::get<std::string>(s_);
  }

  const std::u16string& utf16() const {
    return std::get<std::u16string>(s_);
  }

  const std::u32string& utf32() const {
    return std::get<std::u32string>(s_);
  }

private:
  template <typename T>
  ustring(ustring_encoding e, T&& s) : e_{e}, s_{std::forward<T>(s)} {}

  ustring_encoding e_;
  std::variant<std::string, std::u16string, std::u32string> s_;
};

class ustring_view {
public:
  ustring_view(const ustring& s) : e_{s.encoding()} {
    switch (s.encoding()) {
    case ustring_encoding::kAscii:
      s_ = std::string_view{s.ascii()};
      break;
    case ustring_encoding::kUtf8:
      s_ = std::string_view{s.utf8()};
      break;
    case ustring_encoding::kUtf16:
      s_ = std::u16string_view{s.utf16()};
      break;
    case ustring_encoding::kUtf32:
      s_ = std::u32string_view{s.utf32()};
      break;
    }
  }

  static ustring_view ascii(std::string_view s) {
    return {ustring_encoding::kAscii, s};
  }

  static ustring_view utf8(std::string_view s) {
    return {ustring_encoding::kUtf8, s};
  }

  static ustring_view utf16(std::u16string_view s) {
    return {ustring_encoding::kUtf16, s};
  }

  static ustring_view utf32(std::u32string_view s) {
    return {ustring_encoding::kUtf32, s};
  }

  ustring_encoding encoding() const {
    return e_;
  }

  std::string_view ascii() const {
    return std::get<std::string_view>(s_);
  }

  std::string_view utf8() const {
    return std::get<std::string_view>(s_);
  }

  std::u16string_view utf16() const {
    return std::get<std::u16string_view>(s_);
  }

  std::u32string_view utf32() const {
    return std::get<std::u32string_view>(s_);
  }

private:
  template <typename T>
  ustring_view(ustring_encoding e, T&& s) : e_{e}, s_{std::forward<T>(s)} {}

  ustring_encoding e_;
  std::variant<std::string_view, std::u16string_view, std::u32string_view> s_;
};

}  // namespace ii

#endif