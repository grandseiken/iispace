#include "game/core/replay.h"
#include "game/core/lib.h"
#include "game/io/file/filesystem.h"
#include <algorithm>
#include <sstream>

Replay::Replay(const ii::io::Filesystem& fs, const std::string& path) {
  // TODO: exceptions.
  auto data = fs.read_replay(path);
  if (!data) {
    throw std::runtime_error{data.error()};
  }

  std::string temp;
  try {
    temp = z::decompress_string(z::crypt({data->begin(), data->end()}, Lib::kSuperEncryptionKey));
  } catch (std::exception& e) {
    throw std::runtime_error(e.what());
  }
  if (!replay.ParseFromString(temp)) {
    throw std::runtime_error{"Corrupted replay:\n" + path};
  }
}

Replay::Replay(game_mode mode, std::int32_t players, bool can_face_secret_boss) {
  replay.set_game_version("WiiSPACE v1.3 replay");
  replay.set_game_mode(static_cast<std::int32_t>(mode));
  replay.set_players(players);
  replay.set_can_face_secret_boss(can_face_secret_boss);

  std::int32_t seed = static_cast<std::int32_t>(time(0));
  replay.set_seed(seed);
}

void Replay::record(const vec2& velocity, const vec2& target, std::int32_t keys) {
  proto::PlayerFrame& frame = *replay.add_player_frame();
  frame.set_velocity_x(velocity.x.to_internal());
  frame.set_velocity_y(velocity.y.to_internal());
  frame.set_target_x(target.x.to_internal());
  frame.set_target_y(target.y.to_internal());
  frame.set_keys(keys);
}

void Replay::write(ii::io::Filesystem& fs, const std::string& name, std::int64_t score) const {
  std::stringstream ss;
  auto mode = static_cast<game_mode>(replay.game_mode());
  ss << "replays/" << replay.seed() << "_" << replay.players() << "p_"
     << (mode == game_mode::kBoss       ? "bossmode_"
             : mode == game_mode::kHard ? "hardmode_"
             : mode == game_mode::kFast ? "fastmode_"
             : mode == game_mode::kWhat ? "w-hatmode_"
                                        : "")
     << name << "_" << score;

  std::string temp;
  temp = z::crypt(z::compress_string(temp), Lib::kSuperEncryptionKey);
  replay.SerializeToString(&temp);
  (void)fs.write_replay(ss.str(),
                        {reinterpret_cast<std::uint8_t*>(temp.data()),
                         reinterpret_cast<std::uint8_t*>(temp.data() + temp.size())});
}