
#ifndef II_GAME_CORE_LAYERS_MAIN_MENU_H
#define II_GAME_CORE_LAYERS_MAIN_MENU_H
#include "game/core/game_options.h"
#include "game/core/ui/game_stack.h"
#include <cstdint>

namespace ii {

class MainMenuLayer : public ui::GameLayer {
public:
  MainMenuLayer(ui::GameStack& stack, const game_options_t& options);
  void update_content(const ui::input_frame&, ui::output_frame&) override;

private:
  game_options_t options_;
  std::uint32_t exit_timer_ = 0;
};

}  // namespace ii

#endif
