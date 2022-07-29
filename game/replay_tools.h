#ifndef II_GAME_REPLAY_TOOLS_H
#define II_GAME_REPLAY_TOOLS_H
#include "game/common/printer.h"
#include "game/common/result.h"
#include "game/data/replay.h"
#include "game/logic/sim/sim_io.h"
#include "game/logic/sim/sim_state.h"
#include <optional>
#include <span>

namespace ii {

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

struct replay_results_t {
  initial_conditions conditions;
  sim_results sim;
  std::size_t replay_frames_read = 0;
  std::size_t replay_frames_total = 0;
  std::vector<std::string> state_dumps;
};

result<replay_results_t> inline replay_results(
    std::span<const std::uint8_t> replay_bytes, std::optional<std::uint64_t> max_ticks,
    std::optional<std::uint64_t> dump_state_from_tick = std::nullopt, SimState::query query = {}) {
  auto reader = ReplayReader::create(replay_bytes);
  if (!reader) {
    return unexpected(reader.error());
  }
  ReplayInputAdapter input{*reader};
  replay_results_t results;
  results.conditions = reader->initial_conditions();
  SimState sim{reader->initial_conditions()};
  SimState double_buffer;
  std::size_t i = 0;
  while (!sim.game_over()) {
    auto tick_results = sim.get_results();
    if (dump_state_from_tick && tick_results.tick_count >= *dump_state_from_tick) {
      Printer printer;
      sim.dump(printer, query);
      results.state_dumps.emplace_back(printer.extract());
    }
    if (max_ticks && tick_results.tick_count >= *max_ticks) {
      break;
    }
    sim.update(input);
    if (!(++i % 16)) {
      auto checksum = sim.checksum();
      sim.copy_to(double_buffer);
      if (sim.checksum() != checksum || double_buffer.checksum() != checksum) {
        return unexpected("checksum failure");
      }
      std::swap(sim, double_buffer);
    } else {
      sim.clear_output();
    }
  }
  results.sim = sim.get_results();
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
  ReplayWriter writer{conditions};
  NullInputAdapter input{conditions.player_count, &writer};
  std::vector<std::uint32_t> ai_players;
  for (std::uint32_t i = 0; i < conditions.player_count; ++i) {
    ai_players.emplace_back(i);
  }
  SimState sim{conditions, ai_players};
  while (!sim.game_over()) {
    sim.update(input);
    sim.clear_output();
    if (max_ticks && sim.get_results().tick_count >= *max_ticks) {
      break;
    }
  }
  auto results = sim.get_results();
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

}  // namespace ii

#endif