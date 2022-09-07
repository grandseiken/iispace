#ifndef II_GAME_RENDER_RENDER_COMMON_H
#define II_GAME_RENDER_RENDER_COMMON_H
#include "game/common/math.h"
#include "game/common/rect.h"
#include <glm/glm.hpp>
#include <vector>

namespace ii::render {

enum class coordinate_system {
  kGlobal,
  kLocal,
  kCentered,
};

enum class panel_style : std::uint32_t {
  kNone = 0,
  kFlatColour = 1,
};

enum class font_id : std::uint32_t {
  kDefault,
  kMonospace = kDefault,
  kMonospaceBold,
  kMonospaceItalic,
  kMonospaceBoldItalic,
};

struct panel_data {
  panel_style style = panel_style::kNone;
  glm::vec4 colour{1.f};
  rect bounds;
};

struct target {
  glm::uvec2 screen_dimensions{0};
  glm::uvec2 render_dimensions{0};
  std::vector<rect> clip_stack;

  glm::ivec2 render_to_screen_coords(const glm::ivec2& v) const {
    auto r = scale_factor() * glm::vec2{v} +
        glm::vec2{screen_dimensions} * (glm::vec2{1.f} - aspect_scale()) / 2.f;
    return glm::ivec2{glm::round(r)};
  }

  glm::ivec2 screen_to_render_coords(const glm::ivec2& v) const {
    auto r =
        (glm::vec2{v} - glm::vec2{screen_dimensions} * (glm::vec2{1.f} - aspect_scale()) / 2.f) /
        scale_factor();
    return glm::ivec2{glm::round(r)};
  }

  glm::vec2 aspect_scale() const {
    auto screen = static_cast<glm::vec2>(screen_dimensions);
    auto render = static_cast<glm::vec2>(render_dimensions);
    auto screen_aspect = screen.x / screen.y;
    auto render_aspect = render.x / render.y;

    return screen_aspect > render_aspect ? glm::vec2{render_aspect / screen_aspect, 1.f}
                                         : glm::vec2{1.f, screen_aspect / render_aspect};
  }

  float scale_factor() const {
    auto scale_v = glm::vec2{screen_dimensions} / glm::vec2{render_dimensions};
    return std::min(scale_v.x, scale_v.y);
  }

  rect clip_rect() const {
    rect rect{render_dimensions};
    for (const auto& r : clip_stack) {
      rect = rect.intersect({rect.position + r.position, r.size});
    }
    return rect;
  }

  glm::ivec2 clip_origin() const {
    glm::ivec2 origin{0, 0};
    for (const auto& r : clip_stack) {
      origin += r.position;
    }
    return origin;
  }
};

class clip_handle {
public:
  clip_handle(clip_handle&&) = delete;
  clip_handle& operator=(clip_handle&&) = delete;

  clip_handle(target& t, const rect& r) : target_{t} { target_.clip_stack.emplace_back(r); }
  ~clip_handle() { target_.clip_stack.pop_back(); }

private:
  target& target_;
};

}  // namespace ii::render

#endif