#ifndef II_GAME_LOGIC_SIM_SIM_IO_H
#define II_GAME_LOGIC_SIM_SIM_IO_H
#include "game/common/enum.h"
#include "game/common/math.h"
#include "game/mixer/sound.h"
#include <glm/glm.hpp>
#include <bit>
#include <cstdint>
#include <deque>
#include <functional>
#include <optional>
#include <unordered_map>
#include <vector>

namespace ii {
namespace ecs {
enum class entity_id : std::uint32_t;
}  // namespace ecs
static constexpr std::uint32_t kMaxPlayers = 4;

inline void hash_combine(std::size_t& seed, std::size_t v) {
  seed ^= v + 0x9e3779b9 + (seed << 6) + (seed >> 2);
};

inline glm::vec4 player_colour(std::size_t player_number) {
  return colour_hue360((20 * player_number) % 360);
}

enum class compatibility_level {
  kLegacy,
  kIispaceV0,
};

enum class game_mode : std::uint32_t {
  kNormal,
  kBoss,
  kHard,
  kFast,
  kWhat,
  kMax,
};

enum class boss_flag : std::uint32_t {
  kBoss1A = 1,
  kBoss1B = 2,
  kBoss1C = 4,
  kBoss2A = 8,
  kBoss2B = 16,
  kBoss2C = 32,
  kBoss3A = 64,
};

inline std::uint32_t boss_kill_count(boss_flag flag) {
  return std::popcount(static_cast<std::uint32_t>(flag));
}

template <>
struct bitmask_enum<boss_flag> : std::true_type {};

struct initial_conditions {
  enum class flag : std::uint32_t {
    kNone = 0,
    kLegacy_CanFaceSecretBoss = 0b00000001,
  };
  compatibility_level compatibility = compatibility_level::kIispaceV0;
  std::uint32_t seed = 0;
  std::uint32_t player_count = 0;
  game_mode mode = game_mode::kNormal;
  flag flags = flag::kNone;
};

template <>
struct bitmask_enum<initial_conditions::flag> : std::true_type {};

struct input_frame {
  enum key : std::uint32_t {
    kFire = 1,
    kBomb = 2,
  };
  vec2 velocity{0};
  std::optional<vec2> target_absolute;
  std::optional<vec2> target_relative;
  std::uint32_t keys = 0;
};

enum class resolve {
  // Effects resolved as soon as they occur in the predicted state; ignored in canonical state.
  kPredicted,
  // Effects ignored in predicted state, resolved only when they occur in canonical state.
  kCanonical,
  // Resolved in predicted state when cause is a local player (or empty), and canonical state when
  // source is a remote player.
  kLocal,
  // Resolved in both predicted state and canonical state, with reconciliation to avoid duplication
  // based on resolve_key.
  kReconcile,
};

enum class resolve_tag {
  kUnknown,
  kOnHit,
  kOnDestroy,
  kRespawn,
  kBackgroundFx,
};

template <>
struct integral_enum<resolve_tag> : std::true_type {};

struct resolve_key {
  resolve type = resolve::kPredicted;
  std::optional<std::uint32_t> cause_player_id;
  std::optional<std::uint32_t> source_entity_id;
  resolve_tag reconcile_tag = resolve_tag::kUnknown;
  std::uint32_t reconcile_value = 0;

  std::size_t hash() const {
    std::size_t seed = 0;
    hash_combine(seed, static_cast<std::uint32_t>(type));
    hash_combine(seed, cause_player_id.value_or(0));
    hash_combine(seed, source_entity_id.value_or(0));
    hash_combine(seed, +reconcile_tag);
    hash_combine(seed, reconcile_value);
    return seed;
  }

  bool operator==(const resolve_key&) const = default;
  bool operator!=(const resolve_key&) const = default;

  static resolve_key predicted() {
    resolve_key key;
    key.type = resolve::kPredicted;
    return key;
  }

  static resolve_key canonical() {
    resolve_key key;
    key.type = resolve::kCanonical;
    return key;
  }

  static resolve_key local(std::optional<std::uint32_t> player_id) {
    resolve_key key;
    key.type = resolve::kLocal;
    key.cause_player_id = player_id;
    return key;
  }

  static resolve_key reconcile(std::optional<ecs::entity_id> source_entity_id,
                               resolve_tag reconcile_tag, std::uint32_t reconcile_value = 0) {
    resolve_key key;
    key.type = resolve::kReconcile;
    key.source_entity_id = static_cast<std::uint32_t>(*source_entity_id);
    key.reconcile_tag = reconcile_tag;
    key.reconcile_value = reconcile_value;
    return key;
  }
};

struct particle {
  particle(const glm::vec2& position, const glm::vec4& colour, const glm::vec2& velocity,
           std::uint32_t time)
  : position{position}, colour{colour}, velocity{velocity}, timer{time} {}

  glm::vec2 position{0.f};
  glm::vec4 colour{0.f};
  glm::vec2 velocity{0.f};
  std::uint32_t timer = 0;
};

struct sound_out {
  sound sound_id{0};
  float volume = 0.f;
  float pan = 0.f;
  float pitch = 0.f;
};

enum class background_fx_type {
  kStars,
};

struct background_fx_change {
  background_fx_type type = background_fx_type::kStars;
};

struct aggregate_output {
  void clear() {
    entries.clear();
  }

  void append_to(aggregate_output& output) const {
    output.entries.insert(output.entries.end(), entries.begin(), entries.end());
  }

  struct event {
    std::optional<background_fx_change> background_fx;
    std::vector<particle> particles;
    std::vector<sound_out> sounds;
    std::unordered_map<std::uint32_t, std::uint32_t> rumble_map;
    std::uint32_t global_rumble = 0;
  };

  struct entry {
    resolve_key key;
    event e;
  };
  std::deque<entry> entries;
};

struct render_output {
  struct player_info {
    glm::vec4 colour{0.f};
    std::uint64_t score = 0;
    std::uint32_t multiplier = 0;
    float timer = 0.f;
  };
  struct line_t {
    glm::vec2 a{0.f};
    glm::vec2 b{0.f};
    glm::vec4 c{0.f};
  };
  std::vector<player_info> players;
  std::vector<line_t> lines;
  game_mode mode = game_mode::kNormal;
  std::uint64_t tick_count = 0;
  std::uint32_t lives_remaining = 0;
  std::optional<std::uint32_t> overmind_timer;
  std::optional<float> boss_hp_bar;
  std::uint32_t colour_cycle = 0;
};

struct sim_results {
  game_mode mode = game_mode::kNormal;
  std::uint64_t tick_count = 0;
  std::uint32_t seed = 0;
  std::uint32_t lives_remaining = 0;
  boss_flag bosses_killed{0};

  struct player_result {
    std::uint32_t number = 0;
    std::uint64_t score = 0;
    std::uint32_t deaths = 0;
  };
  std::vector<player_result> players;
  std::uint64_t score = 0;
};

}  // namespace ii

#endif
