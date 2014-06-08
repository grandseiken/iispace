#include "replay.h"
#include "z0_game.h"
#include <algorithm>
#include <fstream>

Replay::Replay(const std::string& path)
  : recording(false)
  , okay(true)
{
  Lib::set_working_directory(true);
  std::ifstream file;
  file.open(path.c_str(), std::ios::binary);
  std::string line;

  if (!file) {
    okay = false;
    error = "Cannot open:\n" + path;
    Lib::set_working_directory(false);
    return;
  }

  std::stringstream b;
  b << file.rdbuf();
  std::string temp;
  try {
    temp = z::decompress_string(z::crypt(b.str(), Lib::SUPER_ENCRYPTION_KEY));
  }
  catch (std::exception& e) {
    okay = false;
    error = e.what();
    Lib::set_working_directory(false);
    return;
  }
  file.close();
  Lib::set_working_directory(false);
  if (!replay.ParseFromString(temp)) {
    okay = false;
    error = "Corrupted replay:\n" + path;
    return;
  }
  z::seed((int32_t) replay.seed());
}

Replay::Replay(int32_t players, Mode::mode mode, bool can_face_secret_boss)
  : recording(true)
  , okay(true)
{
  replay.set_game_version("WiiSPACE v1.3 replay");
  replay.set_game_mode(mode);
  replay.set_players(players);
  replay.set_can_face_secret_boss(can_face_secret_boss);

  int32_t seed = int32_t(time(0));
  replay.set_seed(seed);
  z::seed((int32_t) seed);
}

void Replay::record(const vec2& velocity, const vec2& target, int32_t keys)
{
  proto::PlayerFrame& frame = *replay.add_player_frame();
  frame.set_velocity_x(velocity.x.to_internal());
  frame.set_velocity_y(velocity.y.to_internal());
  frame.set_target_x(target.x.to_internal());
  frame.set_target_y(target.y.to_internal());
  frame.set_keys(keys);
}

void Replay::end_recording(const std::string& name, int64_t score) const
{
  if (!recording) {
    return;
  }

  std::stringstream ss;
  ss << "replays/" << replay.seed() << "_" <<
      replay.players() << "p_" <<
      (replay.game_mode() == Mode::BOSS ? "bossmode_" :
       replay.game_mode() == Mode::HARD ? "hardmode_" :
       replay.game_mode() == Mode::FAST ? "fastmode_" :
       replay.game_mode() == Mode::WHAT ? "w-hatmode_" : "") <<
      name << "_" << score << ".wrp";

  std::string temp;
  replay.SerializeToString(&temp);
  std::ofstream file;
  file.open(ss.str().c_str(), std::ios::binary);
  file << z::crypt(z::compress_string(temp), Lib::SUPER_ENCRYPTION_KEY);
  file.close();
}