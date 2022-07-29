#include "game/data/replay.h"
#include "game/flags.h"
#include "game/io/file/std_filesystem.h"
#include "game/logic/sim/sim_state.h"
#include "game/replay_tools.h"
#include <iostream>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <thread>
#include <vector>

namespace ii {
namespace {

struct options_t {
  compatibility_level compatibility = compatibility_level::kIispaceV0;
  game_mode mode = game_mode::kNormal;
  initial_conditions::flag flags = initial_conditions::flag::kNone;
  std::uint32_t player_count = 0;
  std::uint32_t runs = 0;
  std::optional<std::uint32_t> seed;

  bool verify = false;
  bool save_ticks_exceeded = false;
  boss_flag find_boss_kills{0};
  std::uint64_t max_ticks = 0;
  std::uint32_t thread_count = 0;
  std::optional<std::string> replay_out_path;
};

result<run_data_t>
do_run(const options_t& options, std::uint32_t run_index, const initial_conditions& conditions) {
  bool print_progress = options.thread_count <= 1;
  if (print_progress) {
    if (options.runs > 1) {
      if (run_index) {
        std::cout << '\n';
      }
      std::cout << "================================================\n"
                << "run #" << (run_index + 1) << ": seed " << conditions.seed << "\n"
                << "================================================\n";
    }
    std::cout << "running sim..." << std::endl;
  }

  auto run_result = synthesize_replay(conditions, options.max_ticks,
                                      /* save replay */ options.replay_out_path || options.verify);
  if (!run_result) {
    return unexpected(run_result.error());
  }

  if (print_progress) {
    if (run_result->ticks >= options.max_ticks) {
      std::cout << "max ticks exceeded!\n";
    }
    std::cout << "ticks:\t" << run_result->ticks << "\n"
              << "score:\t" << run_result->score << "\n"
              << "kills:\t" << +run_result->bosses_killed << std::endl;
  }

  if (options.verify) {
    if (print_progress) {
      std::cout << "verifying..." << std::flush;
    }
    auto verify = replay_results(*run_result->replay_bytes, options.max_ticks);
    if (!verify) {
      return unexpected(verify.error());
    }
    if (verify->sim.tick_count == run_result->ticks && verify->sim.score == run_result->score) {
      if (print_progress) {
        std::cout << " done" << std::endl;
      }
    } else {
      if (print_progress) {
        std::cout << std::endl;
      }
      return unexpected("verification failed: ticks " + std::to_string(verify->sim.tick_count) +
                        " score " + std::to_string(verify->sim.score));
    }
  }
  return {std::move(*run_result)};
}

bool run(const options_t& options) {
  std::mt19937_64 engine{std::random_device{}()};
  io::StdFilesystem fs{".", ".", "."};

  auto better_than = [&](const run_data_t& a, const run_data_t& b) {
    if (options.find_boss_kills != boss_flag{0}) {
      if (+(a.bosses_killed & options.find_boss_kills) >
          +(b.bosses_killed & options.find_boss_kills)) {
        return true;
      }
      if (+(a.bosses_killed & options.find_boss_kills) <
          +(b.bosses_killed & options.find_boss_kills)) {
        return false;
      }
    }
    if (options.mode != game_mode::kBoss) {
      return a.score > b.score;
    }
    if (a.score && !b.score) {
      return true;
    }
    if (!a.score && b.score) {
      return false;
    }
    if (!a.score && !b.score) {
      return a.ticks > b.ticks;
    }
    return a.score < b.score;
  };

  std::vector<initial_conditions> run_conditions;
  for (std::uint32_t i = 0; i < options.runs; ++i) {
    initial_conditions conditions;
    conditions.compatibility = options.compatibility;
    conditions.mode = options.mode;
    conditions.flags = options.flags;
    conditions.player_count = options.player_count;
    conditions.seed =
        options.seed ? *options.seed : std::uniform_int_distribution<std::uint32_t>{}(engine);
    run_conditions.emplace_back(conditions);
  }

  std::uint32_t runs_completed = 0;
  std::uint32_t best_run_index = 0;
  std::optional<run_data_t> best_run;
  bool failed = false;

  auto handle_run_result = [&](std::uint32_t run_index, result<run_data_t>& data) {
    if (!data) {
      std::cerr << data.error();
      return failed = true;
    }
    if (options.thread_count > 1) {
      auto p =
          static_cast<std::uint32_t>(100 * static_cast<float>(++runs_completed) / options.runs);
      std::cout << "[" << p << "%] ";
      if (data->ticks >= options.max_ticks) {
        std::cout << "run #" << (run_index + 1) << " max ticks exceeded!" << std::endl;
      } else {
        std::cout << "run #" << (run_index + 1) << " [seed " << run_conditions[run_index].seed
                  << "] finished with ticks " << data->ticks << " score " << data->score
                  << " kills " << +data->bosses_killed << std::endl;
      }
    }

    if (data->ticks >= options.max_ticks && data->replay_bytes && options.save_ticks_exceeded) {
      auto path = "exceeded" + std::to_string(run_index + 1) + ".wrp";
      auto write_result = fs.write(path, *data->replay_bytes);
      if (!write_result) {
        std::cerr << "couldn't write " << path << ": " << write_result.error() << std::endl;
        return failed = true;
      }
      std::cout << "wrote " << path << std::endl;
    }

    if (!best_run || better_than(*data, *best_run)) {
      best_run_index = run_index;
      best_run = std::move(*data);
    }
    return failed;
  };

  std::vector<std::thread> threads;
  std::mutex mutex;
  for (std::uint32_t k = 0; k < options.thread_count; ++k) {
    threads.emplace_back([&, k] {
      for (std::uint32_t i = k; i < options.runs; i += options.thread_count) {
        auto data = do_run(options, i, run_conditions[i]);

        std::lock_guard lock{mutex};
        bool cancel = handle_run_result(i, data);
        if (cancel) {
          break;
        }
      }
    });
  }
  for (auto& thread : threads) {
    thread.join();
  }

  if (failed) {
    return false;
  }
  if (options.replay_out_path) {
    if (options.runs > 1) {
      std::cout << "================================================\n"
                << "best run #" << (best_run_index + 1) << ": ticks " << best_run->ticks
                << " score " << best_run->score << " kills " << +best_run->bosses_killed
                << std::endl;
    }
    auto write_result = fs.write(*options.replay_out_path, *best_run->replay_bytes);
    if (!write_result) {
      std::cerr << "couldn't write " << *options.replay_out_path << ": " << write_result.error()
                << std::endl;
      return false;
    }
    std::cout << "wrote " << *options.replay_out_path << std::endl;
    if (options.runs > 1) {
      std::cout << "================================================" << std::endl;
    }
  }
  std::cout << std::endl;
  return true;
}

}  // namespace
}  // namespace ii

int main(int argc, char** argv) {
  ii::options_t options;
  auto args = ii::args_init(argc, argv);
  if (auto result = ii::flag_parse<std::uint32_t>(args, "players", options.player_count); !result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }
  if (!options.player_count) {
    std::cerr << "error: invalid player count" << std::endl;
    return 1;
  }

  if (auto result = ii::flag_parse<std::uint32_t>(args, "runs", options.runs, 1u); !result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }
  if (!options.runs) {
    std::cerr << "error: invalid run count" << std::endl;
    return 1;
  }

  if (auto result = ii::flag_parse<std::uint32_t>(args, "seed", options.seed); !result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }

  if (auto result =
          ii::flag_parse<std::uint64_t>(args, "max_ticks", options.max_ticks, 1024u * 1024u);
      !result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }

  std::optional<std::string> mode;
  if (auto result = ii::flag_parse<std::string>(args, "mode", mode); !result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }
  if (mode) {
    if (*mode == "normal") {
      options.mode = ii::game_mode::kNormal;
    } else if (*mode == "hard") {
      options.mode = ii::game_mode::kHard;
    } else if (*mode == "boss") {
      options.mode = ii::game_mode::kBoss;
    } else if (*mode == "what") {
      options.mode = ii::game_mode::kWhat;
    } else {
      std::cerr << "error: unknown game mode " << *mode << std::endl;
      return 1;
    }
  }

  std::optional<std::string> compatibility;
  if (auto result = ii::flag_parse<std::string>(args, "compatibility", compatibility); !result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }
  if (compatibility) {
    if (*compatibility == "legacy") {
      options.compatibility = ii::compatibility_level::kLegacy;
    } else if (*compatibility == "v0") {
      options.compatibility = ii::compatibility_level::kIispaceV0;
    } else {
      std::cerr << "error: unknown compatibility level " << *compatibility << std::endl;
      return 1;
    }
  }

  bool can_face_secret_boss = false;
  if (auto result = ii::flag_parse<bool>(args, "can_face_secret_boss", can_face_secret_boss, false);
      !result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }
  if (can_face_secret_boss) {
    options.flags |= ii::initial_conditions::flag::kLegacy_CanFaceSecretBoss;
  }

  std::uint64_t find_boss_kills = 0;
  if (auto result = ii::flag_parse<std::uint64_t>(args, "find_boss_kills", find_boss_kills, 0u);
      !result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }
  options.find_boss_kills = static_cast<ii::boss_flag>(find_boss_kills);

  if (auto result = ii::flag_parse<bool>(args, "verify", options.verify, false); !result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }

  if (auto result =
          ii::flag_parse<bool>(args, "save_ticks_exceeded", options.save_ticks_exceeded, false);
      !result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }

  bool multithreaded = false;
  if (auto result = ii::flag_parse<bool>(args, "multithreaded", multithreaded, false); !result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }
  options.thread_count =
      multithreaded ? std::max<std::uint32_t>(1u, std::thread::hardware_concurrency() - 1u) : 1u;

  if (auto result = ii::flag_parse<std::string>(args, "output", options.replay_out_path); !result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }

  if (auto result = ii::args_finish(args); !result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }

  if (!args.empty()) {
    std::cerr << "error: invalid positional argument" << std::endl;
    return 1;
  }
  return ii::run(options) ? 0 : 1;
}
