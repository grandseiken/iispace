#ifndef II_GAME_TOOLS_CONDITIONS_H
#define II_GAME_TOOLS_CONDITIONS_H
#include "game/logic/sim/io/conditions.h"

namespace ii {

void fill_standard_run_conditions(initial_conditions& conditions) {
  if (conditions.mode != game_mode::kStandardRun) {
    return;
  }
  conditions.biomes.emplace_back(run_biome::kTesting);
}

}  // namespace ii

#endif
