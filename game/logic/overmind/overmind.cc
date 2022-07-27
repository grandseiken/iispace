#include "game/logic/overmind/overmind.h"
#include "game/common/math.h"
#include "game/logic/boss/boss.h"
#include "game/logic/enemy/enemy.h"
#include "game/logic/overmind/stars.h"
#include "game/logic/player/player.h"
#include "game/logic/ship/components.h"
#include <algorithm>

namespace ii {
namespace {
constexpr std::uint32_t kTimer = 2800;
constexpr std::uint32_t kPowerupTime = 1200;
constexpr std::uint32_t kBossRestTime = 240;

constexpr std::uint32_t kInitialPower = 16;
constexpr std::uint32_t kInitialTriggerVal = 0;
constexpr std::uint32_t kLevelsPerGroup = 4;
constexpr std::uint32_t kBaseGroupsPerBoss = 4;

enum class spawn_direction {
  kTop,
  kBottom,
  kMirrorV,
};

struct SpawnContext {
  SimInterface* sim = nullptr;
  std::uint32_t row = 0;
  std::uint32_t power = 0;
  std::uint32_t* hard_already = nullptr;

  std::uint32_t random(std::uint32_t max) const {
    return sim->random(max);
  }

  spawn_direction random_v_direction() const {
    return sim->random(2) ? spawn_direction::kTop : spawn_direction::kBottom;
  }

  vec2 spawn_point(spawn_direction d, std::uint32_t i, std::uint32_t n) const {
    n = std::max(2u, n);
    i = std::min(n - 1, i);

    bool top = d == spawn_direction::kTop;
    auto x = fixed{static_cast<std::int32_t>(top ? i : n - 1 - i)} * kSimDimensions.x /
        fixed{static_cast<std::int32_t>(n - 1)};
    auto y = top ? -((row + 1) * (fixed_c::hundredth * 16) * kSimDimensions.y)
                 : kSimDimensions.y * (1 + (row + 1) * (fixed_c::hundredth * 16));
    return vec2{x, y};
  }

  template <typename F>
  void spawn_at(spawn_direction d, std::uint32_t i, std::uint32_t n, const F& f) const {
    if (d == spawn_direction::kMirrorV || d == spawn_direction::kBottom) {
      f(spawn_point(spawn_direction::kBottom, i, n));
    }
    if (d == spawn_direction::kMirrorV || d == spawn_direction::kTop) {
      f(spawn_point(spawn_direction::kTop, i, n));
    }
  }
};

void spawn_follow(const SpawnContext& context, spawn_direction d, std::uint32_t i,
                  std::uint32_t n) {
  context.spawn_at(d, i, n, [&](const auto& v) { ii::spawn_follow(*context.sim, v); });
}

void spawn_chaser(const SpawnContext& context, spawn_direction d, std::uint32_t i,
                  std::uint32_t n) {
  context.spawn_at(d, i, n, [&](const auto& v) { ii::spawn_chaser(*context.sim, v); });
}

void spawn_square(const SpawnContext& context, spawn_direction d, std::uint32_t i,
                  std::uint32_t n) {
  context.spawn_at(d, i, n, [&](const auto& v) { ii::spawn_square(*context.sim, v); });
}

void spawn_wall(const SpawnContext& context, spawn_direction d, std::uint32_t i, std::uint32_t n,
                bool dir) {
  context.spawn_at(d, i, n, [&](const auto& v) { ii::spawn_wall(*context.sim, v, dir); });
}

void spawn_follow_hub(const SpawnContext& context, spawn_direction d, std::uint32_t i,
                      std::uint32_t n) {
  context.spawn_at(d, i, n, [&](const auto& v) {
    bool p1 = *context.hard_already + context.random(64) <
        std::min(32u, std::max(context.power, 32u) - 32u);
    if (p1) {
      *context.hard_already += 2;
    }
    bool p2 = *context.hard_already + context.random(64) <
        std::min(32u, std::max(context.power, 40u) - 40u);
    if (p2) {
      *context.hard_already += 2;
    }
    ii::spawn_follow_hub(*context.sim, v, p1, p2);
  });
}

void spawn_shielder(const SpawnContext& context, spawn_direction d, std::uint32_t i,
                    std::uint32_t n) {
  context.spawn_at(d, i, n, [&](const auto& v) {
    bool p = *context.hard_already + context.random(64) <
        std::min(32u, std::max(context.power, 36u) - 36u);
    if (p) {
      *context.hard_already += 3;
    }
    ii::spawn_shielder(*context.sim, v, p);
  });
}

void spawn_tractor(const SpawnContext& context, spawn_direction d, std::uint32_t i,
                   std::uint32_t n) {
  context.spawn_at(d, i, n, [&](const auto& v) {
    bool p = *context.hard_already + context.random(64) <
        std::min(32u, std::max(context.power, 46u) - 46u);
    if (p) {
      *context.hard_already += 4;
    }
    ii::spawn_tractor(*context.sim, v, p);
  });
}

}  // namespace

class formation_base {
public:
  virtual ~formation_base() = default;
  virtual void operator()(const SpawnContext& context) const = 0;
};

Overmind::Overmind(SimInterface& sim) : sim_{sim}, stars_{std::make_unique<Stars>()} {
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

  if (sim_.conditions().mode == game_mode::kBoss) {
    return;
  }
  power_ = kInitialPower + 2 - std::min(4u, sim_.player_count()) * 2;
  if (sim_.conditions().mode == game_mode::kHard) {
    power_ += 20;
    waves_total_ = 15;
  }
  timer_ = kPowerupTime;
  boss_rest_timer_ = kBossRestTime / 8;
}

Overmind::~Overmind() = default;

void Overmind::update() {
  stars_->update(sim_);

  std::uint32_t total_enemy_threat = 0;
  sim_.index().iterate<Enemy>([&](const Enemy& e) { total_enemy_threat += e.threat_value; });

  if (sim_.conditions().mode == game_mode::kBoss) {
    if (!total_enemy_threat) {
      stars_->change(sim_);
      if (boss_mod_bosses_ < 6) {
        for (std::uint32_t i = 0; boss_mod_bosses_ && i < sim_.player_count(); ++i) {
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
  auto trigger_stage = groups_mod_ + boss_cycles + 2 * (sim_.conditions().mode == game_mode::kHard);
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
    stars_->change(sim_);

    if (is_boss_level_) {
      ++boss_mod_bosses_;
      is_boss_level_ = false;
      if (bosses_to_go_ <= 0) {
        spawn_boss_reward();
        ++boss_mod_fights_;
        power_ += 2;
        power_ -= 2 * std::min(4u, sim_.player_count());
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

void Overmind::render() const {
  stars_->render(sim_);
}

std::uint32_t Overmind::get_killed_bosses() const {
  return boss_mod_bosses_ ? boss_mod_bosses_ - 1 : 0;
}

std::optional<std::uint32_t> Overmind::get_timer() const {
  if (is_boss_level_ || kTimer < timer_) {
    return std::nullopt;
  }
  return kTimer - timer_;
}

void Overmind::spawn_powerup() {
  if (sim_.index().count<PowerupTag>() >= 4) {
    return;
  }

  powerup_mod_ = (1 + powerup_mod_) % 4;
  if (powerup_mod_ == 0) {
    ++lives_target_;
  }

  auto r = sim_.random(4);
  vec2 v = r == 0 ? vec2{-kSimDimensions.x, kSimDimensions.y / 2}
      : r == 1    ? vec2{kSimDimensions.x * 2, kSimDimensions.y / 2}
      : r == 2    ? vec2{kSimDimensions.x / 2, -kSimDimensions.y}
                  : vec2{kSimDimensions.x / 2, kSimDimensions.y * 2};

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
                    r == 0       ? powerup_type::kBomb
                        : r == 1 ? powerup_type::kMagicShots
                        : r == 2 ? powerup_type::kShield
                                 : (++lives_spawned_, powerup_type::kExtraLife));
}

void Overmind::spawn_boss_reward() {
  auto r = sim_.random(4);
  vec2 v = r == 0 ? vec2{-kSimDimensions.x / 4, kSimDimensions.y / 2}
      : r == 1    ? vec2{kSimDimensions.x + kSimDimensions.x / 4, kSimDimensions.y / 2}
      : r == 2    ? vec2{kSimDimensions.x / 2, -kSimDimensions.y / 4}
                  : vec2{kSimDimensions.x / 2, kSimDimensions.y + kSimDimensions.y / 4};

  ii::spawn_powerup(sim_, v, powerup_type::kExtraLife);
  if (sim_.conditions().mode != game_mode::kBoss) {
    spawn_powerup();
  }
}

void Overmind::wave() {
  if (sim_.conditions().mode == game_mode::kFast) {
    for (std::uint32_t i = 0; i < sim_.random(7); ++i) {
      sim_.random(1);
    }
  }
  if (sim_.conditions().mode == game_mode::kWhat) {
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
    SpawnContext context;
    context.sim = &sim_;
    context.row = perm[row];
    context.power = power_;
    context.hard_already = &hard_already_;
    chosen[perm[row]]->function->operator()(context);
  }
}

void Overmind::boss() {
  auto cycle = (sim_.conditions().mode == game_mode::kHard) + boss_mod_bosses_ / 2;
  bool secret_chance =
      (sim_.conditions().mode != game_mode::kNormal && sim_.conditions().mode != game_mode::kBoss)
      ? (boss_mod_fights_ > 1       ? sim_.random(4) == 0
             : boss_mod_fights_ > 0 ? sim_.random(8) == 0
                                    : false)
      : (boss_mod_fights_ > 2       ? sim_.random(4) == 0
             : boss_mod_fights_ > 1 ? sim_.random(6) == 0
                                    : false);

  if (+(sim_.conditions().flags & initial_conditions::flag::kLegacy_CanFaceSecretBoss) &&
      bosses_to_go_ == 0 && boss_mod_secret_ == 0 && secret_chance) {
    auto secret_cycle = (std::max<std::uint32_t>(
                             2u, boss_mod_bosses_ + (sim_.conditions().mode == game_mode::kHard)) -
                         2) /
        2;
    boss_mod_secret_ = 2;
    spawn_super_boss(sim_, secret_cycle);
  } else if (boss_mod_bosses_ % 2 == 0) {
    if (boss1_queue_[0] == 0) {
      spawn_big_square_boss(sim_, cycle);
    } else if (boss1_queue_[0] == 1) {
      spawn_shield_bomb_boss(sim_, cycle);
    } else {
      spawn_chaser_boss(sim_, cycle);
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
      spawn_big_square_boss(sim_, 0);
    } else if (boss1_queue_[boss] == 1) {
      spawn_shield_bomb_boss(sim_, 0);
    } else {
      spawn_chaser_boss(sim_, 0);
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
  static void init(std::vector<Overmind::entry>& formations) {
    static thread_local F formation;
    formations.emplace_back(Overmind::entry{I, C, R, &formation});
  }
};

struct square1 : formation<0, square1, 4> {
  void operator()(const SpawnContext& context) const override {
    for (std::uint32_t i = 1; i < 5; ++i) {
      spawn_square(context, spawn_direction::kMirrorV, i, 6);
    }
  }
};
struct square2 : formation<1, square2, 11> {
  void operator()(const SpawnContext& context) const override {
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
struct square3 : formation<2, square3, 20, 24> {
  void operator()(const SpawnContext& context) const override {
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
struct square1side : formation<3, square1side, 2> {
  void operator()(const SpawnContext& context) const override {
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
struct square2side : formation<4, square2side, 5> {
  void operator()(const SpawnContext& context) const override {
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
struct square3side : formation<5, square3side, 10, 12> {
  void operator()(const SpawnContext& context) const override {
    auto r = context.random(2);
    auto p = context.random(4);
    auto r2 = context.random(2);
    auto p2 = 1 + context.random(16);

    if (p < 2) {
      auto d = r ? spawn_direction::kTop : spawn_direction::kBottom;
      for (std::uint32_t i = 0; i < 18; ++i) {
        if (r2 == 0 || i != p2) {
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
struct wall1 : formation<6, wall1, 5> {
  void operator()(const SpawnContext& context) const override {
    bool dir = context.random(2) != 0;
    for (std::uint32_t i = 1; i < 3; ++i) {
      spawn_wall(context, spawn_direction::kMirrorV, i, 4, dir);
    }
  }
};
struct wall2 : formation<7, wall2, 12> {
  void operator()(const SpawnContext& context) const override {
    bool dir = context.random(2) != 0;
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
struct wall3 : formation<8, wall3, 22, 26> {
  void operator()(const SpawnContext& context) const override {
    bool dir = context.random(2) != 0;
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
struct wall1side : formation<9, wall1side, 3> {
  void operator()(const SpawnContext& context) const override {
    bool dir = context.random(2) != 0;
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
struct wall2side : formation<10, wall2side, 6> {
  void operator()(const SpawnContext& context) const override {
    bool dir = context.random(2) != 0;
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
struct wall3side : formation<11, wall3side, 11, 13> {
  void operator()(const SpawnContext& context) const override {
    bool dir = context.random(2) != 0;
    auto r = context.random(2);
    auto p = context.random(4);
    auto r2 = context.random(2);
    auto p2 = 1 + context.random(10);

    if (p < 2) {
      auto d = r ? spawn_direction::kTop : spawn_direction::kBottom;
      for (std::uint32_t i = 0; i < 12; ++i) {
        if (r2 == 0 || i != p2) {
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
struct follow1 : formation<12, follow1, 3> {
  void operator()(const SpawnContext& context) const override {
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
struct follow2 : formation<13, follow2, 7> {
  void operator()(const SpawnContext& context) const override {
    auto p = context.random(2);
    if (p == 0) {
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
struct follow3 : formation<14, follow3, 14> {
  void operator()(const SpawnContext& context) const override {
    auto p = context.random(2);
    if (p == 0) {
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
struct follow1side : formation<15, follow1side, 2> {
  void operator()(const SpawnContext& context) const override {
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
struct follow2side : formation<16, follow2side, 3> {
  void operator()(const SpawnContext& context) const override {
    auto d = context.random_v_direction();
    auto p = context.random(2);
    if (p == 0) {
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
struct follow3side : formation<17, follow3side, 7> {
  void operator()(const SpawnContext& context) const override {
    auto d = context.random_v_direction();
    auto p = context.random(2);
    if (p == 0) {
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
struct chaser1 : formation<18, chaser1, 4> {
  void operator()(const SpawnContext& context) const override {
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
struct chaser2 : formation<19, chaser2, 8> {
  void operator()(const SpawnContext& context) const override {
    auto p = context.random(2);
    if (p == 0) {
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
struct chaser3 : formation<20, chaser3, 16> {
  void operator()(const SpawnContext& context) const override {
    auto p = context.random(2);
    if (p == 0) {
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
struct chaser4 : formation<21, chaser4, 20> {
  void operator()(const SpawnContext& context) const override {
    for (std::uint32_t i = 0; i < 22; ++i) {
      spawn_chaser(context, spawn_direction::kMirrorV, i, 22);
    }
  }
};
struct chaser1side : formation<22, chaser1side, 2> {
  void operator()(const SpawnContext& context) const override {
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
struct chaser2side : formation<23, chaser2side, 4> {
  void operator()(const SpawnContext& context) const override {
    auto d = context.random_v_direction();
    auto p = context.random(2);
    if (p == 0) {
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
struct chaser3side : formation<24, chaser3side, 8> {
  void operator()(const SpawnContext& context) const override {
    auto d = context.random_v_direction();
    auto p = context.random(2);
    if (p == 0) {
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
struct chaser4side : formation<25, chaser4side, 10> {
  void operator()(const SpawnContext& context) const override {
    auto d = context.random_v_direction();
    for (std::uint32_t i = 0; i < 22; ++i) {
      spawn_chaser(context, d, i, 22);
    }
  }
};
struct hub1 : formation<26, hub1, 6> {
  void operator()(const SpawnContext& context) const override {
    spawn_follow_hub(context, spawn_direction::kMirrorV, 1 + context.random(3), 5);
  }
};
struct hub2 : formation<27, hub2, 12> {
  void operator()(const SpawnContext& context) const override {
    auto p = context.random(3);
    spawn_follow_hub(context, spawn_direction::kMirrorV, p == 1 ? 2 : 1, 5);
    spawn_follow_hub(context, spawn_direction::kMirrorV, p == 2 ? 2 : 3, 5);
  }
};
struct hub1side : formation<28, hub1side, 3> {
  void operator()(const SpawnContext& context) const override {
    auto d = context.random_v_direction();
    auto p = 1 + context.random(3);
    spawn_follow_hub(context, d, p, 5);
  }
};
struct hub2side : formation<29, hub2side, 6> {
  void operator()(const SpawnContext& context) const override {
    auto d = context.random_v_direction();
    auto p = context.random(3);
    spawn_follow_hub(context, d, p == 1 ? 2 : 1, 5);
    spawn_follow_hub(context, d, p == 2 ? 2 : 3, 5);
  }
};
struct mixed1 : formation<30, mixed1, 6> {
  void operator()(const SpawnContext& context) const override {
    auto p = context.random(2);
    spawn_follow(context, spawn_direction::kMirrorV, p == 0 ? 0 : 2, 4);
    spawn_follow(context, spawn_direction::kMirrorV, p == 0 ? 1 : 3, 4);
    spawn_chaser(context, spawn_direction::kMirrorV, p == 0 ? 2 : 0, 4);
    spawn_chaser(context, spawn_direction::kMirrorV, p == 0 ? 3 : 1, 4);
  }
};
struct mixed2 : formation<31, mixed2, 12> {
  void operator()(const SpawnContext& context) const override {
    for (std::uint32_t i = 0; i < 13; ++i) {
      if (i % 2) {
        spawn_follow(context, spawn_direction::kMirrorV, i, 13);
      } else {
        spawn_chaser(context, spawn_direction::kMirrorV, i, 13);
      }
    }
  }
};
struct mixed3 : formation<32, mixed3, 16> {
  void operator()(const SpawnContext& context) const override {
    bool dir = context.random(2) != 0;
    spawn_wall(context, spawn_direction::kMirrorV, 3, 7, dir);
    spawn_follow_hub(context, spawn_direction::kMirrorV, 1, 7);
    spawn_follow_hub(context, spawn_direction::kMirrorV, 5, 7);
    spawn_chaser(context, spawn_direction::kMirrorV, 2, 7);
    spawn_chaser(context, spawn_direction::kMirrorV, 4, 7);
  }
};
struct mixed4 : formation<33, mixed4, 18> {
  void operator()(const SpawnContext& context) const override {
    spawn_square(context, spawn_direction::kMirrorV, 1, 7);
    spawn_square(context, spawn_direction::kMirrorV, 5, 7);
    spawn_follow_hub(context, spawn_direction::kMirrorV, 3, 7);
    spawn_chaser(context, spawn_direction::kMirrorV, 2, 9);
    spawn_chaser(context, spawn_direction::kMirrorV, 3, 9);
    spawn_chaser(context, spawn_direction::kMirrorV, 5, 9);
    spawn_chaser(context, spawn_direction::kMirrorV, 6, 9);
  }
};
struct mixed5 : formation<34, mixed5, 22, 38> {
  void operator()(const SpawnContext& context) const override {
    spawn_follow_hub(context, spawn_direction::kMirrorV, 1, 7);
    auto d = context.random_v_direction();
    spawn_tractor(context, d, 3, 7);
  }
};
struct mixed6 : formation<35, mixed6, 16, 30> {
  void operator()(const SpawnContext& context) const override {
    spawn_follow_hub(context, spawn_direction::kMirrorV, 1, 5);
    spawn_shielder(context, spawn_direction::kMirrorV, 3, 5);
  }
};
struct mixed7 : formation<36, mixed7, 18, 16> {
  void operator()(const SpawnContext& context) const override {
    bool dir = context.random(2) != 0;
    spawn_shielder(context, spawn_direction::kMirrorV, 2, 5);
    spawn_wall(context, spawn_direction::kMirrorV, 1, 10, dir);
    spawn_wall(context, spawn_direction::kMirrorV, 8, 10, dir);
    spawn_chaser(context, spawn_direction::kMirrorV, 2, 10);
    spawn_chaser(context, spawn_direction::kMirrorV, 3, 10);
    spawn_chaser(context, spawn_direction::kMirrorV, 4, 10);
    spawn_chaser(context, spawn_direction::kMirrorV, 5, 10);
  }
};
struct mixed1side : formation<37, mixed1side, 3> {
  void operator()(const SpawnContext& context) const override {
    auto d = context.random_v_direction();
    auto p = context.random(2);
    spawn_follow(context, d, p == 0 ? 0 : 2, 4);
    spawn_follow(context, d, p == 0 ? 1 : 3, 4);
    spawn_chaser(context, d, p == 0 ? 2 : 0, 4);
    spawn_chaser(context, d, p == 0 ? 3 : 1, 4);
  }
};
struct mixed2side : formation<38, mixed2side, 6> {
  void operator()(const SpawnContext& context) const override {
    auto r = context.random(2);
    auto p = context.random(2);
    auto d0 = r ? spawn_direction::kTop : spawn_direction::kBottom;
    auto d1 = r ? spawn_direction::kBottom : spawn_direction::kTop;
    for (std::uint32_t i = 0; i < 13; ++i) {
      if (i % 2) {
        spawn_follow(context, d0, i, 13);
      } else {
        spawn_chaser(context, p == 0 ? d0 : d1, i, 13);
      }
    }
  }
};
struct mixed3side : formation<39, mixed3side, 8> {
  void operator()(const SpawnContext& context) const override {
    bool dir = context.random(2) != 0;
    auto d = context.random_v_direction();
    spawn_wall(context, d, 3, 7, dir);
    spawn_follow_hub(context, d, 1, 7);
    spawn_follow_hub(context, d, 5, 7);
    spawn_chaser(context, d, 2, 7);
    spawn_chaser(context, d, 4, 7);
  }
};
struct mixed4side : formation<40, mixed4side, 9> {
  void operator()(const SpawnContext& context) const override {
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
struct mixed5side : formation<41, mixed5side, 19, 36> {
  void operator()(const SpawnContext& context) const override {
    auto d = context.random_v_direction();
    spawn_follow_hub(context, d, 1, 7);
    spawn_tractor(context, d, 3, 7);
  }
};
struct mixed6side : formation<42, mixed6side, 8, 20> {
  void operator()(const SpawnContext& context) const override {
    auto d = context.random_v_direction();
    spawn_follow_hub(context, d, 1, 5);
    spawn_shielder(context, d, 3, 5);
  }
};
struct mixed7side : formation<43, mixed7side, 9, 16> {
  void operator()(const SpawnContext& context) const override {
    bool dir = context.random(2);
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
struct tractor1 : formation<44, tractor1, 16, 30> {
  void operator()(const SpawnContext& context) const override {
    auto d = context.random_v_direction();
    auto p = context.random(3) + 1;
    spawn_tractor(context, d, p, 5);
  }
};
struct tractor2 : formation<45, tractor2, 28, 46> {
  void operator()(const SpawnContext& context) const override {
    spawn_tractor(context, spawn_direction::kTop, context.random(3) + 1, 5);
    spawn_tractor(context, spawn_direction::kBottom, context.random(3) + 1, 5);
  }
};
struct tractor1side : formation<46, tractor1side, 16, 36> {
  void operator()(const SpawnContext& context) const override {
    auto d = context.random_v_direction();
    auto p = context.random(7) + 1;
    spawn_tractor(context, d, p, 9);
  }
};
struct tractor2side : formation<47, tractor2side, 14, 32> {
  void operator()(const SpawnContext& context) const override {
    auto d = context.random_v_direction();
    auto p = context.random(5) + 1;
    spawn_tractor(context, d, p, 7);
  }
};
struct shielder1 : formation<48, shielder1, 10, 28> {
  void operator()(const SpawnContext& context) const override {
    spawn_shielder(context, spawn_direction::kMirrorV, context.random(3) + 1, 5);
  }
};
struct shielder1side : formation<49, shielder1side, 5, 22> {
  void operator()(const SpawnContext& context) const override {
    auto d = context.random_v_direction();
    auto p = context.random(3) + 1;
    spawn_shielder(context, d, p, 5);
  }
};

template <typename F>
void init(std::vector<Overmind::entry>& entries) {
  entries.emplace_back(F::init());
}

void Overmind::add_formations() {
  square1::init(formations_);
  square2::init(formations_);
  square3::init(formations_);
  square1side::init(formations_);
  square2side::init(formations_);
  square3side::init(formations_);
  wall1::init(formations_);
  wall2::init(formations_);
  wall3::init(formations_);
  wall1side::init(formations_);
  wall2side::init(formations_);
  wall3side::init(formations_);
  follow1::init(formations_);
  follow2::init(formations_);
  follow3::init(formations_);
  follow1side::init(formations_);
  follow2side::init(formations_);
  follow3side::init(formations_);
  chaser1::init(formations_);
  chaser2::init(formations_);
  chaser3::init(formations_);
  chaser4::init(formations_);
  chaser1side::init(formations_);
  chaser2side::init(formations_);
  chaser3side::init(formations_);
  chaser4side::init(formations_);
  hub1::init(formations_);
  hub2::init(formations_);
  hub1side::init(formations_);
  hub2side::init(formations_);
  mixed1::init(formations_);
  mixed2::init(formations_);
  mixed3::init(formations_);
  mixed4::init(formations_);
  mixed5::init(formations_);
  mixed6::init(formations_);
  mixed7::init(formations_);
  mixed1side::init(formations_);
  mixed2side::init(formations_);
  mixed3side::init(formations_);
  mixed4side::init(formations_);
  mixed5side::init(formations_);
  mixed6side::init(formations_);
  mixed7side::init(formations_);
  tractor1::init(formations_);
  tractor2::init(formations_);
  tractor1side::init(formations_);
  tractor2side::init(formations_);
  shielder1::init(formations_);
  shielder1side::init(formations_);

  auto id_sort = [](const entry& a, const entry& b) { return a.id < b.id; };
  auto cost_sort = [](const entry& a, const entry& b) { return a.cost < b.cost; };
  // Sort by ID first for backwards compatibility.
  std::sort(formations_.begin(), formations_.end(), id_sort);
  std::stable_sort(formations_.begin(), formations_.end(), cost_sort);
}

}  // namespace ii
