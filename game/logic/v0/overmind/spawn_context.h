#ifndef II_GAME_LOGIC_V0_OVERMIND_SPAWN_CONTEXT_H
#define II_GAME_LOGIC_V0_OVERMIND_SPAWN_CONTEXT_H
#include "game/common/math.h"
#include "game/logic/sim/sim_interface.h"
#include "game/logic/v0/overmind/wave_data.h"
#include <sfn/functional.h>
#include <cstdint>
#include <vector>

namespace ii::v0 {

enum class spawn_side : std::uint32_t {
  kTop = 0,
  kBottom = 1,
  kLeft = 2,
  kRight = 3,
  kReverseTop = 4,
  kReverseBottom = 5,
  kReverseLeft = 6,
  kReverseRight = 7,
  kMirrorV = 8,
  kMirrorH = 9,
  kReverseMirrorV = 10,
  kReverseMirrorH = 11,
  kTopLeft = 12,
  kTopRight = 13,
  kBottomLeft = 14,
  kBottomRight = 15,
  kReverseTopLeft = 16,
  kReverseTopRight = 17,
  kReverseBottomLeft = 18,
  kReverseBottomRight = 19,
  kMirrorD0 = 20,
  kMirrorD1 = 21,
  kReverseMirrorD0 = 22,
  kReverseMirrorD1 = 23,
  kMirrorD2 = 24,
  kMirrorD3 = 25,
  kMirrorD4 = 26,
  kMirrorD5 = 27,
  kReverseMirrorD2 = 28,
  kReverseMirrorD3 = 29,
  kReverseMirrorD4 = 30,
  kReverseMirrorD5 = 31,
};

inline spawn_side opposite_side(spawn_side side) {
  switch (side) {
  case spawn_side::kTop:
    return spawn_side::kBottom;
  case spawn_side::kBottom:
    return spawn_side::kTop;
  case spawn_side::kLeft:
    return spawn_side::kRight;
  case spawn_side::kRight:
    return spawn_side::kLeft;
  case spawn_side::kReverseTop:
    return spawn_side::kReverseBottom;
  case spawn_side::kReverseBottom:
    return spawn_side::kReverseTop;
  case spawn_side::kReverseLeft:
    return spawn_side::kReverseRight;
  case spawn_side::kReverseRight:
    return spawn_side::kReverseLeft;
  case spawn_side::kTopLeft:
    return spawn_side::kBottomRight;
  case spawn_side::kTopRight:
    return spawn_side::kBottomLeft;
  case spawn_side::kBottomLeft:
    return spawn_side::kTopRight;
  case spawn_side::kBottomRight:
    return spawn_side::kTopLeft;
  case spawn_side::kReverseTopLeft:
    return spawn_side::kReverseBottomRight;
  case spawn_side::kReverseTopRight:
    return spawn_side::kReverseBottomLeft;
  case spawn_side::kReverseBottomLeft:
    return spawn_side::kReverseTopRight;
  case spawn_side::kReverseBottomRight:
    return spawn_side::kReverseTopLeft;
  default:
    return side;
  }
}

inline spawn_side reverse_side(spawn_side side) {
  switch (side) {
  case spawn_side::kTop:
    return spawn_side::kReverseTop;
  case spawn_side::kBottom:
    return spawn_side::kReverseBottom;
  case spawn_side::kLeft:
    return spawn_side::kReverseLeft;
  case spawn_side::kRight:
    return spawn_side::kReverseRight;
  case spawn_side::kReverseTop:
    return spawn_side::kTop;
  case spawn_side::kReverseBottom:
    return spawn_side::kBottom;
  case spawn_side::kReverseLeft:
    return spawn_side::kLeft;
  case spawn_side::kReverseRight:
    return spawn_side::kRight;
  case spawn_side::kMirrorV:
    return spawn_side::kReverseMirrorV;
  case spawn_side::kMirrorH:
    return spawn_side::kReverseMirrorH;
  case spawn_side::kReverseMirrorV:
    return spawn_side::kMirrorV;
  case spawn_side::kReverseMirrorH:
    return spawn_side::kMirrorH;
  case spawn_side::kTopLeft:
    return spawn_side::kReverseTopLeft;
  case spawn_side::kTopRight:
    return spawn_side::kReverseTopRight;
  case spawn_side::kBottomLeft:
    return spawn_side::kReverseBottomLeft;
  case spawn_side::kBottomRight:
    return spawn_side::kReverseBottomRight;
  case spawn_side::kReverseTopLeft:
    return spawn_side::kTopLeft;
  case spawn_side::kReverseTopRight:
    return spawn_side::kTopRight;
  case spawn_side::kReverseBottomLeft:
    return spawn_side::kBottomLeft;
  case spawn_side::kReverseBottomRight:
    return spawn_side::kBottomRight;
  case spawn_side::kMirrorD0:
    return spawn_side::kReverseMirrorD0;
  case spawn_side::kMirrorD1:
    return spawn_side::kReverseMirrorD1;
  case spawn_side::kReverseMirrorD0:
    return spawn_side::kMirrorD0;
  case spawn_side::kReverseMirrorD1:
    return spawn_side::kMirrorD1;
  case spawn_side::kMirrorD2:
    return spawn_side::kReverseMirrorD2;
  case spawn_side::kMirrorD3:
    return spawn_side::kReverseMirrorD3;
  case spawn_side::kMirrorD4:
    return spawn_side::kReverseMirrorD4;
  case spawn_side::kMirrorD5:
    return spawn_side::kReverseMirrorD5;
  case spawn_side::kReverseMirrorD2:
    return spawn_side::kMirrorD2;
  case spawn_side::kReverseMirrorD3:
    return spawn_side::kMirrorD3;
  case spawn_side::kReverseMirrorD4:
    return spawn_side::kMirrorD4;
  case spawn_side::kReverseMirrorD5:
    return spawn_side::kMirrorD5;
  }
  return side;
}

inline bool is_vertical(spawn_side side) {
  switch (side) {
  case spawn_side::kTop:
  case spawn_side::kBottom:
  case spawn_side::kReverseTop:
  case spawn_side::kReverseBottom:
  case spawn_side::kMirrorV:
  case spawn_side::kReverseMirrorV:
    return true;
  default:
    return false;
  }
}

inline bool is_horizontal(spawn_side side) {
  switch (side) {
  case spawn_side::kLeft:
  case spawn_side::kRight:
  case spawn_side::kReverseLeft:
  case spawn_side::kReverseRight:
  case spawn_side::kMirrorH:
  case spawn_side::kReverseMirrorH:
    return true;
  default:
    return false;
  }
}

inline bool is_diagonal(spawn_side side) {
  return !is_vertical(side) && !is_horizontal(side);
}

inline vec2 spawn_direction(spawn_side side) {
  switch (side) {
  case spawn_side::kTop:
  case spawn_side::kReverseTop:
    return vec2{0, 1};
  case spawn_side::kBottom:
  case spawn_side::kReverseBottom:
    return vec2{0, -1};
  case spawn_side::kLeft:
  case spawn_side::kReverseLeft:
    return vec2{1, 0};
  case spawn_side::kRight:
  case spawn_side::kReverseRight:
    return vec2{-1, 0};
  case spawn_side::kTopLeft:
  case spawn_side::kReverseTopLeft:
    return vec2{1, 1} / sqrt(2_fx);
  case spawn_side::kTopRight:
  case spawn_side::kReverseTopRight:
    return vec2{-1, 1} / sqrt(2_fx);
  case spawn_side::kBottomLeft:
  case spawn_side::kReverseBottomLeft:
    return vec2{1, -1} / sqrt(2_fx);
  case spawn_side::kBottomRight:
  case spawn_side::kReverseBottomRight:
    return vec2{-1, -1} / sqrt(2_fx);
  default:
    return vec2{0};
  }
}

struct spawn_context;
using spawn_function = sfn::ptr<void(spawn_context&, spawn_side, const vec2&)>;

struct spawned_wave {
  struct entry {
    vec2 position{0};
    spawn_side side = spawn_side{128};
    spawn_function function = nullptr;
  };
  std::vector<entry> entries;
};

struct spawn_context {
  SimInterface& sim;
  RandomEngine& random;

  wave_data data;
  std::uint32_t row_number = 0;
  std::uint32_t upgrade_budget = 0;
  spawned_wave output;

  vec2 spawn_point(spawn_side side, fixed t) const {
    static constexpr fixed kDistancePerRow = 100_fx;
    auto distance = fixed{1 + row_number} * kDistancePerRow;

    auto dim = sim.dimensions();
    auto point = [&](const vec2& a, const vec2& b, const vec2& d) {
      return lerp(a, b, t) + distance * d;
    };
    switch (side) {
    case spawn_side::kTop:
      return point(vec2{0}, vec2{dim.x, 0}, vec2{0, -1});
    case spawn_side::kBottom:
      return point(dim, vec2{0, dim.y}, vec2{0, 1});
    case spawn_side::kLeft:
      return point(vec2{0, dim.y}, vec2{0}, vec2{-1, 0});
    case spawn_side::kRight:
      return point(vec2{dim.x, 0}, dim, vec2{1, 0});
    case spawn_side::kReverseTop:
      return point(vec2{dim.x, 0}, vec2{0}, vec2{0, -1});
    case spawn_side::kReverseBottom:
      return point(vec2{0, dim.y}, dim, vec2{0, 1});
    case spawn_side::kReverseLeft:
      return point(vec2{0}, vec2{0, dim.y}, vec2{-1, 0});
    case spawn_side::kReverseRight:
      return point(dim, vec2{dim.x, 0}, vec2{1, 0});
    case spawn_side::kTopLeft:
      return point(vec2{-dim.y / 2, dim.y / 2}, vec2{dim.x / 2, -dim.x / 2},
                   vec2{-1, -1} / sqrt(2_fx));
    case spawn_side::kTopRight:
      return point(vec2{dim.x / 2, -dim.x / 2}, vec2{dim.x + dim.y / 2, dim.y / 2},
                   vec2{1, -1} / sqrt(2_fx));
    case spawn_side::kBottomLeft:
      return point(vec2{dim.x / 2, dim.y + dim.x / 2}, vec2{-dim.y / 2, dim.y / 2},
                   vec2{-1, 1} / sqrt(2_fx));
    case spawn_side::kBottomRight:
      return point(vec2{dim.x + dim.y / 2, dim.y / 2}, vec2{dim.x / 2, dim.y + dim.x / 2},
                   vec2{1, 1} / sqrt(2_fx));
    case spawn_side::kReverseTopLeft:
      return point(vec2{dim.x / 2, -dim.x / 2}, vec2{-dim.y / 2, dim.y / 2},
                   vec2{-1, -1} / sqrt(2_fx));
    case spawn_side::kReverseTopRight:
      return point(vec2{dim.x + dim.y / 2, dim.y / 2}, vec2{dim.x / 2, -dim.x / 2},
                   vec2{1, -1} / sqrt(2_fx));
    case spawn_side::kReverseBottomLeft:
      return point(vec2{-dim.y / 2, dim.y / 2}, vec2{dim.x / 2, dim.y + dim.x / 2},
                   vec2{-1, 1} / sqrt(2_fx));
    case spawn_side::kReverseBottomRight:
      return point(vec2{dim.x / 2, dim.y + dim.x / 2}, vec2{dim.x + dim.y / 2, dim.y / 2},
                   vec2{1, 1} / sqrt(2_fx));
    default:
      return dim / 2_fx;
    }
  }

  std::vector<spawn_side> resolve_sides(spawn_side side) const {
    switch (side) {
    default:
      return {side};
    case spawn_side::kMirrorV:
      return {spawn_side::kTop, spawn_side::kBottom};
    case spawn_side::kMirrorH:
      return {spawn_side::kLeft, spawn_side::kRight};
    case spawn_side::kReverseMirrorV:
      return {spawn_side::kReverseTop, spawn_side::kReverseBottom};
    case spawn_side::kReverseMirrorH:
      return {spawn_side::kReverseLeft, spawn_side::kReverseRight};
    case spawn_side::kMirrorD0:
      return {spawn_side::kTopLeft, spawn_side::kBottomRight};
    case spawn_side::kMirrorD1:
      return {spawn_side::kBottomLeft, spawn_side::kTopRight};
    case spawn_side::kReverseMirrorD0:
      return {spawn_side::kReverseTopLeft, spawn_side::kReverseBottomRight};
    case spawn_side::kReverseMirrorD1:
      return {spawn_side::kReverseBottomLeft, spawn_side::kReverseTopRight};
    case spawn_side::kMirrorD2:
      return {spawn_side::kTopLeft, spawn_side::kReverseTopRight};
    case spawn_side::kMirrorD3:
      return {spawn_side::kBottomLeft, spawn_side::kReverseTopLeft};
    case spawn_side::kMirrorD4:
      return {spawn_side::kBottomRight, spawn_side::kReverseBottomLeft};
    case spawn_side::kMirrorD5:
      return {spawn_side::kTopRight, spawn_side::kReverseBottomRight};
    case spawn_side::kReverseMirrorD2:
      return {spawn_side::kReverseTopLeft, spawn_side::kTopRight};
    case spawn_side::kReverseMirrorD3:
      return {spawn_side::kReverseBottomLeft, spawn_side::kTopLeft};
    case spawn_side::kReverseMirrorD4:
      return {spawn_side::kReverseBottomRight, spawn_side::kBottomLeft};
    case spawn_side::kReverseMirrorD5:
      return {spawn_side::kReverseTopRight, spawn_side::kBottomRight};
    }
    return {};
  }

  spawn_side random_side() { return spawn_side{random.uint(8u)}; }
  spawn_side random_vside() { return spawn_side{(random.rbool() ? 4u : 0u) + random.uint(2u)}; }
  spawn_side random_hside() {
    return spawn_side{2u + (random.rbool() ? 4u : 0u) + random.uint(2u)};
  }
  spawn_side random_mside() { return spawn_side{8u + random.uint(4u)}; }
  spawn_side random_dside() { return spawn_side{12u + random.uint(8u)}; }
  spawn_side random_dmside() { return spawn_side{20u + random.uint(16u) % 12u}; }
  spawn_side random_dmoside() { return spawn_side{20u + random.uint(4u)}; }
  spawn_side random_dmaside() { return spawn_side{24u + random.uint(8u)}; }

  void spawn(spawn_function f, spawn_side side, fixed t) {
    for (auto& s : resolve_sides(side)) {
      auto& e = output.entries.emplace_back();
      e.side = s;
      e.position = spawn_point(s, t);
      e.function = f;
    }
  }

  void spawn(spawn_function f, spawn_side side, std::uint32_t i, std::uint32_t n) {
    spawn(f, side, fixed{i % n + 1_fx / 2} / fixed{n});
  }
};

}  // namespace ii::v0

#endif
