#ifndef II_GAME_LOGIC_LEGACY_OVERMIND_SPAWN_CONTEXT_H
#define II_GAME_LOGIC_LEGACY_OVERMIND_SPAWN_CONTEXT_H
#include "game/common/math.h"
#include "game/logic/legacy/enemy/enemy.h"
#include "game/logic/sim/sim_interface.h"
#include <algorithm>
#include <cstdint>

namespace ii::legacy {

enum class spawn_direction {
  kTop,
  kBottom,
  kMirrorV,
};

struct SpawnContext {
  SimInterface* sim = nullptr;
  std::uint32_t row = 0;
  std::uint32_t power = 0;
  std::uint32_t* hard_already = nullptr;

  std::uint32_t random(std::uint32_t max) const {
    return sim->random(random_source::kGameSequence).uint(max);
  }

  std::uint32_t random_bool() const { return sim->random(random_source::kGameSequence).rbool(); }

  spawn_direction random_v_direction() const {
    return random_bool() ? spawn_direction::kTop : spawn_direction::kBottom;
  }

  vec2 spawn_point(spawn_direction d, std::uint32_t i, std::uint32_t n) const {
    n = std::max(2u, n);
    i = std::min(n - 1, i);

    bool top = d == spawn_direction::kTop;
    auto dim = sim->dimensions();
    auto x = fixed{static_cast<std::int32_t>(top ? i : n - 1 - i)} * dim.x /
        fixed{static_cast<std::int32_t>(n - 1)};
    auto y = top ? -((row + 1) * (fixed_c::hundredth * 16) * dim.y)
                 : dim.y * (1 + (row + 1) * (fixed_c::hundredth * 16));
    return vec2{x, y};
  }

  template <typename F>
  void spawn_at(spawn_direction d, std::uint32_t i, std::uint32_t n, const F& f) const {
    if (d == spawn_direction::kMirrorV || d == spawn_direction::kBottom) {
      f(spawn_point(spawn_direction::kBottom, i, n));
    }
    if (d == spawn_direction::kMirrorV || d == spawn_direction::kTop) {
      f(spawn_point(spawn_direction::kTop, i, n));
    }
  }
};

inline void
spawn_follow(const SpawnContext& context, spawn_direction d, std::uint32_t i, std::uint32_t n) {
  context.spawn_at(d, i, n, [&](const auto& v) { legacy::spawn_follow(*context.sim, v); });
}

inline void
spawn_chaser(const SpawnContext& context, spawn_direction d, std::uint32_t i, std::uint32_t n) {
  context.spawn_at(d, i, n, [&](const auto& v) { legacy::spawn_chaser(*context.sim, v); });
}

inline void
spawn_square(const SpawnContext& context, spawn_direction d, std::uint32_t i, std::uint32_t n) {
  context.spawn_at(d, i, n, [&](const auto& v) { legacy::spawn_square(*context.sim, v); });
}

inline void spawn_wall(const SpawnContext& context, spawn_direction d, std::uint32_t i,
                       std::uint32_t n, bool dir) {
  context.spawn_at(d, i, n, [&](const auto& v) { legacy::spawn_wall(*context.sim, v, dir); });
}

inline void
spawn_follow_hub(const SpawnContext& context, spawn_direction d, std::uint32_t i, std::uint32_t n) {
  context.spawn_at(d, i, n, [&](const auto& v) {
    bool p1 = *context.hard_already + context.random(64) <
        std::min(32u, std::max(context.power, 32u) - 32u);
    if (p1) {
      *context.hard_already += 2;
    }
    bool p2 = *context.hard_already + context.random(64) <
        std::min(32u, std::max(context.power, 40u) - 40u);
    if (p2) {
      *context.hard_already += 2;
    }
    legacy::spawn_follow_hub(*context.sim, v, p1, p2);
  });
}

inline void
spawn_shielder(const SpawnContext& context, spawn_direction d, std::uint32_t i, std::uint32_t n) {
  context.spawn_at(d, i, n, [&](const auto& v) {
    bool p = *context.hard_already + context.random(64) <
        std::min(32u, std::max(context.power, 36u) - 36u);
    if (p) {
      *context.hard_already += 3;
    }
    legacy::spawn_shielder(*context.sim, v, p);
  });
}

inline void
spawn_tractor(const SpawnContext& context, spawn_direction d, std::uint32_t i, std::uint32_t n) {
  context.spawn_at(d, i, n, [&](const auto& v) {
    bool p = *context.hard_already + context.random(64) <
        std::min(32u, std::max(context.power, 46u) - 46u);
    if (p) {
      *context.hard_already += 4;
    }
    legacy::spawn_tractor(*context.sim, v, p);
  });
}

}  // namespace ii::legacy

#endif
