#include "game/logic/sim/networked_sim_state.h"
#include <algorithm>
#include <cassert>

namespace ii {
namespace {
// Input mapping for game with N players should provide exactly one input source (local or remote)
// for each player index 0 <= I < N.
bool check_mapping(const initial_conditions& conditions,
                   NetworkedSimState::input_mapping& mapping) {
  std::vector<std::uint32_t> mapped_indexes;
  auto add = [&](const NetworkedSimState::source_mapping& m) {
    for (auto n : m.player_numbers) {
      mapped_indexes.emplace_back(n);
    }
  };
  add(mapping.local);
  for (const auto& pair : mapping.remote) {
    add(pair.second);
  }
  if (mapped_indexes.size() != conditions.player_count) {
    return false;
  }
  std::ranges::sort(mapped_indexes);
  for (std::uint32_t i = 0; i < mapped_indexes.size(); ++i) {
    if (mapped_indexes[i] != i) {
      return false;
    }
  }
  return true;
}
}  // namespace

NetworkedSimState::NetworkedSimState(const initial_conditions& conditions, input_mapping mapping,
                                     ReplayWriter* writer)
: canonical_state_{conditions, writer}
, mapping_{std::move(mapping)}
, player_count_{conditions.player_count} {
  (void)&check_mapping;
  assert(check_mapping(conditions, mapping_));
  canonical_state_.copy_to(predicted_state_);
  latest_input_.resize(player_count_);
  local_checksums_.emplace_back(0u, canonical_state_.checksum());
}

const std::unordered_set<std::string>& NetworkedSimState::checksum_failed_remote_ids() const {
  return checksum_failed_remote_ids_;
}

void NetworkedSimState::input_packet(const std::string& remote_id, const sim_packet& packet) {
  auto it = mapping_.remote.find(remote_id);
  if (it == mapping_.remote.end()) {
    assert(false);
    return;  // Unknown remote.
  }

  if (packet.tick_count < canonical_state_.tick_count()) {
    assert(false);
    return;  // Duplicate packet?
  }
  auto tick_offset = packet.tick_count - canonical_state_.tick_count();

  if (tick_offset > partial_frames_.size()) {
    assert(false);
    return;  // Reordered packet (tick N + 1 before tick N)?
  }

  // Update remote info.
  auto& remote = remotes_[remote_id];
  remote.latest_tick = 1 + packet.tick_count;
  remote.canonical_tick = packet.canonical_tick_count;
  if (remote.checksums.empty() || remote.checksums.back().tick_count < remote.canonical_tick) {
    remote.checksums.emplace_back(remote.canonical_tick, packet.canonical_checksum);
  }

  if (tick_offset == partial_frames_.size()) {
    partial_frames_.emplace_back().input_frames.resize(player_count_);
  }
  auto& partial = partial_frames_[tick_offset];

  for (std::size_t i = 0; i < it->second.player_numbers.size(); ++i) {
    auto player_number = it->second.player_numbers[i];
    if (partial.input_frames[player_number]) {
      assert(false);
      return;  // Duplicate packet?
    }
    partial.input_frames[player_number] = latest_input_[player_number] =
        i < packet.input_frames.size() ? packet.input_frames[i] : input_frame{};
  }

  bool frame_complete =
      std::ranges::all_of(partial.input_frames, [&](const auto& f) { return f.has_value(); });
  if (tick_offset || !frame_complete) {
    return;
  }

  // Canonical state can be advanced.
  std::vector<input_frame> v;
  for (const auto& f : partial.input_frames) {
    v.emplace_back(*f);
  }
  canonical_state_.update(std::move(v));
  local_checksums_.emplace_back(canonical_state_.tick_count(), canonical_state_.checksum());
  partial_frames_.pop_front();
}

sim_packet NetworkedSimState::update(std::vector<input_frame> local_input) {
  auto frame_for = [&](std::uint64_t tick_offset, std::uint32_t k) {
    return tick_offset < partial_frames_.size() && partial_frames_[tick_offset].input_frames[k]
        ? *partial_frames_[tick_offset].input_frames[k]
        : latest_input_[k];
  };

  if (predicted_tick_base_ < canonical_state_.tick_count()) {
    // Canonical state has advanced since last prediction; rewind and replay.
    auto replay_ticks = predicted_state_.tick_count() >= canonical_state_.tick_count()
        ? predicted_state_.tick_count() - canonical_state_.tick_count()
        : 0u;
    canonical_state_.copy_to(predicted_state_);
    predicted_tick_base_ = canonical_state_.tick_count();
    for (std::uint64_t i = 0; i < replay_ticks; ++i) {
      std::vector<input_frame> predicted_inputs;
      for (std::uint32_t k = 0; k < player_count_; ++k) {
        predicted_inputs.emplace_back(frame_for(i, k));
      }
      predicted_state_.update(std::move(predicted_inputs));
    }
  }

  // Now predicted_state >= canonical_state; advance one tick.
  auto tick_offset = predicted_state_.tick_count() - canonical_state_.tick_count();
  assert(tick_offset <= partial_frames_.size());
  if (tick_offset == partial_frames_.size()) {
    partial_frames_.emplace_back().input_frames.resize(player_count_);
  }
  auto& partial = partial_frames_[tick_offset];

  // Fill in our local inputs.
  sim_packet packet;
  packet.tick_count = predicted_state_.tick_count();
  packet.input_frames = std::move(local_input);
  for (std::size_t i = 0; i < mapping_.local.player_numbers.size(); ++i) {
    auto player_number = mapping_.local.player_numbers[i];
    assert(!partial.input_frames[player_number]);
    partial.input_frames[player_number] =
        i < packet.input_frames.size() ? packet.input_frames[i] : input_frame{};
  }

  bool frame_complete =
      std::ranges::all_of(partial.input_frames, [&](const auto& f) { return f.has_value(); });
  std::vector<input_frame> inputs;
  for (std::uint32_t k = 0; k < player_count_; ++k) {
    inputs.emplace_back(frame_for(tick_offset, k));
  }
  if (!tick_offset && frame_complete) {
    // Can just advance canonical state.
    canonical_state_.update(std::move(inputs));
    local_checksums_.emplace_back(canonical_state_.tick_count(), canonical_state_.checksum());
    partial_frames_.pop_front();
    canonical_state_.copy_to(predicted_state_);
    ++predicted_tick_base_;
  } else {
    predicted_state_.update(std::move(inputs));
  }
  packet.canonical_tick_count = local_checksums_.back().tick_count;
  packet.canonical_checksum = local_checksums_.back().checksum;

  // Verify and discard checksums.
  while (local_checksums_.size() > 1) {
    auto& c = local_checksums_.front();
    bool can_discard = true;
    for (auto& pair : remotes_) {
      auto& remote_checksums = pair.second.checksums;
      while (!remote_checksums.empty() && remote_checksums.front().tick_count < c.tick_count) {
        remote_checksums.pop_front();
      }
      if (remote_checksums.empty()) {
        can_discard = false;
        continue;
      }
      auto& r = remote_checksums.front();
      if (r.tick_count == c.tick_count && r.checksum && r.checksum != c.checksum) {
        checksum_failed_remote_ids_.emplace(pair.first);
      }
    }
    if (!can_discard) {
      break;
    }
    local_checksums_.pop_front();
  }
  return packet;
}

}  // namespace ii
