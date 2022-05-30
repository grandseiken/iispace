#ifndef IISPACE_GAME_DATA_SAVE_H
#define IISPACE_GAME_DATA_SAVE_H
#include "game/common/result.h"
#include "game/common/z.h"
#include <nonstd/span.hpp>
#include <cstdint>
#include <vector>

namespace ii {
namespace io {
class Filesystem;
}  // namespace io

struct HighScores {
  static constexpr std::int32_t kNumScores = 8;
  static constexpr std::int32_t kMaxNameLength = 17;
  static constexpr std::int32_t kMaxScoreLength = 10;

  struct high_score {
    std::string name;
    std::int64_t score = 0;
  };
  using table_t = std::vector<high_score>;

  std::vector<table_t> normal;
  std::vector<table_t> hard;
  std::vector<table_t> fast;
  std::vector<table_t> what;
  table_t boss;

  HighScores();
  static std::size_t size(game_mode mode);
  high_score& get(game_mode mode, std::int32_t players, std::int32_t index);
  const high_score& get(game_mode mode, std::int32_t players, std::int32_t index) const;

  bool is_high_score(game_mode mode, std::int32_t players, std::int64_t score) const;
  void add_score(game_mode mode, std::int32_t players, const std::string& name, std::int64_t score);
};

struct SaveGame {
  static result<SaveGame> load(nonstd::span<const std::uint8_t> bytes);
  result<std::vector<std::uint8_t>> save() const;

  std::int32_t bosses_killed = 0;
  std::int32_t hard_mode_bosses_killed = 0;
  HighScores high_scores;
};

struct Config {
  static result<Config> load(nonstd::span<const std::uint8_t> bytes);
  result<std::vector<std::uint8_t>> save() const;

  float volume = 100.f;
};

}  // namespace ii

#endif
