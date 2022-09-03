#ifndef II_GAME_DATA_SAVE_H
#define II_GAME_DATA_SAVE_H
#include "game/common/result.h"
#include "game/logic/sim/io/conditions.h"
#include "game/logic/sim/io/output.h"
#include <cstdint>
#include <span>
#include <vector>

namespace ii::data {

struct HighScores {
  static constexpr std::uint32_t kNumScores = 8;
  static constexpr std::uint32_t kMaxNameLength = 17;
  static constexpr std::uint32_t kMaxScoreLength = 10;

  struct high_score {
    std::string name;
    std::uint64_t score = 0;
  };
  using table_t = std::vector<high_score>;

  std::vector<table_t> normal;
  std::vector<table_t> hard;
  std::vector<table_t> fast;
  std::vector<table_t> what;
  table_t boss;

  HighScores();
  static std::size_t size(game_mode mode);
  high_score& get(game_mode mode, std::uint32_t players, std::uint32_t index);
  const high_score& get(game_mode mode, std::uint32_t players, std::uint32_t index) const;

  bool is_high_score(game_mode mode, std::uint32_t players, std::uint64_t score) const;
  void
  add_score(game_mode mode, std::uint32_t players, const std::string& name, std::uint64_t score);
};

struct savegame {
  boss_flag bosses_killed{0};
  boss_flag hard_mode_bosses_killed{0};
  HighScores high_scores;
};

result<savegame> read_savegame(std::span<const std::uint8_t>);
result<std::vector<std::uint8_t>> write_savegame(const savegame&);

}  // namespace ii::data

#endif
