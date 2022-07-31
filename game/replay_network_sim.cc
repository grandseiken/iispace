#include "game/common/result.h"
#include "game/data/replay.h"
#include "game/flags.h"
#include "game/io/file/std_filesystem.h"
#include "game/logic/sim/networked_sim_state.h"
#include "game/replay_tools.h"
#include <cstdint>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace ii {
namespace {

struct options_t {
  std::string topology;
};

// For each player index, the remote ID.
using topology_t = std::vector<std::string>;

result<topology_t> parse_topology(const std::string& topology_string, std::uint32_t player_count) {
  if (topology_string.size() != player_count) {
    return unexpected("topology " + topology_string + " is invalid for replay with player count " +
                      std::to_string(player_count));
  }
  topology_t topology;
  for (std::uint32_t i = 0; i < player_count; ++i) {
    topology.emplace_back("peer:" + topology_string.substr(i, 1));
  }
  return topology;
}

NetworkedSimState::input_mapping mapping_for(const topology_t& topology, const std::string& id) {
  NetworkedSimState::input_mapping m;
  for (std::uint32_t i = 0; i < topology.size(); ++i) {
    if (topology[i] == id) {
      m.local.player_numbers.emplace_back(i);
    } else {
      m.remote[topology[i]].player_numbers.emplace_back(i);
    }
  }
  return m;
}

bool run(const options_t& options, const std::string& replay_path) {
  io::StdFilesystem fs{".", ".", "."};
  auto replay_bytes = fs.read(replay_path);
  if (!replay_bytes) {
    std::cerr << replay_bytes.error() << std::endl;
    return false;
  }

  auto replay_reader = ReplayReader::create(*replay_bytes);
  if (!replay_reader) {
    std::cerr << replay_reader.error() << std::endl;
    return false;
  }
  auto player_count = replay_reader->initial_conditions().player_count;
  auto topology_result = parse_topology(options.topology, player_count);
  if (!topology_result) {
    std::cerr << topology_result.error() << std::endl;
    return false;
  }
  auto topology = std::move(*topology_result);

  auto canonical_results = replay_results(*replay_bytes);
  if (!canonical_results) {
    std::cerr << canonical_results.error() << std::endl;
    return false;
  }
  print_replay_info(std::cout, replay_path, *canonical_results);

  using per_player_inputs_t = std::vector<std::vector<input_frame>>;
  per_player_inputs_t per_player_inputs;
  per_player_inputs.resize(player_count);
  ReplayWriter canonical_bytes_writer{replay_reader->initial_conditions()};
  std::size_t canonical_frames_written = 0;
  while (replay_reader->current_input_frame() < replay_reader->total_input_frames()) {
    auto inputs = replay_reader->next_tick_input_frames();
    for (std::size_t i = 0; i < inputs.size(); ++i) {
      per_player_inputs[i].emplace_back(inputs[i]);
      if (canonical_frames_written < canonical_results->replay_frames_read) {
        canonical_bytes_writer.add_input_frame(inputs[i]);
        ++canonical_frames_written;
      }
    }
  }
  auto canonical_bytes = canonical_bytes_writer.write();
  if (!canonical_bytes) {
    std::cerr << canonical_bytes.error() << std::endl;
    return false;
  }
  std::cout << "================================================" << std::endl;

  struct received_packet {
    std::string remote_id;
    sim_packet packet;
  };

  struct peer_t {
    peer_t(const initial_conditions& conditions, const topology_t& topology, const std::string& id,
           std::function<void(sim_packet)> send_function)
    : id{id}
    , replay_writer{conditions}
    , mapping{mapping_for(topology, id)}
    , sim{conditions, mapping, &replay_writer}
    , send_function{std::move(send_function)} {
      std::cout << "initialising " << id << " with local players ";
      bool first = true;
      for (auto n : mapping.local.player_numbers) {
        if (!first) {
          std::cout << "; ";
        }
        first = false;
        std::cout << n;
      }
      std::cout << std::endl;
    }

    std::string id;
    ReplayWriter replay_writer;
    NetworkedSimState::input_mapping mapping;
    NetworkedSimState sim;
    std::vector<received_packet> receive_queue;
    std::function<void(sim_packet)> send_function;

    void update(const per_player_inputs_t& replay) {
      for (const auto& packet : receive_queue) {
        sim.input_packet(packet.remote_id, packet.packet);
      }
      receive_queue.clear();
      auto tick = sim.predicted().tick_count();
      std::vector<input_frame> local_input;
      for (auto n : mapping.local.player_numbers) {
        local_input.emplace_back(tick < replay[n].size() ? replay[n][tick] : input_frame{});
      }
      send_function(sim.update(std::move(local_input)));
    }
  };

  std::unordered_set<std::string> peer_ids{topology.begin(), topology.end()};
  std::vector<peer_t> peers;
  peers.reserve(peer_ids.size());
  for (const auto& id : peer_ids) {
    auto send_function = [&peers, id](sim_packet packet) {
      for (auto& peer : peers) {
        if (peer.id != id) {
          peer.receive_queue.emplace_back(received_packet{id, packet});
        }
      }
    };
    peers.emplace_back(replay_reader->initial_conditions(), topology, id, std::move(send_function));
  }

  while (true) {
    bool done = true;
    for (auto& peer : peers) {
      peer.update(per_player_inputs);
      done &= peer.sim.canonical().game_over();
      if (!peer.sim.checksum_failed_remote_ids().empty()) {
        std::cerr << "error: " << peer.id << " reported checksum failure for "
                  << *peer.sim.checksum_failed_remote_ids().begin() << std::endl;
        return false;
      }
    }
    if (done) {
      break;
    }
  }
  std::cout << "================================================\n"
            << "simulation complete\n"
            << "================================================" << std::endl;

  bool success = true;
  for (const auto& peer : peers) {
    auto results = peer.sim.canonical().get_results();
    if (results.score != canonical_results->sim.score) {
      std::cout << "verification failed for " << peer.id << ": score was " << results.score
                << std::endl;
      success = false;
    }
    if (results.tick_count < canonical_results->sim.tick_count) {
      std::cout << "verification failed for " << peer.id << ": ticks was " << results.tick_count
                << std::endl;
      success = false;
    }
    auto peer_replay_bytes = peer.replay_writer.write();
    if (!peer_replay_bytes) {
      std::cout << "replay serialization failed for " << peer.id << ": "
                << peer_replay_bytes.error() << std::endl;
      success = false;
    }
    if (*peer_replay_bytes != *canonical_bytes) {
      std::cout << "verification failed for " << peer.id << ": replay output bytes did not match"
                << std::endl;
      success = false;
    }
  }
  return success;
}

result<options_t> parse_args(std::vector<std::string>& args) {
  options_t options;
  if (auto r = flag_parse(args, "topology", options.topology); !r) {
    return unexpected(r.error());
  }
  return {std::move(options)};
}

}  // namespace
}  // namespace ii

int main(int argc, char** argv) {
  auto args = ii::args_init(argc, argv);
  auto options = ii::parse_args(args);
  if (!options) {
    std::cerr << options.error() << std::endl;
    return 1;
  }
  if (auto result = ii::args_finish(args); !result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }
  if (args.empty()) {
    std::cerr << "no paths" << std::endl;
    return 1;
  }
  int exit = 0;
  for (const auto& path : args) {
    if (!ii::run(*options, path)) {
      exit = 1;
    }
  }
  return exit;
}
