#ifndef IISPACE_GAME_RENDER_GL_RENDERER_H
#define IISPACE_GAME_RENDER_GL_RENDERER_H
#include "game/common/result.h"
#include <glm/glm.hpp>
#include <memory>
#include <string_view>

namespace ii {

class GlRenderer {
private:
  struct access_tag {};

public:
  static result<GlRenderer> create();
  GlRenderer(access_tag);
  GlRenderer(GlRenderer&&);
  GlRenderer& operator=(GlRenderer&&);
  ~GlRenderer();

  void clear_screen();
  void set_dimensions(const glm::uvec2& screen_dimensions, const glm::uvec2& render_dimensions);
  void
  render_legacy_text(const glm::ivec2& position, const glm::vec4& colour, std::string_view text);
  void render_legacy_line(const glm::vec2& a, const glm::vec2& b, const glm::vec4& colour);
  void render_legacy_rect(const glm::ivec2& lo, const glm::ivec2& hi, std::int32_t line_width,
                          const glm::vec4& colour);

private:
  struct impl_t;
  std::unique_ptr<impl_t> impl_;
};

}  // namespace ii

#endif