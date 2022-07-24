#ifndef II_GAME_LOGIC_GEOMETRY_ENUMS_H
#define II_GAME_LOGIC_GEOMETRY_ENUMS_H
#include "game/common/enum.h"
#include <cstdint>

namespace ii {

enum class shape_flag : std::uint32_t {
  kNone = 0b000000,
  kVulnerable = 0b000001,
  kDangerous = 0b000010,
  kShield = 0b000100,            // Blocks all player projectiles.
  kWeakShield = 0b001000,        // Blocks normal player projectiles, magic shots can penetrate.
  kSafeShield = 0b010000,        // Blocks enemy projectiles.
  kEnemyInteraction = 0b100000,  // Interactions between enemies.
  kEverything = 0b111111,
};

template <>
struct bitmask_enum<shape_flag> : std::true_type {};

}  // namespace ii

#endif