#include "game/data/replay.h"
#include "game/flags.h"
#include "game/io/file/std_filesystem.h"
#include "game/logic/sim/sim_state.h"
#include "game/replay_tools.h"
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace ii {
namespace {

struct options_t {
  SimState::query query;

  std::optional<std::uint64_t> dump_state_from_tick;
  std::optional<std::uint64_t> max_ticks;

  std::optional<std::uint64_t> verify_ticks;
  std::optional<std::uint64_t> verify_score;
};

bool run(const options_t& options, const std::string& replay_path) {
  io::StdFilesystem fs{".", ".", "."};
  auto replay_bytes = fs.read(replay_path);
  if (!replay_bytes) {
    std::cerr << replay_bytes.error() << std::endl;
    return false;
  }
  auto results =
      replay_results(*replay_bytes, options.max_ticks, options.dump_state_from_tick, options.query);
  if (!results) {
    std::cerr << results.error() << std::endl;
    return false;
  }
  std::cout
      << "================================================\n"
      << replay_path << "\n"
      << "================================================\n"
      << "replay progress:\t"
      << (100 * static_cast<float>(results->replay_frames_read) / results->replay_frames_total)
      << "%\n"
      << "compatibility:  \t" << static_cast<std::uint32_t>(results->conditions.compatibility)
      << "\n"
      << "players:        \t" << results->conditions.player_count << "\n"
      << "flags:          \t" << +(results->conditions.flags) << "\n"
      << "mode:           \t" << static_cast<std::uint32_t>(results->conditions.mode) << "\n"
      << "seed:           \t" << results->conditions.seed << "\n"
      << "ticks:          \t" << results->sim.tick_count << "\n"
      << "score:          \t" << results->sim.score << std::endl;
  for (std::size_t i = 0; i < results->state_dumps.size(); ++i) {
    if (results->state_dumps[i].empty()) {
      continue;
    }
    std::cout << "\n================================================\n"
              << "tick " << (options.dump_state_from_tick.value_or(0u) + i) << " state dump\n"
              << "================================================\n";
    std::cout << results->state_dumps[i] << std::flush;
  }

  bool success = true;
  if (options.verify_score && *options.verify_score != results->sim.score) {
    std::cerr << "verification failed: expected score " << *options.verify_score << ", was "
              << results->sim.score << std::endl;
    success = false;
  }
  if (options.verify_ticks && *options.verify_ticks != results->sim.tick_count) {
    std::cerr << "verification failed: expected ticks " << *options.verify_ticks << ", was "
              << results->sim.tick_count << std::endl;
    success = false;
  }
  auto frames_read = results->replay_frames_read;
  if (results->conditions.compatibility == compatibility_level::kLegacy) {
    ++frames_read;
  }
  if (!options.max_ticks && frames_read < results->replay_frames_total) {
    std::cerr << "verification failed: only " << results->replay_frames_read << " of "
              << results->replay_frames_total << " replay frames consumed" << std::endl;
    success = false;
  }
  return success;
}

}  // namespace
}  // namespace ii

int main(int argc, char** argv) {
  auto args = ii::args_init(argc, argv);
  ii::options_t options;
  if (auto result = ii::flag_parse(args, "verify_score", options.verify_score); !result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }
  if (auto result = ii::flag_parse(args, "verify_ticks", options.verify_ticks); !result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }

  if (auto result = ii::flag_parse(args, "dump_tick_from", options.dump_state_from_tick); !result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }
  if (auto result = ii::flag_parse(args, "dump_tick_to", options.max_ticks); !result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }
  std::optional<std::uint64_t> dump_tick;
  if (auto result = ii::flag_parse(args, "dump_tick", dump_tick); !result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }
  if (dump_tick) {
    options.dump_state_from_tick = dump_tick;
    options.max_ticks = dump_tick;
  }

  if (auto result = ii::flag_parse(args, "dump_entity_ids", options.query.entity_ids); !result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }

  if (auto result = ii::flag_parse(args, "dump_components", options.query.component_names);
      !result) {
    std::cerr << result.error() << std::endl;
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
    if (!ii::run(options, path)) {
      exit = 1;
    }
  }
  return exit;
}
