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
  auto r = context.random.uint(8);
  if (r == 0) {
    v0::spawn_chaser_hub(context.sim, position, fast);
  } else if (r == 1) {
    v0::spawn_big_follow_hub(context.sim, position, fast);
  } else {
    v0::spawn_follow_hub(context.sim, position, fast);
  }
}

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
