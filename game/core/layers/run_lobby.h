#ifndef II_GAME_CORE_LAYERS_RUN_LOBBY_H
#define II_GAME_CORE_LAYERS_RUN_LOBBY_H
#include "game/core/ui/game_stack.h"
#include "game/logic/sim/io/player.h"
#include <cstdint>
#include <memory>
#include <optional>
#include <unordered_map>
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
  void start_game(std::optional<network_input_mapping> network);

  std::unique_ptr<LobbySlotCoordinator> coordinator_;
  std::optional<initial_conditions> conditions_;
  std::optional<std::uint64_t> host_;
  bool online_ = false;

  std::optional<std::uint32_t> start_timer_;
  using schedule_t = std::unordered_map<std::uint64_t, std::uint32_t>;
  struct host_countdown {
    host_countdown() {}
    std::uint32_t timer = 0;
    std::uint32_t max_frame_latency = 0;
    std::optional<schedule_t> countdown_schedule;
    std::optional<schedule_t> start_game_schedule;
  };
  std::optional<host_countdown> host_countdown_;

  ui::TextElement* title_ = nullptr;
  ui::TextElement* subtitle_ = nullptr;
  ui::TabContainer* bottom_tabs_ = nullptr;
  ui::Button* back_button_ = nullptr;
  ui::TextElement* all_ready_text_ = nullptr;
};

}  // namespace ii

#endif
