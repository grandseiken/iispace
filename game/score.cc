#include "game/data/replay.h"
#include "game/flags.h"
#include "game/io/file/std_filesystem.h"
#include "game/logic/sim/sim_state.h"
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace ii {
namespace {

bool run(std::optional<std::uint64_t> check, const std::string& replay_path) {
  io::StdFilesystem fs{".", ".", "."};
  auto result = fs.read(replay_path);
  if (!result) {
    std::cerr << result.error() << std::endl;
    return false;
  }
  auto reader = ReplayReader::create(*result);
  if (!reader) {
    std::cerr << reader.error() << std::endl;
    return false;
  }
  ReplayInputAdapter input{*reader};
  SimState sim{reader->initial_conditions(), input};
  while (!sim.game_over()) {
    sim.update();
    sim.clear_output();
  }
  auto results = sim.get_results();
  if (check) {
    if (*check != results.score) {
      std::cerr << "check failed: expected score " << *check << ", was " << results.score << " at "
                << (100 * static_cast<float>(reader->current_input_frame()) /
                    reader->total_input_frames())
                << "% of input" << std::endl;
      return false;
    }
    return true;
  }
  std::cout << results.score << std::endl;
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
