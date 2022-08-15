#ifndef II_GAME_LOGIC_SHIP_COMPONENTS_H
#define II_GAME_LOGIC_SHIP_COMPONENTS_H
#include "game/common/math.h"
#include "game/common/struct_tuple.h"
#include "game/logic/ecs/index.h"
#include "game/logic/geometry/enums.h"
#include "game/logic/sim/sim_io.h"
#include "game/mixer/sound.h"
#include <glm/glm.hpp>
#include <sfn/functional.h>
#include <cstdint>
#include <optional>
#include <vector>

namespace ii {
class EmitHandle;
class SimInterface;

struct GlobalData : ecs::component {
  static constexpr std::uint32_t kStartingLives = 2;
  static constexpr std::uint32_t kBossModeLives = 1;

  std::uint32_t lives = 0;
  std::uint32_t non_wall_enemy_count = 0;
  std::optional<std::uint32_t> overmind_wave_timer;
  std::vector<std::uint32_t> player_kill_queue;

  struct fireworks_entry {
    std::uint32_t time = 0;
    vec2 position{0};
    glm::vec4 colour{0.f};
  };
  std::vector<fireworks_entry> fireworks;
  std::vector<vec2> extra_enemy_warnings;

  GlobalData(const initial_conditions& conditions)
  : lives{conditions.mode == game_mode::kBoss ? conditions.player_count * kBossModeLives
                                              : kStartingLives} {}
  void pre_update(SimInterface&);                // Runs before any other entity updates.
  void post_update(ecs::handle, SimInterface&);  // Runs after any other entity updates.
};
DEBUG_STRUCT_TUPLE(GlobalData, lives, non_wall_enemy_count, overmind_wave_timer, player_kill_queue);

struct Destroy : ecs::component {
  std::optional<ecs::entity_id> source;
};
DEBUG_STRUCT_TUPLE(Destroy);

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
DEBUG_STRUCT_TUPLE(Transform, centre, rotation);

struct Collision : ecs::component {
  shape_flag flags = shape_flag::kNone;
  fixed bounding_width = 0;
  sfn::ptr<shape_flag(ecs::const_handle, const vec2&, shape_flag)> check = nullptr;
};
DEBUG_STRUCT_TUPLE(Collision, flags, bounding_width, check);

struct Update : ecs::component {
  sfn::ptr<void(ecs::handle, SimInterface&)> update;
};
DEBUG_STRUCT_TUPLE(Update, update);

struct PostUpdate : ecs::component {
  sfn::ptr<void(ecs::handle, SimInterface&)> post_update;
};
DEBUG_STRUCT_TUPLE(PostUpdate, post_update);

struct Render : ecs::component {
  sfn::ptr<void(ecs::const_handle, const SimInterface&)> render = nullptr;
};
DEBUG_STRUCT_TUPLE(Render, render);

enum class damage_type {
  kNone,
  kMagic,
  kBomb,
  kPredicted,
};

struct Health : ecs::component {
  std::uint32_t hp = 0;
  std::uint32_t max_hp = hp;

  std::uint32_t hit_timer = 0;
  std::optional<std::size_t> hit_flash_ignore_index;

  std::optional<sound> hit_sound0 = sound::kEnemyHit;
  std::optional<sound> hit_sound1 = sound::kEnemyHit;
  std::optional<sound> destroy_sound = sound::kEnemyDestroy;

  sfn::ptr<std::uint32_t(ecs::handle, SimInterface&, damage_type, std::uint32_t)> damage_transform =
      nullptr;
  sfn::ptr<void(ecs::handle, SimInterface&, EmitHandle&, damage_type)> on_hit = nullptr;
  sfn::ptr<void(ecs::const_handle, SimInterface&, EmitHandle&, damage_type)> on_destroy = nullptr;

  bool is_hp_low() const {
    // hp <= .4 * max_hp.
    return 3 * hp <= max_hp + max_hp / 5;
  }

  void damage(ecs::handle h, SimInterface&, std::uint32_t damage, damage_type type,
              std::optional<ecs::entity_id> source);
};
DEBUG_STRUCT_TUPLE(Health, hp, max_hp, damage_transform, on_hit, on_destroy);

enum class powerup_type {
  kExtraLife,
  kMagicShots,
  kShield,
  kBomb,
};

struct PowerupTag : ecs::component {
  powerup_type type = powerup_type::kExtraLife;
};
DEBUG_STRUCT_TUPLE(PowerupTag);

struct WallTag : ecs::component {};
DEBUG_STRUCT_TUPLE(WallTag);

struct Player : ecs::component {
  static constexpr std::uint32_t kBombDamage = 50;

  std::uint32_t player_number = 0;
  std::uint32_t kill_timer = 0;
  std::uint32_t magic_shot_count = 0;
  bool has_bomb = false;
  bool has_shield = false;
  bool is_predicted = false;

  std::uint64_t score = 0;
  std::uint32_t multiplier = 1;
  std::uint32_t multiplier_count = 0;
  std::uint32_t death_count = 0;

  bool is_killed() const {
    return kill_timer != 0;
  }
  void add_score(SimInterface&, std::uint64_t s);
};
DEBUG_STRUCT_TUPLE(Player, player_number, kill_timer, magic_shot_count, has_bomb, has_shield, score,
                   multiplier, multiplier_count, death_count);

struct Enemy : ecs::component {
  std::uint32_t threat_value = 1;
  std::uint32_t score_reward = 0;
  std::uint32_t boss_score_reward = 0;
};
DEBUG_STRUCT_TUPLE(Enemy, threat_value, score_reward, boss_score_reward);

struct Boss : ecs::component {
  boss_flag boss = boss_flag{0};
  bool show_hp_bar = false;
};
DEBUG_STRUCT_TUPLE(Boss, boss);

}  // namespace ii

#endif
