#ifndef II_GAME_LOGIC_SIM_IO_RESULT_EVENTS_H
#define II_GAME_LOGIC_SIM_IO_RESULT_EVENTS_H
#include "game/common/enum.h"
#include <bit>
#include <cstdint>

namespace ii {
enum class boss_flag : std::uint32_t {
  kBoss1A = 1,
  kBoss1B = 2,
  kBoss1C = 4,
  kBoss2A = 8,
  kBoss2B = 16,
  kBoss2C = 32,
  kBoss3A = 64,
};

template <>
struct bitmask_enum<boss_flag> : std::true_type {};

inline std::uint32_t boss_kill_count(boss_flag flag) {
  return std::popcount(static_cast<std::uint32_t>(flag));
}

}  // namespace ii

#endif
