#include "game/core/save.h"
#include "game/core/lib.h"
#include "game/proto/iispace.pb.h"
#include <algorithm>
#include <fstream>

namespace {
const std::string kSettingsWindowed = "Windowed";
const std::string kSettingsVolume = "Volume";
const std::string kSettingsPath = "wiispace.txt";
const std::string kSavePath = "wiispace.sav";
}  // namespace

std::size_t HighScores::size(game_mode mode) {
  return mode == game_mode::kBoss ? 1 : kNumScores;
}

HighScores::high_score& HighScores::get(game_mode mode, std::int32_t players, std::int32_t index) {
  return mode == game_mode::kNormal ? normal[players][index]
      : mode == game_mode::kHard    ? hard[players][index]
      : mode == game_mode::kFast    ? fast[players][index]
      : mode == game_mode::kWhat    ? what[players][index]
                                    : boss[players];
}

const HighScores::high_score&
HighScores::get(game_mode mode, std::int32_t players, std::int32_t index) const {
  return mode == game_mode::kNormal ? normal[players][index]
      : mode == game_mode::kHard    ? hard[players][index]
      : mode == game_mode::kFast    ? fast[players][index]
      : mode == game_mode::kWhat    ? what[players][index]
                                    : boss[players];
}

bool HighScores::is_high_score(game_mode mode, std::int32_t players, std::int64_t score) const {
  return get(mode, players, size(mode) - 1).score < score;
}

void HighScores::add_score(game_mode mode, std::int32_t players, const std::string& name,
                           std::int64_t score) {
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
  file.open(kSavePath, std::ios::binary);

  if (!file) {
    return;
  }

  std::stringstream b;
  b << file.rdbuf();
  proto::SaveData proto;
  proto.ParseFromString(z::crypt(b.str(), Lib::kSuperEncryptionKey));
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
  std::string encrypted = z::crypt(temp, Lib::kSuperEncryptionKey);
  std::ofstream file;
  file.open(kSavePath, std::ios::binary);
  file << encrypted;
  file.close();
}

Settings::Settings() : windowed(false), volume(100) {
  std::ifstream file;
  file.open(kSettingsPath);

  if (!file) {
    std::ofstream out;
    out.open(kSettingsPath);
    out << kSettingsWindowed << " 0\n" << kSettingsVolume << " 100.0";
    out.close();
    return;
  }

  std::string line;
  while (getline(file, line)) {
    std::stringstream ss(line);
    std::string key;
    ss >> key;
    if (key.compare(kSettingsWindowed) == 0) {
      ss >> windowed;
    }
    if (key.compare(kSettingsVolume) == 0) {
      float t;
      ss >> t;
      volume = std::int32_t(t);
    }
  }
}

void Settings::save() const {
  std::ofstream out;
  out.open(kSettingsPath);
  out << kSettingsWindowed << " " << (windowed ? "1" : "0") << "\n"
      << kSettingsVolume << " " << volume.to_int();
  out.close();
}
