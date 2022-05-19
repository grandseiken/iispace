#include "game/render/gl_renderer.h"
#include "game/render/gl/types.h"
#include <GL/gl3w.h>

namespace ii {

struct GlRenderer::impl_t {};

result<GlRenderer> GlRenderer::create() {
  GlRenderer renderer{{}};
  renderer.impl_ = std::make_unique<impl_t>();
  return {std::move(renderer)};
}

GlRenderer::GlRenderer(access_tag) {}
GlRenderer::GlRenderer(GlRenderer&&) = default;
GlRenderer& GlRenderer::operator=(GlRenderer&&) = default;
GlRenderer::~GlRenderer() = default;

}  // namespace ii