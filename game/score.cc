#include "game/data/replay.h"
#include "game/io/file/std_filesystem.h"
#include "game/logic/sim/sim_state.h"
#include <iostream>
#include <optional>
#include <string>
#include <vector>

namespace ii {

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
  std::uint64_t score = 0;
  if (results.mode == game_mode::kBoss) {
    score = results.elapsed_time;
  } else {
    for (const auto& p : results.players) {
      score += p.score;
    }
  }
  if (check) {
    return *check == score;
  }
  std::cout << score << std::endl;
  return true;
}

}  // namespace ii

int main(int argc, char** argv) {
  std::vector<std::string> args;
  std::optional<std::uint64_t> check;
  bool run = false;
  for (int i = 1; i < argc; ++i) {
    std::string arg = argv[i];
    if (arg == "--check") {
      check = std::stol(argv[++i]);
    } else {
      run = true;
      if (!ii::run(check, arg)) {
        return 1;
      }
    }
  }
  return run ? 0 : 1;
}
