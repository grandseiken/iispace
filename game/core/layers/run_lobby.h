#ifndef II_GAME_CORE_LAYERS_PLAYER_JOIN_H
#define II_GAME_CORE_LAYERS_PLAYER_JOIN_H
#include "game/core/ui/game_stack.h"

namespace ii {
namespace ui {
class Button;
}  // namespace ui
class AssignmentPanel;

class RunLobbyLayer : public ui::GameLayer {
public:
  RunLobbyLayer(ui::GameStack& stack, const initial_conditions& conditions);
  void update_content(const ui::input_frame&, ui::output_frame&) override;

private:
  initial_conditions conditions_;
  ui::Button* back_button_ = nullptr;
  std::vector<AssignmentPanel*> assignment_panels_;
};

}  // namespace ii

#endif
