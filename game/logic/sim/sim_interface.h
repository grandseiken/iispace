#ifndef II_GAME_LOGIC_SIM_SIM_INTERFACE_H
#define II_GAME_LOGIC_SIM_SIM_INTERFACE_H
#include "game/common/math.h"
#include "game/common/random.h"
#include "game/logic/ecs/index.h"
#include "game/logic/sim/io/aggregate.h"
#include "game/mixer/sound.h"
#include <cstdint>
#include <vector>

class Player;

namespace ii {
namespace ecs {
class EntityIndex;
}  // namespace ecs
enum class shape_flag : std::uint32_t;
class SimInterface;
struct SimInternals;
struct aggregate_event;
struct initial_conditions;
struct input_frame;
struct run_event;
namespace render {
struct player_info;
struct shape;
}  // namespace render

// TODO: make this dynamic so it can be changed for non-legacy mode.
constexpr glm::ivec2 kSimDimensions = {640, 480};

// TODO: to reduce prediction errors, allow each entity to have private sources of
// (particularly game-state) randomness that will be less likely to diverge.
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
  EmitHandle& background_fx(background_fx_change);
  EmitHandle& add(particle particle);
  EmitHandle& explosion(const glm::vec2& v, const glm::vec4& c, std::uint32_t time = 8,
                        const std::optional<glm::vec2>& towards = std::nullopt);
  EmitHandle& rumble(std::uint32_t player, std::uint32_t time_ticks);
  EmitHandle& rumble_all(std::uint32_t time_ticks);
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
  bool is_legacy() const;
  input_frame& input(std::uint32_t player_number);
  std::uint64_t tick_count() const;

  const ecs::EntityIndex& index() const;
  ecs::EntityIndex& index();
  ecs::const_handle global_entity() const;
  ecs::handle global_entity();

  RandomEngine& random(random_source s = random_source::kGameState);
  std::uint32_t random_state(random_source s = random_source::kGameState) const;
  std::uint32_t random(std::uint32_t max);
  fixed random_fixed();
  bool random_bool();

  struct collision_info {
    ecs::handle h;
    shape_flag hit_mask;
  };
  std::vector<collision_info> collision_list(const vec2& point, shape_flag mask);
  bool any_collision(const vec2& point, shape_flag mask) const;
  bool is_on_screen(const vec2& point) const;
  vec2 rotate_compatibility(const vec2& v, fixed theta) const;

  std::uint32_t get_lives() const;
  std::uint32_t player_count() const;
  std::uint32_t alive_players() const;
  std::uint32_t killed_players() const;
  vec2 nearest_player_position(const vec2& point) const;
  vec2 nearest_player_direction(const vec2& point) const;
  ecs::const_handle nearest_player(const vec2& point) const;
  ecs::handle nearest_player(const vec2& point);

  // Simulation output (particle / effects stuff; rendering).
  EmitHandle emit(const resolve_key& key);
  void trigger(const run_event&);
  void render(const render::shape&) const;
  void render(std::uint32_t player_number, const render::player_info&) const;

private:
  SimInternals* internals_;
};

}  // namespace ii

#endif
