#ifndef II_GAME_CORE_RENDER_STATE_H
#define II_GAME_CORE_RENDER_STATE_H
#include "game/logic/sim/sim_io.h"
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
  RenderState() = default;

  // If mixer != nullptr, sounds will be handled. If input != nullptr, rumble will be handled.
  void handle_output(ISimState& state, Mixer* mixer, IoInputAdapter* input);
  void update();
  void render(render::GlRenderer& r) const;

private:
  std::vector<particle> particles_;
};

}  // namespace ii

#endif