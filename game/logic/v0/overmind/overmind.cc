#include "game/logic/v0/overmind/overmind.h"
#include "game/common/math.h"
#include "game/logic/ship/components.h"
#include "game/logic/sim/io/conditions.h"
#include "game/logic/sim/sim_interface.h"
#include "game/logic/v0/enemy/enemy.h"

namespace ii::v0 {
namespace {

enum class spawn_side : std::uint32_t {
  kTop = 0,
  kBottom = 1,
  kLeft = 2,
  kRight = 3,
};

spawn_side random_spawn_side(RandomEngine& r) {
  return spawn_side{r.uint(4u)};
}

vec2 spawn_point(const SimInterface& sim, spawn_side side, fixed t, fixed distance) {
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
  }
  return dim / 2_fx;
}

vec2 random_spawn_point(SimInterface& sim, fixed distance) {
  auto& r = sim.random(random_source::kGameSequence);
  return spawn_point(sim, random_spawn_side(r), r.fixed(), distance);
}

struct Overmind : ecs::component {
  std::uint32_t timer = 0;
  void update(ecs::handle h, SimInterface& sim) {
    if (++timer % 2 == 0 && timer % 400 > 200) {
      auto r = sim.random(random_source::kGameSequence).uint(16);
      auto p = random_spawn_point(sim, 200);
      if (!r) {
        spawn_huge_follow(sim, p);
      } else if (r <= 3) {
        spawn_big_follow(sim, p);
      } else {
        spawn_follow(sim, p);
      }
    }
  }
};
DEBUG_STRUCT_TUPLE(Overmind, timer);

}  // namespace

void spawn_overmind(SimInterface& sim) {
  auto h = sim.index().create();
  h.emplace<Overmind>();
  h.emplace<Update>().update = ecs::call<&Overmind::update>;
}

}  // namespace ii::v0