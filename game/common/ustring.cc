#include "game/common/ustring.h"
#include "game/common/ustring_convert.h"
#include <optional>

namespace ii {

bool ustring::empty() const {
  switch (e_) {
  case ustring_encoding::kAscii:
    return std::get<std::string>(s_).empty();
  case ustring_encoding::kUtf8:
    return std::get<std::string>(s_).empty();
  case ustring_encoding::kUtf16:
    return std::get<std::u16string>(s_).empty();
  case ustring_encoding::kUtf32:
    return std::get<std::u32string>(s_).empty();
  }
  return true;
}

std::size_t ustring::size() const {
  switch (e_) {
  case ustring_encoding::kAscii:
    return std::get<std::string>(s_).size();
  case ustring_encoding::kUtf8: {
    std::size_t result = 0;
    iterate_as_utf32(*this, [&](std::size_t, std::uint32_t) { ++result; });
    return result;
  }
  case ustring_encoding::kUtf16:
    return std::get<std::u16string>(s_).size();
  case ustring_encoding::kUtf32:
    return std::get<std::u32string>(s_).size();
  }
  return 0;
}

ustring ustring::substr(std::size_t offset, std::size_t count) const {
  switch (e_) {
  case ustring_encoding::kAscii:
    return ustring::ascii(std::get<std::string>(s_).substr(offset, count));
  case ustring_encoding::kUtf8: {
    std::optional<std::size_t> start_index;
    std::optional<std::size_t> end_index;
    std::size_t i = 0;
    iterate_as_utf32(*this, [&](std::size_t index, std::uint32_t) {
      if (i == offset) {
        start_index = index;
      }
      if (i == offset + count) {
        end_index = index;
      }
      ++i;
    });
    if (!start_index) {
      return ustring::ascii("");
    }
    const auto& s = std::get<std::string>(s_);
    if (!end_index) {
      return ustring::utf8(s.substr(*start_index));
    }
    ustring::utf8(s.substr(*start_index, *end_index - *start_index));
  }
  case ustring_encoding::kUtf16:
    return ustring::utf16(std::get<std::u16string>(s_).substr(offset, count));
  case ustring_encoding::kUtf32:
    return ustring::utf32(std::get<std::u32string>(s_).substr(offset, count));
  }
  return {};
}

ustring ustring::operator+(const ustring& s) const {
  if (e_ == s.e_) {
    switch (e_) {
    case ustring_encoding::kAscii:
      return ustring::ascii(std::get<std::string>(s_) + std::get<std::string>(s.s_));
    case ustring_encoding::kUtf8:
      return ustring::utf8(std::get<std::string>(s_) + std::get<std::string>(s.s_));
    case ustring_encoding::kUtf16:
      return ustring::utf16(std::get<std::u16string>(s_) + std::get<std::u16string>(s.s_));
    case ustring_encoding::kUtf32:
      return ustring::utf32(std::get<std::u32string>(s_) + std::get<std::u32string>(s.s_));
    }
  }
  auto s32 = to_utf32(*this);
  iterate_as_utf32(s, [&](std::size_t, std::uint32_t c) { s32 += static_cast<char32_t>(c); });
  return ustring::utf32(std::move(s32));
}

ustring& ustring::operator+=(const ustring& s) {
  if (e_ == s.e_) {
    switch (e_) {
    case ustring_encoding::kAscii:
    case ustring_encoding::kUtf8:
      std::get<std::string>(s_) += std::get<std::string>(s.s_);
      return *this;
    case ustring_encoding::kUtf16:
      std::get<std::u16string>(s_) += std::get<std::u16string>(s.s_);
      return *this;
    case ustring_encoding::kUtf32:
      std::get<std::u32string>(s_) += std::get<std::u32string>(s.s_);
      return *this;
    }
  }
  if (e_ == ustring_encoding::kAscii || e_ == ustring_encoding::kUtf8) {
    e_ = ustring_encoding::kUtf8;
    auto& s8 = std::get<std::string>(s_);
    iterate_as_utf8(s, [&](std::size_t, std::uint8_t c) { s8 += static_cast<char>(c); });
    return *this;
  }
  if (e_ == ustring_encoding::kUtf32) {
    auto& s32 = std::get<std::u32string>(s_);
    iterate_as_utf32(s, [&](std::size_t, std::uint32_t c) { s32 += static_cast<char32_t>(c); });
    return *this;
  }
  auto s32 = to_utf32(*this);
  iterate_as_utf32(s, [&](std::size_t, std::uint32_t c) { s32 += static_cast<char32_t>(c); });
  return *this = ustring::utf32(std::move(s32));
}

bool ustring_view::empty() const {
  switch (e_) {
  case ustring_encoding::kAscii:
    return std::get<std::string_view>(s_).empty();
  case ustring_encoding::kUtf8:
    return std::get<std::string_view>(s_).empty();
  case ustring_encoding::kUtf16:
    return std::get<std::u16string_view>(s_).empty();
  case ustring_encoding::kUtf32:
    return std::get<std::u32string_view>(s_).empty();
  }
  return true;
}

std::size_t ustring_view::size() const {
  switch (e_) {
  case ustring_encoding::kAscii:
    return std::get<std::string_view>(s_).size();
  case ustring_encoding::kUtf8: {
    std::size_t result = 0;
    iterate_as_utf32(*this, [&](std::size_t, std::uint32_t) { ++result; });
    return result;
  }
  case ustring_encoding::kUtf16:
    return std::get<std::u16string_view>(s_).size();
  case ustring_encoding::kUtf32:
    return std::get<std::u32string_view>(s_).size();
  }
  return 0;
}

ustring_view ustring_view::substr(std::size_t offset, std::size_t count) const {
  switch (e_) {
  case ustring_encoding::kAscii:
    return ustring_view::ascii(std::get<std::string_view>(s_).substr(offset, count));
  case ustring_encoding::kUtf8: {
    std::optional<std::size_t> start_index;
    std::optional<std::size_t> end_index;
    std::size_t i = 0;
    iterate_as_utf32(*this, [&](std::size_t index, std::uint32_t) {
      if (i == offset) {
        start_index = index;
      }
      if (i == offset + count) {
        end_index = index;
      }
      ++i;
    });
    const auto& s = std::get<std::string_view>(s_);
    if (!start_index) {
      return ustring_view::ascii(s.substr(s.size(), 0));
    }
    if (!end_index) {
      return ustring_view::utf8(s.substr(*start_index));
    }
    ustring_view::utf8(s.substr(*start_index, *end_index - *start_index));
  }
  case ustring_encoding::kUtf16:
    return ustring_view::utf16(std::get<std::u16string_view>(s_).substr(offset, count));
  case ustring_encoding::kUtf32:
    return ustring_view::utf32(std::get<std::u32string_view>(s_).substr(offset, count));
  }
  return ustring_view::ascii({});
}

}  // namespace ii
