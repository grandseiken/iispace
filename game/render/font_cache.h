#ifndef II_GAME_RENDER_FONT_CACHE_H
#define II_GAME_RENDER_FONT_CACHE_H
#include "game/common/math.h"
#include "game/common/result.h"
#include "game/common/ustring.h"
#include "game/io/font/font.h"
#include "game/render/gl/types.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <span>
#include <unordered_map>
#include <unordered_set>

namespace ii::render {

class FontCache {
public:
  struct entry {
    RenderedFont font;
    gl::texture texture;
  };

  void clear();
  void set_dimensions(const glm::uvec2& screen_dimensions, const glm::uvec2& render_dimensions);
  result<void> assign(std::uint32_t font_id, std::span<const std::uint8_t> bytes);
  result<const entry*> get(std::uint32_t font_id, const glm::uvec2& dimensions, ustring_view text);

private:
  struct size_entry {
    std::unordered_set<std::uint32_t> utf32_codes;
    std::optional<entry> e;
  };

  struct font_entry {
    ii::Font font;
    std::unordered_map<glm::uvec2, size_entry> sizes;
  };

  glm::uvec2 screen_dimensions_{0, 0};
  glm::uvec2 render_dimensions_{0, 0};
  std::unordered_map<std::uint32_t, font_entry> fonts_;
};

}  // namespace ii::render

#endif