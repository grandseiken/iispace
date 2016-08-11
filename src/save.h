#ifndef IISPACE_SRC_SAVE_H
#define IISPACE_SRC_SAVE_H

#include <array>
#include <vector>
#include "z.h"

struct HighScores {
  static const int32_t NUM_SCORES = 8;
  static const int32_t MAX_NAME_LENGTH = 17;
  static const int32_t MAX_SCORE_LENGTH = 10;

  struct high_score {
    high_score() : score(0) {}
    std::string name;
    int64_t score;
  };
  typedef std::array<high_score, NUM_SCORES> table;
  typedef std::array<table, PLAYERS> mode_table;
  typedef std::array<high_score, PLAYERS> boss_table;

  mode_table normal;
  mode_table hard;
  mode_table fast;
  mode_table what;
  boss_table boss;

  static std::size_t size(Mode::mode mode);
  high_score& get(Mode::mode mode, int32_t players, int32_t index);
  const high_score& get(Mode::mode mode, int32_t players, int32_t index) const;

  bool is_high_score(Mode::mode mode, int32_t players, int64_t score) const;
  void add_score(Mode::mode mode, int32_t players, const std::string& name, int64_t score);
};

struct SaveData {
  SaveData();
  void save() const;

  int32_t bosses_killed;
  int32_t hard_mode_bosses_killed;
  HighScores high_scores;
};

struct Settings {
  Settings();
  void save() const;

  bool windowed;
  fixed volume;
};

#endif
