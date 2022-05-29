#include "game/logic/boss/chaser.h"
#include "game/logic/player.h"
#include <algorithm>

namespace {
const std::int32_t kCbBaseHp = 60;
const std::int32_t kCbMaxSplit = 7;
const fixed kCbSpeed = 4;

// power(HP_REDUCE_POWER = 1.7, split).
const fixed HP_REDUCE_POWER_lookup[8] = {fixed(1),
                                         1 + 7 * (fixed(1) / 10),
                                         2 + 9 * (fixed(1) / 10),
                                         4 + 9 * (fixed(1) / 10),
                                         8 + 4 * (fixed(1) / 10),
                                         14 + 2 * (fixed(1) / 10),
                                         24 + 2 * (fixed(1) / 10),
                                         41};

// power(1.5, split).
const fixed ONE_AND_HALF_lookup[8] = {fixed(1),
                                      1 + (fixed(1) / 2),
                                      2 + (fixed(1) / 4),
                                      3 + 4 * (fixed(1) / 10),
                                      5 + (fixed(1) / 10),
                                      7 + 6 * (fixed(1) / 10),
                                      11 + 4 * (fixed(1) / 10),
                                      17 + (fixed(1) / 10)};

// power(1.1, split).
const fixed ONE_PT_ONE_lookup[8] = {fixed(1),
                                    1 + (fixed(1) / 10),
                                    1 + 2 * (fixed(1) / 10),
                                    1 + 3 * (fixed(1) / 10),
                                    1 + 5 * (fixed(1) / 10),
                                    1 + 6 * (fixed(1) / 10),
                                    1 + 8 * (fixed(1) / 10),
                                    1 + 9 * (fixed(1) / 10)};

// power(1.15, split).
const fixed ONE_PT_ONE_FIVE_lookup[8] = {fixed(1),
                                         1 + 15 * (fixed(1) / 100),
                                         1 + 3 * (fixed(1) / 10),
                                         1 + 5 * (fixed(1) / 10),
                                         1 + 7 * (fixed(1) / 10),
                                         2,
                                         2 + 3 * (fixed(1) / 10),
                                         2 + 7 * (fixed(1) / 10)};

// power(1.2, split).
const fixed ONE_PT_TWO_lookup[8] = {fixed(1),
                                    1 + 2 * (fixed(1) / 10),
                                    1 + 4 * (fixed(1) / 10),
                                    1 + 7 * (fixed(1) / 10),
                                    2 + (fixed(1) / 10),
                                    2 + (fixed(1) / 2),
                                    3,
                                    3 + 6 * (fixed(1) / 10)};

// power(1.15, remaining).
const std::int32_t ONE_PT_ONE_FIVE_intLookup[128] = {
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
}  // namespace

std::int32_t ChaserBoss::count_;
bool ChaserBoss::has_counted_;
std::int32_t ChaserBoss::shared_hp_;

ChaserBoss::ChaserBoss(std::int32_t players, std::int32_t cycle, std::int32_t split,
                       const vec2& position, std::int32_t time, std::int32_t stagger)
: Boss{!split ? vec2{ii::kSimWidth / 2, -ii::kSimHeight / 2} : position,
       ii::SimInterface::kBoss1C,
       1 + kCbBaseHp / (fixed_c::half + HP_REDUCE_POWER_lookup[split]).to_int(),
       players,
       cycle,
       split <= 1}
, on_screen_{split != 0}
, timer_{time}
, players_{players}
, cycle_{cycle}
, split_{split}
, stagger_{stagger} {
  add_new_shape<Polygon>(vec2{}, 10 * ONE_AND_HALF_lookup[kCbMaxSplit - split_], 5, 0x3399ffff, 0,
                         0, Polygon::T::kPolygram);
  add_new_shape<Polygon>(vec2{}, 9 * ONE_AND_HALF_lookup[kCbMaxSplit - split_], 5, 0x3399ffff, 0, 0,
                         Polygon::T::kPolygram);
  add_new_shape<Polygon>(vec2{}, 8 * ONE_AND_HALF_lookup[kCbMaxSplit - split_], 5, 0, 0,
                         kDangerous | kVulnerable, Polygon::T::kPolygram);
  add_new_shape<Polygon>(vec2{}, 7 * ONE_AND_HALF_lookup[kCbMaxSplit - split_], 5, 0, 0, kShield,
                         Polygon::T::kPolygram);

  set_ignore_damage_colour_index(2);
  set_bounding_width(10 * ONE_AND_HALF_lookup[kCbMaxSplit - split_]);
  if (!split_) {
    shared_hp_ = 0;
    count_ = 0;
  }
}

void ChaserBoss::update() {
  const auto& remaining = sim().all_ships();
  last_dir_ = dir_.normalised();
  if (is_on_screen()) {
    on_screen_ = true;
  }

  if (timer_) {
    timer_--;
  }
  if (timer_ <= 0) {
    timer_ = kTimer * (move_ + 1);
    std::int32_t count = remaining.size() - sim().players().size();
    if (split_ != 0 &&
        (move_ || z::rand_int(8 + split_) == 0 || count <= 4 ||
         !z::rand_int(ONE_PT_ONE_FIVE_intLookup[std::max(0, std::min(127, count))]))) {
      move_ = !move_;
    }
    if (move_) {
      dir_ = kCbSpeed * ONE_PT_ONE_lookup[split_] *
          (nearest_player()->shape().centre - shape().centre).normalised();
    }
  }
  if (move_) {
    move(dir_);
  } else {
    const auto& nearby = sim().all_ships(kShipPlayer | kShipBoss);
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

        vec2 v = shape().centre - ship->shape().centre;
        fixed len = v.length();
        if (len > 0) {
          v /= len;
        }
        vec2 p;
        if (ship->type() & kShipBoss) {
          ChaserBoss* c = (ChaserBoss*)ship;
          fixed pow = ONE_PT_ONE_FIVE_lookup[kCbMaxSplit - c->split_];
          v *= pow;
          p = c->last_dir_ * pow;
        } else {
          p = vec2::from_polar(ship->shape().rotation(), 1);
        }

        if (len > attract) {
          continue;
        }
        // Attract.
        dir_ += v * c_attract;
        if (len > align) {
          continue;
        }
        // Align.
        dir_ += p;
        if (len > repulse) {
          continue;
        }
        // Repulse.
        dir_ += v * 3;
      }
    }
    if (static_cast<std::int32_t>(remaining.size()) - sim().players().size() < 4 &&
        split_ >= kCbMaxSplit - 1) {
      if ((shape().centre.x < 32 && dir_.x < 0) ||
          (shape().centre.x >= ii::kSimWidth - 32 && dir_.x > 0)) {
        dir_.x = -dir_.x;
      }
      if ((shape().centre.y < 32 && dir_.y < 0) ||
          (shape().centre.y >= ii::kSimHeight - 32 && dir_.y > 0)) {
        dir_.y = -dir_.y;
      }
    } else if (static_cast<std::int32_t>(remaining.size()) - sim().players().size() < 8 &&
               split_ >= kCbMaxSplit - 1) {
      if ((shape().centre.x < 0 && dir_.x < 0) ||
          (shape().centre.x >= ii::kSimWidth && dir_.x > 0)) {
        dir_.x = -dir_.x;
      }
      if ((shape().centre.y < 0 && dir_.y < 0) ||
          (shape().centre.y >= ii::kSimHeight && dir_.y > 0)) {
        dir_.y = -dir_.y;
      }
    } else {
      if ((shape().centre.x < -32 && dir_.x < 0) ||
          (shape().centre.x >= ii::kSimWidth + 32 && dir_.x > 0)) {
        dir_.x = -dir_.x;
      }
      if ((shape().centre.y < -32 && dir_.y < 0) ||
          (shape().centre.y >= ii::kSimHeight + 32 && dir_.y > 0)) {
        dir_.y = -dir_.y;
      }
    }

    if (shape().centre.x < -256) {
      dir_ = vec2{1, 0};
    } else if (shape().centre.x >= ii::kSimWidth + 256) {
      dir_ = vec2{-1, 0};
    } else if (shape().centre.y < -256) {
      dir_ = vec2{0, 1};
    } else if (shape().centre.y >= ii::kSimHeight + 256) {
      dir_ = vec2{0, -1};
    } else {
      dir_ = dir_.normalised();
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
    shape().rotate(fixed_c::hundredth * 2 + fixed(split_) * c_rotate);
  }
  shared_hp_ = 0;
  if (!has_counted_) {
    has_counted_ = true;
    ++count_;
  }
}

void ChaserBoss::render() const {
  Boss::render();
  static std::vector<std::int32_t> hp_lookup;
  if (hp_lookup.empty()) {
    std::int32_t hp = 0;
    for (std::int32_t i = 0; i < 8; ++i) {
      hp = 2 * hp +
          CalculateHP(1 + kCbBaseHp / (fixed_c::half + HP_REDUCE_POWER_lookup[7 - i]).to_int(),
                      players_, cycle_);
      hp_lookup.push_back(hp);
    }
  }
  shared_hp_ += (split_ == 7 ? 0 : 2 * hp_lookup[6 - split_]) + get_remaining_hp() * 30;
  if (on_screen_) {
    sim().render_hp_bar(static_cast<float>(shared_hp_) / hp_lookup[kCbMaxSplit]);
  }
}

std::int32_t ChaserBoss::get_damage(std::int32_t damage, bool magic) {
  return damage;
}

void ChaserBoss::on_destroy() {
  bool last = false;
  if (split_ < kCbMaxSplit) {
    for (std::int32_t i = 0; i < 2; ++i) {
      vec2 d = vec2::from_polar(i * fixed_c::pi + shape().rotation(),
                                8 * ONE_PT_TWO_lookup[kCbMaxSplit - 1 - split_]);
      auto s = std::make_unique<ChaserBoss>(players_, cycle_, split_ + 1, shape().centre + d,
                                            (i + 1) * kTimer / 2,
                                            z::rand_int(split_ + 1 == 1       ? 2
                                                            : split_ + 1 == 2 ? 4
                                                            : split_ + 1 == 3 ? 8
                                                                              : 16));
      s->shape().set_rotation(shape().rotation());
      spawn(std::move(s));
    }
  } else {
    last = true;
    for (const auto& ship : sim().all_ships(kShipEnemy)) {
      if (!ship->is_destroyed() && ship != this) {
        last = false;
        break;
      }
    }

    if (last) {
      set_killed();
      for (std::int32_t i = 0; i < kPlayers; ++i) {
        sim().rumble(i, 25);
      }
      for (const auto& ship : sim().players()) {
        Player* p = (Player*)ship;
        if (!p->is_killed()) {
          p->add_score(get_score() / sim().alive_players());
        }
      }
      std::int32_t n = 1;
      for (std::int32_t i = 0; i < 16; ++i) {
        vec2 v = vec2::from_polar(z::rand_fixed() * (2 * fixed_c::pi),
                                  8 + z::rand_int(64) + z::rand_int(64));
        fireworks_.push_back(
            std::make_pair(n, std::make_pair(shape().centre + v, shapes()[0]->colour)));
        n += i;
      }
    }
  }

  for (std::int32_t i = 0; i < kPlayers; ++i) {
    sim().rumble(i, split_ < 3 ? 10 : 3);
  }

  explosion();
  explosion(0xffffffff, 12);
  if (split_ < 3 || last) {
    explosion(shapes()[0]->colour, 24);
  }
  if (split_ < 2 || last) {
    explosion(0xffffffff, 36);
  }
  if (split_ < 1 || last) {
    explosion(shapes()[0]->colour, 48);
  }

  if (split_ < 3 || last) {
    play_sound(ii::sound::kExplosion);
  } else {
    play_sound_random(ii::sound::kExplosion);
  }
}
