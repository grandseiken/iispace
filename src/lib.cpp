#include "lib.h"
#include <fstream>
#include <algorithm>
#include <time.h>
#include <cstring>

const std::string Lib::SUPER_ENCRYPTION_KEY = "<>";
#define SETTINGS_DISABLEBACKGROUND "DisableBackground"
#define SETTINGS_HUDCORRECTION "HUDCorrection"
#define SETTINGS_WINDOWED "Windowed"
#define SETTINGS_VOLUME "Volume"

Lib::Lib()
  : _recording(false)
  , _frameCount(1)
  , _cycle(0)
  , _replay()
  , _recordSeed(0)
  , _players(1)
  , _record(std::ios::in | std::ios::out | std::ios::binary)
{
}

bool Lib::IsKeyPressed(Key k) const
{
  for (int i = 0; i < PLAYERS; i++)
  if (IsKeyPressed(i, k)) {
    return true;
  }
  return false;
}

bool Lib::IsKeyReleased(Key k) const
{
  for (int i = 0; i < PLAYERS; i++)
  if (IsKeyReleased(i, k)) {
    return true;
  }
  return false;
}

bool Lib::IsKeyHeld(Key k) const
{
  for (int i = 0; i < PLAYERS; i++)
  if (IsKeyHeld(i, k)) {
    return true;
  }
  return false;
}

#include <string>
#include <stdexcept>
#include <iostream>
#include <iomanip>
#include <sstream>

#include <zlib.h>
std::string compress_string(const std::string& str,
                            int compressionLevel = Z_BEST_COMPRESSION)
{
  z_stream zs;
  memset(&zs, 0, sizeof(zs));

  if (deflateInit(&zs, compressionLevel) != Z_OK) {
    throw std::runtime_error("deflateInit failed while compressing.");
  }

  zs.next_in = (Bytef*) str.data();
  zs.avail_in = uInt(str.size());

  int ret;
  char outbuffer[32768];
  std::string outstring;

  do {
    zs.next_out = reinterpret_cast< Bytef* >(outbuffer);
    zs.avail_out = sizeof(outbuffer);

    ret = deflate(&zs, Z_FINISH);

    if (outstring.size() < zs.total_out) {
      outstring.append(outbuffer, zs.total_out - outstring.size());
    }
  } while (ret == Z_OK);

  deflateEnd(&zs);

  if (ret != Z_STREAM_END) {
    std::ostringstream oss;
    oss << "Exception during zlib compression: (" << ret << ") " << zs.msg;
    throw std::runtime_error(oss.str());
  }

  return outstring;
}

std::string decompress_string(const std::string& str)
{
  z_stream zs;
  memset(&zs, 0, sizeof(zs));

  if (inflateInit(&zs) != Z_OK) {
    throw std::runtime_error("inflateInit failed while decompressing.");
  }

  zs.next_in = (Bytef*) str.data();
  zs.avail_in = uInt(str.size());

  int ret;
  char outbuffer[32768];
  std::string outstring;

  do {
    zs.next_out = reinterpret_cast< Bytef* >(outbuffer);
    zs.avail_out = sizeof(outbuffer);

    ret = inflate(&zs, 0);

    if (outstring.size() < zs.total_out) {
      outstring.append(outbuffer, zs.total_out - outstring.size());
    }
  } while (ret == Z_OK);

  inflateEnd(&zs);

  if (ret != Z_STREAM_END) {
    std::ostringstream oss;
    oss << "Exception during zlib decompression: (" << ret << ") " << zs.msg;
    throw std::runtime_error(oss.str());
  }

  return outstring;
}

Lib::SaveData Lib::LoadSaveData()
{
  HighScoreTable r;
  for (int i = 0; i < 4 * PLAYERS + 1; ++i) {
    r.push_back(HighScoreList());
    for (unsigned int j = 0;
         j < (i != PLAYERS ? NUM_HIGH_SCORES : PLAYERS); ++j) {
      r[i].push_back(HighScore());
    }
  }

  SaveData data;
  data._bossesKilled = 0;
  data._hardModeBossesKilled = 0;
  data._highScores = r;

  std::ifstream file;
  file.open(SavePath().c_str(), std::ios::binary);

  if (!file) {
    return data;
  }
  std::stringstream b;
  b << file.rdbuf();
  std::stringstream decrypted(Crypt(b.str(), SUPER_ENCRYPTION_KEY));

  std::string line;
  getline(decrypted, line);
  if (line.compare("WiiSPACE v1.3") != 0) {
    file.close();
    return data;
  }

  long total = 0;
  long t17 = 0;
  long t701 = 0;
  long t1171 = 0;
  long t1777 = 0;
  getline(decrypted, line);
  std::stringstream ssb(line);
  ssb >> data._bossesKilled;
  ssb >> data._hardModeBossesKilled;
  ssb >> total;
  ssb >> t17;
  ssb >> t701;
  ssb >> t1171;
  ssb >> t1777;

  long findTotal = 0;
  int i = 0;
  unsigned int j = 0;
  while (getline(decrypted, line)) {
    std::size_t split = line.find(' ');
    std::stringstream ss(line.substr(0, split));
    std::string name;
    if (line.length() > split + 1)
      name = line.substr(split + 1);
    long score;
    ss >> score;

    r[i][j].first = name;
    r[i][j].second = score;

    findTotal += score;

    j++;
    if (j >= NUM_HIGH_SCORES || (i == PLAYERS && int(j) >= PLAYERS)) {
      j = 0;
      i++;
    }
    if (i >= 4 * PLAYERS + 1) {
      break;
    }
  }

  if (findTotal != total || findTotal % 17 != t17 ||
      findTotal % 701 != t701 || findTotal % 1171 != t1171 ||
      findTotal % 1777 != t1777) {
    r.clear();
    for (int i = 0; i < 4 * PLAYERS + 1; i++) {
      r.push_back(HighScoreList());
      for (unsigned int j = 0;
           j < (i != PLAYERS ? NUM_HIGH_SCORES : PLAYERS); j++) {
        r[i].push_back(HighScore());
      }
    }
  }

  for (int i = 0; i < 4 * PLAYERS + 1; i++) {
    if (i != PLAYERS) {
      std::sort(r[i].begin(), r[i].end(), &ScoreSort);
    }
  }

  data._highScores = r;
  file.close();
  return data;

}

void Lib::SaveSaveData(const SaveData& version2)
{
  uint64_t total = 0;
  for (int i = 0; i < 4 * PLAYERS + 1; ++i) {
    for (unsigned int j = 0;
         j < (i != PLAYERS ? NUM_HIGH_SCORES : PLAYERS); ++j) {
      if (i < int(version2._highScores.size()) &&
          j < version2._highScores[i].size())
        total += version2._highScores[i][j].second;
    }
  }

  std::stringstream out;
  out << "WiiSPACE v1.3\n" << version2._bossesKilled << " " <<
      version2._hardModeBossesKilled << " " << total << " " <<
      (total % 17) << " " << (total % 701) << " " <<
      (total % 1171) << " " << (total % 1777) << "\n";

  for (int i = 0; i < 4 * PLAYERS + 1; ++i) {
    for (unsigned int j = 0;
         j < (i != PLAYERS ? NUM_HIGH_SCORES : PLAYERS); ++j) {
      if (i < int(version2._highScores.size()) &&
          j < version2._highScores[i].size()) {
        out << version2._highScores[i][j].second << " " <<
            version2._highScores[i][j].first << "\n";
      }
      else {
        out << "0\n";
      }
    }
  }

  std::string encrypted = Crypt(out.str(), SUPER_ENCRYPTION_KEY);

  std::ofstream file;
  file.open(SavePath().c_str(), std::ios::binary);
  file << encrypted;
  file.close();
}

std::string Lib::Crypt(const std::string& text, const std::string& key)
{
  std::string r = "";
  for (unsigned int i = 0; i < text.length(); i++) {
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

Lib::Settings Lib::LoadSaveSettings() const
{
  Settings settings;
  settings._disableBackground = 0;
  settings._hudCorrection = 0;
  settings._windowed = 0;
  settings._volume = 100;
  std::ifstream file;
  file.open(SettingsPath().c_str());

  if (!file) {
    std::ofstream out;
    out.open(SettingsPath().c_str());
    out <<
        SETTINGS_DISABLEBACKGROUND << " 0\n" <<
        SETTINGS_HUDCORRECTION << " 0\n" <<
        SETTINGS_WINDOWED << " 0\n" <<
        SETTINGS_VOLUME << " 100.0";
    out.close();
  }
  else {
    std::string line;
    while (getline(file, line)) {
      std::stringstream ss(line);
      std::string key;
      ss >> key;
      if (key.compare(SETTINGS_DISABLEBACKGROUND) == 0) {
        ss >> settings._disableBackground;
      }
      if (key.compare(SETTINGS_HUDCORRECTION) == 0) {
        ss >> settings._hudCorrection;
      }
      if (key.compare(SETTINGS_WINDOWED) == 0) {
        ss >> settings._windowed;
      }
      if (key.compare(SETTINGS_VOLUME) == 0) {
        float t;
        ss >> t;
        settings._volume = int(t);
      }
    }
  }
  if (settings._hudCorrection < 0) {
    settings._hudCorrection = 0;
  }
  if (settings._hudCorrection > 8) {
    settings._hudCorrection = 8;
  }
  return settings;
}

void Lib::SaveSaveSettings(const Settings& settings)
{
  std::ofstream out;
  out.open(SettingsPath().c_str());
  out <<
      SETTINGS_DISABLEBACKGROUND << " " <<
      (settings._disableBackground ? "1" : "0") << "\n" <<
      SETTINGS_HUDCORRECTION << " " << settings._hudCorrection << "\n" <<
      SETTINGS_WINDOWED << " " << (settings._windowed ? "1" : "0") << "\n" <<
      SETTINGS_VOLUME << " " << settings._volume.to_int();
  out.close();
}

void Lib::StartRecording(int32_t players, bool canFaceSecretBoss, bool isBossMode,
                         bool isHardMode, bool isFastMode, bool isWhatMode)
{
  NewGame();
  _record.str(std::string());
  _record.clear();
  _recordSeed = (unsigned int) time(0);
  z_srand(_recordSeed);

  _record << "WiiSPACE v1.3 replay\n" << players << " " <<
      canFaceSecretBoss << " " << isBossMode << " " <<
      isHardMode << " " << isFastMode << " " << isWhatMode << " " <<
      _recordSeed << "\n";
  _recording = true;
}

void Lib::EndRecording(
    const std::string& name, uint64_t score, int32_t players,
    bool bossMode, bool hardMode, bool fastMode, bool whatMode)
{
  if (!_recording) {
    return;
  }
  _recording = false;
  std::stringstream ss;
  ss << "replays/" << _recordSeed << "_" <<
      players << "p_" <<
      (bossMode ? "bossmode_" :
       hardMode ? "hardmode_" :
       fastMode ? "fastmode_" :
       whatMode ? "w-hatmode_" : "") <<
      name << "_" << score << ".wrp";

  std::ofstream file;
  file.open(ss.str().c_str(), std::ios::binary);
  file << Crypt(compress_string(_record.str()), SUPER_ENCRYPTION_KEY);
  file.close();
}

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

void Lib::Record(Vec2 velocity, Vec2 target, int keys)
{
  char k = keys;
  std::stringstream binary(std::ios::in | std::ios::out | std::ios::binary);
  fixed_write(binary, velocity._x);
  fixed_write(binary, velocity._y);
  fixed_write(binary, target._x);
  fixed_write(binary, target._y);
  binary.write(&k, sizeof(char));
  _record << binary.str();
}

const Lib::Recording& Lib::PlayRecording(const std::string& path)
{
  SetWorkingDirectory(true);
  std::ifstream file;
  file.open(path.c_str(), std::ios::binary);
  std::string line;

  if (!file) {
    _replay._okay = false;
    _replay._error = "Cannot open:\n" + path;
    SetWorkingDirectory(false);
    return _replay;
  }
  std::stringstream b;
  b << file.rdbuf();
  std::string temp;
  try {
    temp = decompress_string(Crypt(b.str(), SUPER_ENCRYPTION_KEY));
  }
  catch (std::exception& e) {
    _replay._okay = false;
    _replay._error = e.what();
    SetWorkingDirectory(false);
    return _replay;
  }
  std::stringstream decrypted(
      temp, std::ios::in | std::ios::out | std::ios::binary);

  getline(decrypted, line);
  std::cout << line << std::endl;
  if (line.compare("WiiSPACE v1.3 replay") != 0) {
    file.close();
    _replay._okay = false;
    _replay._error = "Cannot play:\n" + line + "\nusing WiiSPACE v1.3";
    SetWorkingDirectory(false);
    return _replay;
  }

  getline(decrypted, line);
  std::stringstream ss(line);
  ss >> _replay._players;
  ss >> _replay._canFaceSecretBoss;
  ss >> _replay._isBossMode;
  ss >> _replay._isHardMode;
  ss >> _replay._isFastMode;
  ss >> _replay._isWhatMode;
  ss >> _replay._seed;
  z_srand(_replay._seed);
  _replay._players = std::max(std::min(4, _replay._players), 1);

  _replay._playerFrames.clear();
  while (!decrypted.eof()){
    PlayerFrame pf;
    char k;
    fixed_read(pf._velocity._x, decrypted);
    fixed_read(pf._velocity._y, decrypted);
    fixed_read(pf._target._x, decrypted);
    fixed_read(pf._target._y, decrypted);
    decrypted.read(&k, sizeof(char));
    pf._keys = k;
    _replay._playerFrames.push_back(pf);
  }

  file.close();
  _replay._okay = true;
  SetWorkingDirectory(false);
  return _replay;
}

void Lib::SetColourCycle(int cycle)
{
  _cycle = cycle % 256;
}

int Lib::GetColourCycle() const
{
  return _cycle;
}

Colour Lib::RgbToHsl(Colour rgb) const
{
  double r = ((rgb >> 24) & 0xff) / 255.0;
  double g = ((rgb >> 16) & 0xff) / 255.0;
  double b = ((rgb >> 8) & 0xff) / 255.0;

  double vm, r2, g2, b2;
  double h = 0, s = 0, l = 0;

  double v = std::max(r, std::max(b, g));
  double m = std::min(r, std::min(b, g));
  l = (m + v) / 2.0;
  if (l <= 0.0) {
    return 0;
  }
  s = vm = v - m;
  if (s > 0.0) {
    s /= (l <= 0.5) ? (v + m) : (2.0 - v - m);
  }
  else
    return ((int(0.5 + l * 255) & 0xff) << 8);
  r2 = (v - r) / vm;
  g2 = (v - g) / vm;
  b2 = (v - b) / vm;
  if (r == v) {
    h = (g == m ? 5.0 + b2 : 1.0 - g2);
  }
  else if (g == v) {
    h = (b == m ? 1.0 + r2 : 3.0 - b2);
  }
  else {
    h = (r == m ? 3.0 + g2 : 5.0 - r2);
  }
  h /= 6.0;
  return
      ((int(0.5 + h * 255) & 0xff) << 24) |
      ((int(0.5 + s * 255) & 0xff) << 16) |
      ((int(0.5 + l * 255) & 0xff) << 8);
}

Colour Lib::HslToRgb(Colour hsl) const
{
  double h = ((hsl >> 24) & 0xff) / 255.0;
  double s = ((hsl >> 16) & 0xff) / 255.0;
  double l = ((hsl >> 8) & 0xff) / 255.0;

  double r = l, g = l, b = l;
  double v = (l <= 0.5) ? (l * (1.0 + s)) : (l + s - l * s);
  if (v > 0) {
    h *= 6.0;
    double m = l + l - v;
    double sv = (v - m) / v;;
    int sextant = int(h);
    double vsf = v * sv * (h - sextant);
    double mid1 = m + vsf, mid2 = v - vsf;
    sextant %= 6;

    switch (sextant) {
    case 0:
      r = v;
      g = mid1;
      b = m;
      break;
    case 1:
      r = mid2;
      g = v;
      b = m;
      break;
    case 2:
      r = m;
      g = v;
      b = mid1;
      break;
    case 3:
      r = m;
      g = mid2;
      b = v;
      break;
    case 4:
      r = mid1;
      g = m;
      b = v;
      break;
    case 5:
      r = v;
      g = m;
      b = mid2;
      break;
    }
  }
  return
      ((int(0.5 + r * 255) & 0xff) << 24) |
      ((int(0.5 + g * 255) & 0xff) << 16) |
      ((int(0.5 + b * 255) & 0xff) << 8);
}

Colour Lib::Cycle(Colour rgb, int cycle) const
{
  if (cycle == 0)
    return rgb;
  int a = rgb & 0x000000ff;
  std::pair<Colour, int> key = std::make_pair(rgb & 0xffffff00, cycle);
  if (_cycleMap.find(key) != _cycleMap.end()) {
    return _cycleMap[key] | a;
  }

  Colour hsl = RgbToHsl(rgb & 0xffffff00);
  char c = ((hsl >> 24) & 0xff) + cycle;
  Colour result = HslToRgb((hsl & 0x00ffffff) | (c << 24));
  _cycleMap[key] = result;
  return result | a;
}

Colour Lib::Cycle(Colour rgb) const
{
  return Cycle(rgb, _cycle);
}
