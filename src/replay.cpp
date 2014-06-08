#include "replay.h"
#include "z0_game.h"
#include "../gen/iispace.pb.h"
#include <algorithm>
#include <fstream>

Replay::Replay(const std::string& path)
  : recording(false)
  , okay(true)
  , seed(0)
  , players(0)
  , mode(Mode::NORMAL)
  , can_face_secret_boss(false)
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
  proto::Replay replay;
  replay.ParseFromString(temp);

  mode = Mode::mode(replay.game_mode());
  players = replay.players();
  seed = replay.seed();
  can_face_secret_boss = replay.can_face_secret_boss();

  z::seed((int32_t) seed);

  for (const auto& frame : replay.player_frame()) {
    PlayerFrame pf;
    pf.velocity = vec2(fixed::from_internal(frame.velocity_x()),
                       fixed::from_internal(frame.velocity_y()));
    pf.target = vec2(fixed::from_internal(frame.target_x()),
                     fixed::from_internal(frame.target_y()));
    pf.keys = frame.keys();
    player_frames.push_back(pf);
  }
}

Replay::Replay(int32_t players, Mode::mode mode, bool can_face_secret_boss)
  : recording(true)
  , okay(true)
  , seed(time(0))
  , players(players)
  , mode(mode)
  , can_face_secret_boss(can_face_secret_boss)
{
  z::seed((int32_t) seed);
}

void Replay::record(const vec2& velocity, const vec2& target, int32_t keys)
{
  player_frames.push_back(PlayerFrame{velocity, target, keys});
}

void Replay::end_recording(const std::string& name, int64_t score) const
{
  if (!recording) {
    return;
  }
  proto::Replay replay;
  replay.set_game_version("WiiSPACE v1.3 replay");
  replay.set_game_mode(mode);
  replay.set_seed(seed);
  replay.set_players(players);
  replay.set_can_face_secret_boss(can_face_secret_boss);

  for (const auto& frame : player_frames) {
    proto::PlayerFrame& f = *replay.add_player_frame();
    f.set_velocity_x(frame.velocity.x.to_internal());
    f.set_velocity_y(frame.velocity.y.to_internal());
    f.set_target_x(frame.target.x.to_internal());
    f.set_target_y(frame.target.y.to_internal());
    f.set_keys(frame.keys);
  }

  std::stringstream ss;
  ss << "replays/" << seed << "_" <<
      players << "p_" <<
      (mode == Mode::BOSS ? "bossmode_" :
       mode == Mode::HARD ? "hardmode_" :
       mode == Mode::FAST ? "fastmode_" :
       mode == Mode::WHAT ? "w-hatmode_" : "") <<
      name << "_" << score << ".wrp";

  std::string temp;
  replay.SerializeToString(&temp);
  std::ofstream file;
  file.open(ss.str().c_str(), std::ios::binary);
  file << z::crypt(z::compress_string(temp), Lib::SUPER_ENCRYPTION_KEY);
  file.close();
}