#include "game/logic/boss/boss_internal.h"
#include "game/logic/player/player.h"
#include <algorithm>

namespace ii {
namespace {
constexpr std::uint32_t kCbBaseHp = 60;
constexpr std::uint32_t kCbMaxSplit = 7;
constexpr fixed kCbSpeed = 4;

// power(HP_REDUCE_POWER = 1.7, split).
constexpr fixed HP_REDUCE_POWER_lookup[8] = {1_fx,
                                             1 + 7 * (1_fx / 10),
                                             2 + 9 * (1_fx / 10),
                                             4 + 9 * (1_fx / 10),
                                             8 + 4 * (1_fx / 10),
                                             14 + 2 * (1_fx / 10),
                                             24 + 2 * (1_fx / 10),
                                             41};

// power(1.5, split).
constexpr fixed ONE_AND_HALF_lookup[8] = {1_fx,
                                          1 + (1_fx / 2),
                                          2 + (1_fx / 4),
                                          3 + 4 * (1_fx / 10),
                                          5 + (1_fx / 10),
                                          7 + 6 * (1_fx / 10),
                                          11 + 4 * (1_fx / 10),
                                          17 + (1_fx / 10)};

// power(1.1, split).
constexpr fixed ONE_PT_ONE_lookup[8] = {1_fx,
                                        1 + (1_fx / 10),
                                        1 + 2 * (1_fx / 10),
                                        1 + 3 * (1_fx / 10),
                                        1 + 5 * (1_fx / 10),
                                        1 + 6 * (1_fx / 10),
                                        1 + 8 * (1_fx / 10),
                                        1 + 9 * (1_fx / 10)};

// power(1.15, split).
constexpr fixed ONE_PT_ONE_FIVE_lookup[8] = {
    1_fx, 1 + 15 * (1_fx / 100), 1 + 3 * (1_fx / 10), 1 + 5 * (1_fx / 10), 1 + 7 * (1_fx / 10),
    2,    2 + 3 * (1_fx / 10),   2 + 7 * (1_fx / 10)};

// power(1.2, split).
constexpr fixed ONE_PT_TWO_lookup[8] = {1_fx,
                                        1 + 2 * (1_fx / 10),
                                        1 + 4 * (1_fx / 10),
                                        1 + 7 * (1_fx / 10),
                                        2 + (1_fx / 10),
                                        2 + (1_fx / 2),
                                        3,
                                        3 + 6 * (1_fx / 10)};

// power(1.15, remaining).
constexpr std::uint32_t ONE_PT_ONE_FIVE_intLookup[128] = {
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

class ChaserBoss : public ::Boss {
public:
  static constexpr std::uint32_t kTimer = 60;
  ChaserBoss(SimInterface& sim, std::uint32_t cycle, std::uint32_t split, const vec2& position,
             std::uint32_t time, std::uint32_t stagger);

  void update() override;
  void on_destroy(bool bomb) override;

  static bool has_counted_;

private:
  bool on_screen_ = false;
  bool move_ = false;
  std::uint32_t timer_ = 0;
  vec2 dir_{0};
  vec2 last_dir_{0};

  std::uint32_t cycle_ = 0;
  std::uint32_t split_ = 0;

  std::uint32_t stagger_ = 0;
  static std::uint32_t count_;
};

std::uint32_t ChaserBoss::count_;
bool ChaserBoss::has_counted_;

struct ChaserBossTag : ecs::component {
  std::uint32_t split;
};

ChaserBoss* spawn_chaser_boss_internal(SimInterface& sim, std::uint32_t cycle,
                                       std::uint32_t split = 0, const vec2& position = vec2{0},
                                       std::uint32_t time = ChaserBoss::kTimer,
                                       std::uint32_t stagger = 0) {
  auto u = std::make_unique<ChaserBoss>(sim, cycle, split, position, time, stagger);
  auto p = u.get();
  auto h = sim.create_legacy(std::move(u));
  h.add(legacy_collision(/* bounding width */ 10 * ONE_AND_HALF_lookup[kCbMaxSplit - split]));
  h.add(Enemy{.threat_value = 100});
  h.add(Health{
      .hp = calculate_boss_hp(
          1 + kCbBaseHp / (fixed_c::half + HP_REDUCE_POWER_lookup[split]).to_int(),
          sim.player_count(), cycle),
      .hit_flash_ignore_index = 2,
      .hit_sound0 = std::nullopt,
      .hit_sound1 = sound::kEnemyShatter,
      .destroy_sound = std::nullopt,
      .damage_transform = &scale_boss_damage,
      .on_hit = split <= 1 ? &legacy_boss_on_hit<true> : &legacy_boss_on_hit<false>,
      .on_destroy = &legacy_boss_on_destroy,
  });
  h.add(ChaserBossTag{.split = split});
  return p;
}

ChaserBoss::ChaserBoss(SimInterface& sim, std::uint32_t cycle, std::uint32_t split,
                       const vec2& position, std::uint32_t time, std::uint32_t stagger)
: Boss{sim, !split ? vec2{kSimDimensions.x / 2, -kSimDimensions.y / 2} : position}
, on_screen_{split != 0}
, timer_{time}
, cycle_{cycle}
, split_{split}
, stagger_{stagger} {
  auto c = colour_hue360(210, .6f);
  add_new_shape<Polygon>(vec2{0}, 10 * ONE_AND_HALF_lookup[kCbMaxSplit - split_], 5, c, 0,
                         shape_flag::kNone, Polygon::T::kPolygram);
  add_new_shape<Polygon>(vec2{0}, 9 * ONE_AND_HALF_lookup[kCbMaxSplit - split_], 5, c, 0,
                         shape_flag::kNone, Polygon::T::kPolygram);
  add_new_shape<Polygon>(vec2{0}, 8 * ONE_AND_HALF_lookup[kCbMaxSplit - split_], 5, glm::vec4{0.f},
                         0, shape_flag::kDangerous | shape_flag::kVulnerable,
                         Polygon::T::kPolygram);
  add_new_shape<Polygon>(vec2{0}, 7 * ONE_AND_HALF_lookup[kCbMaxSplit - split_], 5, glm::vec4{0.f},
                         0, shape_flag::kShield, Polygon::T::kPolygram);

  if (!split_) {
    count_ = 0;
  }
}

void ChaserBoss::update() {
  const auto& remaining = sim().all_ships();
  last_dir_ = normalise(dir_);
  if (is_on_screen()) {
    on_screen_ = true;
  }

  if (timer_) {
    timer_--;
  }
  if (!timer_) {
    timer_ = kTimer * (move_ + 1);
    std::uint32_t count = remaining.size();
    count = count > sim().player_count() ? count - sim().player_count() : 0;
    if (split_ != 0 &&
        (move_ || sim().random(8 + split_) == 0 || count <= 4 ||
         !sim().random(ONE_PT_ONE_FIVE_intLookup[std::min(127u, count)]))) {
      move_ = !move_;
    }
    if (move_) {
      dir_ = kCbSpeed * ONE_PT_ONE_lookup[split_] * sim().nearest_player_direction(shape().centre);
    }
  }
  if (move_) {
    move(dir_);
  } else {
    const auto& nearby = sim().all_ships(ship_flag::kPlayer | ship_flag::kBoss);
    const fixed attract = 256 * ONE_PT_ONE_lookup[kCbMaxSplit - split_];
    const fixed align = 128 * ONE_PT_ONE_FIVE_lookup[kCbMaxSplit - split_];
    const fixed repulse = 64 * ONE_PT_TWO_lookup[kCbMaxSplit - split_];
    static const fixed c_attract = -(1 + 2 * fixed_c::tenth);
    static const fixed c_dir0 = kCbSpeed / 14;
    static const fixed c_dir1 = 8 * kCbSpeed / (14 * 9);
    static const fixed c_dir2 = 8 * kCbSpeed / (14 * 11);
    static const fixed c_dir3 = 8 * kCbSpeed / (14 * 2);
    static const fixed c_dir4 = 8 * kCbSpeed / (14 * 3);
    static const fixed c_rotate = fixed_c::hundredth * 5 / fixed{kCbMaxSplit};

    dir_ = last_dir_;
    if (stagger_ ==
        count_ %
            (split_ == 0       ? 1
                 : split_ == 1 ? 2
                 : split_ == 2 ? 4
                 : split_ == 3 ? 8
                               : 16)) {
      dir_.x = dir_.y = 0;
      for (const auto& ship : nearby) {
        if (ship == this) {
          continue;
        }

        auto v = shape().centre - ship->position();
        auto r = length(v);
        if (r > 0) {
          v /= r;
        }
        vec2 p{0};
        if (+(ship->type() & ship_flag::kBoss)) {
          ChaserBoss* c = (ChaserBoss*)ship;
          fixed pow = ONE_PT_ONE_FIVE_lookup[kCbMaxSplit - c->split_];
          v *= pow;
          p = c->last_dir_ * pow;
        } else {
          p = from_polar(ship->rotation(), 1_fx);
        }

        if (r > attract) {
          continue;
        }
        // Attract.
        dir_ += v * c_attract;
        if (r > align) {
          continue;
        }
        // Align.
        dir_ += p;
        if (r > repulse) {
          continue;
        }
        // Repulse.
        dir_ += v * 3;
      }
    }
    if (remaining.size() < 4 + sim().player_count() && split_ >= kCbMaxSplit - 1) {
      if ((shape().centre.x < 32 && dir_.x < 0) ||
          (shape().centre.x >= kSimDimensions.x - 32 && dir_.x > 0)) {
        dir_.x = -dir_.x;
      }
      if ((shape().centre.y < 32 && dir_.y < 0) ||
          (shape().centre.y >= kSimDimensions.y - 32 && dir_.y > 0)) {
        dir_.y = -dir_.y;
      }
    } else if (remaining.size() < 8 + sim().player_count() && split_ >= kCbMaxSplit - 1) {
      if ((shape().centre.x < 0 && dir_.x < 0) ||
          (shape().centre.x >= kSimDimensions.x && dir_.x > 0)) {
        dir_.x = -dir_.x;
      }
      if ((shape().centre.y < 0 && dir_.y < 0) ||
          (shape().centre.y >= kSimDimensions.y && dir_.y > 0)) {
        dir_.y = -dir_.y;
      }
    } else {
      if ((shape().centre.x < -32 && dir_.x < 0) ||
          (shape().centre.x >= kSimDimensions.x + 32 && dir_.x > 0)) {
        dir_.x = -dir_.x;
      }
      if ((shape().centre.y < -32 && dir_.y < 0) ||
          (shape().centre.y >= kSimDimensions.y + 32 && dir_.y > 0)) {
        dir_.y = -dir_.y;
      }
    }

    if (shape().centre.x < -256) {
      dir_ = vec2{1, 0};
    } else if (shape().centre.x >= kSimDimensions.x + 256) {
      dir_ = vec2{-1, 0};
    } else if (shape().centre.y < -256) {
      dir_ = vec2{0, 1};
    } else if (shape().centre.y >= kSimDimensions.y + 256) {
      dir_ = vec2{0, -1};
    } else {
      dir_ = normalise(dir_);
    }

    dir_ = split_ == 0 ? dir_ + last_dir_ * 7
        : split_ == 1  ? dir_ * 2 + last_dir_ * 7
        : split_ == 2  ? dir_ * 4 + last_dir_ * 7
        : split_ == 3  ? dir_ + last_dir_
                       : dir_ * 2 + last_dir_;

    dir_ *= ONE_PT_ONE_lookup[split_] *
        (split_ == 0       ? c_dir0
             : split_ == 1 ? c_dir1
             : split_ == 2 ? c_dir2
             : split_ == 3 ? c_dir3
                           : c_dir4);
    move(dir_);
    shape().rotate(fixed_c::hundredth * 2 + fixed{split_} * c_rotate);
  }
  if (!has_counted_) {
    has_counted_ = true;
    ++count_;
  }
}

void ChaserBoss::on_destroy(bool) {
  bool last = false;
  if (split_ < kCbMaxSplit) {
    for (std::uint32_t i = 0; i < 2; ++i) {
      vec2 d = from_polar(i * fixed_c::pi + shape().rotation(),
                          fixed{8 * ONE_PT_TWO_lookup[kCbMaxSplit - 1 - split_]});
      auto* s = spawn_chaser_boss_internal(sim(), cycle_, split_ + 1, shape().centre + d,
                                           (i + 1) * kTimer / 2,
                                           sim().random(split_ + 1 == 1       ? 2
                                                            : split_ + 1 == 2 ? 4
                                                            : split_ + 1 == 3 ? 8
                                                                              : 16));
      s->shape().set_rotation(shape().rotation());
    }
  } else {
    last = true;
    for (const auto& ship : sim().all_ships(ship_flag::kEnemy)) {
      if (!ship->is_destroyed() && ship != this) {
        last = false;
        break;
      }
    }

    if (last) {
      sim().rumble_all(25);
      std::uint32_t n = 1;
      for (std::uint32_t i = 0; i < 16; ++i) {
        vec2 v = from_polar(sim().random_fixed() * (2 * fixed_c::pi),
                            fixed{8 + sim().random(64) + sim().random(64)});
        fireworks_.push_back(
            std::make_pair(n, std::make_pair(shape().centre + v, shapes()[0]->colour)));
        n += i;
      }
    }
  }

  sim().rumble_all(split_ < 3 ? 10 : 3);
  explosion();
  explosion(glm::vec4{1.f}, 12);
  if (split_ < 3 || last) {
    explosion(shapes()[0]->colour, 24);
  }
  if (split_ < 2 || last) {
    explosion(glm::vec4{1.f}, 36);
  }
  if (split_ < 1 || last) {
    explosion(shapes()[0]->colour, 48);
  }

  if (split_ < 3 || last) {
    play_sound(sound::kExplosion);
  } else {
    play_sound_random(sound::kExplosion);
  }
}

std::vector<std::uint32_t> get_hp_lookup(std::uint32_t players, std::uint32_t cycle) {
  std::vector<std::uint32_t> r;
  std::uint32_t hp = 0;
  for (std::uint32_t i = 0; i < 8; ++i) {
    hp = 2 * hp +
        calculate_boss_hp(1 + kCbBaseHp / (fixed_c::half + HP_REDUCE_POWER_lookup[7 - i]).to_int(),
                          players, cycle);
    r.push_back(hp);
  }
  return r;
}

}  // namespace

void spawn_chaser_boss(SimInterface& sim, std::uint32_t cycle) {
  auto h = sim.index().create();
  h.add(Enemy{.threat_value = 0,
              .boss_score_reward =
                  calculate_boss_score(boss_flag::kBoss1C, sim.player_count(), cycle)});

  struct Cycle : ecs::component {
    std::uint32_t cycle;
  };
  h.add(Cycle{.cycle = cycle});
  h.add(Health{});
  h.add(Boss{.boss = boss_flag::kBoss1C});

  h.add(Update{.update = [](ecs::handle h, SimInterface& sim) {
    auto& health = *h.get<Health>();
    auto hp_lookup = get_hp_lookup(sim.player_count(), h.get<Cycle>()->cycle);
    health.hp = 0;
    health.max_hp = hp_lookup[kCbMaxSplit];
    std::optional<vec2> position;
    sim.index().iterate<ChaserBossTag>([&](ecs::const_handle sub_h, const ChaserBossTag& tag) {
      if (auto* c = sub_h.get<Transform>(); c) {
        position = c->centre;
      } else if (auto* c = sub_h.get<LegacyShip>(); c) {
        position = c->ship->position();
      }
      if (position && sim.is_on_screen(*position)) {
        h.get<Boss>()->show_hp_bar = true;
      }
      health.hp += (tag.split == 7 ? 0 : 2 * hp_lookup[6 - tag.split]) + sub_h.get<Health>()->hp;
    });
    if (!health.hp) {
      h.emplace<Destroy>();
    }
  }});

  spawn_chaser_boss_internal(sim, cycle);
}

void chaser_boss_begin_frame() {
  ChaserBoss::has_counted_ = false;
}
}  // namespace ii
