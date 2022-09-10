#ifndef II_GAME_CORE_LAYERS_MAIN_MENU_H
#define II_GAME_CORE_LAYERS_MAIN_MENU_H
#include "game/core/ui/game_stack.h"
#include <cstdint>
#include <vector>

namespace ii {

class MainMenuLayer : public ui::GameLayer {
public:
  MainMenuLayer(ui::GameStack& stack);
  void update_content(const ui::input_frame&, ui::output_frame&) override;

private:
  std::uint32_t exit_timer_ = 0;
};

}  // namespace ii

#endif
