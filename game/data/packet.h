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
    std::optional<std::uint64_t> assigned_user_id;
  };

  struct start_game {
    bool countdown = false;
    bool lock_slots = false;
  };

  std::optional<initial_conditions> conditions;
  std::optional<std::vector<slot_info>> slots;
  std::optional<start_game> start;
  std::unordered_map<std::uint64_t, std::uint32_t> sequence_numbers;
};

// Sent from lobby member to host to request state change.
// TODO: game version check? Or should this be handled by System layer automatically?
struct lobby_request_packet {
  enum class request_type {
    kNone = 0,
    kPlayerJoin = 1,
    kPlayerReady = 2,
    kPlayerLeave = 3,
  };

  struct request {
    request_type type = request_type::kNone;
    std::uint32_t slot = 0;
  };
  std::vector<request> requests;
  std::uint32_t sequence_number = 0;
};

result<sim_packet> read_sim_packet(std::span<const std::uint8_t>);
result<lobby_update_packet> read_lobby_update_packet(std::span<const std::uint8_t>);
result<lobby_request_packet> read_lobby_request_packet(std::span<const std::uint8_t>);
result<std::vector<std::uint8_t>> write_sim_packet(const sim_packet&);
result<std::vector<std::uint8_t>> write_lobby_update_packet(const lobby_update_packet&);
result<std::vector<std::uint8_t>> write_lobby_request_packet(const lobby_request_packet&);

}  // namespace ii::data

#endif
