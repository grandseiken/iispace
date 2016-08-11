#include "replay.h"
#include <algorithm>
#include <fstream>
#include "z0_game.h"

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
    temp = z::decompress_string(z::crypt(b.str(), Lib::SUPER_ENCRYPTION_KEY));
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

Replay::Replay(Mode::mode mode, int32_t players, bool can_face_secret_boss) {
  replay.set_game_version("WiiSPACE v1.3 replay");
  replay.set_game_mode(mode);
  replay.set_players(players);
  replay.set_can_face_secret_boss(can_face_secret_boss);

  int32_t seed = int32_t(time(0));
  replay.set_seed(seed);
}

void Replay::record(const vec2& velocity, const vec2& target, int32_t keys) {
  proto::PlayerFrame& frame = *replay.add_player_frame();
  frame.set_velocity_x(velocity.x.to_internal());
  frame.set_velocity_y(velocity.y.to_internal());
  frame.set_target_x(target.x.to_internal());
  frame.set_target_y(target.y.to_internal());
  frame.set_keys(keys);
}

void Replay::write(const std::string& name, int64_t score) const {
  std::stringstream ss;
  ss << "replays/" << replay.seed() << "_" << replay.players() << "p_"
     << (replay.game_mode() == Mode::BOSS ? "bossmode_" : replay.game_mode() == Mode::HARD
                 ? "hardmode_"
                 : replay.game_mode() == Mode::FAST ? "fastmode_" : replay.game_mode() == Mode::WHAT
                         ? "w-hatmode_"
                         : "")
     << name << "_" << score << ".wrp";

  std::string temp;
  replay.SerializeToString(&temp);
  std::ofstream file;
  file.open(ss.str().c_str(), std::ios::binary);
  file << z::crypt(z::compress_string(temp), Lib::SUPER_ENCRYPTION_KEY);
  file.close();
}