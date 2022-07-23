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
  game_mode mode = game_mode::kNormal;
  std::uint32_t player_count = 0;
  std::uint32_t runs = 0;

  bool verify = false;
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
      std::cout << "========================" << std::endl;
      std::cout << "run #" << (run_index + 1) << ": seed " << conditions.seed << std::endl;
      std::cout << "========================" << std::endl;
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
      std::cout << "max ticks exceeded!" << std::endl;
    }
    std::cout << "ticks:\t" << run_result->ticks << std::endl;
    std::cout << "score:\t" << run_result->score << std::endl;
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
    conditions.mode = options.mode;
    conditions.player_count = options.player_count;
    conditions.seed = std::uniform_int_distribution<std::uint32_t>{}(engine);
    run_conditions.emplace_back(conditions);
  }

  std::vector<std::thread> threads;
  std::mutex mutex;
  std::uint32_t best_run_index = 0;
  std::optional<run_data_t> best_run;
  bool failed = false;

  auto handle_run_result = [&](std::uint32_t run_index, result<run_data_t>& data) {
    if (!data) {
      std::cerr << data.error();
      return failed = true;
    }
    if (options.thread_count > 1) {
      if (data->ticks >= options.max_ticks) {
        std::cout << "run #" << (run_index + 1) << " max ticks exceeded!" << std::endl;
      } else {
        std::cout << "run #" << (run_index + 1) << " finished with ticks " << data->ticks
                  << " score " << data->score << std::endl;
      }
    }

    if (data->ticks >= options.max_ticks && data->replay_bytes) {
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
      std::cout << "========================" << std::endl;
      std::cout << "best run #" << (best_run_index + 1) << ": ticks " << best_run->ticks
                << " score " << best_run->score << std::endl;
    }
    auto write_result = fs.write(*options.replay_out_path, *best_run->replay_bytes);
    if (!write_result) {
      std::cerr << "couldn't write " << *options.replay_out_path << ": " << write_result.error()
                << std::endl;
      return false;
    }
    std::cout << "wrote " << *options.replay_out_path << std::endl;
  }
  return true;
}

}  // namespace
}  // namespace ii

int main(int argc, char** argv) {
  ii::options_t options;
  auto args = ii::args_init(argc, argv);
  auto players = ii::flag_parse<std::uint32_t>(args, "players", /* required */ true);
  if (!players) {
    std::cerr << players.error() << std::endl;
    return 1;
  }
  options.player_count = **players;
  if (!options.player_count || options.player_count > 4) {
    std::cerr << "error: invalid player count" << std::endl;
    return 1;
  }

  auto runs = ii::flag_parse<std::uint32_t>(args, "runs");
  if (!runs) {
    std::cerr << runs.error() << std::endl;
    return 1;
  }
  options.runs = runs->value_or(1);
  if (!options.runs) {
    std::cerr << "error: invalid run count" << std::endl;
    return 1;
  }

  auto max_ticks = ii::flag_parse<std::uint64_t>(args, "max_ticks");
  if (!max_ticks) {
    std::cerr << max_ticks.error() << std::endl;
    return 1;
  }
  options.max_ticks = max_ticks->value_or(1024u * 1024u);

  auto mode = ii::flag_parse<std::string>(args, "mode");
  if (!mode) {
    std::cerr << mode.error() << std::endl;
    return 1;
  }
  if (mode && *mode) {
    if (**mode == "normal") {
      options.mode = ii::game_mode::kNormal;
    } else if (**mode == "hard") {
      options.mode = ii::game_mode::kHard;
    } else if (**mode == "boss") {
      options.mode = ii::game_mode::kBoss;
    } else if (**mode == "what") {
      options.mode = ii::game_mode::kWhat;
    } else {
      std::cerr << "error: unknown mode " << **mode << std::endl;
      return 1;
    }
  }

  auto verify = ii::flag_parse<bool>(args, "verify");
  if (!verify) {
    std::cerr << mode.error() << std::endl;
    return 1;
  }
  options.verify = verify->value_or(false);

  auto multithreaded = ii::flag_parse<bool>(args, "multithreaded");
  if (!multithreaded) {
    std::cerr << mode.error() << std::endl;
    return 1;
  }
  options.thread_count = multithreaded->value_or(false)
      ? std::max<std::uint32_t>(1u, std::thread::hardware_concurrency() - 1u)
      : 1u;

  auto output_path = ii::flag_parse<std::string>(args, "output");
  if (!output_path) {
    std::cerr << mode.error() << std::endl;
    return 1;
  }
  options.replay_out_path = *output_path;

  if (auto result = ii::args_finish(args); !result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }

  if (!args.empty()) {
    std::cerr << "error: invalid positional argument" << std::endl;
    return 1;
  }
  return ii::run(options);
}
