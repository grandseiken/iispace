#include "game/common/result.h"
#include "game/data/replay.h"
#include "game/flags.h"
#include "game/io/file/std_filesystem.h"
#include "game/mode_flags.h"
#include "game/tools/network_tools.h"
#include "game/tools/replay_tools.h"
#include <cstdint>
#include <iostream>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <thread>
#include <utility>
#include <vector>

namespace ii {
namespace {

struct options_t {
  network_options_t network;
  compatibility_level compatibility = compatibility_level::kIispaceV0;
  game_mode mode = game_mode::kNormal;
  initial_conditions::flag flags = initial_conditions::flag::kNone;
  std::uint32_t player_count = 0;
  std::optional<std::uint32_t> seed;
  std::optional<std::string> replay_out_path;
};

bool run(const options_t& options) {
  std::mt19937_64 engine{std::random_device{}()};
  initial_conditions conditions;
  conditions.compatibility = options.compatibility;
  conditions.mode = options.mode;
  conditions.flags = options.flags;
  conditions.player_count = options.player_count;
  conditions.seed =
      options.seed ? *options.seed : std::uniform_int_distribution<std::uint32_t>{}(engine);

  auto topology_result = parse_topology(options.network.topology, conditions.player_count);
  if (!topology_result) {
    std::cerr << topology_result.error() << std::endl;
    return false;
  }
  auto topology = std::move(*topology_result);

  std::unordered_set<std::string> peer_ids{topology.begin(), topology.end()};
  std::vector<std::unique_ptr<peer_t>> peers;
  std::vector<std::uint32_t> ai_players;
  for (std::uint32_t i = 0; i < conditions.player_count; ++i) {
    ai_players.emplace_back(i);
  }
  peers.reserve(peer_ids.size());
  for (const auto& id : peer_ids) {
    peers.emplace_back(
        std::make_unique<peer_t>(conditions, topology, options.network, id, peers, ai_players));
  }

  for (auto& peer : peers) {
    peer->thread = std::thread{[&] {
      while (!peer->sim.canonical().game_over()) {
        if (!(peer->sim.tick_count() % 1024)) {
          std::uniform_int_distribution<std::uint64_t> d{
              0, options.network.max_tick_delivery_delay / 2};
          peer->sim.set_input_delay_ticks(d(engine));
        }

        std::vector<input_frame> inputs;
        peer->sim.predicted().ai_think(inputs);
        std::vector<input_frame> local_inputs;
        for (auto n : peer->mapping.local.player_numbers) {
          local_inputs.emplace_back(inputs[n]);
        }
        peer->update(std::move(local_inputs));
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
  std::optional<sim_results> expected_results;
  std::optional<std::vector<std::uint8_t>> expected_replay_bytes;
  for (const auto& peer : peers) {
    for (const auto& id : peer->sim.checksum_failed_remote_ids()) {
      std::cerr << "error: " << peer->id << " reported checksum failure for " << id << std::endl;
      success = false;
    }
    const auto& results = peer->sim.canonical().results();
    if (!expected_results) {
      expected_results = results;
      std::cout << peer->id << ": score " << results.score << ", ticks " << results.tick_count
                << std::endl;
    } else {
      if (results.score != expected_results->score) {
        std::cout << "verification failed for " << peer->id << ": score was " << results.score
                  << std::endl;
        success = false;
      }
      if (results.tick_count < expected_results->tick_count) {
        std::cout << "verification failed for " << peer->id << ": ticks was " << results.tick_count
                  << std::endl;
        success = false;
      }
    }
    auto peer_replay_bytes = peer->replay_writer.write();
    if (!peer_replay_bytes) {
      std::cout << "replay serialization failed for " << peer->id << ": "
                << peer_replay_bytes.error() << std::endl;
      success = false;
    } else if (!expected_replay_bytes) {
      expected_replay_bytes = std::move(*peer_replay_bytes);
    } else if (*peer_replay_bytes != *expected_replay_bytes) {
      std::cout << "verification failed for " << peer->id << ": replay output bytes did not match"
                << std::endl;
      success = false;
    }
  }

  if (options.replay_out_path && expected_replay_bytes) {
    io::StdFilesystem fs{".", ".", "."};
    auto write_result = fs.write(*options.replay_out_path, *expected_replay_bytes);
    if (!write_result) {
      std::cerr << "couldn't write " << *options.replay_out_path << ": " << write_result.error()
                << std::endl;
      return false;
    }
    std::cout << "wrote " << *options.replay_out_path << std::endl;
  }
  std::cout << std::endl;
  return success;
}

result<options_t> parse_args(std::vector<std::string>& args) {
  options_t options;
  if (auto r = parse_network_args(args, options.network); !r) {
    return unexpected(r.error());
  }

  if (auto r = flag_parse<std::uint32_t>(args, "players", options.player_count); !r) {
    return unexpected(r.error());
  }
  if (!has_help_flag() && !options.player_count) {
    return unexpected("error: invalid player count");
  }
  if (auto r = flag_parse<std::uint32_t>(args, "seed", options.seed); !r) {
    return unexpected(r.error());
  }

  if (auto r = parse_game_mode(args, options.mode); !r) {
    return unexpected(r.error());
  }
  if (auto r = parse_compatibility_level(args, options.compatibility); !r) {
    return unexpected(r.error());
  }
  if (auto r = parse_initial_conditions_flags(args, options.flags); !r) {
    return unexpected(r.error());
  }

  if (auto r = flag_parse<std::string>(args, "output", options.replay_out_path); !r) {
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
  if (!args.empty()) {
    std::cerr << "error: invalid positional argument" << std::endl;
    return 1;
  }
  return ii::run(*options) ? 0 : 1;
}
