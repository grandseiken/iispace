#ifndef II_GAME_DATA_SAVE_H
#define II_GAME_DATA_SAVE_H
#include "game/common/result.h"
#include "game/logic/sim/io/output.h"
#include <cstdint>
#include <span>
#include <vector>

namespace ii::data {

struct savegame {
  boss_flag bosses_killed{0};
  boss_flag hard_mode_bosses_killed{0};
};

result<savegame> read_savegame(std::span<const std::uint8_t>);
result<std::vector<std::uint8_t>> write_savegame(const savegame&);

}  // namespace ii::data

#endif
