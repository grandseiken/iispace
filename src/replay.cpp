#include "replay.h"
#include "z0_game.h"
#include <algorithm>
#include <fstream>

void fixed_write(std::stringstream& out, const fixed& f)
{
  auto t = f.to_internal();
  out.write((char*) &t, sizeof(t));
}

void fixed_read(fixed& f, std::stringstream& in)
{
  auto t = f.to_internal();
  in.read((char*) &t, sizeof(t));
  f = fixed::from_internal(t);
}

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
  std::stringstream decrypted(
      temp, std::ios::in | std::ios::out | std::ios::binary);

  getline(decrypted, line);
  if (line.compare("WiiSPACE v1.3 replay") != 0) {
    file.close();
    okay = false;
    error = "Cannot play:\n" + line + "\nusing WiiSPACE v1.3";
    Lib::set_working_directory(false);
    return;
  }

  getline(decrypted, line);
  std::stringstream ss(line);
  ss >> players;
  ss >> can_face_secret_boss;

  bool bb;
  ss >> bb;
  mode = Mode::mode(mode | bb * Mode::BOSS);
  ss >> bb;
  mode = Mode::mode(mode | bb * Mode::HARD);
  ss >> bb;
  mode = Mode::mode(mode | bb * Mode::FAST);
  ss >> bb;
  mode = Mode::mode(mode | bb * Mode::WHAT);
  ss >> seed;

  z::seed((int32_t) seed);
  players = std::max(1, std::min(4, players));

  while (!decrypted.eof()) {
    PlayerFrame pf;
    char k;
    fixed_read(pf.velocity.x, decrypted);
    fixed_read(pf.velocity.y, decrypted);
    fixed_read(pf.target.x, decrypted);
    fixed_read(pf.target.y, decrypted);
    decrypted.read(&k, sizeof(char));
    pf.keys = k;
    player_frames.push_back(pf);
  }

  file.close();
  Lib::set_working_directory(false);
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

  std::stringstream stream(std::ios::in | std::ios::out | std::ios::binary);
  stream << "WiiSPACE v1.3 replay\n" << players << " " <<
      can_face_secret_boss << " " <<
      (mode == Mode::BOSS) << " " <<
      (mode == Mode::HARD) << " " <<
      (mode == Mode::FAST) << " " <<
      (mode == Mode::WHAT) << " " << seed << "\n";

  for (const auto& frame : player_frames) {
    char k = frame.keys;
    std::stringstream binary(std::ios::in | std::ios::out | std::ios::binary);
    fixed_write(binary, frame.velocity.x);
    fixed_write(binary, frame.velocity.y);
    fixed_write(binary, frame.target.x);
    fixed_write(binary, frame.target.y);
    binary.write(&k, sizeof(char));
    stream << binary.str();
  }

  std::stringstream ss;
  ss << "replays/" << seed << "_" <<
      players << "p_" <<
      (mode == Mode::BOSS ? "bossmode_" :
       mode == Mode::HARD ? "hardmode_" :
       mode == Mode::FAST ? "fastmode_" :
       mode == Mode::WHAT ? "w-hatmode_" : "") <<
      name << "_" << score << ".wrp";

  std::ofstream file;
  file.open(ss.str().c_str(), std::ios::binary);
  file << z::crypt(z::compress_string(stream.str()), Lib::SUPER_ENCRYPTION_KEY);
  file.close();
}