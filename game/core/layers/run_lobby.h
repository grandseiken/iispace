#ifndef II_GAME_CORE_LAYERS_PLAYER_JOIN_H
#define II_GAME_CORE_LAYERS_PLAYER_JOIN_H
#include "game/core/ui/game_stack.h"
#include <cstdint>
#include <optional>
#include <vector>

namespace ii {
namespace ui {
class Button;
class TabContainer;
class TextElement;
}  // namespace ui
class AssignmentPanel;

class RunLobbyLayer : public ui::GameLayer {
public:
  RunLobbyLayer(ui::GameStack& stack, const initial_conditions& conditions);
  void update_content(const ui::input_frame&, ui::output_frame&) override;

private:
  initial_conditions conditions_;
  ui::TabContainer* bottom_tabs_ = nullptr;
  ui::Button* back_button_ = nullptr;
  ui::TextElement* all_ready_text_ = nullptr;
  std::vector<AssignmentPanel*> assignment_panels_;
  std::optional<std::uint32_t> all_ready_timer_;
};

}  // namespace ii

#endif
