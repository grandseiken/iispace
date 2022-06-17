#ifndef II_GAME_LOGIC_SHIP_ENUMS_H
#define II_GAME_LOGIC_SHIP_ENUMS_H
#include "game/common/enum.h"
#include <cstdint>

namespace ii {

enum class ship_flag : std::uint32_t {
  kNone = 0,
  kPlayer = 1,
  kWall = 2,
  kEnemy = 4,
  kBoss = 8,
  kPowerup = 16,
};

enum class shape_flag : std::uint32_t {
  kNone = 0,
  kVulnerable = 1,
  kDangerous = 2,
  kShield = 4,       // Blocks all player projectiles.
  kWeakShield = 8,   // Blocks normal player projectiles, magic shots can penetrate.
  kSafeShield = 16,  // Blocks enemy projectiles.
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