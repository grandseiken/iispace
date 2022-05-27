#ifndef IISPACE_GAME_CORE_SAVE_H
#define IISPACE_GAME_CORE_SAVE_H
#include "game/common/z.h"
#include <array>
#include <vector>

namespace ii::io {
class Filesystem;
}  // namespace ii::io

struct HighScores {
  static constexpr std::int32_t kNumScores = 8;
  static constexpr std::int32_t kMaxNameLength = 17;
  static constexpr std::int32_t kMaxScoreLength = 10;

  struct high_score {
    std::string name;
    std::int64_t score = 0;
  };
  typedef std::array<high_score, kNumScores> table;
  typedef std::array<table, kPlayers> mode_table;
  typedef std::array<high_score, kPlayers> boss_table;

  mode_table normal;
  mode_table hard;
  mode_table fast;
  mode_table what;
  boss_table boss;

  static std::size_t size(game_mode mode);
  high_score& get(game_mode mode, std::int32_t players, std::int32_t index);
  const high_score& get(game_mode mode, std::int32_t players, std::int32_t index) const;

  bool is_high_score(game_mode mode, std::int32_t players, std::int64_t score) const;
  void add_score(game_mode mode, std::int32_t players, const std::string& name, std::int64_t score);
};

struct SaveData {
  SaveData(ii::io::Filesystem& fs);
  void save() const;

  ii::io::Filesystem& fs;
  std::int32_t bosses_killed = 0;
  std::int32_t hard_mode_bosses_killed = 0;
  HighScores high_scores;
};

struct Settings {
  Settings(ii::io::Filesystem& fs);
  void save() const;

  ii::io::Filesystem& fs;
  bool windowed = false;
  fixed volume = 100;
};

#endif
