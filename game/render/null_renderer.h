#ifndef IISPACE_GAME_RENDER_NULL_RENDERER_H
#define IISPACE_GAME_RENDER_NULL_RENDERER_H
#include "game/common/result.h"
#include "game/render/renderer.h"
#include <glm/glm.hpp>
#include <string_view>

namespace ii {

class NullRenderer : public Renderer {
private:
  struct access_tag {};

public:
  static result<std::unique_ptr<NullRenderer>> create() {
    return std::make_unique<NullRenderer>(access_tag{});
  }
  NullRenderer(access_tag) {}
  ~NullRenderer() override {}

  void clear_screen() override {}
  void set_dimensions(const glm::uvec2&, const glm::uvec2&) override {}
  glm::vec2 legacy_render_scale() const override {
    return {1, 1};
  }
  void render_legacy_text(const glm::ivec2&, const glm::vec4&, std::string_view) override {}
  void render_legacy_line(const glm::vec2&, const glm::vec2&, const glm::vec4&) override {}
  void render_legacy_rect(const glm::ivec2&, const glm::ivec2&, std::int32_t,
                          const glm::vec4&) override {}
};

}  // namespace ii

#endif