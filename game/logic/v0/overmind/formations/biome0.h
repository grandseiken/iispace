#ifndef II_GAME_LOGIC_V0_OVERMIND_FORMATIONS_BIOME0_H
#define II_GAME_LOGIC_V0_OVERMIND_FORMATIONS_BIOME0_H
#include "game/logic/v0/enemy/enemy.h"
#include "game/logic/v0/overmind/spawn_context.h"
#include <cstdint>

namespace ii::v0::formations::biome0 {

template <std::uint32_t Cost, std::uint32_t Min = 0>
struct formation {
  static constexpr std::uint32_t power_cost = Cost;
  static constexpr std::uint32_t power_min = Min;
};

inline bool do_upgrade(spawn_context& context, std::uint32_t cost, std::uint32_t min_power,
                       std::uint32_t chance = 2) {
  if (!context.random.uint(chance) && context.data.power >= min_power &&
      context.upgrade_budget >= cost) {
    context.upgrade_budget -= cost;
    return true;
  }
  return false;
}

inline void spawn_square(spawn_context& context, spawn_side side, const vec2& position) {
  v0::spawn_square(context.sim, position, spawn_direction(side));
}

inline void spawn_wall0(spawn_context& context, spawn_side side, const vec2& position) {
  v0::spawn_wall(context.sim, position, spawn_direction(side), false);
}

inline void spawn_wall1(spawn_context& context, spawn_side side, const vec2& position) {
  v0::spawn_wall(context.sim, position, spawn_direction(side), true);
}

inline spawn_function spawn_wall(bool rotate_direction) {
  return rotate_direction ? &spawn_wall0 : spawn_wall1;
}

inline void spawn_follow(spawn_context& context, spawn_side side, const vec2& position) {
  v0::spawn_follow(context.sim, position, spawn_direction(side));
}

inline void spawn_follow_u0(spawn_context& context, spawn_side side, const vec2& position) {
  if (do_upgrade(context, 1, 24)) {
    v0::spawn_big_follow(context.sim, position, spawn_direction(side));
  } else {
    v0::spawn_follow(context.sim, position, spawn_direction(side));
  }
}

inline void spawn_follow_u1(spawn_context& context, spawn_side side, const vec2& position) {
  if (do_upgrade(context, 6, 36)) {
    v0::spawn_huge_follow(context.sim, position, spawn_direction(side));
  } else {
    v0::spawn_follow(context.sim, position, spawn_direction(side));
  }
}

inline void spawn_chaser(spawn_context& context, spawn_side, const vec2& position) {
  v0::spawn_chaser(context.sim, position);
}

inline void spawn_chaser_u(spawn_context& context, spawn_side, const vec2& position) {
  if (do_upgrade(context, 2, 28)) {
    v0::spawn_big_chaser(context.sim, position);
  } else {
    v0::spawn_chaser(context.sim, position);
  }
}

inline void spawn_follow_hub(spawn_context& context, spawn_side, const vec2& position) {
  if (do_upgrade(context, 5, 32)) {
    if (do_upgrade(context, 5, 40)) {
      v0::spawn_chaser_hub(context.sim, position, /* fast */ true);
    } else {
      v0::spawn_chaser_hub(context.sim, position, /* fast */ false);
    }
  } else if (do_upgrade(context, 12, 44, 4)) {
    v0::spawn_big_follow_hub(context.sim, position, /* fast */ false);
  } else if (do_upgrade(context, 4, 28)) {
    v0::spawn_follow_hub(context.sim, position, /* fast */ true);
  } else {
    v0::spawn_follow_hub(context.sim, position, /* fast */ false);
  }
}

inline void spawn_follow_sponge(spawn_context& context, spawn_side, const vec2& position) {
  v0::spawn_follow_sponge(context.sim, position);
}

inline void spawn_shielder(spawn_context& context, spawn_side, const vec2& position) {
  v0::spawn_shielder(context.sim, position);
}

inline void spawn_tractor(spawn_context& context, spawn_side, const vec2& position) {
  v0::spawn_tractor(context.sim, position);
}

inline void spawn_shield_hub(spawn_context& context, spawn_side, const vec2& position) {
  v0::spawn_shield_hub(context.sim, position);
}

struct square0 : formation<4> {
  void operator()(spawn_context& context) const {
    auto side = context.random_mside();
    for (std::uint32_t i = 1; i < 5; ++i) {
      context.spawn(&spawn_square, side, i, 6);
    }
  }
};

struct square1 : formation<11> {
  void operator()(spawn_context& context) const {
    auto r = context.random.uint(4);
    auto p1 = 2 + context.random.uint(8);
    auto p2 = 2 + context.random.uint(8);
    for (std::uint32_t i = 1; i < 11; ++i) {
      if (r < 2 || i != p1) {
        context.spawn(&spawn_square, spawn_side::kBottom, i, 12);
      }
      if (r < 2 || (r == 2 && i != 11 - p1) || (r == 3 && i != p2)) {
        context.spawn(&spawn_square, spawn_side::kTop, i, 12);
      }
    }
  }
};

struct square2 : formation<20, 24> {
  void operator()(spawn_context& context) const {
    auto r1 = context.random.uint(4);
    auto r2 = context.random.uint(4);
    auto p11 = 2 + context.random.uint(14);
    auto p12 = 2 + context.random.uint(14);
    auto p21 = 2 + context.random.uint(14);
    auto p22 = 2 + context.random.uint(14);

    for (std::uint32_t i = 0; i < 18; ++i) {
      if (r1 < 2 || i != p11) {
        if (r2 < 2 || i != p21) {
          context.spawn(&spawn_square, spawn_side::kBottom, i, 18);
        }
      }
      if (r1 < 2 || (r1 == 2 && i != 17 - p11) || (r1 == 3 && i != p12)) {
        if (r2 < 2 || (r2 == 2 && i != 17 - p21) || (r2 == 3 && i != p22)) {
          context.spawn(&spawn_square, spawn_side::kTop, i, 18);
        }
      }
    }
  }
};

struct square0_side : formation<2> {
  void operator()(spawn_context& context) const {
    auto r = context.random.uint(2);
    auto p = context.random.uint(4);

    if (p < 2) {
      auto d = r ? spawn_side::kTop : spawn_side::kBottom;
      for (std::uint32_t i = 1; i < 5; ++i) {
        context.spawn(&spawn_square, d, i, 6);
      }
    } else if (p == 2) {
      for (std::uint32_t i = 1; i < 5; ++i) {
        auto d = (i + r) % 2 ? spawn_side::kTop : spawn_side::kBottom;
        context.spawn(&spawn_square, d, (i + r) % 2 ? 5 - i : i, 6);
      }
    } else {
      auto side = context.random_mside();
      for (std::uint32_t i = 1; i < 3; ++i) {
        context.spawn(&spawn_square, side, r ? 5 - i : i, 6);
      }
    }
  }
};

struct square1_side : formation<5> {
  void operator()(spawn_context& context) const {
    auto r = context.random.uint(2);
    auto p = context.random.uint(4);

    if (p < 2) {
      auto d = r ? spawn_side::kTop : spawn_side::kBottom;
      for (std::uint32_t i = 1; i < 11; ++i) {
        context.spawn(&spawn_square, d, i, 12);
      }
    } else if (p == 2) {
      for (std::uint32_t i = 1; i < 11; ++i) {
        auto d = (i + r) % 2 ? spawn_side::kTop : spawn_side::kBottom;
        context.spawn(&spawn_square, d, (i + r) % 2 ? 11 - i : i, 12);
      }
    } else {
      auto side = context.random_mside();
      for (std::uint32_t i = 1; i < 6; ++i) {
        context.spawn(&spawn_square, side, r ? 11 - i : i, 12);
      }
    }
  }
};

struct square2_side : formation<10, 12> {
  void operator()(spawn_context& context) const {
    auto r = context.random.uint(2);
    auto p = context.random.uint(4);
    auto r2 = context.random.rbool();
    auto p2 = 1 + context.random.uint(16);

    if (p < 2) {
      auto d = r ? spawn_side::kTop : spawn_side::kBottom;
      for (std::uint32_t i = 0; i < 18; ++i) {
        if (!r2 || i != p2) {
          context.spawn(&spawn_square, d, i, 18);
        }
      }
    } else if (p == 2) {
      for (std::uint32_t i = 0; i < 18; ++i) {
        auto d = (i + r) % 2 ? spawn_side::kTop : spawn_side::kBottom;
        context.spawn(&spawn_square, d, (i + r) % 2 ? 17 - i : i, 18);
      }
    } else {
      auto side = context.random_mside();
      for (std::uint32_t i = 0; i < 9; ++i) {
        context.spawn(&spawn_square, side, r ? 17 - i : i, 18);
      }
    }
  }
};

struct square3_side : formation<12, 24> {
  void operator()(spawn_context& context) const {
    auto p = context.random.uint(4);
    if (p < 2) {
      auto side = context.random_dside();
      for (std::uint32_t i = 0; i < 16; ++i) {
        context.spawn(&spawn_square, side, i, 16);
      }
    } else {
      auto side = context.random_dmside();
      for (std::uint32_t i = 0; i < 8; ++i) {
        context.spawn(&spawn_square, side, i + (p == 2 ? 8 : 0), 16);
      }
    }
  }
};

// TODO: sort out horizontal wall alignment (maybe)?
// On wall0, wall0_side, wall1_side, wall2_side.
struct wall0 : formation<5> {
  void operator()(spawn_context& context) const {
    auto side = context.random_mside();
    auto f = spawn_wall(context.random.rbool());
    auto p = context.random.uint(6);
    if (p == 0 || p == 1) {
      context.spawn(f, side, 2_fx / 10);
      context.spawn(f, side, 4_fx / 10);
    } else if (p == 2 || p == 3) {
      context.spawn(f, side, 2_fx / 10);
      context.spawn(f, side, 6_fx / 10);
    } else if (p == 4) {
      context.spawn(f, side, 4_fx / 10);
      context.spawn(f, side, 6_fx / 10);
    } else if (p == 5) {
      context.spawn(f, side, 2_fx / 10);
      context.spawn(f, side, 8_fx / 10);
    }
  }
};

struct wall1 : formation<12> {
  void operator()(spawn_context& context) const {
    auto d = context.random.uint(3);
    auto r = context.random.uint(4);
    auto p1 = 2 + context.random.uint(6);
    auto p2 = 2 + context.random.uint(6);
    for (std::uint32_t i = 1; i < 9; ++i) {
      auto f = spawn_wall(!d || (d == 1 && i % 2));
      if (r < 2 || i != p1) {
        context.spawn(f, spawn_side::kBottom, i, 10);
      }
      if (r < 2 || (r == 2 && i != 8 - p1) || (r == 3 && i != p2)) {
        context.spawn(f, spawn_side::kTop, i, 10);
      }
    }
  }
};

struct wall2 : formation<20, 24> {
  void operator()(spawn_context& context) const {
    auto d = context.random.uint(3);
    auto r1 = context.random.uint(4);
    auto r2 = context.random.uint(4);
    auto p11 = 1 + context.random.uint(9);
    auto p12 = 1 + context.random.uint(9);
    auto p21 = 1 + context.random.uint(9);
    auto p22 = 1 + context.random.uint(9);

    for (std::uint32_t i = 0; i < 11; ++i) {
      auto f = spawn_wall(!d || (d == 1 && i % 2));
      if (r1 < 2 || i != p11) {
        if (r2 < 2 || i != p21) {
          context.spawn(f, spawn_side::kBottom, fixed{i} / 10);
        }
      }
      if (r1 < 2 || (r1 == 2 && i != 11 - p11) || (r1 == 3 && i != p12)) {
        if (r2 < 2 || (r2 == 2 && i != 11 - p21) || (r2 == 3 && i != p22)) {
          context.spawn(f, spawn_side::kTop, fixed{i} / 10);
        }
      }
    }
  }
};

struct wall0_side : formation<3> {
  void operator()(spawn_context& context) const {
    auto f = spawn_wall(context.random.rbool());
    if (context.random.rbool()) {
      auto side = context.random_side();
      auto p = context.random.uint(6);
      if (p == 0 || p == 1) {
        context.spawn(f, side, 2_fx / 10);
        context.spawn(f, side, 4_fx / 10);
      } else if (p == 2 || p == 3) {
        context.spawn(f, side, 2_fx / 10);
        context.spawn(f, side, 6_fx / 10);
      } else if (p == 4) {
        context.spawn(f, side, 4_fx / 10);
        context.spawn(f, side, 6_fx / 10);
      } else if (p == 5) {
        context.spawn(f, side, 2_fx / 10);
        context.spawn(f, side, 8_fx / 10);
      }
    } else {
      context.spawn(f, context.random_mside(), 2_fx * (1 + context.random.uint(2)) / 10);
    }
  }
};

struct wall1_side : formation<6> {
  void operator()(spawn_context& context) const {
    auto d = context.random.uint(3);
    auto p = context.random.uint(3);

    if (p == 0) {
      auto side = context.random_vside();
      for (std::uint32_t i = 1; i < 9; ++i) {
        context.spawn(spawn_wall(!d || (d == 1 && i % 2)), side, i, 10);
      }
    } else if (p == 1) {
      auto side = context.random_mside();
      for (std::uint32_t i = 0; i < 4; ++i) {
        context.spawn(spawn_wall(!d || (d == 1 && i % 2)), side, 2 * i, 10);
      }
    } else {
      auto side = context.random_mside();
      for (std::uint32_t i = 0; i < 4; ++i) {
        context.spawn(spawn_wall(!d || (d == 1 && i % 2)), side, i, 10);
      }
    }
  }
};

struct wall2_side : formation<10, 12> {
  void operator()(spawn_context& context) const {
    auto d = context.random.uint(3);
    auto p = context.random.uint(4);

    if (p == 0) {
      auto side = context.random_vside();
      auto r2 = context.random.rbool();
      auto p2 = 1 + context.random.uint(9);
      for (std::uint32_t i = 0; i < 11; ++i) {
        if (!r2 || i != p2) {
          context.spawn(spawn_wall(!d || (d == 1 && i % 2)), side, fixed{i} / 10);
        }
      }
    } else if (p == 1) {
      auto side = context.random_mside();
      for (std::uint32_t i = 0; i < 5; ++i) {
        context.spawn(spawn_wall(!d || (d == 1 && i % 2)), side, 2 * i, 10);
      }
    } else if (p == 2) {
      auto side = context.random_mside();
      for (std::uint32_t i = 0; i < 6; ++i) {
        context.spawn(spawn_wall(!d || (d == 1 && i % 2)), side, i, 10);
      }
    } else {
      auto side = context.random_vside();
      for (std::uint32_t i = 0; i < 11; ++i) {
        auto s = context.random.rbool() ? side : reverse_side(opposite_side(side));
        context.spawn(spawn_wall(!d || (d == 1 && i % 2)), s, fixed{i} / 10);
      }
    }
  }
};

struct wall3_side : formation<10, 22> {
  void operator()(spawn_context& context) const {
    auto p = context.random.uint(4);
    auto d = context.random.uint(2);
    if (p < 2) {
      auto side = context.random_dside();
      for (std::uint32_t i = 1; i < 7; ++i) {
        context.spawn(spawn_wall(d), side, i, 8);
      }
    } else {
      auto side = context.random_dmside();
      for (std::uint32_t i = 1; i < 4; ++i) {
        context.spawn(spawn_wall(d), side, i + (p == 2 ? 3 : 0), 8);
      }
    }
  }
};

struct follow0 : formation<3> {
  void operator()(spawn_context& context) const {
    auto side = context.random_mside();
    auto p = context.random.uint(3);
    if (p == 0) {
      context.spawn(&spawn_follow_u0, side, 0, 3);
      context.spawn(&spawn_follow_u0, side, 2, 3);
    } else if (p == 1) {
      context.spawn(&spawn_follow_u0, side, 1, 4);
      context.spawn(&spawn_follow_u0, side, 2, 4);
    } else {
      context.spawn(&spawn_follow_u0, side, 0, 5);
      context.spawn(&spawn_follow_u0, side, 4, 5);
    }
  }
};

struct follow1 : formation<7> {
  void operator()(spawn_context& context) const {
    auto side = context.random_mside();
    if (context.random.rbool()) {
      for (std::uint32_t i = 0; i < 8; ++i) {
        context.spawn(i == 0 || i == 7 ? &spawn_follow_u0 : &spawn_follow, side, i, 8);
      }
    } else {
      for (std::uint32_t i = 0; i < 8; ++i) {
        context.spawn(i == 3 || i == 4 ? &spawn_follow_u0 : &spawn_follow, side, 4 + i, 16);
      }
    }
  }
};

struct follow2 : formation<12> {
  void operator()(spawn_context& context) const {
    auto side = context.random_mside();
    auto p = context.random.uint(4);
    if (p == 3) {
      for (std::uint32_t i = 0; i < 16; ++i) {
        context.spawn(&spawn_follow, side, i, 16);
      }
    } else if (p == 2) {
      for (std::uint32_t i = 0; i < 8; ++i) {
        context.spawn(&spawn_follow, side, i, 28);
        context.spawn(&spawn_follow, side, 27 - i, 28);
      }
    } else if (p == 1) {
      for (std::uint32_t i = 0; i < 16; ++i) {
        context.spawn(&spawn_follow, side, i, 32);
      }
    } else {
      for (std::uint32_t i = 0; i < 16; ++i) {
        context.spawn(&spawn_follow, side, 16 + i, 32);
      }
    }
  }
};

struct follow3 : formation<18, 24> {
  void operator()(spawn_context& context) const {
    auto side = context.random_dmoside();
    for (std::uint32_t i = 0; i < 24; ++i) {
      context.spawn(i == 0 || i == 23 ? &spawn_follow_u0 : &spawn_follow, side, i, 24);
    }
  }
};

struct follow4 : formation<16, 22> {
  void operator()(spawn_context& context) const {
    auto side = context.random_dmaside();
    for (std::uint32_t i = 0; i < 20; ++i) {
      context.spawn(i == 0 || i == 19 ? &spawn_follow_u0 : &spawn_follow, side, i, 32);
    }
  }
};

struct follow0_side : formation<2> {
  void operator()(spawn_context& context) const {
    auto side = context.random_vside();
    auto p = context.random.uint(3);
    if (p == 0) {
      context.spawn(&spawn_follow, side, 0, 3);
      context.spawn(&spawn_follow_u1, side, 1, 3);
      context.spawn(&spawn_follow, side, 2, 3);
    } else if (p == 1) {
      context.spawn(&spawn_follow, side, 1, 5);
      context.spawn(&spawn_follow_u1, side, 2, 5);
      context.spawn(&spawn_follow, side, 3, 5);
    } else {
      context.spawn(&spawn_follow, side, 0, 7);
      context.spawn(&spawn_follow_u1, side, 3, 7);
      context.spawn(&spawn_follow, side, 6, 7);
    }
  }
};

struct follow1_side : formation<3> {
  void operator()(spawn_context& context) const {
    auto side = context.random_vside();
    if (context.random.rbool()) {
      for (std::uint32_t i = 0; i < 8; ++i) {
        context.spawn(i == 0 || i == 7 ? &spawn_follow_u0 : &spawn_follow, side, i, 8);
      }
    } else {
      for (std::uint32_t i = 0; i < 8; ++i) {
        context.spawn(i == 3 || i == 4 ? &spawn_follow_u0 : &spawn_follow, side, 4 + i, 16);
      }
    }
  }
};

struct follow2_side : formation<6> {
  void operator()(spawn_context& context) const {
    auto side = context.random_vside();
    if (context.random.rbool()) {
      for (std::uint32_t i = 0; i < 16; ++i) {
        context.spawn(&spawn_follow, side, i, 16);
      }
    } else {
      for (std::uint32_t i = 0; i < 8; ++i) {
        context.spawn(&spawn_follow, side, i, 28);
        context.spawn(&spawn_follow, side, 27 - i, 28);
      }
    }
  }
};

struct follow3_side : formation<16, 22> {
  void operator()(spawn_context& context) const {
    auto side = context.random_dside();
    for (std::uint32_t i = 0; i < 40; ++i) {
      context.spawn(i == 0 || i == 39 ? &spawn_follow_u0 : &spawn_follow, side, i, 40);
    }
  }
};

struct chaser0 : formation<4> {
  void operator()(spawn_context& context) const {
    auto side = context.random_mside();
    auto p = context.random.uint(3);
    if (p == 0) {
      context.spawn(&spawn_chaser_u, side, 0, 3);
      context.spawn(&spawn_chaser_u, side, 2, 3);
    } else if (p == 1) {
      context.spawn(&spawn_chaser_u, side, 1, 4);
      context.spawn(&spawn_chaser_u, side, 2, 4);
    } else {
      context.spawn(&spawn_chaser_u, side, 0, 5);
      context.spawn(&spawn_chaser_u, side, 4, 5);
    }
  }
};

struct chaser1 : formation<8> {
  void operator()(spawn_context& context) const {
    auto side = context.random_mside();
    if (context.random.rbool()) {
      for (std::uint32_t i = 0; i < 8; ++i) {
        context.spawn(i == 3 || i == 4 ? &spawn_chaser_u : &spawn_chaser, side, i, 8);
      }
    } else {
      for (std::uint32_t i = 0; i < 8; ++i) {
        context.spawn(i == 3 || i == 4 ? &spawn_chaser_u : &spawn_chaser, side, 4 + i, 16);
      }
    }
  }
};

struct chaser2 : formation<16> {
  void operator()(spawn_context& context) const {
    auto side = context.random_mside();
    if (!context.random.rbool()) {
      for (std::uint32_t i = 0; i < 16; ++i) {
        context.spawn(i == 0 || i == 15 || i == 7 || i == 8 ? &spawn_chaser_u : &spawn_chaser, side,
                      i, 16);
      }
    } else {
      for (std::uint32_t i = 0; i < 8; ++i) {
        context.spawn(i == 0 || i == 7 ? &spawn_chaser_u : &spawn_chaser, side, i, 28);
        context.spawn(i == 0 || i == 7 ? &spawn_chaser_u : &spawn_chaser, side, 27 - i, 28);
      }
    }
  }
};

struct chaser3 : formation<20> {
  void operator()(spawn_context& context) const {
    auto side = context.random_mside();
    for (std::uint32_t i = 0; i < 22; ++i) {
      context.spawn(&spawn_chaser, side, i, 22);
    }
  }
};

struct chaser0_side : formation<2> {
  void operator()(spawn_context& context) const {
    auto side = context.random_vside();
    auto p = context.random.uint(3);
    if (p == 0) {
      context.spawn(&spawn_chaser_u, side, 0, 3);
      context.spawn(&spawn_chaser_u, side, 2, 3);
    } else if (p == 1) {
      context.spawn(&spawn_chaser_u, side, 1, 4);
      context.spawn(&spawn_chaser_u, side, 2, 4);
    } else {
      context.spawn(&spawn_chaser_u, side, 0, 5);
      context.spawn(&spawn_chaser_u, side, 4, 5);
    }
  }
};

struct chaser1_side : formation<4> {
  void operator()(spawn_context& context) const {
    auto side = context.random_vside();
    if (context.random.rbool()) {
      for (std::uint32_t i = 0; i < 8; ++i) {
        context.spawn(i == 3 || i == 4 ? &spawn_chaser_u : &spawn_chaser, side, i, 8);
      }
    } else {
      for (std::uint32_t i = 0; i < 8; ++i) {
        context.spawn(i == 3 || i == 4 ? &spawn_chaser_u : &spawn_chaser, side, 4 + i, 16);
      }
    }
  }
};

struct chaser2_side : formation<8> {
  void operator()(spawn_context& context) const {
    auto side = context.random_vside();
    if (!context.random.rbool()) {
      for (std::uint32_t i = 0; i < 16; ++i) {
        context.spawn(i == 0 || i == 15 || i == 7 || i == 8 ? &spawn_chaser_u : &spawn_chaser, side,
                      i, 16);
      }
    } else {
      for (std::uint32_t i = 0; i < 8; ++i) {
        context.spawn(i == 0 || i == 7 ? &spawn_chaser_u : &spawn_chaser, side, i, 28);
        context.spawn(i == 0 || i == 7 ? &spawn_chaser_u : &spawn_chaser, side, 27 - i, 28);
      }
    }
  }
};

struct chaser3_side : formation<10> {
  void operator()(spawn_context& context) const {
    auto side = context.random_vside();
    for (std::uint32_t i = 0; i < 22; ++i) {
      context.spawn(&spawn_chaser, side, i, 22);
    }
  }
};

struct hub0 : formation<6> {
  void operator()(spawn_context& context) const {
    auto side = context.random_mside();
    context.spawn(&spawn_follow_hub, side, 1 + context.random.uint(3), 5);
  }
};

struct hub1 : formation<12> {
  void operator()(spawn_context& context) const {
    auto side = context.random_mside();
    auto p = context.random.uint(3);
    context.spawn(&spawn_follow_hub, side, p == 1 ? 2 : 1, 5);
    context.spawn(&spawn_follow_hub, side, p == 2 ? 2 : 3, 5);
  }
};

struct hub2 : formation<9, 26> {
  void operator()(spawn_context& context) const {
    auto side0 = context.random_side();
    auto side1 = opposite_side(side0);
    auto p = context.random.uint(4);
    context.spawn(&spawn_follow_hub, p == 0 ? side1 : side0, 1, 5);
    context.spawn(&spawn_follow_hub, p == 1 ? side1 : side0, 2, 5);
    context.spawn(&spawn_follow_hub, p == 2 ? side1 : side0, 3, 5);
  }
};

struct hub0_side : formation<3> {
  void operator()(spawn_context& context) const {
    auto side = context.random_vside();
    auto p = 1 + context.random.uint(3);
    context.spawn(&spawn_follow_hub, side, p, 5);
  }
};

struct hub1_side : formation<6> {
  void operator()(spawn_context& context) const {
    auto side = context.random_vside();
    auto p = context.random.uint(3);
    context.spawn(&spawn_follow_hub, side, p == 1 ? 2 : 1, 5);
    context.spawn(&spawn_follow_hub, side, p == 2 ? 2 : 3, 5);
  }
};

struct shielder0 : formation<12, 24> {
  void operator()(spawn_context& context) const {
    auto side = context.random_mside();
    context.spawn(&spawn_shielder, side, context.random.uint(3) + 1, 5);
  }
};

struct shielder0_side : formation<6, 22> {
  void operator()(spawn_context& context) const {
    auto side = context.random_side();
    context.spawn(&spawn_shielder, side, context.random.uint(3) + 1, 5);
  }
};

struct sponge0 : formation<8, 26> {
  void operator()(spawn_context& context) const {
    auto side = context.random_mside();
    context.spawn(&spawn_follow_sponge, side, context.random.uint(7) + 1, 9);
  }
};

struct sponge0_side : formation<4, 22> {
  void operator()(spawn_context& context) const {
    auto side = context.random_side();
    context.spawn(&spawn_follow_sponge, side, context.random.uint(7) + 1, 9);
  }
};

struct tractor0 : formation<12, 28> {
  void operator()(spawn_context& context) const {
    auto side = context.random_vside();
    auto p = context.random.uint(5) + 1;
    context.spawn(&spawn_tractor, side, p, 7);
  }
};

struct tractor1 : formation<26, 40> {
  void operator()(spawn_context& context) const {
    context.spawn(&spawn_tractor, spawn_side::kTop, context.random.uint(5) + 1, 7);
    context.spawn(&spawn_tractor, spawn_side::kBottom, context.random.uint(5) + 1, 7);
  }
};

struct tractor0_side : formation<12, 34> {
  void operator()(spawn_context& context) const {
    auto side = context.random_vside();
    auto p = context.random.uint(9) + 1;
    context.spawn(&spawn_tractor, side, p, 11);
  }
};

struct tractor1_side : formation<12, 32> {
  void operator()(spawn_context& context) const {
    auto side = context.random_vside();
    auto p = context.random.uint(7) + 1;
    context.spawn(&spawn_tractor, side, p, 9);
  }
};

struct shield_hub0_side : formation<7, 30> {
  void operator()(spawn_context& context) const {
    auto side = context.random_side();
    auto p = context.random.uint(7) + 1;
    context.spawn(&spawn_shield_hub, side, p, 9);
  }
};

struct mixed0 : formation<6> {
  void operator()(spawn_context& context) const {
    auto side = context.random_mside();
    auto p = context.random.rbool();
    context.spawn(&spawn_follow_u0, side, !p ? 0 : 2, 4);
    context.spawn(&spawn_follow_u0, side, !p ? 1 : 3, 4);
    context.spawn(&spawn_chaser_u, side, !p ? 2 : 0, 4);
    context.spawn(&spawn_chaser_u, side, !p ? 3 : 1, 4);
  }
};

struct mixed1 : formation<12> {
  void operator()(spawn_context& context) const {
    auto side = context.random_mside();
    for (std::uint32_t i = 0; i < 13; ++i) {
      if (i % 2) {
        context.spawn(&spawn_follow, side, i, 13);
      } else {
        context.spawn(&spawn_chaser, side, i, 13);
      }
    }
  }
};

struct mixed2 : formation<16> {
  void operator()(spawn_context& context) const {
    auto side = context.random_mside();
    context.spawn(spawn_wall(context.random.rbool()), side, 3, 7);
    context.spawn(&spawn_follow_hub, side, 1, 7);
    context.spawn(&spawn_follow_hub, side, 5, 7);
    context.spawn(&spawn_chaser, side, 2, 7);
    context.spawn(&spawn_chaser, side, 4, 7);
  }
};

struct mixed3 : formation<18> {
  void operator()(spawn_context& context) const {
    auto side = context.random_mside();
    context.spawn(&spawn_square, side, 1, 7);
    context.spawn(&spawn_square, side, 5, 7);
    context.spawn(&spawn_follow_hub, side, 3, 7);
    context.spawn(&spawn_chaser, side, 2, 9);
    context.spawn(&spawn_chaser, side, 3, 9);
    context.spawn(&spawn_chaser, side, 5, 9);
    context.spawn(&spawn_chaser, side, 6, 9);
  }
};

struct mixed4 : formation<20, 38> {
  void operator()(spawn_context& context) const {
    context.spawn(&spawn_follow_hub, context.random_mside(), 1, 7);
    context.spawn(&spawn_tractor, context.random_vside(), 3, 7);
  }
};

struct mixed5 : formation<22, 36> {
  void operator()(spawn_context& context) const {
    auto side = context.random_mside();
    context.spawn(&spawn_follow_hub, side, 1, 5);
    context.spawn(&spawn_shielder, side, 3, 5);
  }
};

struct mixed6 : formation<20, 26> {
  void operator()(spawn_context& context) const {
    auto side = context.random_mside();
    bool d = context.random.rbool();
    context.spawn(&spawn_shielder, side, 2, 5);
    context.spawn(spawn_wall(d), side, 1, 10);
    context.spawn(spawn_wall(d), side, 8, 10);
    context.spawn(&spawn_chaser, side, 2, 10);
    context.spawn(&spawn_chaser_u, side, 3, 10);
    context.spawn(&spawn_chaser_u, side, 4, 10);
    context.spawn(&spawn_chaser, side, 5, 10);
  }
};

struct mixed7 : formation<20, 38> {
  void operator()(spawn_context& context) const {
    auto side0 = context.random_vside();
    auto side1 = opposite_side(side0);
    auto side2 = context.random.rbool() ? side0 : side1;
    context.spawn(&spawn_follow_hub, side0, 1, 7);
    context.spawn(&spawn_follow_hub, side1, 2, 7);
    context.spawn(&spawn_shield_hub, side2, 3, 7);
    context.spawn(&spawn_follow_hub, side1, 4, 7);
    context.spawn(&spawn_follow_hub, side0, 5, 7);
  }
};

struct mixed8 : formation<20, 30> {
  void operator()(spawn_context& context) const {
    auto side0 = context.random_vside();
    auto side1 = opposite_side(side0);
    auto d = context.random.uint(3);

    for (std::uint32_t i = 0; i < 11; ++i) {
      context.spawn(spawn_wall(!d || (d == 1 && i % 2)), side0, fixed{i} / 10);
      context.spawn(&spawn_square, side1, fixed{i} / 10);
    }
  }
};

struct mixed0_side : formation<3> {
  void operator()(spawn_context& context) const {
    auto side = context.random_side();
    auto p = context.random.rbool();
    context.spawn(&spawn_follow_u1, side, !p ? 0 : 2, 4);
    context.spawn(&spawn_follow_u1, side, !p ? 1 : 3, 4);
    context.spawn(&spawn_chaser_u, side, !p ? 2 : 0, 4);
    context.spawn(&spawn_chaser_u, side, !p ? 3 : 1, 4);
  }
};

struct mixed1_side : formation<6> {
  void operator()(spawn_context& context) const {
    auto r = context.random.rbool();
    auto p = context.random.rbool();
    auto d0 = r ? spawn_side::kTop : spawn_side::kBottom;
    auto d1 = r ? spawn_side::kBottom : spawn_side::kTop;
    for (std::uint32_t i = 0; i < 13; ++i) {
      if (i % 2) {
        context.spawn(&spawn_follow, d0, i, 13);
      } else {
        context.spawn(&spawn_chaser, p ? d1 : d0, i, 13);
      }
    }
  }
};

struct mixed2_side : formation<8> {
  void operator()(spawn_context& context) const {
    auto side = context.random_side();
    bool d = context.random.rbool();
    context.spawn(spawn_wall(d), side, 3, 7);
    context.spawn(&spawn_follow_hub, side, 1, 7);
    context.spawn(&spawn_follow_hub, side, 5, 7);
    context.spawn(&spawn_chaser, side, 2, 7);
    context.spawn(&spawn_chaser, side, 4, 7);
  }
};

struct mixed3_side : formation<9> {
  void operator()(spawn_context& context) const {
    auto side = context.random_side();
    context.spawn(&spawn_square, side, 1, 7);
    context.spawn(&spawn_square, side, 5, 7);
    context.spawn(&spawn_follow_hub, side, 3, 7);
    context.spawn(&spawn_chaser, side, 2, 9);
    context.spawn(&spawn_chaser, side, 3, 9);
    context.spawn(&spawn_chaser, side, 5, 9);
    context.spawn(&spawn_chaser, side, 6, 9);
  }
};

struct mixed4_side : formation<16, 36> {
  void operator()(spawn_context& context) const {
    auto side = context.random_vside();
    context.spawn(&spawn_follow_hub, side, 1, 7);
    context.spawn(&spawn_tractor, side, 3, 7);
  }
};

struct mixed5_side : formation<10, 22> {
  void operator()(spawn_context& context) const {
    auto side = context.random_side();
    context.spawn(&spawn_follow_hub, side, 1, 5);
    context.spawn(&spawn_shielder, side, 3, 5);
  }
};

struct mixed6_side : formation<10, 18> {
  void operator()(spawn_context& context) const {
    auto side = context.random_side();
    bool d = context.random.rbool();
    context.spawn(&spawn_shielder, side, 2, 5);
    context.spawn(spawn_wall(d), side, 1, 10);
    context.spawn(spawn_wall(d), side, 8, 10);
    context.spawn(&spawn_chaser, side, 2, 10);
    context.spawn(&spawn_chaser_u, side, 3, 10);
    context.spawn(&spawn_chaser_u, side, 4, 10);
    context.spawn(&spawn_chaser, side, 5, 10);
  }
};

struct mixed7_side : formation<14, 34> {
  void operator()(spawn_context& context) const {
    auto side = context.random_side();
    context.spawn(&spawn_follow_hub, side, 1, 5);
    context.spawn(&spawn_shield_hub, side, 2, 5);
    context.spawn(&spawn_follow_hub, side, 3, 5);
  }
};

struct mixed8_side : formation<9, 22> {
  void operator()(spawn_context& context) const {
    auto d = context.random.uint(3);
    auto b = context.random.rbool();

    auto side0 = context.random_vside();
    auto side1 = context.random.rbool() ? reverse_side(opposite_side(side0)) : side0;
    auto n = 1u + context.random.uint(1);
    for (std::uint32_t i = 0; i < 11; ++i) {
      if (i >= 5 - n && i <= 5 + n) {
        context.spawn(&spawn_square, side0, fixed{i} / 10);
      } else {
        context.spawn(spawn_wall(!d || (d == 1 && i % 2)), side1, fixed{i} / 10);
      }
    }
  }
};

struct mixed9_side : formation<15, 36> {
  void operator()(spawn_context& context) const {
    auto d = context.random.uint(3);
    auto p = context.random.uint(3);

    auto side = context.random_vside();
    for (std::uint32_t i = 0; i < 11; ++i) {
      if (i == 5) {
        context.spawn(&spawn_shield_hub, side, fixed{i} / 10);
      } else {
        auto s = p == 2 || p == i % 2 ? side : opposite_side(reverse_side(side));
        context.spawn(spawn_wall(!d || (d == 1 && i % 2)), s, fixed{i} / 10);
      }
    }
  }
};

struct mixed10_side : formation<10, 32> {
  void operator()(spawn_context& context) const {
    auto side = context.random_side();
    context.spawn(&spawn_follow_hub, side, 1, 7);
    context.spawn(&spawn_follow_sponge, side, 3, 7);
    context.spawn(&spawn_follow_hub, side, 5, 7);
  }
};

}  // namespace ii::v0::formations::biome0

#endif
