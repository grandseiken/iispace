#include "game/core/replay.h"
#include "game/core/z0_game.h"
#include <algorithm>
#include <fstream>

Replay::Replay(const std::string& path) {
  Lib::set_working_directory(true);
  std::ifstream file;
  file.open(path.c_str(), std::ios::binary);
  std::string line;

  if (!file) {
    Lib::set_working_directory(false);
    throw std::runtime_error("Cannot open:\n" + path);
  }

  std::stringstream b;
  b << file.rdbuf();
  std::string temp;
  try {
    temp = z::decompress_string(z::crypt(b.str(), Lib::kSuperEncryptionKey));
  } catch (std::exception& e) {
    Lib::set_working_directory(false);
    throw std::runtime_error(e.what());
  }
  file.close();
  Lib::set_working_directory(false);
  if (!replay.ParseFromString(temp)) {
    throw std::runtime_error("Corrupted replay:\n" + path);
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

void Replay::write(const std::string& name, std::int64_t score) const {
  std::stringstream ss;
  auto mode = static_cast<game_mode>(replay.game_mode());
  ss << "replays/" << replay.seed() << "_" << replay.players() << "p_"
     << (mode == game_mode::kBoss       ? "bossmode_"
             : mode == game_mode::kHard ? "hardmode_"
             : mode == game_mode::kFast ? "fastmode_"
             : mode == game_mode::kWhat ? "w-hatmode_"
                                        : "")
     << name << "_" << score << ".wrp";

  std::string temp;
  replay.SerializeToString(&temp);
  std::ofstream file;
  file.open(ss.str().c_str(), std::ios::binary);
  file << z::crypt(z::compress_string(temp), Lib::kSuperEncryptionKey);
  file.close();
}