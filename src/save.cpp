#include "save.h"
#include "lib.h"
#include <algorithm>
#include <fstream>

static const std::string SETTINGS_WINDOWED = "Windowed";
static const std::string SETTINGS_VOLUME = "Volume";
static const std::string SETTINGS_PATH = "wiispace.txt";
static const std::string SAVE_PATH = "wiispace.sav";

SaveData::SaveData()
  : bosses_killed(0)
  , hard_mode_bosses_killed(0)
{
  for (int32_t i = 0; i < 4 * Lib::PLAYERS + 1; ++i) {
    high_scores.emplace_back();
    for (int32_t j = 0;
         j < (i != Lib::PLAYERS ? NUM_HIGH_SCORES : Lib::PLAYERS); ++j) {
      high_scores.back().emplace_back();
    }
  }

  std::ifstream file;
  file.open(SAVE_PATH, std::ios::binary);

  if (!file) {
    return;
  }

  std::stringstream b;
  b << file.rdbuf();
  std::stringstream decrypted(z::crypt(b.str(), Lib::SUPER_ENCRYPTION_KEY));
  file.close();

  std::string line;
  getline(decrypted, line);
  if (line.compare("WiiSPACE v1.3") != 0) {
    return;
  }

  int64_t total = 0;
  int64_t t17 = 0;
  int64_t t701 = 0;
  int64_t t1171 = 0;
  int64_t t1777 = 0;

  getline(decrypted, line);
  std::stringstream ssb(line);
  ssb >> bosses_killed;
  ssb >> hard_mode_bosses_killed;
  ssb >> total;
  ssb >> t17;
  ssb >> t701;
  ssb >> t1171;
  ssb >> t1777;

  int64_t find_total = 0;
  int32_t i = 0;
  int32_t j = 0;
  while (getline(decrypted, line)) {
    std::size_t split = line.find(' ');
    std::stringstream ss(line.substr(0, split));
    std::string name;
    if (line.length() > split + 1) {
      high_scores[i][j].name = line.substr(split + 1);
    }
    ss >> high_scores[i][j].score;
    find_total += high_scores[i][j].score;

    j++;
    if (j >= NUM_HIGH_SCORES || (i == Lib::PLAYERS && j >= Lib::PLAYERS)) {
      j = 0;
      i++;
    }
    if (i >= 4 * Lib::PLAYERS + 1) {
      break;
    }
  }

  if (find_total != total || find_total % 17 != t17 ||
      find_total % 701 != t701 || find_total % 1171 != t1171 ||
      find_total % 1777 != t1777) {
    high_scores.clear();
    for (int32_t i = 0; i < 4 * Lib::PLAYERS + 1; ++i) {
      high_scores.emplace_back();
      for (int32_t j = 0;
           j < (i != Lib::PLAYERS ? NUM_HIGH_SCORES : Lib::PLAYERS); ++j) {
        high_scores.back().emplace_back();
      }
    }
  }

  for (int32_t i = 0; i < 4 * Lib::PLAYERS + 1; i++) {
    if (i != Lib::PLAYERS) {
      std::sort(high_scores[i].begin(), high_scores[i].end(), &score_sort);
    }
  }
}

void SaveData::save() const
{
  int64_t total = 0;
  for (int32_t i = 0; i < 4 * Lib::PLAYERS + 1; ++i) {
    for (int32_t j = 0;
         j < (i != Lib::PLAYERS ? NUM_HIGH_SCORES : Lib::PLAYERS); ++j) {
      if ((std::size_t) i < high_scores.size() &&
          (std::size_t) j < high_scores[i].size()) {
        total += high_scores[i][j].score;
      }
    }
  }

  std::stringstream out;
  out << "WiiSPACE v1.3\n" << bosses_killed << " " <<
      hard_mode_bosses_killed << " " << total << " " <<
      (total % 17) << " " << (total % 701) << " " <<
      (total % 1171) << " " << (total % 1777) << "\n";

  for (int32_t i = 0; i < 4 * Lib::PLAYERS + 1; ++i) {
    for (int32_t j = 0;
         j < (i != Lib::PLAYERS ? NUM_HIGH_SCORES : Lib::PLAYERS); ++j) {
      if ((std::size_t) i < high_scores.size() &&
          (std::size_t) j < high_scores[i].size()) {
        out << high_scores[i][j].score << " " << high_scores[i][j].name << "\n";
      }
      else {
        out << "0\n";
      }
    }
  }

  std::string encrypted = z::crypt(out.str(), Lib::SUPER_ENCRYPTION_KEY);
  std::ofstream file;
  file.open(SAVE_PATH, std::ios::binary);
  file << encrypted;
  file.close();
}

Settings::Settings()
  : windowed(false)
  , volume(100)
{
  std::ifstream file;
  file.open(SETTINGS_PATH);

  if (!file) {
    std::ofstream out;
    out.open(SETTINGS_PATH);
    out << SETTINGS_WINDOWED << " 0\n" << SETTINGS_VOLUME << " 100.0";
    out.close();
    return;
  }

  std::string line;
  while (getline(file, line)) {
    std::stringstream ss(line);
    std::string key;
    ss >> key;
    if (key.compare(SETTINGS_WINDOWED) == 0) {
      ss >> windowed;
    }
    if (key.compare(SETTINGS_VOLUME) == 0) {
      float t;
      ss >> t;
      volume = int32_t(t);
    }
  }
}

void Settings::save() const
{
  std::ofstream out;
  out.open(SETTINGS_PATH);
  out <<
      SETTINGS_WINDOWED << " " << (windowed ? "1" : "0") << "\n" <<
      SETTINGS_VOLUME << " " << volume.to_int();
  out.close();
}
