#include "game/io/font/font.h"
#include "game/common/ustring_convert.h"
#include <freetype/freetype.h>
#include <freetype/ftglyph.h>
#include <ft2build.h>
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <cmath>
#include <memory>

namespace ii {
namespace {
const char* get_error(FT_Error error) {
#undef FTERRORS_H_
#define FT_ERRORDEF(e, v, s) \
  case e:                    \
    return s;
#define FT_ERROR_START_LIST switch (error) {
#define FT_ERROR_END_LIST }
#include FT_ERRORS_H
  return "unknown error";
}
}  // namespace

std::int32_t RenderedFont::calculate_width(ustring_view s) const {
  std::int32_t width = 0;
  iterate_as_utf32(s, [&](std::size_t, std::uint32_t code) { width += (*this)[code].advance; });
  return width;
}

ustring RenderedFont::trim_for_width(ustring_view s, std::int32_t width) const {
  std::int32_t total_width = 0;
  auto dot_width = (*this)['.'].advance;

  std::size_t max_with_dots = 0;
  iterate_as_utf32(s, [&](std::size_t index, std::uint32_t code) {
    auto w = (*this)[code].advance;
    if (total_width + w + 3 * dot_width <= width) {
      max_with_dots = index + 1;
    }
    total_width += w;
  });
  auto max_dots = static_cast<std::size_t>(std::min(3, width / dot_width));
  switch (s.encoding()) {
  default:
    return ustring::ascii("");
  case ustring_encoding::kAscii:
    if (total_width <= width) {
      return ustring::ascii(std::string{s.ascii()});
    } else {
      return ustring::ascii(std::string{s.ascii().substr(0, max_with_dots)} +
                            std::string(max_dots, '.'));
    }
  case ustring_encoding::kUtf8:
    if (total_width <= width) {
      return ustring::utf8(std::string{s.utf8()});
    } else {
      return ustring::utf8(std::string{s.utf8().substr(0, max_with_dots)} +
                           std::string(max_dots, '.'));
    }
  case ustring_encoding::kUtf16:
    if (total_width <= width) {
      return ustring::utf16(std::u16string{s.utf16()});
    } else {
      return ustring::utf16(std::u16string{s.utf16().substr(0, max_with_dots)} +
                            std::u16string(max_dots, '.'));
    }
  case ustring_encoding::kUtf32:
    if (total_width <= width) {
      return ustring::utf32(std::u32string{s.utf32()});
    } else {
      return ustring::utf32(std::u32string{s.utf32().substr(0, max_with_dots)} +
                            std::u32string(max_dots, '.'));
    }
  }
}

std::pair<glm::ivec2, glm::ivec2> RenderedFont::calculate_bounding_box(ustring_view s) const {
  bool first = true;
  glm::ivec2 min;
  glm::ivec2 max;
  glm::ivec2 origin{0, max_ascent_};
  iterate_as_utf32(s, [&](std::size_t, std::uint32_t code) {
    const auto& info = (*this)[code];
    auto top_left = origin + glm::ivec2{info.bearing.x, -info.bearing.y};
    auto bottom_right = top_left + static_cast<glm::ivec2>(info.bitmap_dimensions);
    if (first) {
      min = max = top_left;
      first = false;
    }
    min = glm::min(min, top_left);
    max = glm::max(max, bottom_right);
    origin.x += info.advance;
  });
  return {min, max};
}

std::vector<std::int32_t>
RenderedFont::generate_vertex_data(ustring_view s, const glm::ivec2& origin) const {
  std::vector<std::int32_t> result;
  auto origin_copy = origin;
  origin_copy.y += max_ascent_;
  iterate_as_utf32(s, [&](std::size_t, std::uint32_t code) {
    const auto& info = (*this)[code];
    auto i_dimensions = static_cast<glm::ivec2>(info.bitmap_dimensions);
    auto i_position = static_cast<glm::ivec2>(info.bitmap_position);

    // Top-left.
    result.emplace_back(origin_copy.x + info.bearing.x);
    result.emplace_back(origin_copy.y - info.bearing.y);
    result.emplace_back(i_position.x);
    result.emplace_back(i_position.y);

    // Bottom-left.
    result.emplace_back(origin_copy.x + info.bearing.x);
    result.emplace_back(origin_copy.y - info.bearing.y + i_dimensions.y);
    result.emplace_back(i_position.x);
    result.emplace_back(i_position.y + i_dimensions.y);

    // Bottom-right.
    result.emplace_back(origin_copy.x + info.bearing.x + i_dimensions.x);
    result.emplace_back(origin_copy.y - info.bearing.y + i_dimensions.y);
    result.emplace_back(i_position.x + i_dimensions.x);
    result.emplace_back(i_position.y + i_dimensions.y);

    // Top-right.
    result.emplace_back(origin_copy.x + info.bearing.x + i_dimensions.x);
    result.emplace_back(origin_copy.y - info.bearing.y);
    result.emplace_back(i_position.x + i_dimensions.x);
    result.emplace_back(i_position.y);

    origin_copy.x += info.advance;
  });
  return result;
}

std::vector<std::int32_t>
RenderedFont::generate_vertex_data(std::uint32_t c, const glm::ivec2& origin) const {
  std::vector<std::int32_t> result;
  const auto& info = (*this)[c];
  auto i_dimensions = static_cast<glm::ivec2>(info.bitmap_dimensions);
  auto i_position = static_cast<glm::ivec2>(info.bitmap_position);

  // Top-left.
  result.emplace_back(origin.x);
  result.emplace_back(origin.y);
  result.emplace_back(i_position.x);
  result.emplace_back(i_position.y);

  // Bottom-left.
  result.emplace_back(origin.x);
  result.emplace_back(origin.y + i_dimensions.y);
  result.emplace_back(i_position.x);
  result.emplace_back(i_position.y + i_dimensions.y);

  // Bottom-right.
  result.emplace_back(origin.x + i_dimensions.x);
  result.emplace_back(origin.y + i_dimensions.y);
  result.emplace_back(i_position.x + i_dimensions.x);
  result.emplace_back(i_position.y + i_dimensions.y);

  // Top-right.
  result.emplace_back(origin.x + i_dimensions.x);
  result.emplace_back(origin.y);
  result.emplace_back(i_position.x + i_dimensions.x);
  result.emplace_back(i_position.y);

  return result;
}

result<void> RenderedFont::insert_glyph(std::uint32_t utf32_code, const glyph_info& info,
                                        const std::uint8_t* buffer, std::ptrdiff_t row_stride) {
  using namespace std::string_literals;

  auto x_spacing = lcd_ ? 3u * kBitmapSpacing : kBitmapSpacing;
  while (bitmap_dimensions_.x - current_row_.x < 2u * x_spacing + info.bitmap_dimensions.x) {
    if (!current_row_.x) {
      return unexpected("glyph dimensions too large for font bitmap"s);
    }
    current_row_.y = bitmap_dimensions_.y - kBitmapSpacing;
    current_row_.x = 0u;
  }
  auto required_height = current_row_.y + 2u * kBitmapSpacing + info.bitmap_dimensions.y;
  if (bitmap_dimensions_.y < required_height) {
    bitmap_.resize(required_height * bitmap_dimensions_.x);
    bitmap_dimensions_.y = required_height;
  }

  auto info_copy = info;
  info_copy.bitmap_position = {current_row_.x + x_spacing, current_row_.y + kBitmapSpacing};
  current_row_.x += x_spacing + info_copy.bitmap_dimensions.x;

  for (std::size_t y = 0u; y < info.bitmap_dimensions.y; ++y) {
    for (std::size_t x = 0u; x < info.bitmap_dimensions.x; ++x) {
      auto index = info_copy.bitmap_position.x + x +
          (info_copy.bitmap_position.y + y) * bitmap_dimensions_.x;
      bitmap_[index] = (buffer + row_stride * static_cast<std::ptrdiff_t>(y))[x];
    }
  }
  if (lcd_) {
    info_copy.bitmap_position.x /= 3u;
    info_copy.bitmap_dimensions.x /= 3u;
  }
  max_ascent_ = std::max<std::int32_t>(max_ascent_, info_copy.bearing.y);
  max_descent_ =
      std::max<std::int32_t>(max_descent_, info_copy.bitmap_dimensions.y - info_copy.bearing.y);
  glyph_map_.emplace(utf32_code, info_copy);
  return {};
}

struct Font::Static {
  Static();
  ~Static();
  FT_Library library;
  result<void> success;
};

Font::Static::Static() {
  using namespace std::string_literals;
  auto error = FT_Init_FreeType(&library);
  if (error) {
    success = unexpected("failed to initialize FreeType2: "s + get_error(error));
  }
}

Font::Static::~Static() = default;

Font::Static Font::s;

struct Font::font_data {
  FT_Face face;
  std::unique_ptr<void, void (*)(void*)> memory{nullptr, nullptr};
};

result<Font> Font::create(std::span<const std::uint8_t> data) {
  if (!s.success) {
    return unexpected(s.success.error());
  }

  Font font;
  auto error = FT_New_Memory_Face(s.library, reinterpret_cast<const FT_Byte*>(data.data()),
                                  static_cast<FT_Long>(data.size()),
                                  /* face index */ 0, &font.data_->face);
  if (error) {
    return unexpected(std::string{get_error(error)});
  }
  font.data_->memory = {font.data_->face,
                        +[](void* p) { FT_Done_Face(reinterpret_cast<FT_Face>(p)); }};
  return font;
}

Font::~Font() = default;

Font::Font(Font&&) = default;

Font& Font::operator=(Font&&) = default;

result<void> Font::render_code(std::uint32_t code, bool lcd, const glm::uvec2& base_dimensions,
                               const glm::uvec2& bitmap_dimensions, std::size_t bitmap_stride,
                               std::uint8_t* bitmap, const glm::uvec2& bitmap_position) {
  using namespace std::string_literals;
  auto error = FT_Set_Pixel_Sizes(data_->face, base_dimensions.x, base_dimensions.y);
  if (error) {
    return unexpected(std::string{get_error(error)});
  }
  auto glyph_index = FT_Get_Char_Index(data_->face, code);
  FT_Set_Transform(data_->face, nullptr, nullptr);

  auto d = base_dimensions;
  if (!d.x) {
    d.x = d.y;
  }
  if (!d.y) {
    d.y = d.x;
  }

  if (error = FT_Load_Glyph(data_->face, glyph_index, /* load flags */ FT_LOAD_DEFAULT); error) {
    return unexpected(std::string{get_error(error)});
  }
  auto* glyph = data_->face->glyph;
  if (glyph->format != FT_GLYPH_FORMAT_BITMAP) {
    if (error = FT_Render_Glyph(glyph, lcd ? FT_RENDER_MODE_LCD : FT_RENDER_MODE_NORMAL); error) {
      return unexpected(std::string{get_error(error)});
    }
  }
  if (glyph->format != FT_GLYPH_FORMAT_BITMAP ||
      (!lcd && glyph->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY) ||
      (lcd && glyph->bitmap.pixel_mode != FT_PIXEL_MODE_LCD) || glyph->bitmap.num_grays != 256u) {
    return unexpected("unexpected glyph bitmap format"s);
  }

  auto bpp = lcd ? 3 : 1;
  auto offset = static_cast<glm::ivec2>(bitmap_position) +
      (static_cast<glm::ivec2>(d) - glm::ivec2{glyph->bitmap.width / bpp, glyph->bitmap.rows}) / 2;
  for (std::uint32_t y = 0; y < glyph->bitmap.rows; ++y) {
    for (std::uint32_t x = 0; x < glyph->bitmap.width / bpp; ++x) {
      auto target = glm::ivec2{x, y} + offset;
      if (target.x < 0 || target.x >= static_cast<std::int32_t>(bitmap_dimensions.x) ||
          target.y < 0 || target.y >= static_cast<std::int32_t>(bitmap_dimensions.y)) {
        continue;
      }
      if (lcd) {
        for (std::size_t i = 0; i < 3; ++i) {
          auto index = 4 * static_cast<std::size_t>(target.x) + i +
              static_cast<std::size_t>(target.y) * bitmap_stride;
          bitmap[index] = static_cast<std::uint8_t>(
              (glyph->bitmap.buffer +
               glyph->bitmap.pitch * static_cast<std::ptrdiff_t>(y))[3 * x + i]);
        }
      } else {
        auto index =
            static_cast<std::size_t>(target.x) + static_cast<std::size_t>(target.y) * bitmap_stride;
        bitmap[index] = static_cast<std::uint8_t>(
            (glyph->bitmap.buffer + glyph->bitmap.pitch * static_cast<std::ptrdiff_t>(y))[x]);
      }
    }
  }
  return {};
}

result<RenderedFont> Font::render(std::span<const std::uint32_t> utf32_codes, bool lcd,
                                  const glm::uvec2& base_dimensions) const {
  using namespace std::string_literals;
  auto error = FT_Set_Pixel_Sizes(data_->face, base_dimensions.x, base_dimensions.y);
  if (error) {
    return unexpected(std::string{get_error(error)});
  }

  RenderedFont rendered_font;
  rendered_font.lcd_ = lcd;
  rendered_font.bitmap_dimensions_.x =
      lcd ? 3 * RenderedFont::kBitmapWidth : RenderedFont::kBitmapWidth;

  rendered_font.base_dimensions_ = base_dimensions;
  if (!rendered_font.base_dimensions_.x) {
    rendered_font.base_dimensions_.x = rendered_font.base_dimensions_.y;
  }
  if (!rendered_font.base_dimensions_.y) {
    rendered_font.base_dimensions_.y = rendered_font.base_dimensions_.x;
  }

  auto insert_glyph = [&](std::uint32_t key, unsigned int glyph_index,
                          std::optional<float> rotation = std::nullopt) -> result<void> {
    if (rotation) {
      FT_Matrix matrix;
      matrix.xx = static_cast<FT_Fixed>(std::cos(*rotation) * 0x10000L);
      matrix.xy = static_cast<FT_Fixed>(-std::sin(*rotation) * 0x10000L);
      matrix.yx = static_cast<FT_Fixed>(std::sin(*rotation) * 0x10000L);
      matrix.yy = static_cast<FT_Fixed>(std::cos(*rotation) * 0x10000L);
      FT_Set_Transform(data_->face, &matrix, nullptr);
    } else {
      FT_Set_Transform(data_->face, nullptr, nullptr);
    }
    if (error = FT_Load_Glyph(data_->face, glyph_index, /* load flags */ FT_LOAD_DEFAULT); error) {
      return unexpected(std::string{get_error(error)});
    }
    auto* glyph = data_->face->glyph;
    if (glyph->format != FT_GLYPH_FORMAT_BITMAP) {
      if (error = FT_Render_Glyph(glyph, lcd ? FT_RENDER_MODE_LCD : FT_RENDER_MODE_NORMAL); error) {
        return unexpected(std::string{get_error(error)});
      }
    }
    if (glyph->format != FT_GLYPH_FORMAT_BITMAP ||
        (!lcd && glyph->bitmap.pixel_mode != FT_PIXEL_MODE_GRAY) ||
        (lcd && glyph->bitmap.pixel_mode != FT_PIXEL_MODE_LCD) || glyph->bitmap.num_grays != 256u) {
      return unexpected("unexpected glyph bitmap format"s);
    }

    RenderedFont::glyph_info info;
    info.bearing = {glyph->bitmap_left, glyph->bitmap_top};
    info.advance = glyph->advance.x >> 6;
    info.bitmap_dimensions = {glyph->bitmap.width, glyph->bitmap.rows};
    return rendered_font.insert_glyph(key, info,
                                      reinterpret_cast<const std::uint8_t*>(glyph->bitmap.buffer),
                                      glyph->bitmap.pitch);
  };

  for (auto code : utf32_codes) {
    auto glyph_index = FT_Get_Char_Index(data_->face, code);
    if (!glyph_index) {
      continue;
    }
    if (auto result = insert_glyph(code, glyph_index); !result) {
      return unexpected(result.error());
    }
  }
  if (auto result = insert_glyph(0, 0); !result) {
    return unexpected(result.error());
  }
  return rendered_font;
}

Font::Font() : data_{std::make_unique<font_data>()} {}

}  // namespace ii
