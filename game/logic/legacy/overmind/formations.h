#ifndef II_GAME_LOGIC_LEGACY_OVERMIND_FORMATIONS_H
#define II_GAME_LOGIC_LEGACY_OVERMIND_FORMATIONS_H
#include "game/logic/legacy/overmind/spawn_context.h"
#include <cstdint>

namespace ii::legacy::formations {

template <std::uint32_t C, std::uint32_t R = 0>
struct formation {
  static constexpr std::uint32_t cost = C;
  static constexpr std::uint32_t min_resource = R;
};

struct square1 : formation<4> {
  void operator()(const SpawnContext& context) const {
    for (std::uint32_t i = 1; i < 5; ++i) {
      spawn_square(context, spawn_direction::kMirrorV, i, 6);
    }
  }
};

struct square2 : formation<11> {
  void operator()(const SpawnContext& context) const {
    auto r = context.random(4);
    auto p1 = 2 + context.random(8);
    auto p2 = 2 + context.random(8);
    for (std::uint32_t i = 1; i < 11; ++i) {
      if (r < 2 || i != p1) {
        spawn_square(context, spawn_direction::kBottom, i, 12);
      }
      if (r < 2 || (r == 2 && i != 11 - p1) || (r == 3 && i != p2)) {
        spawn_square(context, spawn_direction::kTop, i, 12);
      }
    }
  }
};

struct square3 : formation<20, 24> {
  void operator()(const SpawnContext& context) const {
    auto r1 = context.random(4);
    auto r2 = context.random(4);
    auto p11 = 2 + context.random(14);
    auto p12 = 2 + context.random(14);
    auto p21 = 2 + context.random(14);
    auto p22 = 2 + context.random(14);

    for (std::uint32_t i = 0; i < 18; ++i) {
      if (r1 < 2 || i != p11) {
        if (r2 < 2 || i != p21) {
          spawn_square(context, spawn_direction::kBottom, i, 18);
        }
      }
      if (r1 < 2 || (r1 == 2 && i != 17 - p11) || (r1 == 3 && i != p12)) {
        if (r2 < 2 || (r2 == 2 && i != 17 - p21) || (r2 == 3 && i != p22)) {
          spawn_square(context, spawn_direction::kTop, i, 18);
        }
      }
    }
  }
};

struct square1side : formation<2> {
  void operator()(const SpawnContext& context) const {
    auto r = context.random(2);
    auto p = context.random(4);

    if (p < 2) {
      auto d = r ? spawn_direction::kTop : spawn_direction::kBottom;
      for (std::uint32_t i = 1; i < 5; ++i) {
        spawn_square(context, d, i, 6);
      }
    } else if (p == 2) {
      for (std::uint32_t i = 1; i < 5; ++i) {
        auto d = (i + r) % 2 ? spawn_direction::kTop : spawn_direction::kBottom;
        spawn_square(context, d, (i + r) % 2 ? 5 - i : i, 6);
      }
    } else {
      for (std::uint32_t i = 1; i < 3; ++i) {
        spawn_square(context, spawn_direction::kMirrorV, r ? 5 - i : i, 6);
      }
    }
  }
};

struct square2side : formation<5> {
  void operator()(const SpawnContext& context) const {
    auto r = context.random(2);
    auto p = context.random(4);

    if (p < 2) {
      auto d = r ? spawn_direction::kTop : spawn_direction::kBottom;
      for (std::uint32_t i = 1; i < 11; ++i) {
        spawn_square(context, d, i, 12);
      }
    } else if (p == 2) {
      for (std::uint32_t i = 1; i < 11; ++i) {
        auto d = (i + r) % 2 ? spawn_direction::kTop : spawn_direction::kBottom;
        spawn_square(context, d, (i + r) % 2 ? 11 - i : i, 12);
      }
    } else {
      for (std::uint32_t i = 1; i < 6; ++i) {
        spawn_square(context, spawn_direction::kMirrorV, r ? 11 - i : i, 12);
      }
    }
  }
};

struct square3side : formation<10, 12> {
  void operator()(const SpawnContext& context) const {
    auto r = context.random(2);
    auto p = context.random(4);
    auto r2 = context.random_bool();
    auto p2 = 1 + context.random(16);

    if (p < 2) {
      auto d = r ? spawn_direction::kTop : spawn_direction::kBottom;
      for (std::uint32_t i = 0; i < 18; ++i) {
        if (!r2 || i != p2) {
          spawn_square(context, d, i, 18);
        }
      }
    } else if (p == 2) {
      for (std::uint32_t i = 0; i < 18; ++i) {
        auto d = (i + r) % 2 ? spawn_direction::kTop : spawn_direction::kBottom;
        spawn_square(context, d, (i + r) % 2 ? 17 - i : i, 18);
      }
    } else {
      for (std::uint32_t i = 0; i < 9; ++i) {
        spawn_square(context, spawn_direction::kMirrorV, r ? 17 - i : i, 18);
      }
    }
  }
};

struct wall1 : formation<5> {
  void operator()(const SpawnContext& context) const {
    bool dir = context.random_bool();
    for (std::uint32_t i = 1; i < 3; ++i) {
      spawn_wall(context, spawn_direction::kMirrorV, i, 4, dir);
    }
  }
};

struct wall2 : formation<12> {
  void operator()(const SpawnContext& context) const {
    bool dir = context.random_bool();
    auto r = context.random(4);
    auto p1 = 2 + context.random(5);
    auto p2 = 2 + context.random(5);
    for (std::uint32_t i = 1; i < 8; ++i) {
      if (r < 2 || i != p1) {
        spawn_wall(context, spawn_direction::kBottom, i, 9, dir);
      }
      if (r < 2 || (r == 2 && i != 8 - p1) || (r == 3 && i != p2)) {
        spawn_wall(context, spawn_direction::kTop, i, 9, dir);
      }
    }
  }
};

struct wall3 : formation<22, 26> {
  void operator()(const SpawnContext& context) const {
    bool dir = context.random_bool();
    auto r1 = context.random(4);
    auto r2 = context.random(4);
    auto p11 = 1 + context.random(10);
    auto p12 = 1 + context.random(10);
    auto p21 = 1 + context.random(10);
    auto p22 = 1 + context.random(10);

    for (std::uint32_t i = 0; i < 12; ++i) {
      if (r1 < 2 || i != p11) {
        if (r2 < 2 || i != p21) {
          spawn_wall(context, spawn_direction::kBottom, i, 12, dir);
        }
      }
      if (r1 < 2 || (r1 == 2 && i != 11 - p11) || (r1 == 3 && i != p12)) {
        if (r2 < 2 || (r2 == 2 && i != 11 - p21) || (r2 == 3 && i != p22)) {
          spawn_wall(context, spawn_direction::kTop, i, 12, dir);
        }
      }
    }
  }
};

struct wall1side : formation<3> {
  void operator()(const SpawnContext& context) const {
    bool dir = context.random_bool();
    auto r = context.random(2);
    auto p = context.random(4);

    if (p < 2) {
      auto d = r ? spawn_direction::kTop : spawn_direction::kBottom;
      for (std::uint32_t i = 1; i < 3; ++i) {
        spawn_wall(context, d, i, 4, dir);
      }
    } else if (p == 2) {
      for (std::uint32_t i = 1; i < 3; ++i) {
        auto d = (i + r) % 2 ? spawn_direction::kTop : spawn_direction::kBottom;
        spawn_wall(context, d, (i + r) % 2 ? 3 - i : i, 4, dir);
      }
    } else {
      spawn_wall(context, spawn_direction::kMirrorV, 1 + r, 4, dir);
    }
  }
};

struct wall2side : formation<6> {
  void operator()(const SpawnContext& context) const {
    bool dir = context.random_bool();
    auto r = context.random(2);
    auto p = context.random(4);

    if (p < 2) {
      auto d = r ? spawn_direction::kTop : spawn_direction::kBottom;
      for (std::uint32_t i = 1; i < 8; ++i) {
        spawn_wall(context, d, i, 9, dir);
      }
    } else if (p == 2) {
      for (std::uint32_t i = 1; i < 8; ++i) {
        auto d = (i + r) % 2 ? spawn_direction::kTop : spawn_direction::kBottom;
        spawn_wall(context, d, i, 9, dir);
      }
    } else {
      for (std::uint32_t i = 0; i < 4; ++i) {
        spawn_wall(context, spawn_direction::kMirrorV, r ? 8 - i : i, 9, dir);
      }
    }
  }
};

struct wall3side : formation<11, 13> {
  void operator()(const SpawnContext& context) const {
    bool dir = context.random_bool();
    auto r = context.random(2);
    auto p = context.random(4);
    auto r2 = context.random_bool();
    auto p2 = 1 + context.random(10);

    if (p < 2) {
      auto d = r ? spawn_direction::kTop : spawn_direction::kBottom;
      for (std::uint32_t i = 0; i < 12; ++i) {
        if (!r2 || i != p2) {
          spawn_wall(context, d, i, 12, dir);
        }
      }
    } else if (p == 2) {
      for (std::uint32_t i = 0; i < 12; ++i) {
        auto d = (i + r) % 2 ? spawn_direction::kTop : spawn_direction::kBottom;
        spawn_wall(context, d, (i + r) % 2 ? 11 - i : i, 12, dir);
      }
    } else {
      for (std::uint32_t i = 0; i < 6; ++i) {
        spawn_wall(context, spawn_direction::kMirrorV, r ? 11 - i : i, 12, dir);
      }
    }
  }
};

struct follow1 : formation<3> {
  void operator()(const SpawnContext& context) const {
    auto p = context.random(3);
    if (p == 0) {
      spawn_follow(context, spawn_direction::kMirrorV, 0, 3);
      spawn_follow(context, spawn_direction::kMirrorV, 2, 3);
    } else if (p == 1) {
      spawn_follow(context, spawn_direction::kMirrorV, 1, 4);
      spawn_follow(context, spawn_direction::kMirrorV, 2, 4);
    } else {
      spawn_follow(context, spawn_direction::kMirrorV, 0, 5);
      spawn_follow(context, spawn_direction::kMirrorV, 4, 5);
    }
  }
};

struct follow2 : formation<7> {
  void operator()(const SpawnContext& context) const {
    if (!context.random_bool()) {
      for (std::uint32_t i = 0; i < 8; ++i) {
        spawn_follow(context, spawn_direction::kMirrorV, i, 8);
      }
    } else {
      for (std::uint32_t i = 0; i < 8; ++i) {
        spawn_follow(context, spawn_direction::kMirrorV, 4 + i, 16);
      }
    }
  }
};

struct follow3 : formation<14> {
  void operator()(const SpawnContext& context) const {
    if (!context.random_bool()) {
      for (std::uint32_t i = 0; i < 16; ++i) {
        spawn_follow(context, spawn_direction::kMirrorV, i, 16);
      }
    } else {
      for (std::uint32_t i = 0; i < 8; ++i) {
        spawn_follow(context, spawn_direction::kMirrorV, i, 28);
        spawn_follow(context, spawn_direction::kMirrorV, 27 - i, 28);
      }
    }
  }
};

struct follow1side : formation<2> {
  void operator()(const SpawnContext& context) const {
    auto d = context.random_v_direction();
    auto p = context.random(3);
    if (p == 0) {
      spawn_follow(context, d, 0, 3);
      spawn_follow(context, d, 2, 3);
    } else if (p == 1) {
      spawn_follow(context, d, 1, 4);
      spawn_follow(context, d, 2, 4);
    } else {
      spawn_follow(context, d, 0, 5);
      spawn_follow(context, d, 4, 5);
    }
  }
};

struct follow2side : formation<3> {
  void operator()(const SpawnContext& context) const {
    auto d = context.random_v_direction();
    if (!context.random_bool()) {
      for (std::uint32_t i = 0; i < 8; ++i) {
        spawn_follow(context, d, i, 8);
      }
    } else {
      for (std::uint32_t i = 0; i < 8; ++i) {
        spawn_follow(context, d, 4 + i, 16);
      }
    }
  }
};

struct follow3side : formation<7> {
  void operator()(const SpawnContext& context) const {
    auto d = context.random_v_direction();
    if (!context.random_bool()) {
      for (std::uint32_t i = 0; i < 16; ++i) {
        spawn_follow(context, d, i, 16);
      }
    } else {
      for (std::uint32_t i = 0; i < 8; ++i) {
        spawn_follow(context, d, i, 28);
        spawn_follow(context, d, 27 - i, 28);
      }
    }
  }
};

struct chaser1 : formation<4> {
  void operator()(const SpawnContext& context) const {
    auto p = context.random(3);
    if (p == 0) {
      spawn_chaser(context, spawn_direction::kMirrorV, 0, 3);
      spawn_chaser(context, spawn_direction::kMirrorV, 2, 3);
    } else if (p == 1) {
      spawn_chaser(context, spawn_direction::kMirrorV, 1, 4);
      spawn_chaser(context, spawn_direction::kMirrorV, 2, 4);
    } else {
      spawn_chaser(context, spawn_direction::kMirrorV, 0, 5);
      spawn_chaser(context, spawn_direction::kMirrorV, 4, 5);
    }
  }
};

struct chaser2 : formation<8> {
  void operator()(const SpawnContext& context) const {
    if (!context.random_bool()) {
      for (std::uint32_t i = 0; i < 8; ++i) {
        spawn_chaser(context, spawn_direction::kMirrorV, i, 8);
      }
    } else {
      for (std::uint32_t i = 0; i < 8; ++i) {
        spawn_chaser(context, spawn_direction::kMirrorV, 4 + i, 16);
      }
    }
  }
};

struct chaser3 : formation<16> {
  void operator()(const SpawnContext& context) const {
    if (!context.random_bool()) {
      for (std::uint32_t i = 0; i < 16; ++i) {
        spawn_chaser(context, spawn_direction::kMirrorV, i, 16);
      }
    } else {
      for (std::uint32_t i = 0; i < 8; ++i) {
        spawn_chaser(context, spawn_direction::kMirrorV, i, 28);
        spawn_chaser(context, spawn_direction::kMirrorV, 27 - i, 28);
      }
    }
  }
};

struct chaser4 : formation<20> {
  void operator()(const SpawnContext& context) const {
    for (std::uint32_t i = 0; i < 22; ++i) {
      spawn_chaser(context, spawn_direction::kMirrorV, i, 22);
    }
  }
};

struct chaser1side : formation<2> {
  void operator()(const SpawnContext& context) const {
    auto d = context.random_v_direction();
    auto p = context.random(3);
    if (p == 0) {
      spawn_chaser(context, d, 0, 3);
      spawn_chaser(context, d, 2, 3);
    } else if (p == 1) {
      spawn_chaser(context, d, 1, 4);
      spawn_chaser(context, d, 2, 4);
    } else {
      spawn_chaser(context, d, 0, 5);
      spawn_chaser(context, d, 4, 5);
    }
  }
};

struct chaser2side : formation<4> {
  void operator()(const SpawnContext& context) const {
    auto d = context.random_v_direction();
    if (!context.random_bool()) {
      for (std::uint32_t i = 0; i < 8; ++i) {
        spawn_chaser(context, d, i, 8);
      }
    } else {
      for (std::uint32_t i = 0; i < 8; ++i) {
        spawn_chaser(context, d, 4 + i, 16);
      }
    }
  }
};

struct chaser3side : formation<8> {
  void operator()(const SpawnContext& context) const {
    auto d = context.random_v_direction();
    if (!context.random_bool()) {
      for (std::uint32_t i = 0; i < 16; ++i) {
        spawn_chaser(context, d, i, 16);
      }
    } else {
      for (std::uint32_t i = 0; i < 8; ++i) {
        spawn_chaser(context, d, i, 28);
        spawn_chaser(context, d, 27 - i, 28);
      }
    }
  }
};

struct chaser4side : formation<10> {
  void operator()(const SpawnContext& context) const {
    auto d = context.random_v_direction();
    for (std::uint32_t i = 0; i < 22; ++i) {
      spawn_chaser(context, d, i, 22);
    }
  }
};

struct hub1 : formation<6> {
  void operator()(const SpawnContext& context) const {
    spawn_follow_hub(context, spawn_direction::kMirrorV, 1 + context.random(3), 5);
  }
};

struct hub2 : formation<12> {
  void operator()(const SpawnContext& context) const {
    auto p = context.random(3);
    spawn_follow_hub(context, spawn_direction::kMirrorV, p == 1 ? 2 : 1, 5);
    spawn_follow_hub(context, spawn_direction::kMirrorV, p == 2 ? 2 : 3, 5);
  }
};

struct hub1side : formation<3> {
  void operator()(const SpawnContext& context) const {
    auto d = context.random_v_direction();
    auto p = 1 + context.random(3);
    spawn_follow_hub(context, d, p, 5);
  }
};

struct hub2side : formation<6> {
  void operator()(const SpawnContext& context) const {
    auto d = context.random_v_direction();
    auto p = context.random(3);
    spawn_follow_hub(context, d, p == 1 ? 2 : 1, 5);
    spawn_follow_hub(context, d, p == 2 ? 2 : 3, 5);
  }
};

struct mixed1 : formation<6> {
  void operator()(const SpawnContext& context) const {
    auto p = context.random_bool();
    spawn_follow(context, spawn_direction::kMirrorV, !p ? 0 : 2, 4);
    spawn_follow(context, spawn_direction::kMirrorV, !p ? 1 : 3, 4);
    spawn_chaser(context, spawn_direction::kMirrorV, !p ? 2 : 0, 4);
    spawn_chaser(context, spawn_direction::kMirrorV, !p ? 3 : 1, 4);
  }
};

struct mixed2 : formation<12> {
  void operator()(const SpawnContext& context) const {
    for (std::uint32_t i = 0; i < 13; ++i) {
      if (i % 2) {
        spawn_follow(context, spawn_direction::kMirrorV, i, 13);
      } else {
        spawn_chaser(context, spawn_direction::kMirrorV, i, 13);
      }
    }
  }
};

struct mixed3 : formation<16> {
  void operator()(const SpawnContext& context) const {
    bool dir = context.random_bool();
    spawn_wall(context, spawn_direction::kMirrorV, 3, 7, dir);
    spawn_follow_hub(context, spawn_direction::kMirrorV, 1, 7);
    spawn_follow_hub(context, spawn_direction::kMirrorV, 5, 7);
    spawn_chaser(context, spawn_direction::kMirrorV, 2, 7);
    spawn_chaser(context, spawn_direction::kMirrorV, 4, 7);
  }
};

struct mixed4 : formation<18> {
  void operator()(const SpawnContext& context) const {
    spawn_square(context, spawn_direction::kMirrorV, 1, 7);
    spawn_square(context, spawn_direction::kMirrorV, 5, 7);
    spawn_follow_hub(context, spawn_direction::kMirrorV, 3, 7);
    spawn_chaser(context, spawn_direction::kMirrorV, 2, 9);
    spawn_chaser(context, spawn_direction::kMirrorV, 3, 9);
    spawn_chaser(context, spawn_direction::kMirrorV, 5, 9);
    spawn_chaser(context, spawn_direction::kMirrorV, 6, 9);
  }
};

struct mixed5 : formation<22, 38> {
  void operator()(const SpawnContext& context) const {
    spawn_follow_hub(context, spawn_direction::kMirrorV, 1, 7);
    auto d = context.random_v_direction();
    spawn_tractor(context, d, 3, 7);
  }
};

struct mixed6 : formation<16, 30> {
  void operator()(const SpawnContext& context) const {
    spawn_follow_hub(context, spawn_direction::kMirrorV, 1, 5);
    spawn_shielder(context, spawn_direction::kMirrorV, 3, 5);
  }
};

struct mixed7 : formation<18, 16> {
  void operator()(const SpawnContext& context) const {
    bool dir = context.random_bool();
    spawn_shielder(context, spawn_direction::kMirrorV, 2, 5);
    spawn_wall(context, spawn_direction::kMirrorV, 1, 10, dir);
    spawn_wall(context, spawn_direction::kMirrorV, 8, 10, dir);
    spawn_chaser(context, spawn_direction::kMirrorV, 2, 10);
    spawn_chaser(context, spawn_direction::kMirrorV, 3, 10);
    spawn_chaser(context, spawn_direction::kMirrorV, 4, 10);
    spawn_chaser(context, spawn_direction::kMirrorV, 5, 10);
  }
};

struct mixed1side : formation<3> {
  void operator()(const SpawnContext& context) const {
    auto d = context.random_v_direction();
    auto p = context.random_bool();
    spawn_follow(context, d, !p ? 0 : 2, 4);
    spawn_follow(context, d, !p ? 1 : 3, 4);
    spawn_chaser(context, d, !p ? 2 : 0, 4);
    spawn_chaser(context, d, !p ? 3 : 1, 4);
  }
};

struct mixed2side : formation<6> {
  void operator()(const SpawnContext& context) const {
    auto r = context.random_bool();
    auto p = context.random_bool();
    auto d0 = r ? spawn_direction::kTop : spawn_direction::kBottom;
    auto d1 = r ? spawn_direction::kBottom : spawn_direction::kTop;
    for (std::uint32_t i = 0; i < 13; ++i) {
      if (i % 2) {
        spawn_follow(context, d0, i, 13);
      } else {
        spawn_chaser(context, p ? d1 : d0, i, 13);
      }
    }
  }
};

struct mixed3side : formation<8> {
  void operator()(const SpawnContext& context) const {
    bool dir = context.random_bool();
    auto d = context.random_v_direction();
    spawn_wall(context, d, 3, 7, dir);
    spawn_follow_hub(context, d, 1, 7);
    spawn_follow_hub(context, d, 5, 7);
    spawn_chaser(context, d, 2, 7);
    spawn_chaser(context, d, 4, 7);
  }
};

struct mixed4side : formation<9> {
  void operator()(const SpawnContext& context) const {
    auto d = context.random_v_direction();
    spawn_square(context, d, 1, 7);
    spawn_square(context, d, 5, 7);
    spawn_follow_hub(context, d, 3, 7);
    spawn_chaser(context, d, 2, 9);
    spawn_chaser(context, d, 3, 9);
    spawn_chaser(context, d, 5, 9);
    spawn_chaser(context, d, 6, 9);
  }
};

struct mixed5side : formation<19, 36> {
  void operator()(const SpawnContext& context) const {
    auto d = context.random_v_direction();
    spawn_follow_hub(context, d, 1, 7);
    spawn_tractor(context, d, 3, 7);
  }
};

struct mixed6side : formation<8, 20> {
  void operator()(const SpawnContext& context) const {
    auto d = context.random_v_direction();
    spawn_follow_hub(context, d, 1, 5);
    spawn_shielder(context, d, 3, 5);
  }
};

struct mixed7side : formation<9, 16> {
  void operator()(const SpawnContext& context) const {
    bool dir = context.random_bool();
    auto d = context.random_v_direction();
    spawn_shielder(context, d, 2, 5);
    spawn_wall(context, d, 1, 10, dir);
    spawn_wall(context, d, 8, 10, dir);
    spawn_chaser(context, d, 2, 10);
    spawn_chaser(context, d, 3, 10);
    spawn_chaser(context, d, 4, 10);
    spawn_chaser(context, d, 5, 10);
  }
};

struct tractor1 : formation<16, 30> {
  void operator()(const SpawnContext& context) const {
    auto d = context.random_v_direction();
    auto p = context.random(3) + 1;
    spawn_tractor(context, d, p, 5);
  }
};

struct tractor2 : formation<28, 46> {
  void operator()(const SpawnContext& context) const {
    spawn_tractor(context, spawn_direction::kTop, context.random(3) + 1, 5);
    spawn_tractor(context, spawn_direction::kBottom, context.random(3) + 1, 5);
  }
};

struct tractor1side : formation<16, 36> {
  void operator()(const SpawnContext& context) const {
    auto d = context.random_v_direction();
    auto p = context.random(7) + 1;
    spawn_tractor(context, d, p, 9);
  }
};

struct tractor2side : formation<14, 32> {
  void operator()(const SpawnContext& context) const {
    auto d = context.random_v_direction();
    auto p = context.random(5) + 1;
    spawn_tractor(context, d, p, 7);
  }
};

struct shielder1 : formation<10, 28> {
  void operator()(const SpawnContext& context) const {
    spawn_shielder(context, spawn_direction::kMirrorV, context.random(3) + 1, 5);
  }
};

struct shielder1side : formation<5, 22> {
  void operator()(const SpawnContext& context) const {
    auto d = context.random_v_direction();
    auto p = context.random(3) + 1;
    spawn_shielder(context, d, p, 5);
  }
};

}  // namespace ii::legacy::formations

#endif
