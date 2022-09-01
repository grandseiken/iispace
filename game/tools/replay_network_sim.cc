#include "game/common/result.h"
#include "game/data/replay.h"
#include "game/flags.h"
#include "game/io/file/std_filesystem.h"
#include "game/tools/network_tools.h"
#include "game/tools/replay_tools.h"
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <thread>
#include <unordered_set>
#include <utility>
#include <vector>

namespace ii {
namespace {

bool run(const network_options_t& options, const std::string& replay_path) {
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

  std::unordered_set<std::string> peer_ids{topology.begin(), topology.end()};
  std::vector<std::unique_ptr<peer_t>> peers;
  peers.reserve(peer_ids.size());
  for (const auto& id : peer_ids) {
    peers.emplace_back(std::make_unique<peer_t>(replay_reader->initial_conditions(), topology,
                                                options, id, peers));
  }

  for (auto& peer : peers) {
    peer->thread = std::thread{[&] {
      while (!peer->sim.canonical().game_over()) {
        auto current_tick = peer->sim.tick_count();
        std::vector<input_frame> local_input;
        for (auto n : peer->mapping.local.player_numbers) {
          local_input.emplace_back(current_tick < per_player_inputs[n].size()
                                       ? per_player_inputs[n][current_tick]
                                       : input_frame{});
        }
        peer->update(std::move(local_input));
      }
    }};
  }
  for (auto& peer : peers) {
    peer->thread.join();
  }
  std::cout << "================================================\n"
            << "simulation complete\n"
            << "================================================" << std::endl;

  bool success = true;
  for (const auto& peer : peers) {
    for (const auto& id : peer->sim.checksum_failed_remote_ids()) {
      std::cerr << "error: " << peer->id << " reported checksum failure for " << id << std::endl;
      success = false;
    }
    const auto& results = peer->sim.canonical().results();
    if (results.score != canonical_results->sim.score) {
      std::cout << "verification failed for " << peer->id << ": score was " << results.score
                << std::endl;
      success = false;
    }
    if (results.tick_count < canonical_results->sim.tick_count) {
      std::cout << "verification failed for " << peer->id << ": ticks was " << results.tick_count
                << std::endl;
      success = false;
    }
    auto peer_replay_bytes = peer->replay_writer.write();
    if (!peer_replay_bytes) {
      std::cout << "replay serialization failed for " << peer->id << ": "
                << peer_replay_bytes.error() << std::endl;
      success = false;
    }
    if (*peer_replay_bytes != *canonical_bytes) {
      std::cout << "verification failed for " << peer->id << ": replay output bytes did not match"
                << std::endl;
      success = false;
    }
  }
  return success;
}

result<network_options_t> parse_args(std::vector<std::string>& args) {
  network_options_t options;
  if (auto r = parse_network_args(args, options); !r) {
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
