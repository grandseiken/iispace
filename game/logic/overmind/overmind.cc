#include "game/logic/overmind/overmind.h"
#include "game/common/math.h"
#include "game/logic/boss/boss.h"
#include "game/logic/overmind/formations.h"
#include "game/logic/overmind/spawn_context.h"
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
}  // namespace

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
    chosen[perm[row]]->function(context);
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

void Overmind::add_formations() {
  auto init = [&]<typename F>(const F&) {
    formations_.emplace_back(Overmind::entry{static_cast<std::uint32_t>(formations_.size()),
                                             F::cost, F::min_resource,
                                             +[](const SpawnContext& context) { F{}(context); }});
  };

  init(formations::square1{});
  init(formations::square2{});
  init(formations::square3{});
  init(formations::square1side{});
  init(formations::square2side{});
  init(formations::square3side{});
  init(formations::wall1{});
  init(formations::wall2{});
  init(formations::wall3{});
  init(formations::wall1side{});
  init(formations::wall2side{});
  init(formations::wall3side{});
  init(formations::follow1{});
  init(formations::follow2{});
  init(formations::follow3{});
  init(formations::follow1side{});
  init(formations::follow2side{});
  init(formations::follow3side{});
  init(formations::chaser1{});
  init(formations::chaser2{});
  init(formations::chaser3{});
  init(formations::chaser4{});
  init(formations::chaser1side{});
  init(formations::chaser2side{});
  init(formations::chaser3side{});
  init(formations::chaser4side{});
  init(formations::hub1{});
  init(formations::hub2{});
  init(formations::hub1side{});
  init(formations::hub2side{});
  init(formations::mixed1{});
  init(formations::mixed2{});
  init(formations::mixed3{});
  init(formations::mixed4{});
  init(formations::mixed5{});
  init(formations::mixed6{});
  init(formations::mixed7{});
  init(formations::mixed1side{});
  init(formations::mixed2side{});
  init(formations::mixed3side{});
  init(formations::mixed4side{});
  init(formations::mixed5side{});
  init(formations::mixed6side{});
  init(formations::mixed7side{});
  init(formations::tractor1{});
  init(formations::tractor2{});
  init(formations::tractor1side{});
  init(formations::tractor2side{});
  init(formations::shielder1{});
  init(formations::shielder1side{});

  std::stable_sort(formations_.begin(), formations_.end(),
                   [](const entry& a, const entry& b) { return a.cost < b.cost; });
}

}  // namespace ii
