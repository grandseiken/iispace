#include "game/logic/overmind.h"
#include "game/logic/boss/chaser.h"
#include "game/logic/boss/deathray.h"
#include "game/logic/boss/ghost.h"
#include "game/logic/boss/shield.h"
#include "game/logic/boss/square.h"
#include "game/logic/boss/super.h"
#include "game/logic/boss/tractor.h"
#include "game/logic/enemy.h"
#include "game/logic/player.h"
#include "game/logic/stars.h"
#include <algorithm>

class formation_base {
public:
  static std::vector<Overmind::entry> static_formations;

  virtual ~formation_base() {}
  virtual void operator()() = 0;

  void
  operator()(SimState* game, std::int32_t row, std::int32_t power, std::int32_t* hard_already) {
    game_ = game;
    row_ = row;
    power_ = power;
    hard_already_ = hard_already;
    operator()();
  }

  vec2 spawn_point(bool top, std::int32_t num, std::int32_t div) {
    div = std::max(2, div);
    num = std::max(0, std::min(div - 1, num));

    fixed x = fixed(top ? num : div - 1 - num) * Lib::kWidth / fixed(div - 1);
    fixed y = top ? -(row_ + 1) * (fixed_c::hundredth * 16) * Lib::kHeight
                  : Lib::kHeight * (1 + (row_ + 1) * (fixed_c::hundredth * 16));
    return vec2{x, y};
  }

  void spawn_follow(std::int32_t num, std::int32_t div, std::int32_t side) {
    do_sides(side, [&](bool b) { game_->add_new_ship<Follow>(spawn_point(b, num, div)); });
  }

  void spawn_chaser(std::int32_t num, std::int32_t div, std::int32_t side) {
    do_sides(side, [&](bool b) { game_->add_new_ship<Chaser>(spawn_point(b, num, div)); });
  }

  void spawn_square(std::int32_t num, std::int32_t div, std::int32_t side) {
    do_sides(side, [&](bool b) { game_->add_new_ship<Square>(spawn_point(b, num, div)); });
  }

  void spawn_wall(std::int32_t num, std::int32_t div, std::int32_t side, bool dir) {
    do_sides(side, [&](bool b) { game_->add_new_ship<Wall>(spawn_point(b, num, div), dir); });
  }

  void spawn_follow_hub(std::int32_t num, std::int32_t div, std::int32_t side) {
    do_sides(side, [&](bool b) {
      bool p1 = z::rand_int(64) < std::min(32, power_ - 32) - *hard_already_;
      if (p1) {
        *hard_already_ += 2;
      }
      bool p2 = z::rand_int(64) < std::min(32, power_ - 40) - *hard_already_;
      if (p2) {
        *hard_already_ += 2;
      }
      game_->add_new_ship<FollowHub>(spawn_point(b, num, div), p1, p2);
    });
  }

  void spawn_shielder(std::int32_t num, std::int32_t div, std::int32_t side) {
    do_sides(side, [&](bool b) {
      bool p = z::rand_int(64) < std::min(32, power_ - 36) - *hard_already_;
      if (p) {
        *hard_already_ += 3;
      }
      game_->add_new_ship<Shielder>(spawn_point(b, num, div), p);
    });
  }

  void spawn_tractor(std::int32_t num, std::int32_t div, std::int32_t side) {
    do_sides(side, [&](bool b) {
      bool p = z::rand_int(64) < std::min(32, power_ - 46) - *hard_already_;
      if (p) {
        *hard_already_ += 4;
      }
      game_->add_new_ship<Tractor>(spawn_point(b, num, div), p);
    });
  }

private:
  template <typename F>
  void do_sides(std::int32_t side, const F& f) {
    if (side == 0 || side == 1) {
      f(false);
    }
    if (side == 0 || side == 2) {
      f(true);
    }
  }

  SimState* game_ = nullptr;
  std::int32_t row_ = 0;
  std::int32_t power_ = 0;
  std::int32_t* hard_already_ = nullptr;
};

std::vector<Overmind::entry> formation_base::static_formations;

Overmind::Overmind(SimState& game, bool can_face_secret_boss)
: game_{game}, can_face_secret_boss_{can_face_secret_boss} {
  add_formations();

  auto queue = [] {
    std::int32_t a = z::rand_int(3);
    std::int32_t b = z::rand_int(2);
    if (a == 0 || (a == 1 && b == 1)) {
      ++b;
    }
    std::int32_t c = (a + b == 3) ? 0 : (a + b == 2) ? 1 : 2;
    return std::vector<std::int32_t>{a, b, c};
  };
  boss1_queue_ = queue();
  boss2_queue_ = queue();

  if (game_.mode() == game_mode::kBoss) {
    return;
  }
  power_ = kInitialPower + 2 - game_.players().size() * 2;
  if (game_.mode() == game_mode::kHard) {
    power_ += 20;
    waves_total_ = 15;
  }
  timer_ = kPowerupTime;
  boss_rest_timer_ = kBossRestTime / 8;
  Stars::clear();
}

Overmind::~Overmind() {}

void Overmind::update() {
  ++elapsed_time_;
  Stars::update();

  if (game_.mode() == game_mode::kBoss) {
    if (count_ <= 0) {
      Stars::change();
      if (boss_mod_bosses_ < 6) {
        if (boss_mod_bosses_)
          for (std::size_t i = 0; i < game_.players().size(); ++i) {
            spawn_boss_reward();
          }
        boss_mode_boss();
      }
      if (boss_mod_bosses_ < 7) {
        ++boss_mod_bosses_;
      }
    }
    return;
  }

  if (boss_rest_timer_ > 0) {
    --boss_rest_timer_;
    return;
  }

  ++timer_;
  if (timer_ == kPowerupTime && !is_boss_level_) {
    spawn_powerup();
  }

  std::int32_t boss_cycles = boss_mod_fights_;
  std::int32_t trigger_stage = groups_mod_ + boss_cycles + 2 * (game_.mode() == game_mode::kHard);
  std::int32_t trigger_val = kInitialTriggerVal;
  for (std::int32_t i = 0; i < trigger_stage; ++i) {
    trigger_val += i < 2 ? 4 : i < 7 - static_cast<std::int32_t>(game_.players().size()) ? 3 : 2;
  }
  if (trigger_val < 0 || is_boss_level_ || is_boss_next_) {
    trigger_val = 0;
  }

  if ((timer_ > kTimer && !is_boss_level_ && !is_boss_next_) || count_ <= trigger_val) {
    if (timer_ < kPowerupTime && !is_boss_level_) {
      spawn_powerup();
    }
    timer_ = 0;
    Stars::change();

    if (is_boss_level_) {
      ++boss_mod_bosses_;
      is_boss_level_ = false;
      if (bosses_to_go_ <= 0) {
        spawn_boss_reward();
        ++boss_mod_fights_;
        power_ += 2 - 2 * game_.players().size();
        boss_rest_timer_ = kBossRestTime;
        bosses_to_go_ = 0;
      } else {
        boss_rest_timer_ = kBossRestTime / 4;
      }
    } else {
      if (is_boss_next_) {
        --bosses_to_go_;
        is_boss_next_ = bosses_to_go_ > 0;
        is_boss_level_ = true;
        boss();
      } else {
        wave();
        if (waves_total_ < 5) {
          power_ += 2;
        } else {
          ++power_;
        }
        ++waves_total_;
        ++levels_mod_;
      }
    }

    if (levels_mod_ >= kLevelsPerGroup) {
      levels_mod_ = 0;
      ++groups_mod_;
    }
    if (groups_mod_ >= kBaseGroupsPerBoss + static_cast<std::int32_t>(game_.players().size())) {
      groups_mod_ = 0;
      is_boss_next_ = true;
      bosses_to_go_ = boss_mod_fights_ >= 4 ? 3 : boss_mod_fights_ >= 2 ? 2 : 1;
    }
  }
}

void Overmind::on_enemy_destroy(const Ship& ship) {
  count_ -= ship.enemy_value();
  if (!(ship.type() & Ship::kShipWall)) {
    non_wall_count_--;
  }
}

void Overmind::on_enemy_create(const Ship& ship) {
  count_ += ship.enemy_value();
  if (!(ship.type() & Ship::kShipWall)) {
    ++non_wall_count_;
  }
}

void Overmind::spawn_powerup() {
  if (game_.all_ships(Ship::kShipPowerup).size() >= 4) {
    return;
  }

  powerup_mod_ = (1 + powerup_mod_) % 4;
  if (powerup_mod_ == 0) {
    ++lives_target_;
  }

  std::int32_t r = z::rand_int(4);
  vec2 v = r == 0 ? vec2{-Lib::kWidth, Lib::kHeight / 2}
      : r == 1    ? vec2{Lib::kWidth * 2, Lib::kHeight / 2}
      : r == 2    ? vec2{Lib::kWidth / 2, -Lib::kHeight}
                  : vec2{Lib::kWidth / 2, Lib::kHeight * 2};

  std::int32_t m = 4;
  for (std::int32_t i = 1; i <= kPlayers; ++i) {
    if (game_.get_lives() <= static_cast<std::int32_t>(game_.players().size()) - i) {
      ++m;
    }
  }
  if (!game_.get_lives()) {
    m += game_.killed_players();
  }
  if (lives_target_ > 0) {
    m += lives_target_;
  }
  if (game_.killed_players() == 0 && lives_target_ < -1) {
    m = 3;
  }

  r = z::rand_int(m);
  game_.add_new_ship<Powerup>(v,
                              r == 0       ? Powerup::type::kBomb
                                  : r == 1 ? Powerup::type::kMagicShots
                                  : r == 2 ? Powerup::type::kShield
                                           : (--lives_target_, Powerup::type::kExtraLife));
}

void Overmind::spawn_boss_reward() {
  std::int32_t r = z::rand_int(4);
  vec2 v = r == 0 ? vec2{-Lib::kWidth / 4, Lib::kHeight / 2}
      : r == 1    ? vec2{Lib::kWidth + Lib::kWidth / 4, Lib::kHeight / 2}
      : r == 2    ? vec2{Lib::kWidth / 2, -Lib::kHeight / 4}
                  : vec2{Lib::kWidth / 2, Lib::kHeight + Lib::kHeight / 4};

  game_.add_new_ship<Powerup>(v, Powerup::type::kExtraLife);
  if (game_.mode() != game_mode::kBoss) {
    spawn_powerup();
  }
}

void Overmind::wave() {
  if (game_.mode() == game_mode::kFast) {
    for (std::int32_t i = 0; i < z::rand_int(7); ++i) {
      z::rand_int(1);
    }
  }
  if (game_.mode() == game_mode::kWhat) {
    for (std::int32_t i = 0; i < z::rand_int(11); ++i) {
      z::rand_int(1);
    }
  }

  std::int32_t resources = power_;
  std::vector<const entry*> valid;
  for (const auto& f : formations_) {
    if (resources >= f.min_resource) {
      valid.emplace_back(&f);
    }
  }

  std::vector<const entry*> chosen;
  while (resources > 0) {
    std::size_t max = 0;
    while (max < valid.size() && valid[max]->cost <= resources) {
      ++max;
    }
    if (max == 0) {
      break;
    }
    std::int32_t n = max == 1 ? 0 : z::rand_int(max);

    chosen.insert(chosen.begin() + z::rand_int(chosen.size() + 1), valid[n]);
    resources -= valid[n]->cost;
  }

  // This permutation is bugged and does nothing but backwards compatibility.
  std::vector<std::size_t> perm;
  for (std::size_t i = 0; i < chosen.size(); ++i) {
    perm.push_back(i);
  }
  for (std::size_t i = 0; i < chosen.size() - 1; ++i) {
    std::swap(perm[i], perm[i + z::rand_int(chosen.size() - i)]);
  }
  hard_already_ = 0;
  for (std::size_t row = 0; row < chosen.size(); ++row) {
    chosen[perm[row]]->function->operator()(&game_, perm[row], power_, &hard_already_);
  }
}

void Overmind::boss() {
  std::int32_t count = game_.players().size();
  std::int32_t cycle = (game_.mode() == game_mode::kHard) + boss_mod_bosses_ / 2;
  bool secret_chance = (game_.mode() != game_mode::kNormal && game_.mode() != game_mode::kBoss)
      ? (boss_mod_fights_ > 1       ? z::rand_int(4) == 0
             : boss_mod_fights_ > 0 ? z::rand_int(8) == 0
                                    : false)
      : (boss_mod_fights_ > 2       ? z::rand_int(4) == 0
             : boss_mod_fights_ > 1 ? z::rand_int(6) == 0
                                    : false);

  if (can_face_secret_boss_ && bosses_to_go_ == 0 && boss_mod_secret_ == 0 && secret_chance) {
    std::int32_t secret_cycle =
        std::max(0, (boss_mod_bosses_ + (game_.mode() == game_mode::kHard) - 2) / 2);
    game_.add_new_ship<SuperBoss>(count, secret_cycle);
    boss_mod_secret_ = 2;
  }

  else if (boss_mod_bosses_ % 2 == 0) {
    if (boss1_queue_[0] == 0) {
      game_.add_new_ship<BigSquareBoss>(count, cycle);
    } else if (boss1_queue_[0] == 1) {
      game_.add_new_ship<ShieldBombBoss>(count, cycle);
    } else {
      game_.add_new_ship<ChaserBoss>(count, cycle);
    }
    boss1_queue_.push_back(boss1_queue_.front());
    boss1_queue_.erase(boss1_queue_.begin());
  } else {
    if (boss_mod_secret_ > 0) {
      --boss_mod_secret_;
    }
    if (boss2_queue_[0] == 0) {
      game_.add_new_ship<TractorBoss>(count, cycle);
    } else if (boss2_queue_[0] == 1) {
      game_.add_new_ship<GhostBoss>(count, cycle);
    } else {
      game_.add_new_ship<DeathRayBoss>(count, cycle);
    }
    boss2_queue_.push_back(boss2_queue_.front());
    boss2_queue_.erase(boss2_queue_.begin());
  }
}

void Overmind::boss_mode_boss() {
  std::int32_t boss = boss_mod_bosses_;
  std::int32_t count = game_.players().size();
  if (boss_mod_bosses_ < 3) {
    if (boss1_queue_[boss] == 0) {
      game_.add_new_ship<BigSquareBoss>(count, 0);
    } else if (boss1_queue_[boss] == 1) {
      game_.add_new_ship<ShieldBombBoss>(count, 0);
    } else {
      game_.add_new_ship<ChaserBoss>(count, 0);
    }
  } else {
    if (boss2_queue_[boss] == 0) {
      game_.add_new_ship<TractorBoss>(count, 0);
    } else if (boss2_queue_[boss] == 1) {
      game_.add_new_ship<GhostBoss>(count, 0);
    } else {
      game_.add_new_ship<DeathRayBoss>(count, 0);
    }
  }
}

// Formations
//------------------------------
template <std::int32_t I, typename F, std::int32_t C, std::int32_t R = 0>
struct formation : formation_base {
  static std::unique_ptr<F> function;
  struct init_t {
    init_t() {
      function = std::make_unique<F>();
      static_formations.emplace_back(Overmind::entry{I, C, R, function.get()});
    }
    // Ensure static initialisation.
    void operator()() {}
  };

  static init_t init_v;
  static void init() {
    init_v();
  }
};
template <std::int32_t I, typename F, std::int32_t C, std::int32_t R>
std::unique_ptr<F> formation<I, F, C, R>::function;
template <std::int32_t I, typename F, std::int32_t C, std::int32_t R>
typename formation<I, F, C, R>::init_t formation<I, F, C, R>::init_v;

struct square1 : formation<0, square1, 4> {
  void operator()() override {
    for (std::int32_t i = 1; i < 5; ++i) {
      spawn_square(i, 6, 0);
    }
  }
};
struct square2 : formation<1, square2, 11> {
  void operator()() override {
    std::int32_t r = z::rand_int(4);
    std::int32_t p1 = 2 + z::rand_int(8);
    std::int32_t p2 = 2 + z::rand_int(8);
    for (std::int32_t i = 1; i < 11; ++i) {
      if (r < 2 || i != p1) {
        spawn_square(i, 12, 1);
      }
      if (r < 2 || (r == 2 && i != 11 - p1) || (r == 3 && i != p2)) {
        spawn_square(i, 12, 2);
      }
    }
  }
};
struct square3 : formation<2, square3, 20, 24> {
  void operator()() override {
    std::int32_t r1 = z::rand_int(4);
    std::int32_t r2 = z::rand_int(4);
    std::int32_t p11 = 2 + z::rand_int(14);
    std::int32_t p12 = 2 + z::rand_int(14);
    std::int32_t p21 = 2 + z::rand_int(14);
    std::int32_t p22 = 2 + z::rand_int(14);

    for (std::int32_t i = 0; i < 18; ++i) {
      if (r1 < 2 || i != p11) {
        if (r2 < 2 || i != p21) {
          spawn_square(i, 18, 1);
        }
      }
      if (r1 < 2 || (r1 == 2 && i != 17 - p11) || (r1 == 3 && i != p12)) {
        if (r2 < 2 || (r2 == 2 && i != 17 - p21) || (r2 == 3 && i != p22)) {
          spawn_square(i, 18, 2);
        }
      }
    }
  }
};
struct square1side : formation<3, square1side, 2> {
  void operator()() override {
    std::int32_t r = z::rand_int(2);
    std::int32_t p = z::rand_int(4);

    if (p < 2) {
      for (std::int32_t i = 1; i < 5; ++i) {
        spawn_square(i, 6, 1 + r);
      }
    } else if (p == 2) {
      for (std::int32_t i = 1; i < 5; ++i) {
        spawn_square((i + r) % 2 == 0 ? i : 5 - i, 6, 1 + ((i + r) % 2));
      }
    } else
      for (std::int32_t i = 1; i < 3; ++i) {
        spawn_square(r == 0 ? i : 5 - i, 6, 0);
      }
  }
};
struct square2side : formation<4, square2side, 5> {
  void operator()() override {
    std::int32_t r = z::rand_int(2);
    std::int32_t p = z::rand_int(4);

    if (p < 2) {
      for (std::int32_t i = 1; i < 11; ++i) {
        spawn_square(i, 12, 1 + r);
      }
    } else if (p == 2) {
      for (std::int32_t i = 1; i < 11; ++i) {
        spawn_square((i + r) % 2 == 0 ? i : 11 - i, 12, 1 + ((i + r) % 2));
      }
    } else
      for (std::int32_t i = 1; i < 6; ++i) {
        spawn_square(r == 0 ? i : 11 - i, 12, 0);
      }
  }
};
struct square3side : formation<5, square3side, 10, 12> {
  void operator()() override {
    std::int32_t r = z::rand_int(2);
    std::int32_t p = z::rand_int(4);
    std::int32_t r2 = z::rand_int(2);
    std::int32_t p2 = 1 + z::rand_int(16);

    if (p < 2) {
      for (std::int32_t i = 0; i < 18; ++i) {
        if (r2 == 0 || i != p2) {
          spawn_square(i, 18, 1 + r);
        }
      }
    } else if (p == 2) {
      for (std::int32_t i = 0; i < 18; ++i) {
        spawn_square((i + r) % 2 == 0 ? i : 17 - i, 18, 1 + ((i + r) % 2));
      }
    } else
      for (std::int32_t i = 0; i < 9; ++i) {
        spawn_square(r == 0 ? i : 17 - i, 18, 0);
      }
  }
};
struct wall1 : formation<6, wall1, 5> {
  void operator()() override {
    bool dir = z::rand_int(2) != 0;
    for (std::int32_t i = 1; i < 3; ++i) {
      spawn_wall(i, 4, 0, dir);
    }
  }
};
struct wall2 : formation<7, wall2, 12> {
  void operator()() override {
    bool dir = z::rand_int(2) != 0;
    std::int32_t r = z::rand_int(4);
    std::int32_t p1 = 2 + z::rand_int(5);
    std::int32_t p2 = 2 + z::rand_int(5);
    for (std::int32_t i = 1; i < 8; ++i) {
      if (r < 2 || i != p1) {
        spawn_wall(i, 9, 1, dir);
      }
      if (r < 2 || (r == 2 && i != 8 - p1) || (r == 3 && i != p2)) {
        spawn_wall(i, 9, 2, dir);
      }
    }
  }
};
struct wall3 : formation<8, wall3, 22, 26> {
  void operator()() override {
    bool dir = z::rand_int(2) != 0;
    std::int32_t r1 = z::rand_int(4);
    std::int32_t r2 = z::rand_int(4);
    std::int32_t p11 = 1 + z::rand_int(10);
    std::int32_t p12 = 1 + z::rand_int(10);
    std::int32_t p21 = 1 + z::rand_int(10);
    std::int32_t p22 = 1 + z::rand_int(10);

    for (std::int32_t i = 0; i < 12; ++i) {
      if (r1 < 2 || i != p11) {
        if (r2 < 2 || i != p21) {
          spawn_wall(i, 12, 1, dir);
        }
      }
      if (r1 < 2 || (r1 == 2 && i != 11 - p11) || (r1 == 3 && i != p12)) {
        if (r2 < 2 || (r2 == 2 && i != 11 - p21) || (r2 == 3 && i != p22)) {
          spawn_wall(i, 12, 2, dir);
        }
      }
    }
  }
};
struct wall1side : formation<9, wall1side, 3> {
  void operator()() override {
    bool dir = z::rand_int(2) != 0;
    std::int32_t r = z::rand_int(2);
    std::int32_t p = z::rand_int(4);

    if (p < 2) {
      for (std::int32_t i = 1; i < 3; ++i) {
        spawn_wall(i, 4, 1 + r, dir);
      }
    } else if (p == 2) {
      for (std::int32_t i = 1; i < 3; ++i) {
        spawn_wall((i + r) % 2 == 0 ? i : 3 - i, 4, 1 + ((i + r) % 2), dir);
      }
    } else {
      spawn_wall(1 + r, 4, 0, dir);
    }
  }
};
struct wall2side : formation<10, wall2side, 6> {
  void operator()() override {
    bool dir = z::rand_int(2) != 0;
    std::int32_t r = z::rand_int(2);
    std::int32_t p = z::rand_int(4);

    if (p < 2) {
      for (std::int32_t i = 1; i < 8; ++i) {
        spawn_wall(i, 9, 1 + r, dir);
      }
    } else if (p == 2) {
      for (std::int32_t i = 1; i < 8; ++i) {
        spawn_wall(i, 9, 1 + ((i + r) % 2), dir);
      }
    } else
      for (std::int32_t i = 0; i < 4; ++i) {
        spawn_wall(r == 0 ? i : 8 - i, 9, 0, dir);
      }
  }
};
struct wall3side : formation<11, wall3side, 11, 13> {
  void operator()() override {
    bool dir = z::rand_int(2) != 0;
    std::int32_t r = z::rand_int(2);
    std::int32_t p = z::rand_int(4);
    std::int32_t r2 = z::rand_int(2);
    std::int32_t p2 = 1 + z::rand_int(10);

    if (p < 2) {
      for (std::int32_t i = 0; i < 12; ++i) {
        if (r2 == 0 || i != p2) {
          spawn_wall(i, 12, 1 + r, dir);
        }
      }
    } else if (p == 2) {
      for (std::int32_t i = 0; i < 12; ++i) {
        spawn_wall((i + r) % 2 == 0 ? i : 11 - i, 12, 1 + ((i + r) % 2), dir);
      }
    } else
      for (std::int32_t i = 0; i < 6; ++i) {
        spawn_wall(r == 0 ? i : 11 - i, 12, 0, dir);
      }
  }
};
struct follow1 : formation<12, follow1, 3> {
  void operator()() override {
    std::int32_t p = z::rand_int(3);
    if (p == 0) {
      spawn_follow(0, 3, 0);
      spawn_follow(2, 3, 0);
    } else if (p == 1) {
      spawn_follow(1, 4, 0);
      spawn_follow(2, 4, 0);
    } else {
      spawn_follow(0, 5, 0);
      spawn_follow(4, 5, 0);
    }
  }
};
struct follow2 : formation<13, follow2, 7> {
  void operator()() override {
    std::int32_t p = z::rand_int(2);
    if (p == 0) {
      for (std::int32_t i = 0; i < 8; ++i) {
        spawn_follow(i, 8, 0);
      }
    } else
      for (std::int32_t i = 0; i < 8; ++i) {
        spawn_follow(4 + i, 16, 0);
      }
  }
};
struct follow3 : formation<14, follow3, 14> {
  void operator()() override {
    std::int32_t p = z::rand_int(2);
    if (p == 0) {
      for (std::int32_t i = 0; i < 16; ++i) {
        spawn_follow(i, 16, 0);
      }
    } else
      for (std::int32_t i = 0; i < 8; ++i) {
        spawn_follow(i, 28, 0);
        spawn_follow(27 - i, 28, 0);
      }
  }
};
struct follow1side : formation<15, follow1side, 2> {
  void operator()() override {
    std::int32_t r = 1 + z::rand_int(2);
    std::int32_t p = z::rand_int(3);
    if (p == 0) {
      spawn_follow(0, 3, r);
      spawn_follow(2, 3, r);
    } else if (p == 1) {
      spawn_follow(1, 4, r);
      spawn_follow(2, 4, r);
    } else {
      spawn_follow(0, 5, r);
      spawn_follow(4, 5, r);
    }
  }
};
struct follow2side : formation<16, follow2side, 3> {
  void operator()() override {
    std::int32_t r = 1 + z::rand_int(2);
    std::int32_t p = z::rand_int(2);
    if (p == 0) {
      for (std::int32_t i = 0; i < 8; ++i) {
        spawn_follow(i, 8, r);
      }
    } else
      for (std::int32_t i = 0; i < 8; ++i) {
        spawn_follow(4 + i, 16, r);
      }
  }
};
struct follow3side : formation<17, follow3side, 7> {
  void operator()() override {
    std::int32_t r = 1 + z::rand_int(2);
    std::int32_t p = z::rand_int(2);
    if (p == 0) {
      for (std::int32_t i = 0; i < 16; ++i) {
        spawn_follow(i, 16, r);
      }
    } else
      for (std::int32_t i = 0; i < 8; ++i) {
        spawn_follow(i, 28, r);
        spawn_follow(27 - i, 28, r);
      }
  }
};
struct chaser1 : formation<18, chaser1, 4> {
  void operator()() override {
    std::int32_t p = z::rand_int(3);
    if (p == 0) {
      spawn_chaser(0, 3, 0);
      spawn_chaser(2, 3, 0);
    } else if (p == 1) {
      spawn_chaser(1, 4, 0);
      spawn_chaser(2, 4, 0);
    } else {
      spawn_chaser(0, 5, 0);
      spawn_chaser(4, 5, 0);
    }
  }
};
struct chaser2 : formation<19, chaser2, 8> {
  void operator()() override {
    std::int32_t p = z::rand_int(2);
    if (p == 0) {
      for (std::int32_t i = 0; i < 8; ++i) {
        spawn_chaser(i, 8, 0);
      }
    } else
      for (std::int32_t i = 0; i < 8; ++i) {
        spawn_chaser(4 + i, 16, 0);
      }
  }
};
struct chaser3 : formation<20, chaser3, 16> {
  void operator()() override {
    std::int32_t p = z::rand_int(2);
    if (p == 0) {
      for (std::int32_t i = 0; i < 16; ++i) {
        spawn_chaser(i, 16, 0);
      }
    } else
      for (std::int32_t i = 0; i < 8; ++i) {
        spawn_chaser(i, 28, 0);
        spawn_chaser(27 - i, 28, 0);
      }
  }
};
struct chaser4 : formation<21, chaser4, 20> {
  void operator()() override {
    for (std::int32_t i = 0; i < 22; ++i) {
      spawn_chaser(i, 22, 0);
    }
  }
};
struct chaser1side : formation<22, chaser1side, 2> {
  void operator()() override {
    std::int32_t r = 1 + z::rand_int(2);
    std::int32_t p = z::rand_int(3);
    if (p == 0) {
      spawn_chaser(0, 3, r);
      spawn_chaser(2, 3, r);
    } else if (p == 1) {
      spawn_chaser(1, 4, r);
      spawn_chaser(2, 4, r);
    } else {
      spawn_chaser(0, 5, r);
      spawn_chaser(4, 5, r);
    }
  }
};
struct chaser2side : formation<23, chaser2side, 4> {
  void operator()() override {
    std::int32_t r = 1 + z::rand_int(2);
    std::int32_t p = z::rand_int(2);
    if (p == 0) {
      for (std::int32_t i = 0; i < 8; ++i) {
        spawn_chaser(i, 8, r);
      }
    } else
      for (std::int32_t i = 0; i < 8; ++i) {
        spawn_chaser(4 + i, 16, r);
      }
  }
};
struct chaser3side : formation<24, chaser3side, 8> {
  void operator()() override {
    std::int32_t r = 1 + z::rand_int(2);
    std::int32_t p = z::rand_int(2);
    if (p == 0) {
      for (std::int32_t i = 0; i < 16; ++i) {
        spawn_chaser(i, 16, r);
      }
    } else
      for (std::int32_t i = 0; i < 8; ++i) {
        spawn_chaser(i, 28, r);
        spawn_chaser(27 - i, 28, r);
      }
  }
};
struct chaser4side : formation<25, chaser4side, 10> {
  void operator()() override {
    std::int32_t r = 1 + z::rand_int(2);
    for (std::int32_t i = 0; i < 22; ++i) {
      spawn_chaser(i, 22, r);
    }
  }
};
struct hub1 : formation<26, hub1, 6> {
  void operator()() override {
    spawn_follow_hub(1 + z::rand_int(3), 5, 0);
  }
};
struct hub2 : formation<27, hub2, 12> {
  void operator()() override {
    std::int32_t p = z::rand_int(3);
    spawn_follow_hub(p == 1 ? 2 : 1, 5, 0);
    spawn_follow_hub(p == 2 ? 2 : 3, 5, 0);
  }
};
struct hub1side : formation<28, hub1side, 3> {
  void operator()() override {
    spawn_follow_hub(1 + z::rand_int(3), 5, 1 + z::rand_int(2));
  }
};
struct hub2side : formation<29, hub2side, 6> {
  void operator()() override {
    std::int32_t r = 1 + z::rand_int(2);
    std::int32_t p = z::rand_int(3);
    spawn_follow_hub(p == 1 ? 2 : 1, 5, r);
    spawn_follow_hub(p == 2 ? 2 : 3, 5, r);
  }
};
struct mixed1 : formation<30, mixed1, 6> {
  void operator()() override {
    std::int32_t p = z::rand_int(2);
    spawn_follow(p == 0 ? 0 : 2, 4, 0);
    spawn_follow(p == 0 ? 1 : 3, 4, 0);
    spawn_chaser(p == 0 ? 2 : 0, 4, 0);
    spawn_chaser(p == 0 ? 3 : 1, 4, 0);
  }
};
struct mixed2 : formation<31, mixed2, 12> {
  void operator()() override {
    for (std::int32_t i = 0; i < 13; ++i) {
      if (i % 2) {
        spawn_follow(i, 13, 0);
      } else {
        spawn_chaser(i, 13, 0);
      }
    }
  }
};
struct mixed3 : formation<32, mixed3, 16> {
  void operator()() override {
    bool dir = z::rand_int(2) != 0;
    spawn_wall(3, 7, 0, dir);
    spawn_follow_hub(1, 7, 0);
    spawn_follow_hub(5, 7, 0);
    spawn_chaser(2, 7, 0);
    spawn_chaser(4, 7, 0);
  }
};
struct mixed4 : formation<33, mixed4, 18> {
  void operator()() override {
    spawn_square(1, 7, 0);
    spawn_square(5, 7, 0);
    spawn_follow_hub(3, 7, 0);
    spawn_chaser(2, 9, 0);
    spawn_chaser(3, 9, 0);
    spawn_chaser(5, 9, 0);
    spawn_chaser(6, 9, 0);
  }
};
struct mixed5 : formation<34, mixed5, 22, 38> {
  void operator()() override {
    spawn_follow_hub(1, 7, 0);
    spawn_tractor(3, 7, 1 + z::rand_int(2));
  }
};
struct mixed6 : formation<35, mixed6, 16, 30> {
  void operator()() override {
    spawn_follow_hub(1, 5, 0);
    spawn_shielder(3, 5, 0);
  }
};
struct mixed7 : formation<36, mixed7, 18, 16> {
  void operator()() override {
    bool dir = z::rand_int(2) != 0;
    spawn_shielder(2, 5, 0);
    spawn_wall(1, 10, 0, dir);
    spawn_wall(8, 10, 0, dir);
    spawn_chaser(2, 10, 0);
    spawn_chaser(3, 10, 0);
    spawn_chaser(4, 10, 0);
    spawn_chaser(5, 10, 0);
  }
};
struct mixed1side : formation<37, mixed1side, 3> {
  void operator()() override {
    std::int32_t r = 1 + z::rand_int(2);
    std::int32_t p = z::rand_int(2);
    spawn_follow(p == 0 ? 0 : 2, 4, r);
    spawn_follow(p == 0 ? 1 : 3, 4, r);
    spawn_chaser(p == 0 ? 2 : 0, 4, r);
    spawn_chaser(p == 0 ? 3 : 1, 4, r);
  }
};
struct mixed2side : formation<38, mixed2side, 6> {
  void operator()() override {
    std::int32_t r = z::rand_int(2);
    std::int32_t p = z::rand_int(2);
    for (std::int32_t i = 0; i < 13; ++i) {
      if (i % 2) {
        spawn_follow(i, 13, 1 + r);
      } else {
        spawn_chaser(i, 13, p == 0 ? 1 + r : 2 - r);
      }
    }
  }
};
struct mixed3side : formation<39, mixed3side, 8> {
  void operator()() override {
    bool dir = z::rand_int(2) != 0;
    std::int32_t r = 1 + z::rand_int(2);
    spawn_wall(3, 7, r, dir);
    spawn_follow_hub(1, 7, r);
    spawn_follow_hub(5, 7, r);
    spawn_chaser(2, 7, r);
    spawn_chaser(4, 7, r);
  }
};
struct mixed4side : formation<40, mixed4side, 9> {
  void operator()() override {
    std::int32_t r = 1 + z::rand_int(2);
    spawn_square(1, 7, r);
    spawn_square(5, 7, r);
    spawn_follow_hub(3, 7, r);
    spawn_chaser(2, 9, r);
    spawn_chaser(3, 9, r);
    spawn_chaser(5, 9, r);
    spawn_chaser(6, 9, r);
  }
};
struct mixed5side : formation<41, mixed5side, 19, 36> {
  void operator()() override {
    std::int32_t r = 1 + z::rand_int(2);
    spawn_follow_hub(1, 7, r);
    spawn_tractor(3, 7, r);
  }
};
struct mixed6side : formation<42, mixed6side, 8, 20> {
  void operator()() override {
    std::int32_t r = 1 + z::rand_int(2);
    spawn_follow_hub(1, 5, r);
    spawn_shielder(3, 5, r);
  }
};
struct mixed7side : formation<43, mixed7side, 9, 16> {
  void operator()() override {
    bool dir = z::rand_int(2) != 0;
    std::int32_t r = 1 + z::rand_int(2);
    spawn_shielder(2, 5, r);
    spawn_wall(1, 10, r, dir);
    spawn_wall(8, 10, r, dir);
    spawn_chaser(2, 10, r);
    spawn_chaser(3, 10, r);
    spawn_chaser(4, 10, r);
    spawn_chaser(5, 10, r);
  }
};
struct tractor1 : formation<44, tractor1, 16, 30> {
  void operator()() override {
    spawn_tractor(z::rand_int(3) + 1, 5, 1 + z::rand_int(2));
  }
};
struct tractor2 : formation<45, tractor2, 28, 46> {
  void operator()() override {
    spawn_tractor(z::rand_int(3) + 1, 5, 2);
    spawn_tractor(z::rand_int(3) + 1, 5, 1);
  }
};
struct tractor1side : formation<46, tractor1side, 16, 36> {
  void operator()() override {
    spawn_tractor(z::rand_int(7) + 1, 9, 1 + z::rand_int(2));
  }
};
struct tractor2side : formation<47, tractor2side, 14, 32> {
  void operator()() override {
    spawn_tractor(z::rand_int(5) + 1, 7, 1 + z::rand_int(2));
  }
};
struct shielder1 : formation<48, shielder1, 10, 28> {
  void operator()() override {
    spawn_shielder(z::rand_int(3) + 1, 5, 0);
  }
};
struct shielder1side : formation<49, shielder1side, 5, 22> {
  void operator()() override {
    spawn_shielder(z::rand_int(3) + 1, 5, 1 + z::rand_int(2));
  }
};

void Overmind::add_formations() {
  square1::init();
  square2::init();
  square3::init();
  square1side::init();
  square2side::init();
  square3side::init();
  wall1::init();
  wall2::init();
  wall3::init();
  wall1side::init();
  wall2side::init();
  wall3side::init();
  follow1::init();
  follow2::init();
  follow3::init();
  follow1side::init();
  follow2side::init();
  follow3side::init();
  chaser1::init();
  chaser2::init();
  chaser3::init();
  chaser4::init();
  chaser1side::init();
  chaser2side::init();
  chaser3side::init();
  chaser4side::init();
  hub1::init();
  hub2::init();
  hub1side::init();
  hub2side::init();
  mixed1::init();
  mixed2::init();
  mixed3::init();
  mixed4::init();
  mixed5::init();
  mixed6::init();
  mixed7::init();
  mixed1side::init();
  mixed2side::init();
  mixed3side::init();
  mixed4side::init();
  mixed5side::init();
  mixed6side::init();
  mixed7side::init();
  tractor1::init();
  tractor2::init();
  tractor1side::init();
  tractor2side::init();
  shielder1::init();
  shielder1side::init();

  for (const auto& f : formation_base::static_formations) {
    formations_.emplace_back(f);
  }
  auto id_sort = [](const entry& a, const entry& b) { return a.id < b.id; };
  auto cost_sort = [](const entry& a, const entry& b) { return a.cost < b.cost; };
  // Sort by ID first for backwards compatibility.
  std::sort(formations_.begin(), formations_.end(), id_sort);
  std::stable_sort(formations_.begin(), formations_.end(), cost_sort);
}
