#include "game/data/packet.h"
#include "game/data/conditions.h"
#include "game/data/input_frame.h"
#include "game/data/proto/packet.pb.h"
#include "game/data/proto_tools.h"
#include <algorithm>
#include <array>
#include <sstream>

namespace ii::data {

result<sim_packet> read_sim_packet(std::span<const std::uint8_t> bytes) {
  auto proto = read_proto<proto::SimPacket>(bytes);
  if (!proto) {
    return unexpected(proto.error());
  }

  sim_packet data;
  data.tick_count = proto->tick_count();
  for (const auto& frame : proto->input_frame()) {
    data.input_frames.emplace_back(read_input_frame(frame));
  }
  data.canonical_tick_count = proto->canonical_tick_count();
  data.canonical_checksum = proto->canonical_checksum();
  return {data};
}

result<lobby_update_packet> read_lobby_update_packet(std::span<const std::uint8_t> bytes) {
  auto proto = read_proto<proto::LobbyUpdatePacket>(bytes);
  if (!proto) {
    return unexpected(proto.error());
  }

  lobby_update_packet data;
  if (proto->has_conditions()) {
    auto conditions = read_initial_conditions(proto->conditions());
    if (!conditions) {
      return unexpected(conditions.error());
    }
    data.conditions.emplace(*conditions);
  }
  if (proto->has_slots()) {
    data.slots.emplace();
    for (const auto& ps : proto->slots().slot()) {
      auto& slot = data.slots->emplace_back();
      if (ps.is_assigned()) {
        slot.owner_user_id = ps.owner_user_id();
      }
      slot.is_ready = ps.is_ready();
    }
  }
  if (proto->has_start()) {
    lobby_update_packet::start_game start;
    start.countdown = proto->start().countdown();
    start.lock_slots = proto->start().lock_slots();
    data.start = start;
  }
  for (const auto& pair : proto->sequence_numbers()) {
    data.sequence_numbers[pair.first] = pair.second;
  }
  return {std::move(data)};
}

result<lobby_request_packet> read_lobby_request_packet(std::span<const std::uint8_t> bytes) {
  auto proto = read_proto<proto::LobbyRequestPacket>(bytes);
  if (!proto) {
    return unexpected(proto.error());
  }

  lobby_request_packet data;
  data.sequence_number = proto->sequence_number();
  data.slots_requested = proto->slots_requested();
  for (const auto& ps : proto->slot()) {
    auto& slot = data.slots.emplace_back();
    slot.index = ps.index();
    slot.is_ready = ps.is_ready();
  }
  return {std::move(data)};
}

result<std::vector<std::uint8_t>> write_sim_packet(const sim_packet& data) {
  proto::SimPacket proto;
  proto.set_tick_count(data.tick_count);
  for (const auto& frame : data.input_frames) {
    *proto.add_input_frame() = write_input_frame(frame);
  }
  proto.set_canonical_tick_count(data.canonical_tick_count);
  proto.set_canonical_checksum(data.canonical_checksum);
  return write_proto(proto);
}

result<std::vector<std::uint8_t>> write_lobby_update_packet(const lobby_update_packet& data) {
  proto::LobbyUpdatePacket proto;
  if (data.conditions) {
    *proto.mutable_conditions() = write_initial_conditions(*data.conditions);
  }
  if (data.slots) {
    for (const auto& slot : *data.slots) {
      auto& ps = *proto.mutable_slots()->add_slot();
      if (slot.owner_user_id) {
        ps.set_is_assigned(true);
        ps.set_owner_user_id(*slot.owner_user_id);
      }
      ps.set_is_ready(slot.is_ready);
    }
  }
  if (data.start) {
    auto& ps = *proto.mutable_start();
    ps.set_countdown(data.start->countdown);
    ps.set_lock_slots(data.start->lock_slots);
  }
  for (const auto& pair : data.sequence_numbers) {
    (*proto.mutable_sequence_numbers())[pair.first] = pair.second;
  }
  return write_proto(proto);
}

result<std::vector<std::uint8_t>> write_lobby_request_packet(const lobby_request_packet& data) {
  proto::LobbyRequestPacket proto;
  proto.set_sequence_number(data.sequence_number);
  proto.set_slots_requested(data.slots_requested);
  for (const auto& slot : data.slots) {
    auto& ps = *proto.add_slot();
    ps.set_index(slot.index);
    ps.set_is_ready(slot.is_ready);
  }
  return write_proto(proto);
}

}  // namespace ii::data
