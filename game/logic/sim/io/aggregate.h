#ifndef II_GAME_LOGIC_SIM_IO_AGGREGATE_H
#define II_GAME_LOGIC_SIM_IO_AGGREGATE_H
#include "game/common/math.h"
#include "game/mixer/sound.h"
#include <glm/glm.hpp>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <variant>

namespace ii {
namespace ecs {
enum class entity_id : std::uint32_t;
}  // namespace ecs

enum class resolve : std::uint32_t {
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

enum class resolve_tag : std::uint32_t {
  kUnknown,
  kOnHit,
  kOnDestroy,
  kRespawn,
  kBackgroundFx,
};

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
    hash_combine(seed, static_cast<std::uint32_t>(reconcile_tag));
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

struct dot_particle {
  float radius = 1.5f;
  float line_width = 1.f;
};

struct line_particle {
  float radius = 0.f;
  float rotation = 0.f;
  float angular_velocity = 0.f;
  float width = 1.f;
};

using particle_data = std::variant<dot_particle, line_particle>;

struct particle {
  glm::vec2 position{0.f};
  glm::vec2 velocity{0.f};
  glm::vec4 colour{0.f};
  particle_data data;
  std::uint32_t time = 0;
  std::uint32_t end_time = 0;
  std::uint32_t flash_time = 0;
  bool fade = false;
};

enum class background_fx_type {
  kStars,
};

struct background_fx_change {
  background_fx_type type = background_fx_type::kStars;
};

}  // namespace ii

#endif
