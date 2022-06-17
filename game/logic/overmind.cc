#include "game/logic/overmind.h"
#include "game/common/math.h"
#include "game/logic/boss/boss.h"
#include "game/logic/enemy/enemy.h"
#include "game/logic/player.h"
#include "game/logic/stars.h"
#include <algorithm>

namespace {
constexpr std::uint32_t kTimer = 2800;
constexpr std::uint32_t kPowerupTime = 1200;
constexpr std::uint32_t kBossRestTime = 240;

constexpr std::uint32_t kInitialPower = 16;
constexpr std::uint32_t kInitialTriggerVal = 0;
constexpr std::uint32_t kLevelsPerGroup = 4;
constexpr std::uint32_t kBaseGroupsPerBoss = 4;
}  // namespace

class formation_base {
public:
  static std::vector<Overmind::entry> static_formations;

  virtual ~formation_base() {}
  virtual void operator()() = 0;

  void operator()(ii::SimInterface* sim, std::uint32_t row, std::uint32_t power,
                  std::uint32_t* hard_already) {
    sim_ = sim;
    row_ = row;
    power_ = power;
    hard_already_ = hard_already;
    operator()();
  }

  vec2 spawn_point(bool top, std::uint32_t num, std::uint32_t div) {
    div = std::max(2u, div);
    num = std::min(div - 1, num);

    fixed x = fixed{static_cast<std::int32_t>(top ? num : div - 1 - num)} * ii::kSimDimensions.x /
        fixed{static_cast<std::int32_t>(div - 1)};
    fixed y = top ? -((row_ + 1) * (fixed_c::hundredth * 16) * ii::kSimDimensions.y)
                  : ii::kSimDimensions.y * (1 + (row_ + 1) * (fixed_c::hundredth * 16));
    return vec2{x, y};
  }

  void spawn_follow(std::uint32_t num, std::uint32_t div, std::uint32_t side) {
    do_sides(side, [&](bool b) { ii::spawn_follow(*sim_, spawn_point(b, num, div)); });
  }

  void spawn_chaser(std::uint32_t num, std::uint32_t div, std::uint32_t side) {
    do_sides(side, [&](bool b) { ii::spawn_chaser(*sim_, spawn_point(b, num, div)); });
  }

  void spawn_square(std::uint32_t num, std::uint32_t div, std::uint32_t side) {
    do_sides(side, [&](bool b) { ii::spawn_square(*sim_, spawn_point(b, num, div)); });
  }

  void spawn_wall(std::uint32_t num, std::uint32_t div, std::uint32_t side, bool dir) {
    do_sides(side, [&](bool b) { ii::spawn_wall(*sim_, spawn_point(b, num, div), dir); });
  }

  void spawn_follow_hub(std::uint32_t num, std::uint32_t div, std::uint32_t side) {
    do_sides(side, [&](bool b) {
      bool p1 = *hard_already_ + sim_->random(64) < std::min(32u, std::max(power_, 32u) - 32u);
      if (p1) {
        *hard_already_ += 2;
      }
      bool p2 = *hard_already_ + sim_->random(64) < std::min(32u, std::max(power_, 40u) - 40u);
      if (p2) {
        *hard_already_ += 2;
      }
      ii::spawn_follow_hub(*sim_, spawn_point(b, num, div), p1, p2);
    });
  }

  void spawn_shielder(std::uint32_t num, std::uint32_t div, std::uint32_t side) {
    do_sides(side, [&](bool b) {
      bool p = *hard_already_ + sim_->random(64) < std::min(32u, std::max(power_, 36u) - 36u);
      if (p) {
        *hard_already_ += 3;
      }
      ii::spawn_shielder(*sim_, spawn_point(b, num, div), p);
    });
  }

  void spawn_tractor(std::uint32_t num, std::uint32_t div, std::uint32_t side) {
    do_sides(side, [&](bool b) {
      bool p = *hard_already_ + sim_->random(64) < std::min(32u, std::max(power_, 46u) - 46u);
      if (p) {
        *hard_already_ += 4;
      }
      ii::spawn_tractor(*sim_, spawn_point(b, num, div), p);
    });
  }

protected:
  ii::SimInterface* sim_ = nullptr;

private:
  template <typename F>
  void do_sides(std::uint32_t side, const F& f) {
    if (side == 0 || side == 1) {
      f(false);
    }
    if (side == 0 || side == 2) {
      f(true);
    }
  }

  std::uint32_t row_ = 0;
  std::uint32_t power_ = 0;
  std::uint32_t* hard_already_ = nullptr;
};

std::vector<Overmind::entry> formation_base::static_formations;

Overmind::Overmind(ii::SimInterface& sim) : sim_{sim} {
  add_formations();

  auto queue = [this] {
    auto a = sim_.random(3);
    auto b = sim_.random(2);
    if (a == 0 || (a == 1 && b == 1)) {
      ++b;
    }
    auto c = (a + b == 3) ? 0u : (a + b == 2) ? 1u : 2u;
    return std::vector<std::uint32_t>{a, b, c};
  };
  boss1_queue_ = queue();
  boss2_queue_ = queue();

  if (sim_.conditions().mode == ii::game_mode::kBoss) {
    return;
  }
  power_ = kInitialPower + 2 - sim_.player_count() * 2;
  if (sim_.conditions().mode == ii::game_mode::kHard) {
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
  Stars::update(sim_);

  std::uint32_t total_enemy_threat = 0;
  sim_.index().iterate<ii::Enemy>(
      [&](const ii::Enemy& e) { total_enemy_threat += e.threat_value; });

  if (sim_.conditions().mode == ii::game_mode::kBoss) {
    if (!total_enemy_threat) {
      Stars::change(sim_);
      if (boss_mod_bosses_ < 6) {
        if (boss_mod_bosses_)
          for (std::uint32_t i = 0; i < sim_.player_count(); ++i) {
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

  auto boss_cycles = boss_mod_fights_;
  auto trigger_stage =
      groups_mod_ + boss_cycles + 2 * (sim_.conditions().mode == ii::game_mode::kHard);
  auto trigger_val = kInitialTriggerVal;
  for (std::uint32_t i = 0; i < trigger_stage; ++i) {
    trigger_val += i < 2 ? 4 : i + sim_.player_count() < 7 ? 3 : 2;
  }
  if (trigger_val < 0 || is_boss_level_ || is_boss_next_) {
    trigger_val = 0;
  }

  if ((timer_ > kTimer && !is_boss_level_ && !is_boss_next_) || total_enemy_threat <= trigger_val) {
    if (timer_ < kPowerupTime && !is_boss_level_) {
      spawn_powerup();
    }
    timer_ = 0;
    Stars::change(sim_);

    if (is_boss_level_) {
      ++boss_mod_bosses_;
      is_boss_level_ = false;
      if (bosses_to_go_ <= 0) {
        spawn_boss_reward();
        ++boss_mod_fights_;
        power_ += 2;
        power_ -= 2 * sim_.player_count();
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
    if (groups_mod_ >= kBaseGroupsPerBoss + sim_.player_count()) {
      groups_mod_ = 0;
      is_boss_next_ = true;
      bosses_to_go_ = boss_mod_fights_ >= 4 ? 3 : boss_mod_fights_ >= 2 ? 2 : 1;
    }
  }
}

std::uint32_t Overmind::get_killed_bosses() const {
  return boss_mod_bosses_ ? boss_mod_bosses_ - 1 : 0;
}

std::uint32_t Overmind::get_elapsed_time() const {
  return elapsed_time_;
}

std::optional<std::uint32_t> Overmind::get_timer() const {
  if (is_boss_level_ || kTimer < timer_) {
    return std::nullopt;
  }
  return kTimer - timer_;
}

void Overmind::spawn_powerup() {
  if (sim_.count_ships(ii::ship_flag::kPowerup) >= 4) {
    return;
  }

  powerup_mod_ = (1 + powerup_mod_) % 4;
  if (powerup_mod_ == 0) {
    ++lives_target_;
  }

  auto r = sim_.random(4);
  vec2 v = r == 0 ? vec2{-ii::kSimDimensions.x, ii::kSimDimensions.y / 2}
      : r == 1    ? vec2{ii::kSimDimensions.x * 2, ii::kSimDimensions.y / 2}
      : r == 2    ? vec2{ii::kSimDimensions.x / 2, -ii::kSimDimensions.y}
                  : vec2{ii::kSimDimensions.x / 2, ii::kSimDimensions.y * 2};

  std::uint32_t m = 4;
  if (sim_.player_count() > sim_.get_lives()) {
    m += sim_.player_count() - sim_.get_lives();
  }
  if (!sim_.get_lives()) {
    m += sim_.killed_players();
  }
  if (lives_target_ > lives_spawned_) {
    m += lives_target_ - lives_spawned_;
  }
  if (sim_.killed_players() == 0 && lives_target_ + 1 < lives_spawned_) {
    m = 3;
  }

  r = sim_.random(m);
  ii::spawn_powerup(sim_, v,
                    r == 0       ? ii::powerup_type::kBomb
                        : r == 1 ? ii::powerup_type::kMagicShots
                        : r == 2 ? ii::powerup_type::kShield
                                 : (++lives_spawned_, ii::powerup_type::kExtraLife));
}

void Overmind::spawn_boss_reward() {
  auto r = sim_.random(4);
  vec2 v = r == 0 ? vec2{-ii::kSimDimensions.x / 4, ii::kSimDimensions.y / 2}
      : r == 1    ? vec2{ii::kSimDimensions.x + ii::kSimDimensions.x / 4, ii::kSimDimensions.y / 2}
      : r == 2    ? vec2{ii::kSimDimensions.x / 2, -ii::kSimDimensions.y / 4}
                  : vec2{ii::kSimDimensions.x / 2, ii::kSimDimensions.y + ii::kSimDimensions.y / 4};

  ii::spawn_powerup(sim_, v, ii::powerup_type::kExtraLife);
  if (sim_.conditions().mode != ii::game_mode::kBoss) {
    spawn_powerup();
  }
}

void Overmind::wave() {
  if (sim_.conditions().mode == ii::game_mode::kFast) {
    for (std::uint32_t i = 0; i < sim_.random(7); ++i) {
      sim_.random(1);
    }
  }
  if (sim_.conditions().mode == ii::game_mode::kWhat) {
    for (std::uint32_t i = 0; i < sim_.random(11); ++i) {
      sim_.random(1);
    }
  }

  auto resources = power_;
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
    std::uint32_t n = max == 1 ? 0 : sim_.random(max);

    chosen.insert(chosen.begin() + sim_.random(chosen.size() + 1), valid[n]);
    resources -= valid[n]->cost;
  }

  // This permutation is bugged and does nothing but backwards compatibility.
  std::vector<std::size_t> perm;
  for (std::size_t i = 0; i < chosen.size(); ++i) {
    perm.push_back(i);
  }
  for (std::size_t i = 0; i < chosen.size() - 1; ++i) {
    std::swap(perm[i], perm[i + sim_.random(chosen.size() - i)]);
  }
  hard_already_ = 0;
  for (std::size_t row = 0; row < chosen.size(); ++row) {
    chosen[perm[row]]->function->operator()(&sim_, perm[row], power_, &hard_already_);
  }
}

void Overmind::boss() {
  auto cycle = (sim_.conditions().mode == ii::game_mode::kHard) + boss_mod_bosses_ / 2;
  bool secret_chance = (sim_.conditions().mode != ii::game_mode::kNormal &&
                        sim_.conditions().mode != ii::game_mode::kBoss)
      ? (boss_mod_fights_ > 1       ? sim_.random(4) == 0
             : boss_mod_fights_ > 0 ? sim_.random(8) == 0
                                    : false)
      : (boss_mod_fights_ > 2       ? sim_.random(4) == 0
             : boss_mod_fights_ > 1 ? sim_.random(6) == 0
                                    : false);

  if (sim_.conditions().can_face_secret_boss && bosses_to_go_ == 0 && boss_mod_secret_ == 0 &&
      secret_chance) {
    auto secret_cycle =
        (std::max<std::uint32_t>(
             2u, boss_mod_bosses_ + (sim_.conditions().mode == ii::game_mode::kHard)) -
         2) /
        2;
    boss_mod_secret_ = 2;
    ii::spawn_super_boss(sim_, cycle);
  } else if (boss_mod_bosses_ % 2 == 0) {
    if (boss1_queue_[0] == 0) {
      ii::spawn_big_square_boss(sim_, cycle);
    } else if (boss1_queue_[0] == 1) {
      ii::spawn_shield_bomb_boss(sim_, cycle);
    } else {
      ii::spawn_chaser_boss(sim_, cycle);
    }
    boss1_queue_.push_back(boss1_queue_.front());
    boss1_queue_.erase(boss1_queue_.begin());
  } else {
    if (boss_mod_secret_ > 0) {
      --boss_mod_secret_;
    }
    if (boss2_queue_[0] == 0) {
      spawn_tractor_boss(sim_, cycle);
    } else if (boss2_queue_[0] == 1) {
      spawn_ghost_boss(sim_, cycle);
    } else {
      spawn_death_ray_boss(sim_, cycle);
    }
    boss2_queue_.push_back(boss2_queue_.front());
    boss2_queue_.erase(boss2_queue_.begin());
  }
}

void Overmind::boss_mode_boss() {
  auto boss = boss_mod_bosses_;
  if (boss_mod_bosses_ < 3) {
    if (boss1_queue_[boss] == 0) {
      ii::spawn_big_square_boss(sim_, 0);
    } else if (boss1_queue_[boss] == 1) {
      ii::spawn_shield_bomb_boss(sim_, 0);
    } else {
      ii::spawn_chaser_boss(sim_, 0);
    }
  } else {
    boss = boss - 3;
    if (boss2_queue_[boss] == 0) {
      spawn_tractor_boss(sim_, 0);
    } else if (boss2_queue_[boss] == 1) {
      spawn_ghost_boss(sim_, 0);
    } else {
      spawn_death_ray_boss(sim_, 0);
    }
  }
}

// Formations
//------------------------------
template <std::uint32_t I, typename F, std::uint32_t C, std::uint32_t R = 0>
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
template <std::uint32_t I, typename F, std::uint32_t C, std::uint32_t R>
std::unique_ptr<F> formation<I, F, C, R>::function;
template <std::uint32_t I, typename F, std::uint32_t C, std::uint32_t R>
typename formation<I, F, C, R>::init_t formation<I, F, C, R>::init_v;

struct square1 : formation<0, square1, 4> {
  void operator()() override {
    for (std::uint32_t i = 1; i < 5; ++i) {
      spawn_square(i, 6, 0);
    }
  }
};
struct square2 : formation<1, square2, 11> {
  void operator()() override {
    auto r = sim_->random(4);
    auto p1 = 2 + sim_->random(8);
    auto p2 = 2 + sim_->random(8);
    for (std::uint32_t i = 1; i < 11; ++i) {
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
    auto r1 = sim_->random(4);
    auto r2 = sim_->random(4);
    auto p11 = 2 + sim_->random(14);
    auto p12 = 2 + sim_->random(14);
    auto p21 = 2 + sim_->random(14);
    auto p22 = 2 + sim_->random(14);

    for (std::uint32_t i = 0; i < 18; ++i) {
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
    auto r = sim_->random(2);
    auto p = sim_->random(4);

    if (p < 2) {
      for (std::uint32_t i = 1; i < 5; ++i) {
        spawn_square(i, 6, 1 + r);
      }
    } else if (p == 2) {
      for (std::uint32_t i = 1; i < 5; ++i) {
        spawn_square((i + r) % 2 == 0 ? i : 5 - i, 6, 1 + ((i + r) % 2));
      }
    } else
      for (std::uint32_t i = 1; i < 3; ++i) {
        spawn_square(r == 0 ? i : 5 - i, 6, 0);
      }
  }
};
struct square2side : formation<4, square2side, 5> {
  void operator()() override {
    auto r = sim_->random(2);
    auto p = sim_->random(4);

    if (p < 2) {
      for (std::uint32_t i = 1; i < 11; ++i) {
        spawn_square(i, 12, 1 + r);
      }
    } else if (p == 2) {
      for (std::uint32_t i = 1; i < 11; ++i) {
        spawn_square((i + r) % 2 == 0 ? i : 11 - i, 12, 1 + ((i + r) % 2));
      }
    } else
      for (std::uint32_t i = 1; i < 6; ++i) {
        spawn_square(r == 0 ? i : 11 - i, 12, 0);
      }
  }
};
struct square3side : formation<5, square3side, 10, 12> {
  void operator()() override {
    auto r = sim_->random(2);
    auto p = sim_->random(4);
    auto r2 = sim_->random(2);
    auto p2 = 1 + sim_->random(16);

    if (p < 2) {
      for (std::uint32_t i = 0; i < 18; ++i) {
        if (r2 == 0 || i != p2) {
          spawn_square(i, 18, 1 + r);
        }
      }
    } else if (p == 2) {
      for (std::uint32_t i = 0; i < 18; ++i) {
        spawn_square((i + r) % 2 == 0 ? i : 17 - i, 18, 1 + ((i + r) % 2));
      }
    } else
      for (std::uint32_t i = 0; i < 9; ++i) {
        spawn_square(r == 0 ? i : 17 - i, 18, 0);
      }
  }
};
struct wall1 : formation<6, wall1, 5> {
  void operator()() override {
    bool dir = sim_->random(2) != 0;
    for (std::uint32_t i = 1; i < 3; ++i) {
      spawn_wall(i, 4, 0, dir);
    }
  }
};
struct wall2 : formation<7, wall2, 12> {
  void operator()() override {
    bool dir = sim_->random(2) != 0;
    auto r = sim_->random(4);
    auto p1 = 2 + sim_->random(5);
    auto p2 = 2 + sim_->random(5);
    for (std::uint32_t i = 1; i < 8; ++i) {
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
    bool dir = sim_->random(2) != 0;
    auto r1 = sim_->random(4);
    auto r2 = sim_->random(4);
    auto p11 = 1 + sim_->random(10);
    auto p12 = 1 + sim_->random(10);
    auto p21 = 1 + sim_->random(10);
    auto p22 = 1 + sim_->random(10);

    for (std::uint32_t i = 0; i < 12; ++i) {
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
    bool dir = sim_->random(2) != 0;
    auto r = sim_->random(2);
    auto p = sim_->random(4);

    if (p < 2) {
      for (std::uint32_t i = 1; i < 3; ++i) {
        spawn_wall(i, 4, 1 + r, dir);
      }
    } else if (p == 2) {
      for (std::uint32_t i = 1; i < 3; ++i) {
        spawn_wall((i + r) % 2 == 0 ? i : 3 - i, 4, 1 + ((i + r) % 2), dir);
      }
    } else {
      spawn_wall(1 + r, 4, 0, dir);
    }
  }
};
struct wall2side : formation<10, wall2side, 6> {
  void operator()() override {
    bool dir = sim_->random(2) != 0;
    auto r = sim_->random(2);
    auto p = sim_->random(4);

    if (p < 2) {
      for (std::uint32_t i = 1; i < 8; ++i) {
        spawn_wall(i, 9, 1 + r, dir);
      }
    } else if (p == 2) {
      for (std::uint32_t i = 1; i < 8; ++i) {
        spawn_wall(i, 9, 1 + ((i + r) % 2), dir);
      }
    } else
      for (std::uint32_t i = 0; i < 4; ++i) {
        spawn_wall(r == 0 ? i : 8 - i, 9, 0, dir);
      }
  }
};
struct wall3side : formation<11, wall3side, 11, 13> {
  void operator()() override {
    bool dir = sim_->random(2) != 0;
    auto r = sim_->random(2);
    auto p = sim_->random(4);
    auto r2 = sim_->random(2);
    auto p2 = 1 + sim_->random(10);

    if (p < 2) {
      for (std::uint32_t i = 0; i < 12; ++i) {
        if (r2 == 0 || i != p2) {
          spawn_wall(i, 12, 1 + r, dir);
        }
      }
    } else if (p == 2) {
      for (std::uint32_t i = 0; i < 12; ++i) {
        spawn_wall((i + r) % 2 == 0 ? i : 11 - i, 12, 1 + ((i + r) % 2), dir);
      }
    } else
      for (std::uint32_t i = 0; i < 6; ++i) {
        spawn_wall(r == 0 ? i : 11 - i, 12, 0, dir);
      }
  }
};
struct follow1 : formation<12, follow1, 3> {
  void operator()() override {
    auto p = sim_->random(3);
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
    auto p = sim_->random(2);
    if (p == 0) {
      for (std::uint32_t i = 0; i < 8; ++i) {
        spawn_follow(i, 8, 0);
      }
    } else
      for (std::uint32_t i = 0; i < 8; ++i) {
        spawn_follow(4 + i, 16, 0);
      }
  }
};
struct follow3 : formation<14, follow3, 14> {
  void operator()() override {
    auto p = sim_->random(2);
    if (p == 0) {
      for (std::uint32_t i = 0; i < 16; ++i) {
        spawn_follow(i, 16, 0);
      }
    } else
      for (std::uint32_t i = 0; i < 8; ++i) {
        spawn_follow(i, 28, 0);
        spawn_follow(27 - i, 28, 0);
      }
  }
};
struct follow1side : formation<15, follow1side, 2> {
  void operator()() override {
    auto r = 1 + sim_->random(2);
    auto p = sim_->random(3);
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
    auto r = 1 + sim_->random(2);
    auto p = sim_->random(2);
    if (p == 0) {
      for (std::uint32_t i = 0; i < 8; ++i) {
        spawn_follow(i, 8, r);
      }
    } else
      for (std::uint32_t i = 0; i < 8; ++i) {
        spawn_follow(4 + i, 16, r);
      }
  }
};
struct follow3side : formation<17, follow3side, 7> {
  void operator()() override {
    auto r = 1 + sim_->random(2);
    auto p = sim_->random(2);
    if (p == 0) {
      for (std::uint32_t i = 0; i < 16; ++i) {
        spawn_follow(i, 16, r);
      }
    } else
      for (std::uint32_t i = 0; i < 8; ++i) {
        spawn_follow(i, 28, r);
        spawn_follow(27 - i, 28, r);
      }
  }
};
struct chaser1 : formation<18, chaser1, 4> {
  void operator()() override {
    auto p = sim_->random(3);
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
    auto p = sim_->random(2);
    if (p == 0) {
      for (std::uint32_t i = 0; i < 8; ++i) {
        spawn_chaser(i, 8, 0);
      }
    } else
      for (std::uint32_t i = 0; i < 8; ++i) {
        spawn_chaser(4 + i, 16, 0);
      }
  }
};
struct chaser3 : formation<20, chaser3, 16> {
  void operator()() override {
    auto p = sim_->random(2);
    if (p == 0) {
      for (std::uint32_t i = 0; i < 16; ++i) {
        spawn_chaser(i, 16, 0);
      }
    } else
      for (std::uint32_t i = 0; i < 8; ++i) {
        spawn_chaser(i, 28, 0);
        spawn_chaser(27 - i, 28, 0);
      }
  }
};
struct chaser4 : formation<21, chaser4, 20> {
  void operator()() override {
    for (std::uint32_t i = 0; i < 22; ++i) {
      spawn_chaser(i, 22, 0);
    }
  }
};
struct chaser1side : formation<22, chaser1side, 2> {
  void operator()() override {
    auto r = 1 + sim_->random(2);
    auto p = sim_->random(3);
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
    auto r = 1 + sim_->random(2);
    auto p = sim_->random(2);
    if (p == 0) {
      for (std::uint32_t i = 0; i < 8; ++i) {
        spawn_chaser(i, 8, r);
      }
    } else
      for (std::uint32_t i = 0; i < 8; ++i) {
        spawn_chaser(4 + i, 16, r);
      }
  }
};
struct chaser3side : formation<24, chaser3side, 8> {
  void operator()() override {
    auto r = 1 + sim_->random(2);
    auto p = sim_->random(2);
    if (p == 0) {
      for (std::uint32_t i = 0; i < 16; ++i) {
        spawn_chaser(i, 16, r);
      }
    } else
      for (std::uint32_t i = 0; i < 8; ++i) {
        spawn_chaser(i, 28, r);
        spawn_chaser(27 - i, 28, r);
      }
  }
};
struct chaser4side : formation<25, chaser4side, 10> {
  void operator()() override {
    auto r = 1 + sim_->random(2);
    for (std::uint32_t i = 0; i < 22; ++i) {
      spawn_chaser(i, 22, r);
    }
  }
};
struct hub1 : formation<26, hub1, 6> {
  void operator()() override {
    spawn_follow_hub(1 + sim_->random(3), 5, 0);
  }
};
struct hub2 : formation<27, hub2, 12> {
  void operator()() override {
    auto p = sim_->random(3);
    spawn_follow_hub(p == 1 ? 2 : 1, 5, 0);
    spawn_follow_hub(p == 2 ? 2 : 3, 5, 0);
  }
};
struct hub1side : formation<28, hub1side, 3> {
  void operator()() override {
    spawn_follow_hub(1 + sim_->random(3), 5, 1 + sim_->random(2));
  }
};
struct hub2side : formation<29, hub2side, 6> {
  void operator()() override {
    auto r = 1 + sim_->random(2);
    auto p = sim_->random(3);
    spawn_follow_hub(p == 1 ? 2 : 1, 5, r);
    spawn_follow_hub(p == 2 ? 2 : 3, 5, r);
  }
};
struct mixed1 : formation<30, mixed1, 6> {
  void operator()() override {
    auto p = sim_->random(2);
    spawn_follow(p == 0 ? 0 : 2, 4, 0);
    spawn_follow(p == 0 ? 1 : 3, 4, 0);
    spawn_chaser(p == 0 ? 2 : 0, 4, 0);
    spawn_chaser(p == 0 ? 3 : 1, 4, 0);
  }
};
struct mixed2 : formation<31, mixed2, 12> {
  void operator()() override {
    for (std::uint32_t i = 0; i < 13; ++i) {
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
    bool dir = sim_->random(2) != 0;
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
    spawn_tractor(3, 7, 1 + sim_->random(2));
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
    bool dir = sim_->random(2) != 0;
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
    auto r = 1 + sim_->random(2);
    auto p = sim_->random(2);
    spawn_follow(p == 0 ? 0 : 2, 4, r);
    spawn_follow(p == 0 ? 1 : 3, 4, r);
    spawn_chaser(p == 0 ? 2 : 0, 4, r);
    spawn_chaser(p == 0 ? 3 : 1, 4, r);
  }
};
struct mixed2side : formation<38, mixed2side, 6> {
  void operator()() override {
    auto r = sim_->random(2);
    auto p = sim_->random(2);
    for (std::uint32_t i = 0; i < 13; ++i) {
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
    bool dir = sim_->random(2) != 0;
    auto r = 1 + sim_->random(2);
    spawn_wall(3, 7, r, dir);
    spawn_follow_hub(1, 7, r);
    spawn_follow_hub(5, 7, r);
    spawn_chaser(2, 7, r);
    spawn_chaser(4, 7, r);
  }
};
struct mixed4side : formation<40, mixed4side, 9> {
  void operator()() override {
    auto r = 1 + sim_->random(2);
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
    auto r = 1 + sim_->random(2);
    spawn_follow_hub(1, 7, r);
    spawn_tractor(3, 7, r);
  }
};
struct mixed6side : formation<42, mixed6side, 8, 20> {
  void operator()() override {
    auto r = 1 + sim_->random(2);
    spawn_follow_hub(1, 5, r);
    spawn_shielder(3, 5, r);
  }
};
struct mixed7side : formation<43, mixed7side, 9, 16> {
  void operator()() override {
    bool dir = sim_->random(2) != 0;
    auto r = 1 + sim_->random(2);
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
    spawn_tractor(sim_->random(3) + 1, 5, 1 + sim_->random(2));
  }
};
struct tractor2 : formation<45, tractor2, 28, 46> {
  void operator()() override {
    spawn_tractor(sim_->random(3) + 1, 5, 2);
    spawn_tractor(sim_->random(3) + 1, 5, 1);
  }
};
struct tractor1side : formation<46, tractor1side, 16, 36> {
  void operator()() override {
    spawn_tractor(sim_->random(7) + 1, 9, 1 + sim_->random(2));
  }
};
struct tractor2side : formation<47, tractor2side, 14, 32> {
  void operator()() override {
    spawn_tractor(sim_->random(5) + 1, 7, 1 + sim_->random(2));
  }
};
struct shielder1 : formation<48, shielder1, 10, 28> {
  void operator()() override {
    spawn_shielder(sim_->random(3) + 1, 5, 0);
  }
};
struct shielder1side : formation<49, shielder1side, 5, 22> {
  void operator()() override {
    spawn_shielder(sim_->random(3) + 1, 5, 1 + sim_->random(2));
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
