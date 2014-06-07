#include "save.h"
#include "lib.h"
#include <algorithm>
#include <fstream>

static const std::string SETTINGS_WINDOWED = "Windowed";
static const std::string SETTINGS_VOLUME = "Volume";
static const std::string SETTINGS_PATH = "wiispace.txt";
static const std::string SAVE_PATH = "wiispace.sav";

std::size_t HighScores::size(Mode::mode mode)
{
  return mode == Mode::BOSS ? 1 : NUM_SCORES;
}

HighScores::high_score& HighScores::get(
    Mode::mode mode, int32_t players, int32_t index)
{
  return mode == Mode::NORMAL ? normal[players][index] :
         mode == Mode::HARD ? hard[players][index] :
         mode == Mode::FAST ? fast[players][index] :
         mode == Mode::WHAT ? what[players][index] : boss[players];
}

const HighScores::high_score& HighScores::get(
    Mode::mode mode, int32_t players, int32_t index) const
{
  return mode == Mode::NORMAL ? normal[players][index] :
         mode == Mode::HARD ? hard[players][index] :
         mode == Mode::FAST ? fast[players][index] :
         mode == Mode::WHAT ? what[players][index] : boss[players];
}

bool HighScores::is_high_score(
    Mode::mode mode, int32_t players, int64_t score) const
{
  return get(mode, players, size(mode) - 1).score < score;
}

void HighScores::add_score(Mode::mode mode, int32_t players,
                           const std::string& name, int64_t score)
{
  for (int32_t find = 0; find < size(mode); ++find) {
    if (score <= get(mode, players, find).score) {
      continue;
    }
    for (int32_t move = find; 1 + move < size(mode); ++move) {
      get(mode, players, 1 + move) = get(mode, players, move);
    }
    get(mode, players, find).name = name;
    get(mode, players, find).score = score;
    break;
  }
}

SaveData::SaveData()
  : bosses_killed(0)
  , hard_mode_bosses_killed(0)
{
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
  int32_t table = 0;
  int32_t players = 0;
  int32_t index = 0;
  while (getline(decrypted, line)) {
    auto& s = high_scores.get(Mode::mode(table), players, index);

    std::size_t split = line.find(' ');
    std::stringstream ss(line.substr(0, split));
    std::string name;
    if (line.length() > split + 1) {
      s.name = line.substr(split + 1);
    }
    ss >> s.score;
    find_total += s.score;

    ++index;
    if (index >= high_scores.size(Mode::mode(table))) {
      index = 0;
      ++players;
    }
    if (players >= PLAYERS) {
      players = 0;
      ++table;
    }
    if (table >= Mode::END_MODES) {
      break;
    }
  }

  if (find_total != total || find_total % 17 != t17 ||
      find_total % 701 != t701 || find_total % 1171 != t1171 ||
      find_total % 1777 != t1777) {
    high_scores = HighScores();
  }
}

void SaveData::save() const
{
  int64_t total = 0;
  for (int32_t mode = 0; mode < Mode::END_MODES; ++mode) {
    for (int32_t players = 0; players < PLAYERS; ++players) {
      for (int32_t i = 0; i < high_scores.size(Mode::mode(mode)); ++i) {
        total += high_scores.get(Mode::mode(mode), players, i).score;
      }
    }
  }
  std::stringstream out;
  out << "WiiSPACE v1.3\n" << bosses_killed << " " <<
      hard_mode_bosses_killed << " " << total << " " <<
      (total % 17) << " " << (total % 701) << " " <<
      (total % 1171) << " " << (total % 1777) << "\n";
  
  for (int32_t mode = 0; mode < Mode::END_MODES; ++mode) {
    for (int32_t players = 0; players < PLAYERS; ++players) {
      for (int32_t i = 0; i < high_scores.size(Mode::mode(mode)); ++i) {
        const auto& s = high_scores.get(Mode::mode(mode), players, i);
        out << s.score << " " << s.name << "\n";
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
