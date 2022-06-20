#ifndef II_GAME_LOGIC_SHIP_ENUMS_H
#define II_GAME_LOGIC_SHIP_ENUMS_H
#include "game/common/enum.h"
#include <cstdint>

namespace ii {

// TODO: can surely be removed and replaced with marker components soon.
// One issue is chaser boss doesn't have the boss component for (reasons).
enum class ship_flag : std::uint32_t {
  kNone = 0,
  kPlayer = 1,
  kWall = 2,
  kEnemy = 4,
  kBoss = 8,
  kPowerup = 16,
};

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

enum class damage_type {
  kNone,
  kMagic,
  kBomb,
};

template <>
struct bitmask_enum<ship_flag> : std::true_type {};
template <>
struct bitmask_enum<shape_flag> : std::true_type {};

}  // namespace ii

#endif