#ifndef II_GAME_DATA_PACKET_H
#define II_GAME_DATA_PACKET_H
#include "game/common/result.h"
#include "game/logic/sim/io/conditions.h"
#include "game/logic/sim/io/player.h"
#include <cstdint>
#include <optional>
#include <span>
#include <unordered_map>
#include <vector>

namespace ii::data {

// Sent from each peer to all others every tick.
struct sim_packet {
  // Tick count at which the input frames apply.
  std::uint64_t tick_count = 0;
  std::vector<input_frame> input_frames;
  // Tick count at which canonical state is known, and optional checksum thereof.
  std::uint64_t canonical_tick_count = 0;
  std::uint32_t canonical_checksum = 0;  // Zero to skip verification.
};

// Sent from host to all lobby members on lobby state change.
struct lobby_update_packet {
  struct slot_info {
    std::optional<std::uint64_t> owner_user_id;
    bool is_ready = false;
  };

  enum start_flags : std::uint32_t {
    kNone = 0,
    kStartTimer = 0b0001,
    kCancelTimer = 0b0010,
    kLockSlots = 0b0100,
    kStartGame = 0b1000,
  };

  std::optional<initial_conditions> conditions;
  std::optional<std::vector<slot_info>> slots;
  std::uint32_t start = start_flags::kNone;
};

// Sent from lobby member to host to request state change.
// TODO: game version check? Or should this be handled by System layer automatically?
struct lobby_request_packet {
  struct slot_info {
    std::uint32_t index = 0;
    bool is_ready = false;
  };

  std::uint32_t slots_requested = 0;
  std::vector<slot_info> slots;
};

result<sim_packet> read_sim_packet(std::span<const std::uint8_t>);
result<lobby_update_packet> read_lobby_update_packet(std::span<const std::uint8_t>);
result<lobby_request_packet> read_lobby_request_packet(std::span<const std::uint8_t>);
result<std::vector<std::uint8_t>> write_sim_packet(const sim_packet&);
result<std::vector<std::uint8_t>> write_lobby_update_packet(const lobby_update_packet&);
result<std::vector<std::uint8_t>> write_lobby_request_packet(const lobby_request_packet&);

}  // namespace ii::data

#endif
