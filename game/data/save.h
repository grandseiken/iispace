#ifndef II_GAME_DATA_SAVE_H
#define II_GAME_DATA_SAVE_H
#include "game/common/result.h"
#include "game/logic/sim/sim_io.h"
#include <cstdint>
#include <span>
#include <vector>

namespace ii {
namespace io {
class Filesystem;
}  // namespace io

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

struct SaveGame {
  static result<SaveGame> load(std::span<const std::uint8_t> bytes);
  result<std::vector<std::uint8_t>> save() const;

  std::uint32_t bosses_killed = 0;
  std::uint32_t hard_mode_bosses_killed = 0;
  HighScores high_scores;
};

struct Config {
  static result<Config> load(std::span<const std::uint8_t> bytes);
  result<std::vector<std::uint8_t>> save() const;

  float volume = 100.f;
};

}  // namespace ii

#endif
