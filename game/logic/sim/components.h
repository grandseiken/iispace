#ifndef II_GAME_LOGIC_SIM_COMPONENTS_H
#define II_GAME_LOGIC_SIM_COMPONENTS_H
#include "game/common/math.h"
#include "game/common/struct_tuple.h"
#include "game/logic/ecs/index.h"
#include "game/logic/geometry/enums.h"
#include "game/logic/sim/io/render.h"
#include "game/mixer/sound.h"
#include <sfn/functional.h>

namespace ii {
class EmitHandle;
class SimInterface;
enum class boss_flag : std::uint32_t;

enum class damage_type {
  kNone,
  kMagic,
  kBomb,
  kPredicted,
};

enum class rumble_type {
  kNone,
  kLow,
  kSmall,
  kMedium,
  kLarge,
};

enum class powerup_type {
  kExtraLife,
  kMagicShots,
  kShield,
  kBomb,
};

struct Destroy : ecs::component {
  std::optional<ecs::entity_id> source;
  std::optional<damage_type> destroy_type;
};
DEBUG_STRUCT_TUPLE(Destroy);

struct Transform : ecs::component {
  vec2 centre = {0, 0};
  fixed rotation = 0;

  void move(const vec2& v) { centre += v; }
  void rotate(fixed amount) { set_rotation(rotation + amount); }
  void set_rotation(fixed r) { rotation = normalise_angle(r); }
};
DEBUG_STRUCT_TUPLE(Transform, centre, rotation);

struct Collision : ecs::component {
  shape_flag flags = shape_flag::kNone;
  fixed bounding_width = 0;

  using check_t = shape_flag(ecs::const_handle, const vec2&, shape_flag);
  sfn::ptr<check_t> check = nullptr;
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
  using render_t = void(ecs::const_handle, std::vector<render::shape>&, const SimInterface&);
  sfn::ptr<render_t> render = nullptr;
  bool clear_trails = false;

  void render_shapes(ecs::const_handle, render::entity_state& state, bool paused,
                     std::vector<render::shape>&, const SimInterface&);
};
DEBUG_STRUCT_TUPLE(Render, render);

struct WallTag : ecs::component {};
DEBUG_STRUCT_TUPLE(WallTag);

// TODO: should really be in legacy (but AI needs to know powerup type to know whether to chase it).
struct PowerupTag : ecs::component {
  powerup_type type = powerup_type::kExtraLife;
};
DEBUG_STRUCT_TUPLE(PowerupTag);

struct Boss : ecs::component {
  boss_flag boss = boss_flag{0};
  bool show_hp_bar = false;
};
DEBUG_STRUCT_TUPLE(Boss, boss);

struct Enemy : ecs::component {
  std::uint32_t threat_value = 1;
  std::uint32_t score_reward = 0;
  std::uint32_t boss_score_reward = 0;
};
DEBUG_STRUCT_TUPLE(Enemy, threat_value, score_reward, boss_score_reward);

struct Health : ecs::component {
  std::uint32_t hp = 0;
  std::uint32_t max_hp = hp;

  std::uint32_t hit_timer = 0;
  std::optional<std::size_t> hit_flash_ignore_index;

  std::optional<sound> hit_sound0 = sound::kEnemyHit;
  std::optional<sound> hit_sound1 = sound::kEnemyHit;
  std::optional<sound> destroy_sound = sound::kEnemyDestroy;
  std::optional<rumble_type> destroy_rumble;

  struct hit_t {
    vec2 source{0, 0};
    std::uint64_t tick = 0;
  };
  std::vector<hit_t> hits;

  using damage_transform_t = std::uint32_t(ecs::handle, SimInterface&, damage_type, std::uint32_t);
  using on_hit_t = void(ecs::handle, SimInterface&, EmitHandle&, damage_type, const vec2&);
  using on_destroy_t = void(ecs::const_handle, SimInterface&, EmitHandle&, damage_type,
                            const vec2&);

  sfn::ptr<damage_transform_t> damage_transform = nullptr;
  sfn::ptr<on_hit_t> on_hit = nullptr;
  sfn::ptr<on_destroy_t> on_destroy = nullptr;

  bool is_hp_low() const {
    // hp <= .4 * max_hp.
    return 3 * hp <= max_hp + max_hp / 5;
  }

  void damage(ecs::handle h, SimInterface&, std::uint32_t damage, damage_type type,
              ecs::entity_id source_id, std::optional<vec2> source_position = std::nullopt);
};
DEBUG_STRUCT_TUPLE(Health, hp, max_hp, damage_transform, on_hit, on_destroy);

// TODO: split this up.
struct Player : ecs::component {
  std::uint32_t player_number = 0;
  std::uint32_t kill_timer = 0;

  std::uint32_t super_charge = 0;
  std::uint32_t bomb_count = 0;
  std::uint32_t shield_count = 0;

  bool is_predicted = false;
  fixed speed = 0;  // Used for CSP.

  std::uint64_t score = 0;
  std::uint32_t multiplier = 1;
  std::uint32_t multiplier_count = 0;
  std::uint32_t death_count = 0;

  sfn::ptr<std::optional<render::player_info>(ecs::const_handle, const SimInterface&)> render_info =
      nullptr;

  bool is_killed() const { return kill_timer != 0; }
  void add_score(SimInterface&, std::uint64_t s);
};
DEBUG_STRUCT_TUPLE(Player, player_number, kill_timer, super_charge, bomb_count, shield_count, score,
                   multiplier, multiplier_count, death_count);

}  // namespace ii

#endif
