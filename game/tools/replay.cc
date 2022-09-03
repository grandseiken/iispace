#include "game/data/replay.h"
#include "game/flags.h"
#include "game/io/file/std_filesystem.h"
#include "game/logic/sim/sim_state.h"
#include "game/tools/replay_tools.h"
#include <cstdint>
#include <iostream>
#include <optional>
#include <string>
#include <unordered_set>
#include <vector>

namespace ii {
namespace {

struct options_t {
  SimState::query query;
  std::optional<std::uint64_t> dump_state_from_tick;
  std::optional<std::uint64_t> max_ticks;

  std::optional<std::uint64_t> verify_ticks;
  std::optional<std::uint64_t> verify_score;
  std::optional<std::string> convert_out_path;
};

bool run(const options_t& options, const std::string& replay_path) {
  io::StdFilesystem fs{".", ".", "."};
  auto replay_bytes = fs.read(replay_path);
  if (!replay_bytes) {
    std::cerr << replay_bytes.error() << std::endl;
    return false;
  }
  auto results =
      replay_results(*replay_bytes, options.max_ticks, options.dump_state_from_tick, options.query);
  if (!results) {
    std::cerr << results.error() << std::endl;
    return false;
  }
  print_replay_info(std::cout, replay_path, *results);
  for (std::size_t i = 0; i < results->state_dumps.size(); ++i) {
    if (results->state_dumps[i].empty()) {
      continue;
    }
    std::cout << "\n================================================\n"
              << "tick " << (options.dump_state_from_tick.value_or(0u) + i) << " state dump\n"
              << "================================================\n";
    std::cout << results->state_dumps[i] << std::flush;
  }

  bool success = true;
  if (options.verify_score && *options.verify_score != results->sim.score) {
    std::cerr << "verification failed: expected score " << *options.verify_score << ", was "
              << results->sim.score << std::endl;
    success = false;
  }
  if (options.verify_ticks && *options.verify_ticks != results->sim.tick_count) {
    std::cerr << "verification failed: expected ticks " << *options.verify_ticks << ", was "
              << results->sim.tick_count << std::endl;
    success = false;
  }
  auto frames_read = results->replay_frames_read;
  if (results->conditions.compatibility == compatibility_level::kLegacy) {
    ++frames_read;
  }
  if (!options.max_ticks && frames_read < results->replay_frames_total) {
    std::cerr << "verification failed: only " << results->replay_frames_read << " of "
              << results->replay_frames_total << " replay frames consumed" << std::endl;
    success = false;
  }
  if (!success) {
    return false;
  }

  if (options.convert_out_path) {
    std::cout << "converting replay..." << std::endl;
    auto reader = data::ReplayReader::create(*replay_bytes);
    if (!reader) {
      std::cerr << reader.error() << std::endl;
      return false;
    }
    data::ReplayWriter writer{reader->initial_conditions()};
    std::size_t frames_written = 0;
    while (auto frame = reader->next_input_frame()) {
      if (frames_written < results->replay_frames_read) {
        writer.add_input_frame(*frame);
        ++frames_written;
      }
    }
    auto out_bytes = writer.write();
    if (!out_bytes) {
      std::cerr << out_bytes.error() << std::endl;
      return false;
    }
    auto result = fs.write(*options.convert_out_path, *out_bytes);
    if (!result) {
      std::cerr << result.error() << std::endl;
      return false;
    }
    std::cout << "wrote " << *options.convert_out_path << std::endl;
  }
  return true;
}

result<options_t> parse_args(std::vector<std::string>& args) {
  options_t options;
  if (auto r = flag_parse(args, "verify_score", options.verify_score); !r) {
    return unexpected(r.error());
  }
  if (auto r = flag_parse(args, "verify_ticks", options.verify_ticks); !r) {
    return unexpected(r.error());
  }
  if (auto r = flag_parse(args, "dump_tick_from", options.dump_state_from_tick); !r) {
    return unexpected(r.error());
  }
  if (auto r = flag_parse(args, "dump_tick_to", options.max_ticks); !r) {
    return unexpected(r.error());
  }

  std::optional<std::uint64_t> dump_tick;
  if (auto r = flag_parse(args, "dump_tick", dump_tick); !r) {
    return unexpected(r.error());
  }
  if (dump_tick) {
    options.dump_state_from_tick = dump_tick;
    options.max_ticks = dump_tick;
  }

  if (auto r = flag_parse<bool>(args, "dump_portable", options.query.portable, false); !r) {
    return unexpected(r.error());
  }
  if (auto r = flag_parse(args, "dump_entity_ids", options.query.entity_ids); !r) {
    return unexpected(r.error());
  }
  if (auto r = flag_parse(args, "dump_components", options.query.component_names); !r) {
    return unexpected(r.error());
  }
  if (auto r = flag_parse(args, "output", options.convert_out_path); !r) {
    return unexpected(r.error());
  }
  return {std::move(options)};
}

}  // namespace
}  // namespace ii

int main(int argc, const char** argv) {
  std::vector<std::string> args;
  ii::args_init(args, argc, argv);
  auto options = ii::parse_args(args);
  if (!options) {
    std::cerr << options.error() << std::endl;
    return 1;
  }
  if (auto result = ii::args_finish(args); !result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }
  if (args.empty()) {
    std::cerr << "no paths" << std::endl;
    return 1;
  }
  int exit = 0;
  for (const auto& path : args) {
    if (!ii::run(*options, path)) {
      exit = 1;
    }
  }
  return exit;
}
