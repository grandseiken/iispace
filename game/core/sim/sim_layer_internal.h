#ifndef II_GAME_CORE_SIM_SIM_LAYER_INTERNAL_H
#define II_GAME_CORE_SIM_SIM_LAYER_INTERNAL_H
#include "game/core/ui/element.h"
#include "game/core/ui/game_stack.h"
#include "game/logic/sim/io/render.h"
#include "game/logic/sim/sim_state.h"
#include "game/render/gl_renderer.h"
#include <glm/glm.hpp>
#include <sstream>
#include <string>

namespace ii {
// TODO: used for some stuff that should use value from sim state instead.
constexpr glm::uvec2 kDimensions = {640, 480};
constexpr glm::uvec2 kTextSize = {16, 16};

constexpr glm::vec4 kPanelText = {0.f, 0.f, .925f, 1.f};
constexpr glm::vec4 kPanelTran = {0.f, 0.f, .925f, .6f};

inline void set_sim_layer(const ISimState& state, ui::GameStack& stack, ui::Element& e) {
  e.set_bounds(rect{state.dimensions()});
  stack.set_fps(state.fps());
}

inline std::string convert_to_time(std::uint64_t score) {
  if (score == 0) {
    return "--:--";
  }
  std::uint64_t mins = 0;
  while (score >= 60 * 60 && mins < 99) {
    score -= 60 * 60;
    ++mins;
  }
  std::uint64_t secs = score / 60;

  std::stringstream r;
  if (mins < 10) {
    r << "0";
  }
  r << mins << ":";
  if (secs < 10) {
    r << "0";
  }
  r << secs;
  return r.str();
}

inline void render_text(const render::GlRenderer& r, const glm::vec2& v, const std::string& text,
                        const glm::vec4& c) {
  r.render_text(render::font_id::kDefault, {16, 16}, 16 * static_cast<glm::ivec2>(v), c,
                ustring_view::utf8(text));
}

inline void render_hud(render::GlRenderer& r, game_mode mode, const render_output& render) {
  std::uint32_t n = 0;
  for (const auto& p : render.players) {
    std::stringstream ss;
    ss << p.multiplier << "X";
    std::string s = ss.str();
    auto v = n == 1 ? glm::vec2{kDimensions.x / 16 - 1.f - s.size(), 1.f}
        : n == 2    ? glm::vec2{1.f, kDimensions.y / 16 - 2.f}
        : n == 3    ? glm::vec2{kDimensions.x / 16 - 1.f - s.size(), kDimensions.y / 16 - 2.f}
                    : glm::vec2{1.f, 1.f};
    render_text(r, v, s, kPanelText);

    ss.str("");
    n % 2 ? ss << p.score << "   " : ss << "   " << p.score;
    render_text(
        r,
        v - (n % 2 ? glm::vec2{static_cast<float>(ss.str().size() - s.size()), 0} : glm::vec2{0.f}),
        ss.str(), p.colour);

    if (p.timer) {
      v.x += n % 2 ? -1 : ss.str().size();
      v *= 16;
      auto lo = v + glm::vec2{5.f, 11.f - 10 * p.timer};
      auto hi = v + glm::vec2{9.f, 13.f};
      std::vector<render::shape> shapes{render::shape{
          .origin = (lo + hi) / 2.f,
          .colour = glm::vec4{1.f},
          .data = render::box{.dimensions = (hi - lo) / 2.f},
      }};
      r.render_shapes(render::coordinate_system::kGlobal, shapes, 1.f);
    }
    ++n;
  }

  std::stringstream ss;
  ss << render.lives_remaining << " live(s)";
  if (mode != game_mode::kBoss && render.overmind_timer) {
    auto t = *render.overmind_timer / 60;
    ss << " " << (t < 10 ? "0" : "") << t;
  }

  render_text(r,
              {kDimensions.x / (2.f * kTextSize.x) - ss.str().size() / 2,
               kDimensions.y / kTextSize.y - 2.f},
              ss.str(), kPanelTran);

  if (mode == game_mode::kBoss) {
    ss.str({});
    ss << convert_to_time(render.tick_count);
    render_text(r, {kDimensions.x / (2 * kTextSize.x) - ss.str().size() - 1.f, 1.f}, ss.str(),
                kPanelTran);
  }
}

}  // namespace ii

#endif