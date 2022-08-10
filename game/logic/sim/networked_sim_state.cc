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

std::vector<std::uint32_t> remote_players(const NetworkedSimState::input_mapping& mapping) {
  std::vector<std::uint32_t> result;
  for (const auto& pair : mapping.remote) {
    result.insert(result.end(), pair.second.player_numbers.begin(),
                  pair.second.player_numbers.end());
  }
  return result;
}

std::vector<std::uint32_t> filter_ai_players(const NetworkedSimState::input_mapping& mapping,
                                             std::span<std::uint32_t> ai_players) {
  std::vector<std::uint32_t> result;
  for (auto n : ai_players) {
    if (std::ranges::find(mapping.local.player_numbers, n) != mapping.local.player_numbers.end()) {
      result.emplace_back(n);
    }
  }
  return result;
}
}  // namespace

NetworkedSimState::NetworkedSimState(const initial_conditions& conditions, input_mapping mapping,
                                     ReplayWriter* writer, std::span<std::uint32_t> ai_players)
: local_ai_players_{filter_ai_players(mapping, ai_players)}
, canonical_state_{conditions, writer, local_ai_players_}
, mapping_{std::move(mapping)}
, player_count_{conditions.player_count} {
  (void)&check_mapping;
  assert(check_mapping(conditions, mapping_));
  canonical_state_.copy_to(predicted_state_);
  latest_input_.resize(player_count_);
  local_checksums_.emplace_back(0u, canonical_state_.checksum());
  for (const auto& pair : mapping_.remote) {
    for (auto n : pair.second.player_numbers) {
      smoothing_data_.players[n];
    }
  }
}

const std::unordered_set<std::string>& NetworkedSimState::checksum_failed_remote_ids() const {
  return checksum_failed_remote_ids_;
}

void NetworkedSimState::set_input_delay_ticks(std::uint64_t delay_ticks) {
  input_delay_ticks_ = delay_ticks;
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
  if (tick_offset || !frame_complete || canonical_state_.game_over()) {
    return;
  }

  // Canonical state can be advanced.
  std::vector<input_frame> v;
  for (const auto& f : partial.input_frames) {
    v.emplace_back(*f);
  }
  canonical_state_.update(std::move(v));
  handle_canonical_output(canonical_state_.tick_count(), canonical_state_.output());
  local_checksums_.emplace_back(canonical_state_.tick_count(), canonical_state_.checksum());
  partial_frames_.pop_front();
}

std::vector<sim_packet> NetworkedSimState::update(std::vector<input_frame> local_input) {
  std::vector<sim_packet> packet_output;
  while (input_delayed_frames_.size() < 1 + input_delay_ticks_) {
    auto& packet = packet_output.emplace_back();
    packet.input_frames = local_input;
    packet.tick_count = predicted_state_.tick_count() + input_delayed_frames_.size();
    input_delayed_frames_.emplace_back(std::move(local_input));
  }
  local_input = std::move(input_delayed_frames_.front());
  input_delayed_frames_.pop_front();

  auto frame_for = [&](std::uint64_t tick_offset, std::uint32_t k) {
    if (tick_offset < partial_frames_.size() && partial_frames_[tick_offset].input_frames[k]) {
      return *partial_frames_[tick_offset].input_frames[k];
    }
    // Smoothing of latest input reduces predition jerkiness.
    static constexpr fixed kInputPredictionSmoothingFactor = 7_fx / 8_fx;
    latest_input_[k].velocity *= kInputPredictionSmoothingFactor;
    return latest_input_[k];
  };
  auto predicted_players = remote_players(mapping_);

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
      predicted_state_.set_predicted_players(predicted_players);
      predicted_state_.update(std::move(predicted_inputs));
      predicted_state_.output().clear();
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
  for (std::size_t i = 0; i < mapping_.local.player_numbers.size(); ++i) {
    auto player_number = mapping_.local.player_numbers[i];
    assert(!partial.input_frames[player_number]);
    partial.input_frames[player_number] = i < local_input.size() ? local_input[i] : input_frame{};
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
    handle_dual_output(canonical_state_.output());

    local_checksums_.emplace_back(canonical_state_.tick_count(), canonical_state_.checksum());
    partial_frames_.pop_front();
    canonical_state_.copy_to(predicted_state_);
    ++predicted_tick_base_;
  } else {
    predicted_state_.set_predicted_players(predicted_players);
    predicted_state_.update(std::move(inputs));
    handle_predicted_output(predicted_state_.tick_count(), predicted_state_.output());
  }
  predicted_state_.update_smoothing(smoothing_data_);
  if (!packet_output.empty()) {
    packet_output.front().canonical_tick_count = local_checksums_.back().tick_count;
    packet_output.front().canonical_checksum = local_checksums_.back().checksum;
  }

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
  return packet_output;
}

std::pair<std::string, std::uint64_t> NetworkedSimState::min_remote_latest_tick() const {
  std::pair<std::string, std::uint64_t> result{{}, predicted_state_.tick_count()};
  bool first = true;
  for (const auto& pair : remotes_) {
    if (first || pair.second.latest_tick < result.second) {
      result = {pair.first, pair.second.latest_tick};
    }
    first = false;
  }
  return result;
}

std::pair<std::string, std::uint64_t> NetworkedSimState::min_remote_canonical_tick() const {
  std::pair<std::string, std::uint64_t> result{{}, canonical_state_.tick_count()};
  bool first = true;
  for (const auto& pair : remotes_) {
    if (first || pair.second.canonical_tick < result.second) {
      result = {pair.first, pair.second.canonical_tick};
    }
    first = false;
  }
  return result;
}

void NetworkedSimState::handle_predicted_output(std::uint64_t tick_count,
                                                aggregate_output& output) {
  auto remote = remote_players(mapping_);
  for (auto& e : output.entries) {
    bool resolve = false;
    switch (e.key.type) {
    case resolve::kPredicted:
      resolve = true;
      break;
    case resolve::kCanonical:
      break;
    case resolve::kLocal:
      resolve = !e.key.cause_player_id ||
          std::ranges::find(remote, *e.key.cause_player_id) == remote.end();
      break;
    case resolve::kReconcile:
      resolve = true;
      reconciliation_map_[e.key] = tick_count;
      break;
    }
    if (resolve) {
      merged_output_.entries.emplace_back(std::move(e));
    }
  }
  output.clear();
}

void NetworkedSimState::handle_canonical_output(std::uint64_t tick_count,
                                                aggregate_output& output) {
  std::erase_if(reconciliation_map_, [&](const auto& pair) {
    return pair.second + kMaxReconcileTickDifference <= tick_count;
  });
  auto remote = remote_players(mapping_);
  for (auto& e : output.entries) {
    bool resolve = false;
    switch (e.key.type) {
    case resolve::kPredicted:
      break;
    case resolve::kCanonical:
      resolve = true;
      break;
    case resolve::kLocal:
      resolve = e.key.cause_player_id &&
          std::ranges::find(remote, *e.key.cause_player_id) != remote.end();
      break;
    case resolve::kReconcile:
      auto it = reconciliation_map_.find(e.key);
      if (it == reconciliation_map_.end()) {
        resolve = true;
      } else {
        reconciliation_map_.erase(it);
      }
      break;
    }
    if (resolve) {
      merged_output_.entries.emplace_back(std::move(e));
    }
  }
  output.clear();
}

void NetworkedSimState::handle_dual_output(aggregate_output& output) {
  // Called when updating predicted and canonical state in sync, so no fancy stuff needed.
  output.append_to(merged_output_);
  output.clear();
}

}  // namespace ii
