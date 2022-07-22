#include "game/data/replay.h"
#include "game/flags.h"
#include "game/io/file/std_filesystem.h"
#include "game/logic/sim/sim_state.h"
#include <iostream>
#include <optional>
#include <random>
#include <string>
#include <vector>

namespace ii {
namespace {

class NullInputAdapter : public InputAdapter {
public:
  ~NullInputAdapter() override = default;
  NullInputAdapter(std::uint32_t player_count, ReplayWriter* replay_writer)
  : player_count_{player_count}, replay_writer_{replay_writer} {}

  std::vector<input_frame>& get() override {
    frames_.clear();
    frames_.resize(player_count_);
    return frames_;
  }

  void next() override {
    if (replay_writer_) {
      for (const auto& frame : frames_) {
        replay_writer_->add_input_frame(frame);
      }
    }
  }

private:
  std::uint32_t player_count_ = 0;
  ReplayWriter* replay_writer_ = nullptr;
  std::vector<input_frame> frames_;
};

struct options_t {
  game_mode mode = game_mode::kNormal;
  std::uint32_t player_count = 0;
  std::uint32_t runs = 0;
  std::uint64_t max_ticks = 0;
  bool verify = false;
};

bool run(const options_t& options, const std::optional<std::string>& replay_out_path) {
  std::mt19937_64 engine{std::random_device{}()};
  io::StdFilesystem fs{".", ".", "."};

  struct run_data {
    std::uint32_t run = 0;
    std::uint64_t ticks = 0;
    std::uint64_t score = 0;
    std::optional<std::vector<std::uint8_t>> replay_bytes;

    bool better_than(const run_data& other, game_mode mode) const {
      if (mode != game_mode::kBoss) {
        return score > other.score;
      }
      if (score && !other.score) {
        return true;
      }
      if (!score && other.score) {
        return false;
      }
      if (!score && !other.score) {
        return ticks > other.ticks;
      }
      return score < other.score;
    }
  };
  std::optional<run_data> best_run;

  for (std::uint32_t i = 0; i < options.runs; ++i) {
    initial_conditions conditions;
    conditions.mode = options.mode;
    conditions.player_count = options.player_count;
    conditions.seed = std::uniform_int_distribution<std::uint32_t>{}(engine);

    ReplayWriter writer{conditions};
    NullInputAdapter input{options.player_count, &writer};
    std::vector<std::uint32_t> ai_players;
    for (std::uint32_t i = 0; i < conditions.player_count; ++i) {
      ai_players.emplace_back(i);
    }

    run_data data;
    data.run = i;
    {
      if (options.runs > 1) {
        if (i) {
          std::cout << '\n';
        }
        std::cout << "============" << std::endl;
        std::cout << "run #" << (i + 1) << std::endl;
        std::cout << "============\n" << std::endl;
      }
      std::cout << "running sim..." << std::endl;
      SimState sim{conditions, input, ai_players};
      while (!sim.game_over()) {
        sim.update();
        sim.clear_output();
        if (sim.tick_count() >= options.max_ticks) {
          std::cout << "max ticks exceeded!" << std::endl;
          break;
        }
      }

      data.ticks = sim.tick_count();
      data.score = sim.get_results().score;
      std::cout << "ticks:\t" << data.ticks << std::endl;
      std::cout << "score:\t" << data.score << std::endl;
      if (replay_out_path || options.verify || data.ticks >= options.max_ticks) {
        auto bytes = writer.write();
        if (!bytes) {
          std::cerr << "couldn't serialize replay: " << bytes.error() << std::endl;
          return false;
        }
        data.replay_bytes.emplace(std::move(*bytes));
      }
      if (data.ticks >= options.max_ticks) {
        auto path = "exceeded" + std::to_string(i + 1) + ".wrp";
        auto write_result = fs.write(path, *data.replay_bytes);
        if (!write_result) {
          std::cerr << "couldn't write " << path << ": " << write_result.error() << std::endl;
        }
        std::cout << "wrote " << path << std::endl;
      }
    }

    if (options.verify) {
      std::cout << "verifying..." << std::flush;
      auto reader = ReplayReader::create(*data.replay_bytes);
      if (!reader) {
        std::cerr << reader.error() << std::endl;
        return false;
      }
      ReplayInputAdapter input{*reader};
      SimState sim{reader->initial_conditions(), input};
      while (!sim.game_over() && sim.tick_count() < options.max_ticks) {
        sim.update();
        sim.clear_output();
      }
      auto results = sim.get_results();
      if (sim.tick_count() == data.ticks && results.score == data.score) {
        std::cout << " done" << std::endl;
      } else {
        std::cout << std::endl;
        std::cerr << "verification failed: ticks " << sim.tick_count() << " score " << results.score
                  << std::endl;
        return false;
      }
    }

    if (!best_run || data.better_than(*best_run, options.mode)) {
      best_run = std::move(data);
    }
  }

  if (replay_out_path) {
    if (options.runs > 1) {
      std::cout << "\n================\n" << std::endl;
      std::cout << "best run #" << (best_run->run + 1) << ": ticks " << best_run->ticks << " score "
                << best_run->score << std::endl;
    }
    auto write_result = fs.write(*replay_out_path, *best_run->replay_bytes);
    if (!write_result) {
      std::cerr << "couldn't write " << *replay_out_path << ": " << write_result.error()
                << std::endl;
    }
    std::cout << "wrote " << *replay_out_path << std::endl;
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

  auto output_path = ii::flag_parse<std::string>(args, "output");
  if (!output_path) {
    std::cerr << mode.error() << std::endl;
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
  return ii::run(options, *output_path);
}
