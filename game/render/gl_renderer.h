#ifndef II_GAME_RENDER_GL_RENDERER_H
#define II_GAME_RENDER_GL_RENDERER_H
#include "game/common/result.h"
#include "game/common/ustring.h"
#include <glm/glm.hpp>
#include <nonstd/span.hpp>
#include <memory>
#include <optional>
#include <string_view>

namespace ii::io {
class Filesystem;
}  // namespace ii::io
namespace ii::render {

struct line_t {
  glm::vec2 a{0.f};
  glm::vec2 b{0.f};
  glm::vec4 colour{0.f};
};

class GlRenderer {
private:
  struct access_tag {};

public:
  static result<std::unique_ptr<GlRenderer>> create();
  GlRenderer(access_tag);
  ~GlRenderer();

  result<void> status() const;
  void clear_screen();
  void set_dimensions(const glm::uvec2& screen_dimensions, const glm::uvec2& render_dimensions);

  void set_colour_cycle(std::int32_t cycle) {
    colour_cycle_ = cycle;
  }

  std::int32_t colour_cycle() const {
    return colour_cycle_;
  }

  void render_text(std::uint32_t font_index, const glm::ivec2& position, const glm::vec4& colour,
                   ustring_view s);
  void render_rect(const glm::ivec2& position, const glm::ivec2& size, std::int32_t border_width,
                   const glm::vec4& colour_lo, const glm::vec4& colour_hi,
                   const glm::vec4& border_lo, const glm::vec4& border_hi);
  void render_lines(nonstd::span<const line_t> lines);

private:
  std::int32_t colour_cycle_ = 0;
  struct impl_t;
  std::unique_ptr<impl_t> impl_;
};

}  // namespace ii::render

#endif