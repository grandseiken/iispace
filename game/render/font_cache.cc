#include "game/render/font_cache.h"
#include "game/common/ustring_convert.h"
#include "game/render/gl/texture.h"

namespace ii::render {
namespace {
constexpr std::uint32_t kMaxLcdDimensions = 32;
}

void FontCache::clear() {
  for (auto& pair : fonts_) {
    pair.second.sizes.clear();
  }
}

result<void> FontCache::assign(std::uint32_t font_id, std::span<const std::uint8_t> bytes) {
  auto font = Font::create(bytes);
  if (!font) {
    return unexpected(font.error());
  }
  fonts_.emplace(font_id, font_entry{std::move(*font), {}});
  return {};
}

auto FontCache::get(const target& t, std::uint32_t font_id, const glm::uvec2& dimensions,
                    ustring_view text) -> result<const entry*> {
  auto it = fonts_.find(font_id);
  if (it == fonts_.end()) {
    return unexpected("invalid font ID");
  }
  auto scale = t.scale_factor();
  glm::uvec2 pixel_dimensions{static_cast<std::uint32_t>(scale * static_cast<float>(dimensions.x)),
                              static_cast<std::uint32_t>(scale * static_cast<float>(dimensions.y))};
  bool is_lcd = pixel_dimensions.x <= kMaxLcdDimensions || pixel_dimensions.y <= kMaxLcdDimensions;

  auto jt = it->second.sizes.find(pixel_dimensions);
  if (jt == it->second.sizes.end()) {
    jt = it->second.sizes.emplace(pixel_dimensions, size_entry{}).first;
  }

  auto codes_size = jt->second.utf32_codes.size();
  iterate_as_utf32(text,
                   [&](std::size_t, std::uint32_t code) { jt->second.utf32_codes.insert(code); });
  bool render = !jt->second.e || jt->second.utf32_codes.size() != codes_size;

  if (render) {
    std::vector<std::uint32_t> codes{jt->second.utf32_codes.begin(), jt->second.utf32_codes.end()};
    auto rendered_font = it->second.font.render(codes, is_lcd, pixel_dimensions);
    if (!rendered_font) {
      return unexpected("error rendering font: " + rendered_font.error());
    }

    auto texture = gl::make_texture();
    gl::texture_image_2d(
        texture, 0, rendered_font->is_lcd() ? gl::internal_format::kRgb : gl::internal_format::kRed,
        rendered_font->bitmap_dimensions(),
        rendered_font->is_lcd() ? gl::texture_format::kRgb : gl::texture_format::kRed,
        gl::type_of<std::byte>(), rendered_font->bitmap());
    jt->second.e = entry{std::move(*rendered_font), std::move(texture)};
  }
  return {&*jt->second.e};
}

}  // namespace ii::render