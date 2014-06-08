#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>
#include "../z.h"
#include "../../gen/iispace.pb.h"

std::string crypt(const std::string& text, const std::string& key)
{
  std::string r = "";
  for (std::size_t i = 0; i < text.length(); i++) {
    char c = text[i] ^ key[i % key.length()];
    if (text[i] == '\0' || c == '\0') {
      r += text[i];
    }
    else {
      r += c;
    }
  }
  return r;
}

std::string read_file(const std::string& filename)
{
  std::ifstream in(filename, std::ios::binary);
  std::string contents;
  in.seekg(0, std::ios::end);
  contents.resize((unsigned) in.tellg());
  in.seekg(0, std::ios::beg);
  in.read(&contents[0], contents.size());
  in.close();
  return contents;
}

void fixed_read(fixed& f, std::stringstream& in)
{
  auto t = f.to_internal();
  in.read((char*) &t, sizeof(t));
  f = fixed::from_internal(t);
}

proto::Replay convert(const std::string& contents)
{
  std::string decompress;
  decompress = z::decompress_string(contents);
  std::stringstream dc(decompress,
                       std::ios::in | std::ios::out | std::ios::binary);

  std::string line;
  getline(dc, line);
  if (line.compare("WiiSPACE v1.3 replay") != 0) {
    throw std::runtime_error("invalid replay");
  }
  proto::Replay replay;
  replay.set_game_version("WiiSPACE v1.3 replay");

  getline(dc, line);
  std::stringstream ss(line);
  int32_t players;
  ss >> players;
  bool can_face_secret_boss;
  ss >> can_face_secret_boss;
  replay.set_players(std::max(1, std::min(4, players)));
  replay.set_can_face_secret_boss(can_face_secret_boss);

  Mode::mode mode = Mode::NORMAL;
  bool bb;
  ss >> bb;
  mode = Mode::mode(mode | bb * Mode::BOSS);
  ss >> bb;
  mode = Mode::mode(mode | bb * Mode::HARD);
  ss >> bb;
  mode = Mode::mode(mode | bb * Mode::FAST);
  ss >> bb;
  mode = Mode::mode(mode | bb * Mode::WHAT);
  int32_t seed;
  ss >> seed;
  replay.set_game_mode(mode);
  replay.set_seed(seed);

  while (!dc.eof()) {
    proto::PlayerFrame& frame = *replay.add_player_frame();
    fixed fix;
    fixed_read(fix, dc);
    frame.set_velocity_x(fix.to_internal());
    fixed_read(fix, dc);
    frame.set_velocity_y(fix.to_internal());
    fixed_read(fix, dc);
    frame.set_target_x(fix.to_internal());
    fixed_read(fix, dc);
    frame.set_target_y(fix.to_internal());
    char k;
    dc.read(&k, sizeof(char));
    frame.set_keys(k);
  }

  return replay;
}

int main(int argc, char** argv)
{
  if (argc < 4) {
    std::cerr <<
        "usage: " << argv[0] << " old_key_file new_key_file files..." << std::endl;
    return 1;
  }
 
  std::string old_key = read_file(argv[1]);
  std::string new_key = read_file(argv[2]);

  for (int i = 3; i < argc; ++i) {
    std::string filename = argv[i];
    std::cout << "re-encrypting " << filename << std::endl;

    std::string temp = crypt(read_file(filename), old_key);
    proto::Replay replay = convert(temp);
    replay.SerializeToString(&temp);

    std::ofstream out(filename + ".recrypt", std::ios::binary);
    out << crypt(z::compress_string(temp), new_key);
    out.close();
  }
  return 0;
}