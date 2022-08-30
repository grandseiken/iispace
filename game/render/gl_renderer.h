#ifndef II_GAME_RENDER_GL_RENDERER_H
#define II_GAME_RENDER_GL_RENDERER_H
#include "game/common/result.h"
#include "game/common/ustring.h"
#include "game/logic/sim/io/render.h"
#include <glm/glm.hpp>
#include <memory>
#include <optional>
#include <span>
#include <string_view>

namespace ii::io {
class Filesystem;
}  // namespace ii::io
namespace ii::render {

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

  void set_colour_cycle(std::uint32_t cycle) {
    colour_cycle_ = cycle;
  }

  std::uint32_t colour_cycle() const {
    return colour_cycle_;
  }

  void render_text(std::uint32_t font_index, const glm::ivec2& position, const glm::vec4& colour,
                   ustring_view s);
  void render_rect(const glm::ivec2& position, const glm::ivec2& size, std::uint32_t border_width,
                   const glm::vec4& colour_lo, const glm::vec4& colour_hi,
                   const glm::vec4& border_lo, const glm::vec4& border_hi);
  void render_shapes(std::span<const shape> shapes, float trail_alpha);

private:
  std::uint32_t colour_cycle_ = 0;
  struct impl_t;
  std::unique_ptr<impl_t> impl_;
};

}  // namespace ii::render

#endif