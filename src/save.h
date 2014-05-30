#ifndef IISPACE_SRC_SAVE_H
#define IISPACE_SRC_SAVE_H

#include "z.h"
#include <vector>

struct HighScore {
  std::string name;
  int64_t score;
};

typedef std::vector<HighScore> HighScoreList;
typedef std::vector<HighScoreList> HighScoreTable;
inline bool score_sort(const HighScore& a, const HighScore& b)
{
  return a.score > b.score;
}

struct SaveData {
  SaveData();
  void save() const;

  static const int32_t MAX_NAME_LENGTH = 17;
  static const int32_t MAX_SCORE_LENGTH = 10;
  static const int32_t NUM_HIGH_SCORES = 8;

  int32_t bosses_killed;
  int32_t hard_mode_bosses_killed;
  HighScoreTable high_scores;
};

struct Settings {
  Settings();
  void save() const;

  bool windowed;
  fixed volume;
};

#endif