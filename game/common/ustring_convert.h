#ifndef II_GAME_COMMON_USTRING_CONVERT_H
#define II_GAME_COMMON_USTRING_CONVERT_H
#include "game/common/ustring.h"
#include <cstddef>
#include <cstdint>

namespace ii {

template <typename F>
void iterate_as_utf32(ustring_view s, F&& callback) {
  auto get = [](auto ss, std::size_t i) -> std::uint32_t {
    if (i < ss.size()) {
      return ss[i];
    }
    return 0;
  };

  switch (s.encoding()) {
  case ustring_encoding::kAscii:
    for (std::size_t i = 0; i < s.ascii().size(); ++i) {
      callback(i, static_cast<std::uint32_t>(s.ascii()[i]));
    }
    break;
  case ustring_encoding::kUtf8:
    for (std::size_t i = 0; i < s.utf8().size();) {
      std::uint32_t c = s.utf8()[i];
      if ((c & 0b10000000) == 0b00000000) {
        callback(i, c);
        i += 1;
      } else if ((c & 0b11100000) == 0b11000000) {
        callback(i, (c & 0b00011111) << 6 | (get(s.utf8(), i + 1) & 0b00111111));
        i += 2;
      } else if ((c & 0b11110000) == 0b11100000) {
        callback(i,
                 (c & 0b00001111) << 12 | (get(s.utf8(), i + 1) & 0b00111111) << 6 |
                     (get(s.utf8(), i + 2) & 0b00111111));
        i += 3;
      } else {
        callback(i,
                 (c & 0b00000111) << 18 | (get(s.utf8(), i + 1) & 0b00111111) << 12 |
                     (get(s.utf8(), i + 2) & 0b00111111) << 6 |
                     (get(s.utf8(), i + 1) & 0b00111111));
        i += 4;
      }
    }
    break;
  case ustring_encoding::kUtf16:
    for (std::size_t i = 0; i < s.utf16().size();) {
      char16_t c = s.utf16()[i];
      if ((c - 0xd800u) >= 2048u) {
        callback(i, static_cast<std::uint32_t>(c));
        i += 1;
      } else {
        char16_t n = get(s.utf16(), i + 1);
        if ((c & 0xfffffc00) == 0xd800 && (n & 0xfffffc00) == 0xdc00) {
          callback(
              i, (static_cast<std::uint32_t>(c) << 10) + static_cast<std::uint32_t>(n) - 0x35fdc00);
        }
        i += 2;
      }
    }
    break;
  case ustring_encoding::kUtf32:
    for (std::size_t i = 0; i < s.utf32().size(); ++i) {
      callback(i, static_cast<std::uint32_t>(s.utf32()[i]));
    }
    break;
  }
}

template <typename F>
void iterate_as_utf8(ustring_view s, F&& callback) {
  switch (s.encoding()) {
  case ustring_encoding::kAscii:
    for (std::size_t i = 0; i < s.ascii().size(); ++i) {
      callback(i, static_cast<std::uint8_t>(s.ascii()[i]));
    }
    break;
  case ustring_encoding::kUtf8:
    for (std::size_t i = 0; i < s.utf8().size(); ++i) {
      callback(i, static_cast<std::uint8_t>(s.utf8()[i]));
    }
    break;
  case ustring_encoding::kUtf16:
  case ustring_encoding::kUtf32:
    iterate_as_utf32(s, [&](std::size_t i, char32_t c) {
      if (c <= 0x7f) {
        callback(i, static_cast<std::uint8_t>(c));
      } else if (c <= 0x7ff) {
        callback(i, static_cast<std::uint8_t>(0xc0 | (c >> 6)));
        callback(i, static_cast<std::uint8_t>(0x80 | (0x3f & c)));
      } else if (c <= 0xffff) {
        callback(i, static_cast<std::uint8_t>(0xe0 | (c >> 12)));
        callback(i, static_cast<std::uint8_t>(0x80 | (0x3f & (c >> 6))));
        callback(i, static_cast<std::uint8_t>(0x80 | (0x3f & c)));
      } else if (c <= 0x10ffff) {
        callback(i, static_cast<std::uint8_t>(0xf0 | (c >> 18)));
        callback(i, static_cast<std::uint8_t>(0x80 | (0x3f & (c >> 12))));
        callback(i, static_cast<std::uint8_t>(0x80 | (0x3f & (c >> 6))));
        callback(i, static_cast<std::uint8_t>(0x80 | (0x3f & c)));
      }
    });
    break;
  }
}

inline std::u32string to_utf32(ustring_view s) {
  std::u32string r;
  iterate_as_utf32(s, [&](std::size_t, char32_t c) { r += c; });
  return r;
}

inline std::string to_utf8(ustring_view s) {
  std::string r;
  iterate_as_utf8(s, [&](std::size_t, char c) { r += c; });
  return r;
}

}  // namespace ii

#endif
