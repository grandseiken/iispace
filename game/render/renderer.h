#ifndef IISPACE_GAME_RENDER_RENDERER_H
#define IISPACE_GAME_RENDER_RENDERER_H
#include "game/common/result.h"
#include <glm/glm.hpp>
#include <string_view>

namespace ii {

class Renderer {
public:
  virtual ~Renderer() {}

  virtual void clear_screen() = 0;
  virtual void
  set_dimensions(const glm::uvec2& screen_dimensions, const glm::uvec2& render_dimensions) = 0;

  virtual void render_legacy_text(const glm::ivec2& position, const glm::vec4& colour,
                                  std::string_view text) = 0;
  virtual void
  render_legacy_line(const glm::vec2& a, const glm::vec2& b, const glm::vec4& colour) = 0;
  virtual void render_legacy_rect(const glm::ivec2& lo, const glm::ivec2& hi,
                                  std::int32_t line_width, const glm::vec4& colour) = 0;
};

}  // namespace ii

#endif