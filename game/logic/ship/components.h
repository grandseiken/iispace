#ifndef II_GAME_LOGIC_SHIP_COMPONENTS_H
#define II_GAME_LOGIC_SHIP_COMPONENTS_H
#include "game/common/functional.h"
#include "game/common/math.h"
#include "game/logic/ecs/index.h"
#include "game/logic/ship/enums.h"
#include "game/logic/sim/sim_io.h"
#include "game/mixer/sound.h"
#include <glm/glm.hpp>
#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace ii {
class SimInterface;

struct GlobalData : ecs::component {
  static constexpr std::uint32_t kStartingLives = 2;
  static constexpr std::uint32_t kBossModeLives = 1;

  std::uint32_t lives = 0;
  std::vector<std::uint32_t> player_kill_queue;

  GlobalData(const initial_conditions& conditions)
  : lives{conditions.mode == game_mode::kBoss ? conditions.player_count * kBossModeLives
                                              : kStartingLives} {}
};

class IShip;
struct LegacyShip : ecs::component {
  std::unique_ptr<IShip> ship;
};

struct Destroy : ecs::component {
  std::optional<ecs::entity_id> source;
};

struct Transform : ecs::component {
  vec2 centre = {0, 0};
  fixed rotation = 0;

  void move(const vec2& v) {
    centre += v;
  }
  void rotate(fixed amount) {
    set_rotation(rotation + amount);
  }
  void set_rotation(fixed r) {
    rotation = normalise_angle(r);
  }
};

struct Collision : ecs::component {
  shape_flag flags = shape_flag::kNone;
  fixed bounding_width = 0;
  function_ptr<bool(ecs::const_handle, const vec2&, shape_flag)> check = nullptr;
};

struct Update : ecs::component {
  function_ptr<void(ecs::handle, SimInterface&)> update;
};

struct Render : ecs::component {
  function_ptr<void(ecs::const_handle, const SimInterface&)> render = nullptr;
};

struct Health : ecs::component {
  std::uint32_t hp = 0;
  std::uint32_t max_hp = hp;

  std::uint32_t hit_timer = 0;
  std::optional<std::uint32_t> hit_flash_ignore_index;

  std::optional<sound> hit_sound0 = sound::kEnemyHit;
  std::optional<sound> hit_sound1 = sound::kEnemyHit;
  std::optional<sound> destroy_sound = sound::kEnemyDestroy;

  function_ptr<std::uint32_t(ecs::handle, SimInterface&, damage_type, std::uint32_t)>
      damage_transform = nullptr;
  function_ptr<void(ecs::handle, SimInterface&, damage_type)> on_hit = nullptr;
  function_ptr<void(ecs::const_handle, SimInterface&, damage_type)> on_destroy = nullptr;

  bool is_hp_low() const {
    // hp <= .4 * max_hp.
    return 3 * hp <= max_hp + max_hp / 5;
  }

  void damage(ecs::handle h, SimInterface&, std::uint32_t damage, damage_type type,
              std::optional<ecs::entity_id> source);
};

struct PowerupTag : ecs::component {};
struct WallTag : ecs::component {};

struct Player : ecs::component {
  static constexpr std::uint32_t kBombDamage = 50;

  std::uint32_t player_number = 0;
  std::uint32_t kill_timer = 0;
  std::uint32_t magic_shot_count = 0;
  bool has_bomb = false;
  bool has_shield = false;

  std::uint64_t score = 0;
  std::uint32_t multiplier = 1;
  std::uint32_t multiplier_count = 0;
  std::uint32_t death_count = 0;

  bool is_killed() const {
    return kill_timer != 0;
  }
  void add_score(SimInterface&, std::uint64_t s);
};

struct Enemy : ecs::component {
  std::uint32_t threat_value = 1;
  std::uint32_t score_reward = 0;
  std::uint32_t boss_score_reward = 0;
};

struct Boss : ecs::component {
  boss_flag boss = boss_flag{0};
  bool show_hp_bar = false;
};

}  // namespace ii

#endif