#include "game/data/save.h"
#include "game/data/crypt.h"
#include "game/data/proto/config.pb.h"
#include "game/data/proto/savegame.pb.h"
#include <algorithm>
#include <array>
#include <sstream>

namespace ii {
namespace {
const std::array<std::uint8_t, 2> kSaveEncryptionKey = {'<', '>'};
}  // namespace

HighScores::HighScores() {
  normal.resize(kMaxPlayers);
  hard.resize(kMaxPlayers);
  fast.resize(kMaxPlayers);
  what.resize(kMaxPlayers);
  boss.resize(kMaxPlayers);

  for (std::size_t i = 0; i < kMaxPlayers; ++i) {
    normal[i].resize(kNumScores);
    hard[i].resize(kNumScores);
    fast[i].resize(kNumScores);
    what[i].resize(kNumScores);
  }
}

std::size_t HighScores::size(game_mode mode) {
  return mode == game_mode::kBoss ? 1 : kNumScores;
}

HighScores::high_score&
HighScores::get(game_mode mode, std::uint32_t players, std::uint32_t index) {
  return mode == game_mode::kNormal ? normal[players][index]
      : mode == game_mode::kHard    ? hard[players][index]
      : mode == game_mode::kFast    ? fast[players][index]
      : mode == game_mode::kWhat    ? what[players][index]
                                    : boss[players];
}

const HighScores::high_score&
HighScores::get(game_mode mode, std::uint32_t players, std::uint32_t index) const {
  return mode == game_mode::kNormal ? normal[players][index]
      : mode == game_mode::kHard    ? hard[players][index]
      : mode == game_mode::kFast    ? fast[players][index]
      : mode == game_mode::kWhat    ? what[players][index]
                                    : boss[players];
}

bool HighScores::is_high_score(game_mode mode, std::uint32_t players, std::uint64_t score) const {
  return get(mode, players, size(mode) - 1).score < score;
}

void HighScores::add_score(game_mode mode, std::uint32_t players, const std::string& name,
                           std::uint64_t score) {
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

result<SaveGame> SaveGame::load(std::span<const std::uint8_t> bytes) {
  proto::SaveGame proto;
  auto d = crypt(bytes, kSaveEncryptionKey);
  if (!proto.ParseFromArray(d.data(), static_cast<int>(d.size()))) {
    return unexpected("Couldn't parse savegame");
  }

  SaveGame save;
  save.bosses_killed = boss_flag{static_cast<std::uint32_t>(proto.bosses_killed())};
  save.hard_mode_bosses_killed =
      boss_flag{static_cast<std::uint32_t>(proto.hard_mode_bosses_killed())};

  auto get_mode_table = [&](const proto::ModeTable& in, std::vector<HighScores::table_t>& out) {
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
  get_mode_table(proto.normal(), save.high_scores.normal);
  get_mode_table(proto.hard(), save.high_scores.hard);
  get_mode_table(proto.fast(), save.high_scores.fast);
  get_mode_table(proto.what(), save.high_scores.what);
  std::size_t players = 0;
  for (const auto& s : proto.boss().score()) {
    save.high_scores.boss[players].name = s.name();
    save.high_scores.boss[players].score = s.score();
    ++players;
  }
  return {std::move(save)};
}

result<std::vector<std::uint8_t>> SaveGame::save() const {
  proto::SaveGame proto;
  proto.set_bosses_killed(+bosses_killed);
  proto.set_hard_mode_bosses_killed(+hard_mode_bosses_killed);
  auto put_mode_table = [&](const std::vector<HighScores::table_t>& in, proto::ModeTable& out) {
    for (const auto& t : in) {
      auto& out_t = *out.add_table();
      for (const auto& s : t) {
        auto& out_s = *out_t.add_score();
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
    auto& out_s = *proto.mutable_boss()->add_score();
    out_s.set_name(s.name);
    out_s.set_score(s.score);
  }

  std::vector<std::uint8_t> data;
  data.resize(proto.ByteSizeLong());
  if (!proto.SerializeToArray(data.data(), static_cast<int>(data.size()))) {
    return unexpected("Couldn't serialize savegame");
  }
  return crypt(data, kSaveEncryptionKey);
}

result<Config> Config::load(std::span<const std::uint8_t> bytes) {
  proto::Config proto;
  if (!proto.ParseFromArray(bytes.data(), static_cast<int>(bytes.size()))) {
    return unexpected("Couldn't parse settings");
  }

  Config settings;
  settings.volume = proto.volume();
  return {settings};
}

result<std::vector<std::uint8_t>> Config::save() const {
  proto::Config proto;
  proto.set_volume(volume);
  std::vector<std::uint8_t> data;
  data.resize(proto.ByteSizeLong());
  if (!proto.SerializeToArray(data.data(), static_cast<int>(data.size()))) {
    return unexpected("Couldn't serialize settings");
  }
  return {std::move(data)};
}

}  // namespace ii
