#ifndef II_GAME_CORE_RENDER_STATE_H
#define II_GAME_CORE_RENDER_STATE_H
#include "game/common/random.h"
#include "game/logic/sim/io/aggregate.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <vector>

namespace ii {
namespace render {
class GlRenderer;
}  // namespace render
class ISimState;
class IoInputAdapter;
class Mixer;

class RenderState {
public:
  ~RenderState() = default;
  RenderState(std::uint32_t seed) : engine_{seed} {}
  void set_dimensions(const glm::uvec2& dimensions) { dimensions_ = dimensions; }

  // If mixer != nullptr, sounds will be handled. If input != nullptr, rumble will be handled.
  void handle_output(ISimState& state, Mixer* mixer, IoInputAdapter* input);
  void update(IoInputAdapter* input);
  void render(render::GlRenderer& r) const;

private:
  void handle_background_fx(const background_fx_change& change);

  RandomEngine engine_;
  glm::ivec2 dimensions_{0, 0};
  std::vector<particle> particles_;

  struct rumble_t {
    std::uint32_t time_ticks = 0;
    std::uint16_t lf = 0;
    std::uint16_t hf = 0;
  };
  std::vector<std::vector<rumble_t>> rumble_;
  rumble_t resolve_rumble(std::uint32_t player) const;

  enum class star_type {
    kDotStar,
    kFarStar,
    kBigStar,
    kPlanet,
  };

  // TODO: could be merged into particles?
  struct star_data {
    std::uint32_t timer = 0;
    star_type type = star_type::kDotStar;
    glm::vec2 position{0.f};
    float speed = 0;
    float size = 0;
    glm::vec4 colour{0.f};
  };

  glm::vec2 star_direction_ = {0, 1};
  std::uint32_t star_rate_ = 0;
  std::vector<star_data> stars_;
};

}  // namespace ii

#endif