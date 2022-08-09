#ifndef II_GAME_REPLAY_TOOLS_H
#define II_GAME_REPLAY_TOOLS_H
#include "game/common/printer.h"
#include "game/common/result.h"
#include "game/data/replay.h"
#include "game/logic/sim/sim_io.h"
#include "game/logic/sim/sim_state.h"
#include <optional>
#include <ostream>
#include <span>

namespace ii {

struct replay_results_t {
  initial_conditions conditions;
  sim_results sim;
  std::size_t replay_frames_read = 0;
  std::size_t replay_frames_total = 0;
  std::vector<std::string> state_dumps;
};

result<replay_results_t> inline replay_results(
    std::span<const std::uint8_t> replay_bytes,
    std::optional<std::uint64_t> max_ticks = std::nullopt,
    std::optional<std::uint64_t> dump_state_from_tick = std::nullopt, SimState::query query = {}) {
  auto reader = ReplayReader::create(replay_bytes);
  if (!reader) {
    return unexpected(reader.error());
  }
  replay_results_t results;
  results.conditions = reader->initial_conditions();
  SimState sim{reader->initial_conditions()};
  SimState double_buffer;
  std::size_t i = 0;
  while (!sim.game_over()) {
    if (dump_state_from_tick && sim.tick_count() >= *dump_state_from_tick) {
      Printer printer;
      sim.dump(printer, query);
      results.state_dumps.emplace_back(printer.extract());
    }
    if (max_ticks && sim.tick_count() >= *max_ticks) {
      break;
    }
    sim.update(reader->next_tick_input_frames());
    if (!(++i % 16)) {
      auto checksum = sim.checksum();
      sim.copy_to(double_buffer);
      if (sim.checksum() != checksum || double_buffer.checksum() != checksum) {
        return unexpected("checksum failure");
      }
      std::swap(sim, double_buffer);
    } else {
      sim.output().clear();
    }
  }
  results.sim = sim.results();
  results.replay_frames_read = reader->current_input_frame();
  results.replay_frames_total = reader->total_input_frames();
  return results;
}

struct run_data_t {
  std::uint64_t ticks = 0;
  std::uint64_t score = 0;
  boss_flag bosses_killed{0};
  std::optional<std::vector<std::uint8_t>> replay_bytes;
};

inline result<run_data_t> synthesize_replay(const initial_conditions& conditions,
                                            std::optional<std::uint64_t> max_ticks,
                                            bool save_replay) {
  std::vector<std::uint32_t> ai_players;
  for (std::uint32_t i = 0; i < conditions.player_count; ++i) {
    ai_players.emplace_back(i);
  }
  ReplayWriter writer{conditions};
  SimState sim{conditions, &writer, ai_players};
  while (!sim.game_over()) {
    std::vector<input_frame> input;
    sim.ai_think(input);
    sim.update(input);
    sim.output().clear();
    if (max_ticks && sim.tick_count() >= *max_ticks) {
      break;
    }
  }
  auto results = sim.results();
  run_data_t data;
  data.ticks = results.tick_count;
  data.score = results.score;
  data.bosses_killed |= results.bosses_killed;
  if (save_replay || (max_ticks && data.ticks >= *max_ticks)) {
    auto bytes = writer.write();
    if (!bytes) {
      return unexpected(bytes.error());
    }
    data.replay_bytes.emplace(std::move(*bytes));
  }
  return data;
}

inline void print_replay_info(std::ostream& os, const std::string& replay_path,
                              const replay_results_t& results) {
  os << "================================================\n"
     << replay_path << "\n"
     << "================================================\n"
     << "replay progress:\t"
     << (100 * static_cast<float>(results.replay_frames_read) / results.replay_frames_total)
     << "%\n"
     << "compatibility:  \t" << static_cast<std::uint32_t>(results.conditions.compatibility) << "\n"
     << "players:        \t" << results.conditions.player_count << "\n"
     << "flags:          \t" << +(results.conditions.flags) << "\n"
     << "mode:           \t" << static_cast<std::uint32_t>(results.conditions.mode) << "\n"
     << "seed:           \t" << results.conditions.seed << "\n"
     << "ticks:          \t" << results.sim.tick_count << "\n"
     << "score:          \t" << results.sim.score << std::endl;
}

}  // namespace ii

#endif