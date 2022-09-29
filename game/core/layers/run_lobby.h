#ifndef II_GAME_CORE_LAYERS_RUN_LOBBY_H
#define II_GAME_CORE_LAYERS_RUN_LOBBY_H
#include "game/core/ui/game_stack.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace ii {
namespace ui {
class Button;
class TabContainer;
class TextElement;
}  // namespace ui
class LobbySlotCoordinator;

class RunLobbyLayer : public ui::GameLayer {
public:
  ~RunLobbyLayer() override;
  RunLobbyLayer(ui::GameStack& stack, std::optional<initial_conditions> conditions, bool online);
  void update_content(const ui::input_frame&, ui::output_frame&) override;

private:
  void disconnect_and_remove();
  void clear_and_remove();
  void start_game();

  std::optional<initial_conditions> conditions_;
  std::optional<std::uint64_t> host_;
  bool online_ = false;
  ui::TextElement* title_ = nullptr;
  ui::TextElement* subtitle_ = nullptr;
  ui::TabContainer* bottom_tabs_ = nullptr;
  ui::Button* back_button_ = nullptr;
  ui::TextElement* all_ready_text_ = nullptr;
  std::unique_ptr<LobbySlotCoordinator> coordinator_;
  std::optional<std::uint32_t> all_ready_timer_;
};

}  // namespace ii

#endif
