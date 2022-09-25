#ifndef II_GAME_CORE_SIM_PAUSE_LAYER_H
#define II_GAME_CORE_SIM_PAUSE_LAYER_H
#include "game/core/ui/game_stack.h"
#include <cstdint>
#include <functional>

namespace ii {
bool sim_should_pause(ui::GameStack& stack);

class PauseLayer : public ui::GameLayer {
public:
  PauseLayer(ui::GameStack& stack, std::function<void()> on_quit);
  ~PauseLayer() override = default;
  void update_content(const ui::input_frame&, ui::output_frame&) override;

private:
  std::function<void()> on_quit_;
};
}  // namespace ii

#endif
