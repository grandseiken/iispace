#ifndef IISPACE_GAME_RENDER_GL_RENDERER_H
#define IISPACE_GAME_RENDER_GL_RENDERER_H
#include "game/common/result.h"
#include <glm/glm.hpp>
#include <nonstd/span.hpp>
#include <memory>
#include <string_view>

namespace ii::io {
class Filesystem;
}  // namespace ii::io
namespace ii::render {

struct line_t {
  glm::vec2 a;
  glm::vec2 b;
  glm::vec4 colour;
};

class GlRenderer {
private:
  struct access_tag {};

public:
  static result<std::unique_ptr<GlRenderer>> create(const io::Filesystem& fs);
  GlRenderer(access_tag);
  ~GlRenderer();

  void clear_screen();
  void set_dimensions(const glm::uvec2& screen_dimensions, const glm::uvec2& render_dimensions);

  glm::vec2 legacy_render_scale() const;
  void
  render_legacy_text(const glm::ivec2& position, const glm::vec4& colour, std::string_view text);
  void render_legacy_rect(const glm::ivec2& lo, const glm::ivec2& hi, std::int32_t line_width,
                          const glm::vec4& colour);
  void render_legacy_line(const glm::vec2& a, const glm::vec2& b, const glm::vec4& colour);
  void render_legacy_lines(nonstd::span<const line_t> lines);

private:
  struct impl_t;
  std::unique_ptr<impl_t> impl_;
};

}  // namespace ii::render

#endif