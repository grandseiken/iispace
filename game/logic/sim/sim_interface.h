#ifndef II_GAME_LOGIC_SIM_SIM_INTERFACE_H
#define II_GAME_LOGIC_SIM_SIM_INTERFACE_H
#include "game/common/math.h"
#include "game/logic/ecs/index.h"
#include "game/logic/sim/random_engine.h"
#include "game/logic/sim/sim_io.h"
#include "game/mixer/sound.h"
#include <cstdint>
#include <vector>

class Player;

namespace ii {
namespace ecs {
class EntityIndex;
}  // namespace ecs
enum class shape_flag : std::uint32_t;
struct SimInternals;
class Stars;

// TODO: make this dynamic so we can change it for non-legacy mode.
constexpr glm::ivec2 kSimDimensions = {640, 480};

// TODO: to reduce prediction errors, allow each entity to have private sources of
// (particularly game-state) randomness that will be less likely to diverge.
enum class random_source {
  // For randomness which affects the game state and must be reproduced exactly for replay
  // compatibility.
  kGameState,
  // For randomness which does not affect the game state, and can therefore be used differently
  // between versions without breaking replays.
  // TODO: may be advantageous to have multiple of these in order to minimize divergence
  // in networked rewind/reapply.
  kAesthetic,
  // As above, but must actually use game-state randomness in legacy compatibility mode.
  kLegacyAesthetic,
};

class SimInterface {
public:
  SimInterface(SimInternals* internals) : internals_{internals} {}

  // State manipulation.
  const initial_conditions& conditions() const;
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
  vec2 rotate_compatibility(const vec2& v, fixed theta) const {
    return conditions().compatibility == compatibility_level::kLegacy ? rotate_legacy(v, theta)
                                                                      : rotate(v, theta);
  }

  std::uint32_t get_lives() const;
  std::uint32_t player_count() const;
  std::uint32_t alive_players() const;
  std::uint32_t killed_players() const;
  vec2 nearest_player_position(const vec2& point) const;
  vec2 nearest_player_direction(const vec2& point) const;
  ecs::const_handle nearest_player(const vec2& point) const;
  ecs::handle nearest_player(const vec2& point);

  // Simulation output (particle / effects stuff; rendering).
  class EmitHandle {
  public:
    EmitHandle& add(particle particle);
    EmitHandle& explosion(const glm::vec2& v, const glm::vec4& c, std::uint32_t time = 8,
                          const std::optional<glm::vec2>& towards = std::nullopt);
    EmitHandle& rumble(std::uint32_t player, std::uint32_t time_ticks);
    EmitHandle& rumble_all(std::uint32_t time_ticks);
    EmitHandle& play(sound, float volume = 1.f, float pan = 0.f, float repitch = 0.f);
    EmitHandle& play(sound, const vec2& position, float volume = 1.f, float repitch = 0.f);
    EmitHandle&
    play_random(sound, const vec2& position, float volume = 1.f);  // TODO: random pitch?

  private:
    friend class SimInterface;
    EmitHandle(SimInterface& sim, aggregate_output::event& e) : sim{&sim}, e{&e} {}
    SimInterface* sim = nullptr;
    aggregate_output::event* e = nullptr;
  };

  Stars& stars();
  const Stars& stars() const;
  EmitHandle emit(const resolve_key& key);
  void add_particle(const particle& particle);
  void explosion(const glm::vec2& v, const glm::vec4& c, std::uint32_t time = 8,
                 const std::optional<glm::vec2>& towards = std::nullopt);
  void rumble_all(std::uint32_t time);
  void rumble(std::uint32_t player, std::uint32_t time);
  void play_sound(sound, const vec2& position, bool random = false, float volume = 1.f);
  void play_sound(sound, float volume = 1.f, float pan = 0.f, float repitch = 0.f);
  void render_line(const glm::vec2& a, const glm::vec2& b, const glm::vec4& c) const;
  // TODO: only used by Stars, which should be extracted anyway.
  void render_line_rect(const glm::vec2& lo, const glm::vec2& hi, const glm::vec4& c) const;
  void render_player_info(std::uint32_t player_number, const glm::vec4& colour, std::uint64_t score,
                          std::uint32_t multiplier, float timer) const;

private:
  SimInternals* internals_;
};

}  // namespace ii

#endif
