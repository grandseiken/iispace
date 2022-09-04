#ifndef II_GAME_RENDER_RENDER_COMMON_H
#define II_GAME_RENDER_RENDER_COMMON_H
#include "game/common/math.h"
#include <glm/glm.hpp>

namespace ii::render {

inline glm::vec2 render_aspect_scale(const glm::uvec2& screen_dimensions, const glm::uvec2& render_dimensions) {
  auto screen = static_cast<glm::vec2>(screen_dimensions);
  auto render = static_cast<glm::vec2>(render_dimensions);
  auto screen_aspect = screen.x / screen.y;
  auto render_aspect = render.x / render.y;

  return screen_aspect > render_aspect ? glm::vec2{render_aspect / screen_aspect, 1.f}
                                        : glm::vec2{1.f, screen_aspect / render_aspect};
}

inline float render_scale(const glm::uvec2& screen_dimensions, const glm::uvec2& render_dimensions) {
  auto scale_v = glm::vec2{screen_dimensions} / glm::vec2{render_dimensions};
  return std::min(scale_v.x, scale_v.y);
}

}  // namespace ii::render

#endif