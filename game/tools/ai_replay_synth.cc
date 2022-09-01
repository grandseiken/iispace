#include "game/flags.h"
#include "game/io/file/std_filesystem.h"
#include "game/logic/sim/sim_state.h"
#include "game/mode_flags.h"
#include "game/tools/replay_tools.h"
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
                << "best run #" << (best_run_index + 1) << ":\n\tseed " << best_run->seed
                << "\n\tticks " << best_run->ticks << "\n\tscore " << best_run->score
                << "\n\tkills " << +best_run->bosses_killed << std::endl;
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

result<options_t> parse_args(std::vector<std::string>& args) {
  options_t options;

  if (auto r = flag_parse<std::uint32_t>(args, "players", options.player_count); !r) {
    return unexpected(r.error());
  }
  if (!has_help_flag() && !options.player_count) {
    return unexpected("error: invalid player count");
  }
  if (auto r = flag_parse<std::uint32_t>(args, "runs", options.runs, 1u); !r) {
    return unexpected(r.error());
  }
  if (!has_help_flag() && !options.runs) {
    return unexpected("error: invalid run count");
  }
  if (auto r = flag_parse<std::uint32_t>(args, "seed", options.seed); !r) {
    return unexpected(r.error());
  }
  if (auto r = flag_parse<std::uint64_t>(args, "max_ticks", options.max_ticks, 1024u * 1024u); !r) {
    return unexpected(r.error());
  }

  if (auto r = parse_game_mode(args, options.mode); !r) {
    return unexpected(r.error());
  }
  if (auto r = parse_compatibility_level(args, options.compatibility); !r) {
    return unexpected(r.error());
  }
  if (auto r = parse_initial_conditions_flags(args, options.flags); !r) {
    return unexpected(r.error());
  }

  std::uint64_t find_boss_kills = 0;
  if (auto r = flag_parse<std::uint64_t>(args, "find_boss_kills", find_boss_kills, 0u); !r) {
    return unexpected(r.error());
  }
  options.find_boss_kills = static_cast<boss_flag>(find_boss_kills);

  if (auto r = flag_parse<bool>(args, "verify", options.verify, false); !r) {
    return unexpected(r.error());
  }

  if (auto r = flag_parse<bool>(args, "save_ticks_exceeded", options.save_ticks_exceeded, false);
      !r) {
    return unexpected(r.error());
  }

  bool multithreaded = false;
  if (auto r = flag_parse<bool>(args, "multithreaded", multithreaded, false); !r) {
    return unexpected(r.error());
  }
  options.thread_count =
      multithreaded ? std::max<std::uint32_t>(1u, std::thread::hardware_concurrency() - 1u) : 1u;

  if (auto r = flag_parse<std::string>(args, "output", options.replay_out_path); !r) {
    return unexpected(r.error());
  }
  return {std::move(options)};
}

}  // namespace
}  // namespace ii

int main(int argc, char** argv) {
  auto args = ii::args_init(argc, argv);
  auto options = ii::parse_args(args);
  if (!options) {
    std::cerr << options.error() << std::endl;
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
  return ii::run(*options) ? 0 : 1;
}
