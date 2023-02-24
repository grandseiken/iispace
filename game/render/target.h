#ifndef II_GAME_RENDER_TARGET_H
#define II_GAME_RENDER_TARGET_H
#include "game/common/math.h"
#include "game/common/rect.h"
#include <optional>
#include <vector>

namespace ii::render {

enum class coordinate_system {
  kGlobal,
  kLocal,
  kCentered,
};

struct target {
  // TODO: support different screen dimension than actual screen (i.e. resolution scaling).
  uvec2 screen_dimensions{0};
  uvec2 render_dimensions{0};
  std::optional<std::uint32_t> msaa_samples;
  std::vector<frect> clip_stack;

  fvec2 render_to_fscreen_coords(const fvec2& v) const {
    return scale_factor() * v + fvec2{screen_dimensions} * (fvec2{1.f} - aspect_scale()) / 2.f;
  }
  fvec2 screen_to_frender_coords(const fvec2& v) const {
    return (v - fvec2{screen_dimensions} * (fvec2{1.f} - aspect_scale()) / 2.f) / scale_factor();
  }
  ivec2 render_to_iscreen_coords(const fvec2& v) const {
    return ivec2{glm::round(render_to_fscreen_coords(v))};
  }
  ivec2 screen_to_irender_coords(const fvec2& v) const {
    return ivec2{glm::round(screen_to_frender_coords(v))};
  }
  fvec2 render_to_fscreen_coords(const ivec2& v) const {
    return render_to_fscreen_coords(fvec2{v});
  }
  fvec2 screen_to_frender_coords(const ivec2& v) const {
    return screen_to_frender_coords(fvec2{v});
  }
  ivec2 render_to_iscreen_coords(const ivec2& v) const {
    return render_to_iscreen_coords(fvec2{v});
  }
  ivec2 screen_to_irender_coords(const ivec2& v) const {
    return screen_to_irender_coords(fvec2{v});
  }
  fvec2 snap_render_to_screen_coords(const fvec2& v) const {
    return screen_to_frender_coords(render_to_iscreen_coords(v));
  }

  std::uint32_t msaa() const { return msaa_samples.value_or(16u); }

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

  frect clip_rect() const {
    frect result{render_dimensions};
    for (const auto& r : clip_stack) {
      result = result.intersect({result.position + r.position, r.size});
    }
    return result;
  }

  frect top_clip_rect() const {
    frect result{render_dimensions};
    for (const auto& r : clip_stack) {
      result.position += r.position;
      result.size = r.size;
    }
    return result;
  }

  std::int32_t border_size(std::uint32_t x) const {
    return static_cast<std::int32_t>(
        std::round(static_cast<float>(x) *
                   std::min(static_cast<float>(screen_dimensions.x) / render_dimensions.x,
                            static_cast<float>(screen_dimensions.y) / render_dimensions.y)));
  }
};

class clip_handle {
public:
  clip_handle(clip_handle&&) = delete;
  clip_handle& operator=(clip_handle&&) = delete;

  clip_handle(target& t, const frect& r) : target_{t} { target_.clip_stack.emplace_back(r); }
  clip_handle(target& t, const irect& r) : target_{t} { target_.clip_stack.emplace_back(r); }
  ~clip_handle() { target_.clip_stack.pop_back(); }

private:
  target& target_;
};

}  // namespace ii::render

#endif