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

private:
  struct impl_t;
  std::unique_ptr<impl_t> impl_;
};

}  // namespace ii

#endif