#ifndef II_GAME_LOGIC_V0_OVERMIND_FORMATIONS_H
#define II_GAME_LOGIC_V0_OVERMIND_FORMATIONS_H
#include "game/logic/v0/enemy/enemy.h"
#include "game/logic/v0/overmind/spawn_context.h"
#include <cstdint>

namespace ii::v0::formations {

template <std::uint32_t Cost, std::uint32_t Min = 0>
struct formation {
  static constexpr std::uint32_t power_cost = Cost;
  static constexpr std::uint32_t power_min = Min;
};

inline void spawn_square(spawn_context& context, spawn_side side, const vec2& position) {
  v0::spawn_square(context.sim, position, spawn_direction(side));
}

inline void spawn_wall0(spawn_context& context, spawn_side side, const vec2& position) {
  v0::spawn_wall(context.sim, position, spawn_direction(side), false);
}

inline void spawn_wall1(spawn_context& context, spawn_side side, const vec2& position) {
  v0::spawn_wall(context.sim, position, spawn_direction(side), true);
}

inline void spawn_follow(spawn_context& context, spawn_side side, const vec2& position) {
  // TODO: work out how this should actually work.
  auto r = context.random.uint(64);
  if (!r) {
    v0::spawn_huge_follow(context.sim, position, spawn_direction(side));
  } else if (r <= 6) {
    v0::spawn_big_follow(context.sim, position, spawn_direction(side));
  } else {
    v0::spawn_follow(context.sim, position, spawn_direction(side));
  }
}

inline void spawn_chaser(spawn_context& context, spawn_side, const vec2& position) {
  if (!context.random.uint(16)) {
    v0::spawn_big_chaser(context.sim, position);
  } else {
    v0::spawn_chaser(context.sim, position);
  }
}

inline void spawn_follow_hub(spawn_context& context, spawn_side, const vec2& position) {
  bool fast = !context.random.uint(8);
  auto r = context.random.uint(16);
  if (r == 0) {
    v0::spawn_big_follow_hub(context.sim, position, fast);
  } else if (r <= 3) {
    v0::spawn_chaser_hub(context.sim, position, fast);
  } else {
    v0::spawn_follow_hub(context.sim, position, fast);
  }
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

struct wall0 : formation<5> {
  void operator()(spawn_context& context) const {
    auto side = context.random_mside();
    auto f = context.random.rbool() ? &spawn_wall0 : &spawn_wall1;
    auto p = context.random.uint(6);
    if (p == 0 || p == 1) {
      context.spawn(f, side, 0, 4);
      context.spawn(f, side, 1, 4);
    } else if (p == 2 || p == 3) {
      context.spawn(f, side, 0, 4);
      context.spawn(f, side, 2, 4);
    } else if (p == 4) {
      context.spawn(f, side, 1, 4);
      context.spawn(f, side, 2, 4);
    } else if (p == 5) {
      context.spawn(f, side, 0, 4);
      context.spawn(f, side, 3, 4);
    }
  }
};

struct wall1 : formation<12> {
  void operator()(spawn_context& context) const {
    auto f = context.random.rbool() ? &spawn_wall0 : &spawn_wall1;
    auto r = context.random.uint(4);
    auto p1 = 2 + context.random.uint(6);
    auto p2 = 2 + context.random.uint(6);
    for (std::uint32_t i = 1; i < 9; ++i) {
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
    // TODO: sometimes alternate rotate dir on this and wall2_side?
    // Also: sometimes stagger row rotate timing?
    auto f = context.random.rbool() ? &spawn_wall0 : &spawn_wall1;
    auto r1 = context.random.uint(4);
    auto r2 = context.random.uint(4);
    auto p11 = 1 + context.random.uint(8);
    auto p12 = 1 + context.random.uint(8);
    auto p21 = 1 + context.random.uint(8);
    auto p22 = 1 + context.random.uint(8);

    for (std::uint32_t i = 0; i < 10; ++i) {
      if (r1 < 2 || i != p11) {
        if (r2 < 2 || i != p21) {
          context.spawn(f, spawn_side::kBottom, i, 10);
        }
      }
      if (r1 < 2 || (r1 == 2 && i != 11 - p11) || (r1 == 3 && i != p12)) {
        if (r2 < 2 || (r2 == 2 && i != 11 - p21) || (r2 == 3 && i != p22)) {
          context.spawn(f, spawn_side::kTop, i, 10);
        }
      }
    }
  }
};

struct wall0_side : formation<3> {
  void operator()(spawn_context& context) const {
    auto f = context.random.rbool() ? &spawn_wall0 : &spawn_wall1;
    if (context.random.rbool()) {
      auto side = context.random_vside();
      auto p = context.random.uint(6);
      if (p == 0 || p == 1) {
        context.spawn(f, side, 0, 4);
        context.spawn(f, side, 1, 4);
      } else if (p == 2 || p == 3) {
        context.spawn(f, side, 0, 4);
        context.spawn(f, side, 2, 4);
      } else if (p == 4) {
        context.spawn(f, side, 1, 4);
        context.spawn(f, side, 2, 4);
      } else if (p == 5) {
        context.spawn(f, side, 0, 4);
        context.spawn(f, side, 3, 4);
      }
    } else {
      context.spawn(f, context.random_mside(), context.random.uint(2), 4);
    }
  }
};

struct wall1_side : formation<6> {
  void operator()(spawn_context& context) const {
    auto f = context.random.rbool() ? &spawn_wall0 : &spawn_wall1;
    auto p = context.random.uint(3);

    // TODO: need better ways of dividing the space to line up walls nicely.
    // 10 walls exactly fills horizontal space.
    // But 5.625 fills vertical.
    if (p == 0) {
      auto side = context.random_vside();
      for (std::uint32_t i = 1; i < 9; ++i) {
        context.spawn(f, side, i, 10);
      }
    } else if (p == 1) {
      auto side = context.random_mside();
      if (is_vertical(side)) {
        for (std::uint32_t i = 0; i < 4; ++i) {
          context.spawn(f, side, 2 * i, 10);
        }
      } else {
        for (std::uint32_t i = 0; i < 4; ++i) {
          context.spawn(f, side, 4 + 12 * i, 45);
        }
      }
    } else {
      auto side = context.random_mside();
      if (is_vertical(side)) {
        for (std::uint32_t i = 0; i < 4; ++i) {
          context.spawn(f, side, i, 10);
        }
      } else {
        for (std::uint32_t i = 0; i < 4; ++i) {
          context.spawn(f, side, 4 + 8 * i, 45);
        }
      }
    }
  }
};

struct wall2_side : formation<10, 12> {
  void operator()(spawn_context& context) const {
    auto f = context.random.rbool() ? &spawn_wall0 : &spawn_wall1;
    auto p = context.random.uint(3);

    if (p == 0) {
      auto side = context.random_vside();
      auto r2 = context.random.rbool();
      auto p2 = 1 + context.random.uint(8);
      for (std::uint32_t i = 0; i < 10; ++i) {
        if (!r2 || i != p2) {
          context.spawn(f, side, i, 10);
        }
      }
    } else if (p == 1) {
      auto side = context.random_mside();
      if (is_vertical(side)) {
        for (std::uint32_t i = 0; i < 5; ++i) {
          context.spawn(f, side, 2 * i, 10);
        }
      } else {
        for (std::uint32_t i = 0; i < 5; ++i) {
          context.spawn(f, side, 2 + 8 * i, 45);
        }
      }
    } else {
      auto side = context.random.rbool() ? spawn_side::kMirrorV : spawn_side::kReverseMirrorV;
      for (std::uint32_t i = 0; i < 5; ++i) {
        context.spawn(f, side, i, 10);
      }
    }
  }
};

struct follow0 : formation<3> {
  void operator()(spawn_context& context) const {
    auto side = context.random_mside();
    auto p = context.random.uint(3);
    if (p == 0) {
      context.spawn(&spawn_follow, side, 0, 3);
      context.spawn(&spawn_follow, side, 2, 3);
    } else if (p == 1) {
      context.spawn(&spawn_follow, side, 1, 4);
      context.spawn(&spawn_follow, side, 2, 4);
    } else {
      context.spawn(&spawn_follow, side, 0, 5);
      context.spawn(&spawn_follow, side, 4, 5);
    }
  }
};

struct follow1 : formation<7> {
  void operator()(spawn_context& context) const {
    auto side = context.random_mside();
    if (context.random.rbool()) {
      for (std::uint32_t i = 0; i < 8; ++i) {
        context.spawn(&spawn_follow, side, i, 8);
      }
    } else {
      for (std::uint32_t i = 0; i < 8; ++i) {
        context.spawn(&spawn_follow, side, 4 + i, 16);
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

struct follow3 : formation<16> {
  void operator()(spawn_context& context) const {
    auto side = context.random_dmoside();
    for (std::uint32_t i = 0; i < 24; ++i) {
      context.spawn(&spawn_follow, side, i, 24);
    }
  }
};

struct follow4 : formation<14> {
  void operator()(spawn_context& context) const {
    auto side = context.random_dmaside();
    for (std::uint32_t i = 0; i < 20; ++i) {
      context.spawn(&spawn_follow, side, i, 32);
    }
  }
};

struct follow0_side : formation<2> {
  void operator()(spawn_context& context) const {
    auto side = context.random_vside();
    auto p = context.random.uint(3);
    if (p == 0) {
      context.spawn(&spawn_follow, side, 0, 3);
      context.spawn(&spawn_follow, side, 1, 3);
      context.spawn(&spawn_follow, side, 2, 3);
    } else if (p == 1) {
      context.spawn(&spawn_follow, side, 1, 5);
      context.spawn(&spawn_follow, side, 2, 5);
      context.spawn(&spawn_follow, side, 3, 5);
    } else {
      context.spawn(&spawn_follow, side, 0, 7);
      context.spawn(&spawn_follow, side, 3, 7);
      context.spawn(&spawn_follow, side, 6, 7);
    }
  }
};

struct follow1_side : formation<3> {
  void operator()(spawn_context& context) const {
    auto side = context.random_vside();
    if (context.random.rbool()) {
      for (std::uint32_t i = 0; i < 8; ++i) {
        context.spawn(&spawn_follow, side, i, 8);
      }
    } else {
      for (std::uint32_t i = 0; i < 8; ++i) {
        context.spawn(&spawn_follow, side, 4 + i, 16);
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

struct follow3_side : formation<16> {
  void operator()(spawn_context& context) const {
    auto side = context.random_dside();
    for (std::uint32_t i = 0; i < 40; ++i) {
      context.spawn(&spawn_follow, side, i, 40);
    }
  }
};

struct chaser0 : formation<4> {
  void operator()(spawn_context& context) const {
    auto side = context.random_mside();
    auto p = context.random.uint(3);
    if (p == 0) {
      context.spawn(&spawn_chaser, side, 0, 3);
      context.spawn(&spawn_chaser, side, 2, 3);
    } else if (p == 1) {
      context.spawn(&spawn_chaser, side, 1, 4);
      context.spawn(&spawn_chaser, side, 2, 4);
    } else {
      context.spawn(&spawn_chaser, side, 0, 5);
      context.spawn(&spawn_chaser, side, 4, 5);
    }
  }
};

struct chaser1 : formation<8> {
  void operator()(spawn_context& context) const {
    auto side = context.random_mside();
    if (context.random.rbool()) {
      for (std::uint32_t i = 0; i < 8; ++i) {
        context.spawn(&spawn_chaser, side, i, 8);
      }
    } else {
      for (std::uint32_t i = 0; i < 8; ++i) {
        context.spawn(&spawn_chaser, side, 4 + i, 16);
      }
    }
  }
};

struct chaser2 : formation<16> {
  void operator()(spawn_context& context) const {
    auto side = context.random_mside();
    if (!context.random.rbool()) {
      for (std::uint32_t i = 0; i < 16; ++i) {
        context.spawn(&spawn_chaser, side, i, 16);
      }
    } else {
      for (std::uint32_t i = 0; i < 8; ++i) {
        context.spawn(&spawn_chaser, side, i, 28);
        context.spawn(&spawn_chaser, side, 27 - i, 28);
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
      context.spawn(&spawn_chaser, side, 0, 3);
      context.spawn(&spawn_chaser, side, 2, 3);
    } else if (p == 1) {
      context.spawn(&spawn_chaser, side, 1, 4);
      context.spawn(&spawn_chaser, side, 2, 4);
    } else {
      context.spawn(&spawn_chaser, side, 0, 5);
      context.spawn(&spawn_chaser, side, 4, 5);
    }
  }
};

struct chaser1_side : formation<4> {
  void operator()(spawn_context& context) const {
    auto side = context.random_vside();
    if (context.random.rbool()) {
      for (std::uint32_t i = 0; i < 8; ++i) {
        context.spawn(&spawn_chaser, side, i, 8);
      }
    } else {
      for (std::uint32_t i = 0; i < 8; ++i) {
        context.spawn(&spawn_chaser, side, 4 + i, 16);
      }
    }
  }
};

struct chaser2_side : formation<8> {
  void operator()(spawn_context& context) const {
    auto side = context.random_vside();
    if (!context.random.rbool()) {
      for (std::uint32_t i = 0; i < 16; ++i) {
        context.spawn(&spawn_chaser, side, i, 16);
      }
    } else {
      for (std::uint32_t i = 0; i < 8; ++i) {
        context.spawn(&spawn_chaser, side, i, 28);
        context.spawn(&spawn_chaser, side, 27 - i, 28);
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

}  // namespace ii::v0::formations

#endif
