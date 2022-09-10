#ifndef II_GAME_CORE_LAYERS_PLAYER_JOIN_H
#define II_GAME_CORE_LAYERS_PLAYER_JOIN_H
#include "game/core/ui/game_stack.h"

namespace ii {
namespace ui {
class TextElement;
}  // namespace ui

// TODO: need some system to handle multi-input simultaneously in different sections of the UI.
class RunLobbyLayer : public ui::GameLayer {
public:
  RunLobbyLayer(ui::GameStack& stack, const initial_conditions& conditions);
  void update_content(const ui::input_frame&, ui::output_frame&) override;

private:
  initial_conditions conditions_;
  ui::TextElement* status_ = nullptr;
};

}  // namespace ii

#endif
