#ifndef II_GAME_CORE_LAYERS_SLOT_COORDINATOR_H
#define II_GAME_CORE_LAYERS_SLOT_COORDINATOR_H
#include "game/core/ui/game_stack.h"
#include "game/data/packet.h"
#include <cstdint>

namespace ii {
class LobbySlotPanel;

class LobbySlotCoordinator {
public:
  LobbySlotCoordinator(ui::GameStack& stack, ui::Element& element, bool online);
  void handle(const std::vector<data::lobby_update_packet::slot_info>& info);
  bool update(const std::vector<ui::input_device_id>& joins);

  bool game_ready() const;
  std::vector<ui::input_device_id> input_devices() const;
  std::uint32_t player_count() const;

private:
  ui::GameStack& stack_;
  bool online_ = false;
  std::vector<LobbySlotPanel*> slots_;
  std::vector<std::uint32_t> owned_slots_;
  std::vector<ui::input_device_id> input_devices_;
};

}  // namespace ii

#endif
