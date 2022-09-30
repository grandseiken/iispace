#ifndef II_GAME_CORE_LAYERS_SLOT_COORDINATOR_H
#define II_GAME_CORE_LAYERS_SLOT_COORDINATOR_H
#include "game/core/ui/game_stack.h"
#include "game/data/packet.h"
#include <cstdint>
#include <optional>
#include <unordered_map>
#include <vector>

namespace ii {
class LobbySlotPanel;

class LobbySlotCoordinator {
public:
  LobbySlotCoordinator(ui::GameStack& stack, ui::Element& element, bool online);

  void
  handle(const std::vector<data::lobby_update_packet::slot_info>& info, ui::output_frame& output);
  void handle(std::uint64_t client_user_id, const data::lobby_request_packet& request,
              ui::output_frame& output);
  bool update(const std::vector<ui::input_device_id>& joins);

  bool game_ready() const;
  std::vector<ui::input_device_id> input_devices() const;
  std::uint32_t player_count() const;

  void set_dirty();
  std::optional<data::lobby_request_packet> client_request();
  std::optional<std::vector<data::lobby_update_packet::slot_info>> host_slot_update();

private:
  bool is_host() const;
  bool is_client() const;

  ui::GameStack& stack_;
  bool online_ = false;

  struct slot_data {
    LobbySlotPanel* panel = nullptr;
    std::optional<std::uint64_t> owner;
    std::optional<ui::input_device_id> assignment;
    bool is_ready = false;
  };

  std::vector<slot_data> slots_;
  std::vector<ui::input_device_id> queued_devices_;
  std::unordered_map<std::uint64_t, std::uint32_t> host_sequence_numbers_;

  bool host_slot_info_dirty_ = false;
  bool client_slot_info_dirty_ = false;
};

}  // namespace ii

#endif
