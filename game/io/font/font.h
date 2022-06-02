#ifndef II_GAME_IO_FONT_FONT_H
#define II_GAME_IO_FONT_FONT_H
#include "game/common/result.h"
#include "game/common/ustring.h"
#include <glm/glm.hpp>
#include <nonstd/span.hpp>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>

namespace ii {

class RenderedFont {
public:
  struct glyph_info {
    // Distance from baseline origin to top-left of draw position.
    glm::ivec2 bearing{0, 0};
    // Distance origin should advance after this glyph.
    std::int32_t advance = 0;

    // Glyph position within bitmap.
    glm::uvec2 bitmap_position{0, 0};
    glm::uvec2 bitmap_dimensions{0, 0};
  };

  const glyph_info& operator[](std::uint32_t utf32_code) const {
    auto it = glyph_map_.find(utf32_code);
    return it == glyph_map_.end() ? glyph_map_.find(0)->second : it->second;
  }

  glm::uvec2 bitmap_dimensions() const {
    return {lcd_ ? bitmap_dimensions_.x / 3 : bitmap_dimensions_.x, bitmap_dimensions_.y};
  }

  nonstd::span<const std::uint8_t> bitmap() {
    return bitmap_;
  }

  bool is_lcd() const {
    return lcd_;
  }

  glm::uvec2 base_dimensions() const {
    return base_dimensions_;
  }

  std::int32_t line_height() const {
    return max_descent_ + max_ascent_;
  }

  std::int32_t max_descent() const {
    return max_descent_;
  }

  std::int32_t max_ascent() const {
    return max_ascent_;
  }

  // TODO: support kerning?
  // Calculate width of the given string when rendered in this font.
  std::int32_t calculate_width(ustring_view s) const;
  ustring trim_for_width(ustring_view s, std::int32_t width) const;
  // Calculate bounding box min/max (relative to the origin) of the given string when
  // rendered in this font.
  std::pair<glm::ivec2, glm::ivec2> calculate_bounding_box(ustring_view s) const;

  // Generate vector of vertex data to render the given string.
  // Output is sets of 4x4-tuples describing character quads; each 2-tuple
  // in the format (x, y, tex_x, tex_y).
  std::vector<std::int32_t> generate_vertex_data(ustring_view s, const glm::ivec2& origin) const;
  // Generate vertex data to render a single character.
  std::vector<std::int32_t> generate_vertex_data(std::uint32_t c, const glm::ivec2& origin) const;

private:
  friend class Font;
  static constexpr std::uint32_t kBitmapSpacing = 2;
  static constexpr std::uint32_t kBitmapWidth = 512;

  RenderedFont() = default;
  result<void> insert_glyph(std::uint32_t utf32_code, const glyph_info& info,
                            const std::uint8_t* buffer, std::ptrdiff_t row_stride);

  bool lcd_ = false;
  std::int32_t max_ascent_ = 0;
  std::int32_t max_descent_ = 0;
  glm::uvec2 base_dimensions_{0, 0};
  glm::uvec2 bitmap_dimensions_{0, 0};
  glm::uvec2 current_row_{0, 0};
  std::vector<std::uint8_t> bitmap_;
  std::unordered_map<std::uint32_t, glyph_info> glyph_map_;
};

class Font {
public:
  // TODO: currently data must live as long as the font.
  static result<Font> create(nonstd::span<const std::uint8_t> data);

  ~Font();
  Font(Font&&);
  Font& operator=(Font&&);

  result<void> render_code(std::uint32_t code, bool lcd, const glm::uvec2& base_dimensions,
                           const glm::uvec2& bitmap_dimensions, std::size_t bitmap_stride,
                           std::uint8_t* bitmap, const glm::uvec2& bitmap_position);

  result<RenderedFont> render(nonstd::span<const std::uint32_t> utf32_codes, bool lcd,
                              const glm::uvec2& base_dimensions) const;

private:
  Font();

  struct Static;
  static Static s;
  struct font_data;
  std::unique_ptr<font_data> data_;
};

}  // namespace ii

#endif