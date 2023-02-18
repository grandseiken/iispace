#ifndef II_GAME_LOGIC_SIM_SIM_INTERFACE_H
#define II_GAME_LOGIC_SIM_SIM_INTERFACE_H
#include "game/common/math.h"
#include "game/common/random.h"
#include "game/geom2/types.h"
#include "game/logic/ecs/index.h"
#include "game/logic/sim/io/aggregate.h"
#include "game/mixer/sound.h"
#include <cstdint>
#include <span>
#include <vector>

class Player;

namespace ii {
namespace ecs {
class EntityIndex;
}  // namespace ecs
namespace geom2 {
class ShapeBank;
};
enum class shape_flag : std::uint32_t;
class SimInterface;
struct SimInternals;
struct aggregate_event;
struct initial_conditions;
struct input_frame;
struct run_event;
struct sim_results;
namespace render {
struct player_info;
struct shape;
}  // namespace render

enum class random_source {
  // For randomness which affects the game state and must be reproduced exactly for replay
  // compatibility.
  kGameState,
  // Similar, but for meta-level state, so that running the same seed actually gives the same
  // run, other than in legacy compatibility mode.
  kGameSequence,
  // For randomness which does not affect the game state, and can therefore be used differently
  // between versions without breaking replays.
  // TODO: may be advantageous to have multiple of these in order to minimize divergence
  // in networked rewind/reapply.
  kAesthetic,
  // As above, but must actually use game-state randomness in legacy compatibility mode.
  kLegacyAesthetic,
};

class EmitHandle {
public:
  RandomEngine& random();
  EmitHandle& set_delay_ticks(std::uint32_t ticks);
  EmitHandle& background_fx(background_fx_change);
  EmitHandle& add(particle particle);
  // TODO: remove this.
  EmitHandle& explosion(const fvec2& v, const cvec4& c, std::uint32_t time = 8,
                        const std::optional<fvec2>& towards = std::nullopt,
                        std::optional<float> speed = std::nullopt);
  EmitHandle& rumble(std::uint32_t player, std::uint32_t time_ticks, float lf, float hf);
  EmitHandle& rumble_all(std::uint32_t time_ticks, float lf, float hf);
  EmitHandle& play(sound, float volume = 1.f, float pan = 0.f, float repitch = 0.f);
  EmitHandle& play(sound, const vec2& position, float volume = 1.f, float repitch = 0.f);
  EmitHandle& play_random(sound, const vec2& position, float volume = 1.f);  // TODO: random pitch?

private:
  friend class SimInterface;
  EmitHandle(SimInterface& sim, aggregate_event& e) : sim{&sim}, e{&e} {}
  SimInterface* sim = nullptr;
  aggregate_event* e = nullptr;
};

class SimInterface {
public:
  SimInterface(SimInternals* internals) : internals_{internals} {}

  // State manipulation.
  const initial_conditions& conditions() const;
  vec2 dimensions() const;
  bool is_legacy() const;
  input_frame& input(std::uint32_t player_number);
  std::uint64_t tick_count() const;

  const ecs::EntityIndex& index() const;
  ecs::EntityIndex& index();
  ecs::const_handle global_entity() const;
  ecs::handle global_entity();

  RandomEngine& random(ecs::handle);
  RandomEngine& random(random_source s = random_source::kGameState);
  std::uint32_t random_state(random_source s = random_source::kGameState) const;
  std::uint32_t random(std::uint32_t max);
  fixed random_fixed();
  bool random_bool();

  struct collision_info {
    ecs::handle h;
    shape_flag hit_mask{0};
    std::vector<vec2> shape_centres;
  };
  struct range_info {
    ecs::handle h;
    vec2 d{0};
    fixed distance_sq = 0;
  };

  geom2::ShapeBank& shape_bank() const;
  bool collide_any(const geom2::check_t&) const;
  std::vector<collision_info> collide(const geom2::check_t&) const;
  bool is_on_screen(const vec2& point) const;
  vec2 rotate_compatibility(const vec2& v, fixed theta) const;

  template <ecs::Component C>
  void
  in_range(const vec2& point, fixed distance, std::size_t max_n, std::vector<range_info>& output) {
    in_range(point, distance, ecs::id<C>(), max_n, output);
  }
  void in_range(const vec2& point, fixed distance, ecs::component_id, std::size_t max_n,
                std::vector<range_info>& output);

  std::vector<ecs::const_handle> alive_player_handles() const;
  std::vector<ecs::handle> alive_player_handles();
  std::optional<ecs::handle> random_alive_player(RandomEngine&);

  std::uint32_t get_lives() const;
  std::uint32_t player_count() const;
  std::uint32_t alive_players() const;
  std::uint32_t killed_players() const;
  vec2 nearest_player_position(const vec2& point) const;
  vec2 nearest_player_direction(const vec2& point) const;
  ecs::const_handle nearest_player(const vec2& point) const;
  ecs::handle nearest_player(const vec2& point);

  // Simulation output.
  EmitHandle emit(const resolve_key& key);
  void trigger(const run_event&);
  const sim_results& results() const;

private:
  SimInternals* internals_;
};

}  // namespace ii

#endif
