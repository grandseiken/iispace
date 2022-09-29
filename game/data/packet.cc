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
        slot.assigned_user_id = ps.user_id();
      }
    }
  }
  if (proto->has_start()) {
    auto& start = data.start.emplace();
    start.countdown = proto->start().countdown();
    start.lock_slots = proto->start().lock_slots();
  }
  return {std::move(data)};
}

result<lobby_request_packet> read_lobby_request_packet(std::span<const std::uint8_t> bytes) {
  auto proto = read_proto<proto::LobbyRequestPacket>(bytes);
  if (!proto) {
    return unexpected(proto.error());
  }

  lobby_request_packet data;
  for (const auto& pr : proto->request()) {
    auto& request = data.requests.emplace_back();
    switch (pr.type()) {
    default:
    case proto::LobbyRequestPacket::kNone:
      request.type = lobby_request_packet::request_type::kNone;
      break;
    case proto::LobbyRequestPacket::kPlayerJoin:
      request.type = lobby_request_packet::request_type::kPlayerJoin;
      break;
    case proto::LobbyRequestPacket::kPlayerReady:
      request.type = lobby_request_packet::request_type::kPlayerReady;
      break;
    case proto::LobbyRequestPacket::kPlayerLeave:
      request.type = lobby_request_packet::request_type::kPlayerLeave;
      break;
    }
    request.slot = pr.slot();
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
      if (slot.assigned_user_id) {
        ps.set_is_assigned(true);
        ps.set_user_id(*slot.assigned_user_id);
      }
    }
  }
  if (data.start) {
    auto& ps = *proto.mutable_start();
    ps.set_countdown(data.start->countdown);
    ps.set_lock_slots(data.start->lock_slots);
  }
  return write_proto(proto);
}

result<std::vector<std::uint8_t>> write_lobby_request_packet(const lobby_request_packet& data) {
  proto::LobbyRequestPacket proto;
  for (const auto& request : data.requests) {
    auto& pr = *proto.add_request();
    switch (request.type) {
    case lobby_request_packet::request_type::kNone:
      pr.set_type(proto::LobbyRequestPacket::kNone);
      break;
    case lobby_request_packet::request_type::kPlayerJoin:
      pr.set_type(proto::LobbyRequestPacket::kPlayerJoin);
      break;
    case lobby_request_packet::request_type::kPlayerReady:
      pr.set_type(proto::LobbyRequestPacket::kPlayerReady);
      break;
    case lobby_request_packet::request_type::kPlayerLeave:
      pr.set_type(proto::LobbyRequestPacket::kPlayerLeave);
      break;
    }
    pr.set_slot(request.slot);
  }
  return write_proto(proto);
}

}  // namespace ii::data
