#ifndef II_GAME_LOGIC_SIM_COMPONENTS_H
#define II_GAME_LOGIC_SIM_COMPONENTS_H
#include "game/common/math.h"
#include "game/common/random.h"
#include "game/common/struct_tuple.h"
#include "game/common/ustring.h"
#include "game/geometry/types.h"
#include "game/logic/ecs/index.h"
#include "game/logic/sim/io/output.h"
#include "game/mixer/sound.h"
#include "game/render/data/background.h"
#include "game/render/data/panel.h"
#include "game/render/data/shapes.h"
#include <sfn/functional.h>
#include <optional>
#include <span>
#include <vector>

namespace ii {
class EmitHandle;
class SimInterface;
enum class boss_flag : std::uint32_t;

enum class damage_type {
  kNormal,
  kPredicted,
  kPenetrating,
  kBomb,
};

enum class rumble_type {
  kNone,
  kLow,
  kSmall,
  kMedium,
  kLarge,
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

  using check_collision_t = geom::hit_result(ecs::const_handle, const geom::check_t&,
                                             const SimInterface&);
  sfn::ptr<check_collision_t> check_collision = nullptr;
};
DEBUG_STRUCT_TUPLE(Collision, flags, bounding_width, check_collision);

struct Update : ecs::component {
  bool skip_update = false;
  using update_t = void(ecs::handle, SimInterface&);
  sfn::ptr<update_t> update;
};
DEBUG_STRUCT_TUPLE(Update, update);

struct PostUpdate : ecs::component {
  using update_t = void(ecs::handle, SimInterface&);
  sfn::ptr<update_t> post_update;
};
DEBUG_STRUCT_TUPLE(PostUpdate, post_update);

struct Render : ecs::component {
  using render_t = void(ecs::const_handle, std::vector<render::shape>&, const SimInterface&);
  using render_panel_t = void(ecs::const_handle, std::vector<render::combo_panel>&,
                              const SimInterface&);
  sfn::ptr<render_t> render = nullptr;
  sfn::ptr<render_panel_t> render_panel = nullptr;
  bool clear_trails = false;

  void render_all(ecs::const_handle, transient_render_state::entity_state& state, bool paused,
                  std::vector<render::shape>&, std::vector<render::combo_panel>&,
                  const SimInterface&);
};
DEBUG_STRUCT_TUPLE(Render, render);

struct PrivateRandom : ecs::component {
  PrivateRandom(std::uint32_t seed) : engine{seed} {}
  PrivateRandom(const PrivateRandom& r) : engine{r.engine.state()} {}
  PrivateRandom& operator=(const PrivateRandom& r) {
    if (this != &r) {
      engine.set_state(r.engine.state());
    }
    return *this;
  }
  RandomEngine engine;
};
DEBUG_STRUCT_TUPLE(PrivateRandom, engine.state());

struct WallTag : ecs::component {};
DEBUG_STRUCT_TUPLE(WallTag);

struct PowerupTag : ecs::component {
  using ai_requires_t = bool(ecs::const_handle, const SimInterface&, ecs::const_handle);
  sfn::ptr<ai_requires_t> ai_requires = nullptr;
};
DEBUG_STRUCT_TUPLE(PowerupTag);

struct Boss : ecs::component {
  boss_flag boss = boss_flag{0};
  ustring name;
  cvec4 colour = colour::kWhite0;
  bool show_hp_bar = false;
};
DEBUG_STRUCT_TUPLE(Boss, boss, show_hp_bar);

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

  std::uint32_t threat_level(std::uint32_t max) const { return (max * max_hp - max * hp) / max_hp; }

  void damage(ecs::handle h, SimInterface&, std::uint32_t damage, damage_type type,
              ecs::entity_id source_id, std::optional<vec2> source_position = std::nullopt);
};
DEBUG_STRUCT_TUPLE(Health, hp, max_hp, damage_transform, on_hit, on_destroy);

struct Player : ecs::component {
  std::uint32_t player_number = 0;
  std::uint32_t death_count = 0;
  bool is_killed = false;
  bool is_clicking = false;
  bool mod_upgrade_chosen = false;

  std::uint32_t super_charge = 0;
  std::uint32_t bomb_count = 0;
  std::uint32_t shield_count = 0;

  bool is_predicted = false;
  fixed speed = 0;  // Used for CSP.

  using render_info_t = std::optional<player_info>(ecs::const_handle, const SimInterface&);
  sfn::ptr<render_info_t> render_info = nullptr;
};
DEBUG_STRUCT_TUPLE(Player, player_number, death_count, is_killed, super_charge, bomb_count,
                   shield_count, is_predicted, speed);

struct Background : ecs::component {
  render::background background;
};
DEBUG_STRUCT_TUPLE(Background);

void add(ecs::handle, const Destroy&);
void add(ecs::handle, const Transform&);
void add(ecs::handle, const Collision&);
void add(ecs::handle, const Update&);
void add(ecs::handle, const PostUpdate&);
void add(ecs::handle, const Render&);
void add(ecs::handle, const PrivateRandom&);
void add(ecs::handle, const WallTag&);
void add(ecs::handle, const PowerupTag&);
void add(ecs::handle, const Boss&);
void add(ecs::handle, const Enemy&);
void add(ecs::handle, const Health&);
void add(ecs::handle, const Player&);
void add(ecs::handle, const Background&);

}  // namespace ii

#endif
