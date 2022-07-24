#include "game/data/replay.h"
#include "game/flags.h"
#include "game/io/file/std_filesystem.h"
#include "game/logic/sim/sim_state.h"
#include "game/replay_tools.h"
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace ii {
namespace {

struct options_t {
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
  auto results = replay_results(*replay_bytes, /* max ticks */ std::nullopt);
  if (!results) {
    std::cerr << results.error() << std::endl;
    return false;
  }
  std::cout << "================================================" << std::endl;
  std::cout << replay_path << std::endl;
  std::cout << "================================================" << std::endl;
  std::cout
      << "replay progress:\t"
      << (100 * static_cast<float>(results->replay_frames_read) / results->replay_frames_total)
      << "%" << std::endl;
  std::cout << "compatibility:  \t" << static_cast<std::uint32_t>(results->conditions.compatibility)
            << std::endl;
  std::cout << "players:        \t" << results->conditions.player_count << std::endl;
  std::cout << "flags:          \t" << +(results->conditions.flags) << std::endl;
  std::cout << "mode:           \t" << static_cast<std::uint32_t>(results->conditions.mode)
            << std::endl;
  std::cout << "seed:           \t" << results->conditions.seed << std::endl;
  std::cout << "ticks:          \t" << results->sim.tick_count << std::endl;
  std::cout << "score:          \t" << results->sim.score << std::endl;

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
  if (frames_read < results->replay_frames_total) {
    std::cerr << "verification failed: only " << results->replay_frames_read << " of "
              << results->replay_frames_total << " replay frames consumed" << std::endl;
    success = false;
  }
  std::cout << std::endl;
  return success;
}

}  // namespace
}  // namespace ii

int main(int argc, char** argv) {
  auto args = ii::args_init(argc, argv);
  ii::options_t options;
  if (auto result = ii::flag_parse<std::uint64_t>(args, "verify_score", options.verify_score);
      !result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }
  if (auto result = ii::flag_parse<std::uint64_t>(args, "verify_ticks", options.verify_ticks);
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
