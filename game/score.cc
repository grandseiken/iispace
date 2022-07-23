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

bool run(std::optional<std::uint64_t> check, const std::string& replay_path) {
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
  if (check) {
    if (*check != results->sim.score) {
      std::cerr
          << "check failed: expected score " << *check << ", was " << results->sim.score << " at "
          << (100 * static_cast<float>(results->replay_frames_read) / results->replay_frames_total)
          << "% of input" << std::endl;
      return false;
    }
    return true;
  }
  std::cout << results->sim.score << std::endl;
  return true;
}

}  // namespace
}  // namespace ii

int main(int argc, char** argv) {
  auto args = ii::args_init(argc, argv);
  auto check = ii::flag_parse<std::uint64_t>(args, "check");
  if (!check) {
    std::cerr << check.error() << std::endl;
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
  for (const auto& path : args) {
    if (!ii::run(*check, path)) {
      return 1;
    }
  }
  return 0;
}
