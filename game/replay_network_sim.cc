#include "game/common/result.h"
#include "game/data/replay.h"
#include "game/flags.h"
#include "game/io/file/std_filesystem.h"
#include "game/logic/sim/networked_sim_state.h"
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace ii {
namespace {

struct options_t {
  std::string topology;
  std::optional<std::uint64_t> verify_ticks;
  std::optional<std::uint64_t> verify_score;
};

// For each player index, the remote ID (or none - local).
using topology_t = std::vector<std::optional<std::string>>;

result<topology_t> parse_topology(const std::string& topology_string, std::uint32_t player_count) {
  if (topology_string.size() != player_count) {
    return unexpected("topology " + topology_string + " is invalid for replay with player count " +
                      std::to_string(player_count));
  }
  topology_t topology;
  for (std::uint32_t i = 0; i < player_count; ++i) {
    if (topology_string[i] == '0') {
      topology.emplace_back();
    } else {
      topology.emplace_back(topology_string.substr(i, 1));
    }
  }
  return topology;
}

NetworkedSimState::input_mapping local_mapping(const topology_t& topology) {
  NetworkedSimState::input_mapping m;
  for (std::uint32_t i = 0; i < topology.size(); ++i) {
    if (topology[i]) {
      m.remote[*topology[i]].player_numbers.emplace_back(i);
    } else {
      m.local.player_numbers.emplace_back(i);
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
  auto topology_result =
      parse_topology(options.topology, replay_reader->initial_conditions().player_count);
  if (!topology_result) {
    std::cerr << topology_result.error() << std::endl;
  }
  auto topology = std::move(*topology_result);

  NetworkedSimState network_sim{replay_reader->initial_conditions(), local_mapping(topology),
                                /* TODO - verify replay output */ nullptr};
  std::uint64_t tick = 0;
  while (!network_sim.canonical().game_over()) {
    auto inputs = replay_reader->next_tick_input_frames();
    std::vector<input_frame> local_inputs;
    std::unordered_map<std::string, sim_packet> remote_packets;
    for (std::size_t i = 0; i < inputs.size(); ++i) {
      if (topology[i]) {
        auto& p = remote_packets[*topology[i]];
        p.tick_count = p.canonical_tick_count = tick;
        p.input_frames.emplace_back(inputs[i]);
      } else {
        local_inputs.emplace_back(inputs[i]);
      }
    }
    for (const auto& pair : remote_packets) {
      network_sim.input_packet(pair.first, pair.second);
    }
    network_sim.update(std::move(local_inputs));
    ++tick;
  }

  bool success = true;
  auto results = network_sim.canonical().get_results();
  if (options.verify_score && *options.verify_score != results.score) {
    std::cerr << "verification failed: expected score " << *options.verify_score << ", was "
              << results.score << std::endl;
    success = false;
  }
  if (options.verify_ticks && *options.verify_ticks != results.tick_count) {
    std::cerr << "verification failed: expected ticks " << *options.verify_ticks << ", was "
              << results.tick_count << std::endl;
    success = false;
  }
  return success;
}

result<options_t> parse_args(std::vector<std::string>& args) {
  options_t options;
  if (auto r = flag_parse(args, "topology", options.topology); !r) {
    return unexpected(r.error());
  }
  if (auto r = flag_parse(args, "verify_score", options.verify_score); !r) {
    return unexpected(r.error());
  }
  if (auto r = flag_parse(args, "verify_ticks", options.verify_ticks); !r) {
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
