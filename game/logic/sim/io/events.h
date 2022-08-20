#ifndef II_GAME_LOGIC_SIM_IO_EVENTS_H
#define II_GAME_LOGIC_SIM_IO_EVENTS_H
#include "game/common/enum.h"
#include <cstdint>
#include <optional>

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

struct run_event {
  std::optional<boss_flag> boss_kill;

  static run_event boss_kill_event(boss_flag boss) {
    run_event e;
    e.boss_kill = boss;
    return e;
  }
};

}  // namespace ii

#endif
