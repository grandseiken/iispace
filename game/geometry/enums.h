#ifndef II_LOGIC_GEOMETRY_ENUMS_H
#define II_LOGIC_GEOMETRY_ENUMS_H
#include "game/common/enum.h"
#include <cstdint>

namespace ii {

enum class shape_flag : std::uint32_t {
  kNone = 0b00000000,
  kVulnerable = 0b00000001,        // Can be hit by player (and consumes non-penetrating shots).
  kWeakVulnerable = 0b00000010,    // Can be hit by player, and shots automatically penetrate.
  kBombVulnerable = 0b00000100,    // Can be hit by bomb.
  kDangerous = 0b00001000,         // Kills player.
  kShield = 0b00010000,            // Blocks all player projectiles.
  kWeakShield = 0b00100000,        // Blocks normal player projectiles, magic shots can penetrate.
  kSafeShield = 0b01000000,        // Blocks enemy projectiles.
  kEnemyInteraction = 0b10000000,  // Interactions between enemies.
  kEverything = 0b11111111,
};

template <>
struct bitmask_enum<shape_flag> : std::true_type {};

}  // namespace ii

#endif