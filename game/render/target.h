#ifndef II_GAME_RENDER_TARGET_H
#define II_GAME_RENDER_TARGET_H
#include "game/common/math.h"
#include "game/common/rect.h"
#include <vector>

namespace ii::render {

enum class coordinate_system {
  kGlobal,
  kLocal,
  kCentered,
};

struct target {
  uvec2 screen_dimensions{0};
  uvec2 render_dimensions{0};
  std::vector<irect> clip_stack;

  ivec2 render_to_screen_coords(const ivec2& v) const {
    auto r =
        scale_factor() * fvec2{v} + fvec2{screen_dimensions} * (fvec2{1.f} - aspect_scale()) / 2.f;
    return ivec2{glm::round(r)};
  }

  ivec2 screen_to_render_coords(const ivec2& v) const {
    auto r = (fvec2{v} - fvec2{screen_dimensions} * (fvec2{1.f} - aspect_scale()) / 2.f) /
        scale_factor();
    return ivec2{glm::round(r)};
  }

  fvec2 aspect_scale() const {
    auto screen = static_cast<fvec2>(screen_dimensions);
    auto render = static_cast<fvec2>(render_dimensions);
    auto screen_aspect = screen.x / screen.y;
    auto render_aspect = render.x / render.y;

    return screen_aspect > render_aspect ? fvec2{render_aspect / screen_aspect, 1.f}
                                         : fvec2{1.f, screen_aspect / render_aspect};
  }

  float scale_factor() const {
    auto scale_v = fvec2{screen_dimensions} / fvec2{render_dimensions};
    return std::min(scale_v.x, scale_v.y);
  }

  irect clip_rect() const {
    irect result{render_dimensions};
    for (const auto& r : clip_stack) {
      result = result.intersect({result.position + r.position, r.size});
    }
    return result;
  }

  ivec2 clip_origin() const {
    ivec2 origin{0, 0};
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

  clip_handle(target& t, const irect& r) : target_{t} { target_.clip_stack.emplace_back(r); }
  ~clip_handle() { target_.clip_stack.pop_back(); }

private:
  target& target_;
};

}  // namespace ii::render

#endif