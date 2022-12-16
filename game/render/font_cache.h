#ifndef II_GAME_RENDER_FONT_CACHE_H
#define II_GAME_RENDER_FONT_CACHE_H
#include "game/common/math.h"
#include "game/common/result.h"
#include "game/common/ustring.h"
#include "game/io/font/font.h"
#include "game/render/data/text.h"
#include "game/render/gl/types.h"
#include "game/render/target.h"
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
  result<void> assign(font_id font, std::span<const std::uint8_t> bytes);
  result<const entry*>
  get(const target& t, font_id font, const glm::uvec2& dimensions, ustring_view text);

private:
  struct size_entry {
    std::unordered_set<std::uint32_t> utf32_codes;
    std::optional<entry> e;
  };

  struct font_entry {
    ii::Font font;
    std::unordered_map<glm::uvec2, size_entry, vec_hash<2, unsigned>> sizes;
  };

  std::unordered_map<font_id, font_entry> fonts_;
};

}  // namespace ii::render

#endif
