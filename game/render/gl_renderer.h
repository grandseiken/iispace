#ifndef IISPACE_GAME_RENDER_GL_RENDERER_H
#define IISPACE_GAME_RENDER_GL_RENDERER_H
#include "game/common/result.h"
#include <glm/glm.hpp>
#include <memory>

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

private:
  struct impl_t;
  std::unique_ptr<impl_t> impl_;
};

}  // namespace ii

#endif