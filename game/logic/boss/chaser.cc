#include "game/logic/boss/boss_internal.h"
#include "game/logic/geometry/shapes/ngon.h"
#include "game/logic/ship/ship_template.h"
#include "game/logic/sim/io/events.h"
#include <array>

namespace ii {
namespace {

struct ChaserBossSharedState : ecs::component {
  bool has_counted = false;
  std::uint32_t count = 0;
  std::uint32_t cycle = 0;
};
DEBUG_STRUCT_TUPLE(ChaserBossSharedState, count, cycle);

struct split_lookup_entry {
  fixed hp_reduce_power = 0;
  fixed pow_1_5 = 0;
  fixed pow_1_2 = 0;
  fixed pow_1_15 = 0;
  fixed pow_1_1 = 0;
};

constexpr std::array<split_lookup_entry, 8> kSplitLookup = {
    split_lookup_entry{1_fx, 1_fx, 1_fx, 1_fx, 1_fx},
    {1 + 7 * (1_fx / 10), 1 + (1_fx / 2), 1 + 2 * (1_fx / 10), 1 + 15 * (1_fx / 100),
     1 + (1_fx / 10)},
    {2 + 9 * (1_fx / 10), 2 + (1_fx / 4), 1 + 4 * (1_fx / 10), 1 + 3 * (1_fx / 10),
     1 + 2 * (1_fx / 10)},
    {4 + 9 * (1_fx / 10), 3 + 4 * (1_fx / 10), 1 + 7 * (1_fx / 10), 1 + 5 * (1_fx / 10),
     1 + 3 * (1_fx / 10)},
    {8 + 4 * (1_fx / 10), 5 + (1_fx / 10), 2 + (1_fx / 10), 1 + 7 * (1_fx / 10),
     1 + 5 * (1_fx / 10)},
    {14 + 2 * (1_fx / 10), 7 + 6 * (1_fx / 10), 2 + (1_fx / 2), 2, 1 + 6 * (1_fx / 10)},
    {24 + 2 * (1_fx / 10), 11 + 4 * (1_fx / 10), 3, 2 + 3 * (1_fx / 10), 1 + 8 * (1_fx / 10)},
    {41, 17 + (1_fx / 10), 3 + 6 * (1_fx / 10), 2 + 7 * (1_fx / 10), 1 + 9 * (1_fx / 10)},
};

constexpr std::array<std::uint32_t, 128> kPow1_15Lookup = {
    1,        1,        1,        2,        2,        2,        2,        3,        3,
    4,        4,        5,        5,        6,        7,        8,        9,        11,
    12,       14,       16,       19,       22,       25,       29,       33,       38,
    44,       50,       58,       66,       76,       88,       101,      116,      133,
    153,      176,      203,      233,      268,      308,      354,      407,      468,
    539,      620,      713,      819,      942,      1084,     1246,     1433,     1648,
    1895,     2180,     2507,     2883,     3315,     3812,     4384,     5042,     5798,
    6668,     7668,     8818,     10140,    11662,    13411,    15422,    17736,    20396,
    23455,    26974,    31020,    35673,    41024,    47177,    54254,    62392,    71751,
    82513,    94890,    109124,   125493,   144316,   165964,   190858,   219487,   252410,
    290272,   333813,   383884,   441467,   507687,   583840,   671416,   772129,   887948,
    1021140,  1174311,  1350458,  1553026,  1785980,  2053877,  2361959,  2716252,  3123690,
    3592244,  4131080,  4750742,  5463353,  6282856,  7225284,  8309077,  9555438,  10988754,
    12637066, 14532626, 16712520, 19219397, 22102306, 25417652, 29230299, 33614843, 38657069,
    44455628, 51123971};

struct ChaserBoss : ecs::component {
  static constexpr std::uint32_t kBaseHp = 60;
  static constexpr std::uint32_t kMaxSplit = 7;
  static constexpr std::uint32_t kTimer = 60;
  static constexpr fixed kSpeed = 4;
  static constexpr float kZIndex = -4.f;
  static constexpr glm::vec4 c = colour_hue360(210, .6f);

  template <fixed R, glm::vec4 C, shape_flag Flags = shape_flag::kNone>
  using scale_shape =
      geom::ngon_eval<geom::multiply_p<R, 2>, geom::constant<5u>, geom::constant<C>,
                      geom::constant<geom::ngon_style::kPolygram>, geom::constant<Flags>>;
  using shape = standard_transform<
      scale_shape<10, c>, scale_shape<9, c>,
      scale_shape<8, glm::vec4{0}, shape_flag::kDangerous | shape_flag::kVulnerable>,
      scale_shape<7, glm::vec4{0}, shape_flag::kShield>>;

  std::tuple<vec2, fixed, fixed> shape_parameters(const Transform& transform) const {
    return {transform.centre, transform.rotation, kSplitLookup[kMaxSplit - split].pow_1_5};
  }

  ChaserBoss(SimInterface& sim, std::uint32_t cycle, std::uint32_t split, std::uint32_t time,
             std::uint32_t stagger)
  : on_screen{split != 0}, timer{time}, cycle{cycle}, split{split}, stagger{stagger} {
    if (!split) {
      sim.index().iterate<ChaserBossSharedState>(
          [&](ChaserBossSharedState& state) { state.count = 0; });
    }
  }
  bool on_screen = false;
  bool move = false;
  vec2 dir{0};
  vec2 last_dir{0};

  std::uint32_t timer = 0;
  std::uint32_t cycle = 0;
  std::uint32_t split = 0;
  std::uint32_t stagger = 0;

  void update(ecs::handle h, Transform& transform, Boss& boss, SimInterface& sim) {
    boss.show_hp_bar = false;
    ChaserBossSharedState* state = nullptr;
    sim.index().iterate<ChaserBossSharedState>([&](ChaserBossSharedState& s) { state = &s; });
    // Compatibility: obviously a mistake, we're supposed to be counting bosses - players but
    // instead counting shots/powerups etc as well.
    auto remaining = sim.index().count<Transform>() - sim.index().count<Player>();
    last_dir = normalise(dir);
    if (sim.is_on_screen(transform.centre)) {
      on_screen = true;
    }

    if (timer && !--timer) {
      timer = kTimer * (move + 1);
      if (split &&
          (move || !sim.random(8 + split) || remaining <= 4 ||
           !sim.random(kPow1_15Lookup[std::min(127u, remaining)]))) {
        move = !move;
      }
      if (move) {
        dir = kSpeed * kSplitLookup[split].pow_1_1 * sim.nearest_player_direction(transform.centre);
      }
    }
    if (move) {
      transform.move(dir);
    } else {
      static constexpr fixed c_attract = -(1 + 2 * fixed_c::tenth);
      static constexpr fixed c_dir0 = kSpeed / 14;
      static constexpr fixed c_dir1 = 8 * kSpeed / (14 * 9);
      static constexpr fixed c_dir2 = 8 * kSpeed / (14 * 11);
      static constexpr fixed c_dir3 = 8 * kSpeed / (14 * 2);
      static constexpr fixed c_dir4 = 8 * kSpeed / (14 * 3);
      static constexpr fixed c_rotate = fixed_c::hundredth * 5 / fixed{kMaxSplit};

      const fixed attract = 256 * kSplitLookup[kMaxSplit - split].pow_1_1;
      const fixed align = 128 * kSplitLookup[kMaxSplit - split].pow_1_15;
      const fixed repulse = 64 * kSplitLookup[kMaxSplit - split].pow_1_2;

      dir = last_dir;
      if (stagger ==
          state->count %
              (split == 0       ? 1
                   : split == 1 ? 2
                   : split == 2 ? 4
                   : split == 3 ? 8
                                : 16)) {
        dir.x = dir.y = 0;

        auto handle_ship = [&](ecs::const_handle sh, const ChaserBoss* cb) {
          if (sh.id() == h.id()) {
            return;
          }
          auto v = transform.centre - sh.get<Transform>()->centre;
          auto r = length(v);
          if (r > 0) {
            v /= r;
          }
          vec2 p{0};
          if (cb) {
            fixed pow = kSplitLookup[kMaxSplit - cb->split].pow_1_15;
            v *= pow;
            p = cb->last_dir * pow;
          } else {
            p = from_polar(sh.get<Transform>()->rotation, 1_fx);
          }

          if (r > attract) {
            return;
          }
          // Attract.
          dir += v * c_attract;
          if (r > align) {
            return;
          }
          // Align.
          dir += p;
          if (r > repulse) {
            return;
          }
          // Repulse.
          dir += v * 3;
        };

        sim.index().iterate_dispatch<Player>(
            [&](ecs::const_handle sh) { handle_ship(sh, nullptr); });
        sim.index().iterate_dispatch<ChaserBoss>(
            [&](ecs::const_handle sh, const ChaserBoss& cb) { handle_ship(sh, &cb); });
      }
      auto d = sim.dimensions();
      if (remaining < 4 && split >= kMaxSplit - 1) {
        if ((transform.centre.x < 32 && dir.x < 0) ||
            (transform.centre.x >= d.x - 32 && dir.x > 0)) {
          dir.x = -dir.x;
        }
        if ((transform.centre.y < 32 && dir.y < 0) ||
            (transform.centre.y >= d.y - 32 && dir.y > 0)) {
          dir.y = -dir.y;
        }
      } else if (remaining < 8 && split >= kMaxSplit - 1) {
        if ((transform.centre.x < 0 && dir.x < 0) || (transform.centre.x >= d.x && dir.x > 0)) {
          dir.x = -dir.x;
        }
        if ((transform.centre.y < 0 && dir.y < 0) || (transform.centre.y >= d.y && dir.y > 0)) {
          dir.y = -dir.y;
        }
      } else {
        if ((transform.centre.x < -32 && dir.x < 0) ||
            (transform.centre.x >= d.x + 32 && dir.x > 0)) {
          dir.x = -dir.x;
        }
        if ((transform.centre.y < -32 && dir.y < 0) ||
            (transform.centre.y >= d.y + 32 && dir.y > 0)) {
          dir.y = -dir.y;
        }
      }

      if (transform.centre.x < -256) {
        dir = vec2{1, 0};
      } else if (transform.centre.x >= d.x + 256) {
        dir = vec2{-1, 0};
      } else if (transform.centre.y < -256) {
        dir = vec2{0, 1};
      } else if (transform.centre.y >= d.y + 256) {
        dir = vec2{0, -1};
      } else {
        dir = normalise(dir);
      }

      dir = split == 0 ? dir + last_dir * 7
          : split == 1 ? dir * 2 + last_dir * 7
          : split == 2 ? dir * 4 + last_dir * 7
          : split == 3 ? dir + last_dir
                       : dir * 2 + last_dir;

      dir *= kSplitLookup[split].pow_1_1 *
          (split == 0       ? c_dir0
               : split == 1 ? c_dir1
               : split == 2 ? c_dir2
               : split == 3 ? c_dir3
                            : c_dir4);
      transform.move(dir);
      transform.rotate(fixed_c::hundredth * 2 + fixed{split} * c_rotate);
    }
    if (!state->has_counted) {
      // TODO: what even is this?
      state->has_counted = true;
      ++state->count;
    }
  }

  void on_destroy(ecs::const_handle h, const Transform& transform, SimInterface& sim, EmitHandle& e,
                  damage_type, const vec2& source) const {
    bool last = false;
    if (split < kMaxSplit) {
      for (std::uint32_t i = 0; i < 2; ++i) {
        vec2 d = from_polar(i * fixed_c::pi + transform.rotation,
                            fixed{8 * kSplitLookup[kMaxSplit - 1 - split].pow_1_2});
        auto sh = spawn_internal(sim, cycle, split + 1, transform.centre + d, (i + 1) * kTimer / 2,
                                 sim.random(split + 1 == 1       ? 2
                                                : split + 1 == 2 ? 4
                                                : split + 1 == 3 ? 8
                                                                 : 16));
        sh.get<Transform>()->set_rotation(transform.rotation);
      }
    } else {
      last = true;
      sim.index().iterate_dispatch<ChaserBoss>([&](ecs::const_handle sh) {
        if (!sh.has<Destroy>() && sh.id() != h.id()) {
          last = false;
        }
      });

      if (last) {
        e.rumble_all(30, 1.f, 1.f);
        std::uint32_t n = 1;
        auto& random = sim.random(random_source::kLegacyAesthetic);
        for (std::uint32_t i = 0; i < 16; ++i) {
          vec2 v = from_polar(random.fixed() * (2 * fixed_c::pi),
                              fixed{8 + random.uint(64) + random.uint(64)});
          sim.global_entity().get<GlobalData>()->fireworks.push_back(GlobalData::fireworks_entry{
              .time = n, .position = transform.centre + v, .colour = c});
          n += i;
        }
      }
    }

    explode_entity_shapes<ChaserBoss>(h, e);
    explode_entity_shapes<ChaserBoss>(h, e, glm::vec4{1.f}, 12);
    if (split < 3 || last) {
      explode_entity_shapes<ChaserBoss>(h, e, c, 24);
    }
    if (split < 2 || last) {
      explode_entity_shapes<ChaserBoss>(h, e, glm::vec4{1.f}, 36);
    }
    if (split < 1 || last) {
      explode_entity_shapes<ChaserBoss>(h, e, c, 48);
    }
    destruct_entity_lines<ChaserBoss>(h, e, source);

    if (split < 3 || last) {
      e.play(sound::kExplosion, transform.centre);
    } else {
      e.play_random(sound::kExplosion, transform.centre);
    }
  }

  static ecs::handle spawn_internal(SimInterface& sim, std::uint32_t cycle, std::uint32_t split = 0,
                                    const vec2& position = vec2{0}, std::uint32_t time = kTimer,
                                    std::uint32_t stagger = 0) {
    auto h = create_ship<ChaserBoss>(
        sim, !split ? vec2{sim.dimensions().x / 2, -sim.dimensions().y / 2} : position);
    h.add(Collision{.flags = shape_flag::kDangerous | shape_flag::kVulnerable | shape_flag::kShield,
                    .bounding_width = 10 * kSplitLookup[ChaserBoss::kMaxSplit - split].pow_1_5,
                    .check = sim.is_legacy()
                        ? &ship_check_point_legacy<ChaserBoss, ChaserBoss::shape>
                        : &ship_check_point<ChaserBoss, ChaserBoss::shape>});
    h.add(Enemy{.threat_value = 100});

    auto rumble = split < 3 ? rumble_type::kLarge
        : split < 6         ? rumble_type::kMedium
                            : rumble_type::kSmall;
    h.add(Health{
        .hp = calculate_boss_hp(
            1 +
                ChaserBoss::kBaseHp /
                    (fixed_c::half + kSplitLookup[split].hp_reduce_power).to_int(),
            sim.player_count(), cycle),
        .hit_flash_ignore_index = 2,
        .hit_sound0 = std::nullopt,
        .hit_sound1 = sound::kEnemyShatter,
        .destroy_sound = std::nullopt,
        .destroy_rumble = rumble,
        .damage_transform = &scale_boss_damage,
        .on_hit = split <= 1 ? &boss_on_hit<true, ChaserBoss> : &boss_on_hit<false, ChaserBoss>,
        .on_destroy = ecs::call<&ChaserBoss::on_destroy>,
    });
    h.add(Boss{});
    h.add(ChaserBoss{sim, cycle, split, time, stagger});
    return h;
  }

  static std::vector<std::uint32_t> get_hp_lookup(std::uint32_t players, std::uint32_t cycle) {
    std::vector<std::uint32_t> r;
    std::uint32_t hp = 0;
    for (std::uint32_t i = 0; i < 8; ++i) {
      hp = 2 * hp +
          calculate_boss_hp(
               1 + kBaseHp / (fixed_c::half + kSplitLookup[kMaxSplit - i].hp_reduce_power).to_int(),
               players, cycle);
      r.push_back(hp);
    }
    return r;
  }
};
DEBUG_STRUCT_TUPLE(ChaserBoss, on_screen, move, dir, last_dir, timer, cycle, split, stagger);

}  // namespace

void spawn_chaser_boss(SimInterface& sim, std::uint32_t cycle) {
  auto h = sim.index().create();
  h.add(Enemy{.threat_value = 0,
              .boss_score_reward =
                  calculate_boss_score(boss_flag::kBoss1C, sim.player_count(), cycle)});

  h.add(ChaserBossSharedState{.cycle = cycle});
  h.add(Health{});
  h.add(Boss{.boss = boss_flag::kBoss1C});

  h.add(Update{.update = [](ecs::handle h, SimInterface& sim) {
    auto& health = *h.get<Health>();
    auto& state = *h.get<ChaserBossSharedState>();
    state.has_counted = false;
    auto hp_lookup = ChaserBoss::get_hp_lookup(sim.player_count(), state.cycle);
    health.hp = 0;
    health.max_hp = hp_lookup[ChaserBoss::kMaxSplit];
    sim.index().iterate_dispatch<ChaserBoss>(
        [&](const ChaserBoss& cb, const Transform& transform, const Health& sub_health) {
          if (sim.is_on_screen(transform.centre)) {
            h.get<Boss>()->show_hp_bar = true;
          }
          health.hp += (cb.split == 7 ? 0 : 2 * hp_lookup[6 - cb.split]) + sub_health.hp;
        });
    if (!health.hp) {
      h.emplace<Destroy>();
    }
  }});

  ChaserBoss::spawn_internal(sim, cycle);
}
}  // namespace ii
