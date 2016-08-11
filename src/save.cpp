#include "save.h"
#include <iispace.pb.h>
#include <algorithm>
#include <fstream>
#include "lib.h"

static const std::string SETTINGS_WINDOWED = "Windowed";
static const std::string SETTINGS_VOLUME = "Volume";
static const std::string SETTINGS_PATH = "wiispace.txt";
static const std::string SAVE_PATH = "wiispace.sav";

std::size_t HighScores::size(Mode::mode mode) {
  return mode == Mode::BOSS ? 1 : NUM_SCORES;
}

HighScores::high_score& HighScores::get(Mode::mode mode, int32_t players, int32_t index) {
  return mode == Mode::NORMAL ? normal[players][index] : mode == Mode::HARD
          ? hard[players][index]
          : mode == Mode::FAST ? fast[players][index] : mode == Mode::WHAT ? what[players][index]
                                                                           : boss[players];
}

const HighScores::high_score& HighScores::get(Mode::mode mode, int32_t players,
                                              int32_t index) const {
  return mode == Mode::NORMAL ? normal[players][index] : mode == Mode::HARD
          ? hard[players][index]
          : mode == Mode::FAST ? fast[players][index] : mode == Mode::WHAT ? what[players][index]
                                                                           : boss[players];
}

bool HighScores::is_high_score(Mode::mode mode, int32_t players, int64_t score) const {
  return get(mode, players, size(mode) - 1).score < score;
}

void HighScores::add_score(Mode::mode mode, int32_t players, const std::string& name,
                           int64_t score) {
  for (std::size_t find = 0; find < size(mode); ++find) {
    if (score <= get(mode, players, find).score) {
      continue;
    }
    for (std::size_t move = size(mode) - 1; move > find; --move) {
      get(mode, players, move) = get(mode, players, move - 1);
    }
    get(mode, players, find).name = name;
    get(mode, players, find).score = score;
    break;
  }
}

SaveData::SaveData() : bosses_killed(0), hard_mode_bosses_killed(0) {
  std::ifstream file;
  file.open(SAVE_PATH, std::ios::binary);

  if (!file) {
    return;
  }

  std::stringstream b;
  b << file.rdbuf();
  proto::SaveData proto;
  proto.ParseFromString(z::crypt(b.str(), Lib::SUPER_ENCRYPTION_KEY));
  file.close();

  bosses_killed = proto.bosses_killed();
  hard_mode_bosses_killed = proto.hard_mode_bosses_killed();

  auto get_mode_table = [&](const proto::ModeTable& in, HighScores::mode_table& out) {
    std::size_t players = 0;
    for (const auto& t : in.table()) {
      std::size_t index = 0;
      for (const auto s : t.score()) {
        out[players][index].name = s.name();
        out[players][index].score = s.score();
        ++index;
      }
      ++players;
    }
  };
  get_mode_table(proto.normal(), high_scores.normal);
  get_mode_table(proto.hard(), high_scores.hard);
  get_mode_table(proto.fast(), high_scores.fast);
  get_mode_table(proto.what(), high_scores.what);
  std::size_t players = 0;
  for (const auto& s : proto.boss().score()) {
    high_scores.boss[players].name = s.name();
    high_scores.boss[players].score = s.score();
  }
}

void SaveData::save() const {
  proto::SaveData proto;
  proto.set_bosses_killed(bosses_killed);
  proto.set_hard_mode_bosses_killed(hard_mode_bosses_killed);
  auto put_mode_table = [&](const HighScores::mode_table& in, proto::ModeTable& out) {
    for (const auto& t : in) {
      proto::Table& out_t = *out.add_table();
      for (const auto& s : t) {
        proto::Score& out_s = *out_t.add_score();
        out_s.set_name(s.name);
        out_s.set_score(s.score);
      }
    }
  };
  put_mode_table(high_scores.normal, *proto.mutable_normal());
  put_mode_table(high_scores.hard, *proto.mutable_hard());
  put_mode_table(high_scores.fast, *proto.mutable_fast());
  put_mode_table(high_scores.what, *proto.mutable_what());
  for (const auto& s : high_scores.boss) {
    proto::Score& out_s = *proto.mutable_boss()->add_score();
    out_s.set_name(s.name);
    out_s.set_score(s.score);
  }

  std::string temp;
  proto.SerializeToString(&temp);
  std::string encrypted = z::crypt(temp, Lib::SUPER_ENCRYPTION_KEY);
  std::ofstream file;
  file.open(SAVE_PATH, std::ios::binary);
  file << encrypted;
  file.close();
}

Settings::Settings() : windowed(false), volume(100) {
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

void Settings::save() const {
  std::ofstream out;
  out.open(SETTINGS_PATH);
  out << SETTINGS_WINDOWED << " " << (windowed ? "1" : "0") << "\n"
      << SETTINGS_VOLUME << " " << volume.to_int();
  out.close();
}
