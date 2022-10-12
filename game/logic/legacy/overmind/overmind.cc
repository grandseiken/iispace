#include "game/logic/legacy/overmind/overmind.h"
#include "game/common/math.h"
#include "game/logic/ecs/index.h"
#include "game/logic/legacy/boss/boss.h"
#include "game/logic/legacy/overmind/formations.h"
#include "game/logic/legacy/overmind/spawn_context.h"
#include "game/logic/legacy/player/powerup.h"
#include "game/logic/ship/components.h"
#include "game/logic/sim/io/conditions.h"
#include <sfn/functional.h>
#include <algorithm>
#include <cstdint>
#include <vector>

namespace ii::legacy {
namespace {

struct Overmind : ecs::component {
  static constexpr std::uint32_t kTimer = 2800;
  static constexpr std::uint32_t kPowerupTime = 1200;
  static constexpr std::uint32_t kBossRestTime = 240;

  static constexpr std::uint32_t kInitialPower = 16;
  static constexpr std::uint32_t kInitialTriggerVal = 0;
  static constexpr std::uint32_t kLevelsPerGroup = 4;
  static constexpr std::uint32_t kBaseGroupsPerBoss = 4;

  struct entry {
    std::uint32_t id = 0;
    std::uint32_t cost = 0;
    std::uint32_t min_resource = 0;
    sfn::ptr<void(const SpawnContext&)> function = nullptr;
  };

  std::uint32_t power = 0;
  std::uint32_t timer = 0;
  std::uint32_t levels_mod = 0;
  std::uint32_t groups_mod = 0;
  std::uint32_t boss_mod_bosses = 0;
  std::uint32_t boss_mod_fights = 0;
  std::uint32_t boss_mod_secret = 0;
  std::uint32_t powerup_mod = 0;
  std::uint32_t lives_spawned = 0;
  std::uint32_t lives_target = 0;
  std::uint32_t boss_rest_timer = 0;
  std::uint32_t waves_total = 0;
  std::uint32_t stars_compatibility = 0;
  bool is_boss_next = false;
  bool is_boss_level = false;

  std::vector<std::uint32_t> boss1_queue;
  std::vector<std::uint32_t> boss2_queue;
  std::uint32_t bosses_to_go = 0;
  std::vector<entry> formations;

  Overmind(SimInterface& sim) {
    add_formations();

    auto& random = sim.random(random_source::kGameSequence);
    auto queue = [&] {
      auto a = random.uint(3);
      auto b = random.uint(2);
      if (a == 0 || (a == 1 && b == 1)) {
        ++b;
      }
      auto c = (a + b == 3) ? 0u : (a + b == 2) ? 1u : 2u;
      return std::vector<std::uint32_t>{a, b, c};
    };
    boss1_queue = queue();
    boss2_queue = queue();

    if (sim.conditions().mode == game_mode::kLegacy_Boss) {
      return;
    }
    power = kInitialPower + 2 - std::min(4u, sim.player_count()) * 2;
    if (sim.conditions().mode == game_mode::kLegacy_Hard) {
      power += 20;
      waves_total = 15;
    }
    timer = kPowerupTime;
    boss_rest_timer = kBossRestTime / 8;
  }

  void update(ecs::handle h, SimInterface& sim) {
    auto& stars_random = sim.random(random_source::kLegacyAesthetic);
    auto stars_change = [&] {
      if (sim.conditions().compatibility == compatibility_level::kLegacy) {
        stars_random.fixed();
        stars_compatibility = stars_random.uint(3) + 2;
      }
      background_fx_change change;
      change.type = background_fx_type::kStars;
      sim.emit(resolve_key::reconcile(h.id(), resolve_tag::kBackgroundFx)).background_fx(change);
    };
    if (sim.conditions().compatibility == compatibility_level::kLegacy) {
      auto rc = stars_compatibility > 1 ? stars_random.uint(stars_compatibility) : 0u;
      for (std::uint32_t i = 0; i < rc; ++i) {
        auto r = stars_random.uint(12);
        if (r <= 0 && stars_random.uint(4)) {
          continue;
        }
        stars_random.uint(4);
        stars_random.fixed();
        if (r > 0) {
          stars_random.rbool();
        } else {
          stars_random.uint(4);
        }
      }
    }

    std::uint32_t total_enemy_threat = 0;
    sim.index().iterate<Enemy>([&](const Enemy& e) { total_enemy_threat += e.threat_value; });

    auto output_wave_timer = [&] {
      auto& data = *sim.global_entity().get<GlobalData>();
      if (is_boss_level || kTimer < timer) {
        data.overmind_wave_timer.reset();
      } else {
        data.overmind_wave_timer = kTimer - timer;
      }
    };

    if (sim.conditions().mode == game_mode::kLegacy_Boss) {
      if (!total_enemy_threat) {
        stars_change();
        if (boss_mod_bosses < 6) {
          for (std::uint32_t i = 0; boss_mod_bosses && i < sim.player_count(); ++i) {
            spawn_boss_reward(sim);
          }
          boss_mode_boss(sim);
        }
        if (boss_mod_bosses < 7) {
          ++boss_mod_bosses;
        }
      }
      output_wave_timer();
      return;
    }

    if (boss_rest_timer > 0) {
      --boss_rest_timer;
      output_wave_timer();
      return;
    }

    if (++timer == kPowerupTime && !is_boss_level) {
      spawn_powerup(sim);
    }

    auto boss_cycles = boss_mod_fights;
    auto trigger_stage =
        groups_mod + boss_cycles + 2 * (sim.conditions().mode == game_mode::kLegacy_Hard);
    auto trigger_val = kInitialTriggerVal;
    for (std::uint32_t i = 0; i < trigger_stage; ++i) {
      trigger_val += i < 2 ? 4 : i + sim.player_count() < 7 ? 3 : 2;
    }
    if (trigger_val < 0 || is_boss_level || is_boss_next) {
      trigger_val = 0;
    }

    if ((timer > kTimer && !is_boss_level && !is_boss_next) || total_enemy_threat <= trigger_val) {
      if (timer < kPowerupTime && !is_boss_level) {
        spawn_powerup(sim);
      }
      timer = 0;
      stars_change();

      if (is_boss_level) {
        ++boss_mod_bosses;
        is_boss_level = false;
        if (bosses_to_go <= 0) {
          spawn_boss_reward(sim);
          ++boss_mod_fights;
          power += 2;
          power -= 2 * std::min(4u, sim.player_count());
          boss_rest_timer = kBossRestTime;
          bosses_to_go = 0;
        } else {
          boss_rest_timer = kBossRestTime / 4;
        }
      } else {
        if (is_boss_next) {
          --bosses_to_go;
          is_boss_next = bosses_to_go > 0;
          is_boss_level = true;
          boss(sim);
        } else {
          wave(sim);
          if (waves_total < 5) {
            power += 2;
          } else {
            ++power;
          }
          ++waves_total;
          ++levels_mod;
        }
      }

      if (levels_mod >= kLevelsPerGroup) {
        levels_mod = 0;
        ++groups_mod;
      }
      if (groups_mod >= kBaseGroupsPerBoss + sim.player_count()) {
        groups_mod = 0;
        is_boss_next = true;
        bosses_to_go = boss_mod_fights >= 4 ? 3 : boss_mod_fights >= 2 ? 2 : 1;
      }
    }
    output_wave_timer();
  }

  void spawn_powerup(SimInterface& sim) {
    if (sim.index().count<PowerupTag>() >= 4) {
      return;
    }

    powerup_mod = (1 + powerup_mod) % 4;
    if (!powerup_mod) {
      ++lives_target;
    }

    auto& random = sim.random(random_source::kGameSequence);
    auto r = random.uint(4);
    auto dim = sim.dimensions();
    vec2 v = r == 0 ? vec2{-dim.x, dim.y / 2}
        : r == 1    ? vec2{dim.x * 2, dim.y / 2}
        : r == 2    ? vec2{dim.x / 2, -dim.y}
                    : vec2{dim.x / 2, dim.y * 2};

    std::uint32_t m = 4;
    if (sim.player_count() > sim.get_lives()) {
      m += sim.player_count() - sim.get_lives();
    }
    if (!sim.get_lives()) {
      m += sim.killed_players();
    }
    if (lives_target > lives_spawned) {
      m += lives_target - lives_spawned;
    }
    if (sim.killed_players() == 0 && lives_target + 1 < lives_spawned) {
      m = 3;
    }

    r = random.uint(m);
    legacy::spawn_powerup(sim, v,
                          r == 0       ? powerup_type::kBomb
                              : r == 1 ? powerup_type::kMagicShots
                              : r == 2 ? powerup_type::kShield
                                       : (++lives_spawned, powerup_type::kExtraLife));
  }

  void spawn_boss_reward(SimInterface& sim) {
    auto r = sim.random(random_source::kGameSequence).uint(4);
    auto dim = sim.dimensions();
    vec2 v = r == 0 ? vec2{-dim.x / 4, dim.y / 2}
        : r == 1    ? vec2{dim.x + dim.x / 4, dim.y / 2}
        : r == 2    ? vec2{dim.x / 2, -dim.y / 4}
                    : vec2{dim.x / 2, dim.y + dim.y / 4};

    legacy::spawn_powerup(sim, v, powerup_type::kExtraLife);
    if (sim.conditions().mode != game_mode::kLegacy_Boss) {
      spawn_powerup(sim);
    }
  }

  void wave(SimInterface& sim) {
    auto& random = sim.random(random_source::kGameSequence);
    if (sim.conditions().mode == game_mode::kLegacy_Fast) {
      for (std::uint32_t i = 0; i < random.uint(7); ++i) {
        random.uint(1);
      }
    }
    if (sim.conditions().mode == game_mode::kLegacy_What) {
      for (std::uint32_t i = 0; i < random.uint(11); ++i) {
        random.uint(1);
      }
    }

    auto resources = power;
    std::vector<const entry*> valid;
    for (const auto& f : formations) {
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
      std::uint32_t n = max == 1 ? 0 : random.uint(max);

      chosen.insert(chosen.begin() + random.uint(chosen.size() + 1), valid[n]);
      resources -= valid[n]->cost;
    }

    // This permutation is bugged and does nothing but backwards compatibility.
    std::vector<std::size_t> perm;
    for (std::size_t i = 0; i < chosen.size(); ++i) {
      perm.push_back(i);
    }
    for (std::size_t i = 0; i < chosen.size() - 1; ++i) {
      std::swap(perm[i], perm[i + random.uint(chosen.size() - i)]);
    }
    std::uint32_t hard_already = 0;
    for (std::size_t row = 0; row < chosen.size(); ++row) {
      SpawnContext context;
      context.sim = &sim;
      context.row = perm[row];
      context.power = power;
      context.hard_already = &hard_already;
      chosen[perm[row]]->function(context);
    }
  }

  void boss(SimInterface& sim) {
    auto& random = sim.random(random_source::kGameSequence);
    auto cycle = (sim.conditions().mode == game_mode::kLegacy_Hard) + boss_mod_bosses / 2;
    bool secret_chance = (sim.conditions().mode != game_mode::kLegacy_Normal &&
                          sim.conditions().mode != game_mode::kLegacy_Boss)
        ? (boss_mod_fights > 1       ? random.uint(4) == 0
               : boss_mod_fights > 0 ? random.uint(8) == 0
                                     : false)
        : (boss_mod_fights > 2       ? random.uint(4) == 0
               : boss_mod_fights > 1 ? random.uint(6) == 0
                                     : false);

    if (+(sim.conditions().flags & initial_conditions::flag::kLegacy_CanFaceSecretBoss) &&
        bosses_to_go == 0 && boss_mod_secret == 0 && secret_chance) {
      auto secret_cycle =
          (std::max<std::uint32_t>(
               2u, boss_mod_bosses + (sim.conditions().mode == game_mode::kLegacy_Hard)) -
           2) /
          2;
      boss_mod_secret = 2;
      spawn_super_boss(sim, secret_cycle);
    } else if (boss_mod_bosses % 2 == 0) {
      if (boss1_queue[0] == 0) {
        spawn_big_square_boss(sim, cycle);
      } else if (boss1_queue[0] == 1) {
        spawn_shield_bomb_boss(sim, cycle);
      } else {
        spawn_chaser_boss(sim, cycle);
      }
      boss1_queue.push_back(boss1_queue.front());
      boss1_queue.erase(boss1_queue.begin());
    } else {
      if (boss_mod_secret > 0) {
        --boss_mod_secret;
      }
      if (boss2_queue[0] == 0) {
        spawn_tractor_boss(sim, cycle);
      } else if (boss2_queue[0] == 1) {
        spawn_ghost_boss(sim, cycle);
      } else {
        spawn_death_ray_boss(sim, cycle);
      }
      boss2_queue.push_back(boss2_queue.front());
      boss2_queue.erase(boss2_queue.begin());
    }
  }

  void boss_mode_boss(SimInterface& sim) {
    auto boss = boss_mod_bosses;
    if (boss_mod_bosses < 3) {
      if (boss1_queue[boss] == 0) {
        spawn_big_square_boss(sim, 0);
      } else if (boss1_queue[boss] == 1) {
        spawn_shield_bomb_boss(sim, 0);
      } else {
        spawn_chaser_boss(sim, 0);
      }
    } else {
      boss = boss - 3;
      if (boss2_queue[boss] == 0) {
        spawn_tractor_boss(sim, 0);
      } else if (boss2_queue[boss] == 1) {
        spawn_ghost_boss(sim, 0);
      } else {
        spawn_death_ray_boss(sim, 0);
      }
    }
  }

  void add_formations() {
    auto init = [&]<typename F>(const F&) {
      formations.emplace_back(entry{static_cast<std::uint32_t>(formations.size()), F::cost,
                                    F::min_resource,
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

    std::stable_sort(formations.begin(), formations.end(),
                     [](const entry& a, const entry& b) { return a.cost < b.cost; });
  }
};
DEBUG_STRUCT_TUPLE(Overmind, power, timer, levels_mod, groups_mod, boss_mod_bosses, boss_mod_fights,
                   boss_mod_secret, powerup_mod, lives_spawned, lives_target, boss_rest_timer,
                   waves_total, is_boss_next, is_boss_level);

}  // namespace

void spawn_overmind(SimInterface& sim) {
  auto h = sim.index().create();
  h.emplace<Overmind>(sim);
  h.emplace<PostUpdate>().post_update = ecs::call<&Overmind::update>;
}

}  // namespace ii::legacy
