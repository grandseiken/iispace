#include "lib.h"

#include <algorithm>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

const std::string Lib::SUPER_ENCRYPTION_KEY = "<>";
#define SETTINGS_WINDOWED "Windowed"
#define SETTINGS_VOLUME "Volume"
#define SETTINGS_PATH "wiispace.txt"
#define SAVE_PATH "wiispace.sav"
#define SOUND_MAX 16

#ifdef PLATFORM_LINUX
#include <sys/stat.h>
#endif

#ifndef PLATFORM_SCORE
#include "util/gamepad.h"
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <OISJoyStick.h>
#include <OISInputManager.h>
#include <OISForceFeedback.h>

#define RgbaToColor(colour)\
    (sf::Color((colour) >> 24, (colour) >> 16, (colour) >> 8, (colour)))

bool SfkToKey(sf::Key::Code code, Lib::Key key)
{
  if ((code == sf::Key::W || code == sf::Key::Up) && key == Lib::KEY_UP) {
    return true;
  }
  if ((code == sf::Key::A || code == sf::Key::Left) && key == Lib::KEY_LEFT) {
    return true;
  }
  if ((code == sf::Key::S || code == sf::Key::Down) && key == Lib::KEY_DOWN) {
    return true;
  }
  if ((code == sf::Key::D || code == sf::Key::Right) && key == Lib::KEY_RIGHT) {
    return true;
  }
  if ((code == sf::Key::Escape || code == sf::Key::Return) &&
      key == Lib::KEY_MENU) {
    return true;
  }
  if ((code == sf::Key::Z || code == sf::Key::LControl ||
       code == sf::Key::RControl) &&
      key == Lib::KEY_FIRE) {
    return true;
  }
  if ((code == sf::Key::X || code == sf::Key::Space) && key == Lib::KEY_BOMB) {
    return true;
  }
  if ((code == sf::Key::Z || code == sf::Key::Space ||
       code == sf::Key::LControl || code == sf::Key::RControl) &&
      key == Lib::KEY_ACCEPT) {
    return true;
  }
  if ((code == sf::Key::X || code == sf::Key::Escape) &&
      key == Lib::KEY_CANCEL) {
    return true;
  }
  return false;
}

bool SfmToKey(sf::Mouse::Button code, Lib::Key key)
{
  if (code == sf::Mouse::Left &&
      (key == Lib::KEY_FIRE || key == Lib::KEY_ACCEPT)) {
    return true;
  }
  if (code == sf::Mouse::Right &&
      (key == Lib::KEY_BOMB || key == Lib::KEY_CANCEL)) {
    return true;
  }
  return false;
}

bool PadToKey(PadConfig config, int button, Lib::Key key)
{
  PadConfig::Buttons none;
  const PadConfig::Buttons& buttons =
      key == Lib::KEY_MENU ? config._startButtons :
      key == Lib::KEY_FIRE || key == Lib::KEY_ACCEPT ? config._fireButtons :
      key == Lib::KEY_BOMB || key == Lib::KEY_CANCEL ? config._bombButtons :
      key == Lib::KEY_UP ? config._moveUpButtons :
      key == Lib::KEY_DOWN ? config._moveDownButtons :
      key == Lib::KEY_LEFT ? config._moveLeftButtons :
      key == Lib::KEY_RIGHT ? config._moveRightButtons : none;

  for (std::size_t i = 0; i < buttons.size(); ++i) {
    if (buttons[i] == button) {
      return true;
    }
  }
  return false;
}

class Handler : public OIS::JoyStickListener {
public:

  void SetLib(Lib* lib)
  {
    _lib = lib;
  }

  bool buttonPressed(const OIS::JoyStickEvent& arg, int button);
  bool buttonReleased(const OIS::JoyStickEvent& arg, int button);
  bool axisMoved(const OIS::JoyStickEvent& arg, int axis);
  bool povMoved(const OIS::JoyStickEvent& arg, int pov);

private:

  Lib* _lib;

};

struct Internals {
  mutable sf::RenderWindow _window;
  sf::Image _image;
  mutable sf::Sprite _font;

  OIS::InputManager*  _manager;
  Handler _padHandler;
  int32_t _padCount;
  PadConfig _padConfig[Lib::PLAYERS];
  OIS::JoyStick* _pads[Lib::PLAYERS];
  OIS::ForceFeedback* _ff[Lib::PLAYERS];
  fixed _padMoveVAxes[Lib::PLAYERS];
  fixed _padMoveHAxes[Lib::PLAYERS];
  fixed _padAimVAxes[Lib::PLAYERS];
  fixed _padAimHAxes[Lib::PLAYERS];
  int _padMoveDpads[Lib::PLAYERS];
  int _padAimDpads[Lib::PLAYERS];
  mutable vec2 _padLastAim[Lib::PLAYERS];

  typedef std::pair<int, sf::SoundBuffer*> NamedSound;
  typedef std::pair<int, NamedSound> SoundResource;
  typedef std::vector<SoundResource> SoundList;
  SoundList _sounds;
  std::vector<sf::Sound*> _voices;
};

bool Handler::buttonPressed(const OIS::JoyStickEvent& arg, int button)
{
  int p = -1;
  for (int32_t i = 0; i < Lib::PLAYERS &&
                      i < _lib->_internals->_padCount; ++i) {
    if (_lib->_internals->_pads[i] == arg.device) {
      p = i;
    }
  }
  if (p < 0) {
    return true;
  }

  for (int i = 0; i < Lib::KEY_MAX; ++i) {
    if (PadToKey(_lib->_internals->_padConfig[p], button, Lib::Key(i))) {
      _lib->_keysPressed[i][p] = true;
      _lib->_keysHeld[i][p] = true;
    }
  }
  return true;
}

bool Handler::buttonReleased(const OIS::JoyStickEvent& arg, int button)
{
  int p = -1;
  for (int32_t i = 0; i < Lib::PLAYERS &&
                      i < _lib->_internals->_padCount; ++i) {
    if (_lib->_internals->_pads[i] == arg.device) {
      p = i;
    }
  }
  if (p < 0) {
    return true;
  }

  for (int i = 0; i < Lib::KEY_MAX; ++i) {
    if (PadToKey(_lib->_internals->_padConfig[p], button, Lib::Key(i))) {
      _lib->_keysReleased[i][p] = true;
      _lib->_keysHeld[i][p] = false;
    }
  }
  return true;
}

bool Handler::axisMoved(const OIS::JoyStickEvent& arg, int axis)
{
  int p = -1;
  for (int32_t i = 0; i < Lib::PLAYERS &&
                      i < _lib->_internals->_padCount; ++i) {
    if (_lib->_internals->_pads[i] == arg.device) {
      p = i;
    }
  }
  if (p < 0) {
    return true;
  }

  fixed v = std::max(-fixed(1), std::min(
      fixed(1), fixed(arg.state.mAxes[axis].abs) / OIS::JoyStick::MAX_AXIS));
  PadConfig& config = _lib->_internals->_padConfig[p];

  for (std::size_t i = 0; i < config._moveSticks.size(); ++i) {
    if (config._moveSticks[i]._axis1 == axis) {
      _lib->_internals->_padMoveVAxes[p] =
          config._moveSticks[i]._axis1r ? -v : v;
    }
    if (config._moveSticks[i]._axis2 == axis) {
      _lib->_internals->_padMoveHAxes[p] =
          config._moveSticks[i]._axis2r ? -v : v;
    }
  }
  for (std::size_t i = 0; i < config._aimSticks.size(); ++i) {
    if (config._aimSticks[i]._axis1 == axis) {
      _lib->_internals->_padAimVAxes[p] = config._aimSticks[i]._axis1r ? -v : v;
    }
    if (config._aimSticks[i]._axis2 == axis) {
      _lib->_internals->_padAimHAxes[p] = config._aimSticks[i]._axis2r ? -v : v;
    }
  }
  return true;
}

bool Handler::povMoved(const OIS::JoyStickEvent& arg, int pov)
{
  int p = -1;
  for (int32_t i = 0; i < Lib::PLAYERS &&
                      i < _lib->_internals->_padCount; ++i) {
    if (_lib->_internals->_pads[i] == arg.device) {
      p = i;
    }
  }
  if (p < 0) {
    return true;
  }

  PadConfig& config = _lib->_internals->_padConfig[p];
  int d = arg.state.mPOV[pov].direction;
  for (std::size_t i = 0; i < config._moveDpads.size(); ++i) {
    if (config._moveDpads[i] == pov) {
      _lib->_internals->_padMoveDpads[p] = d;
      if (d & OIS::Pov::North) {
        _lib->_keysPressed[Lib::KEY_UP][p] = true;
        _lib->_keysHeld[Lib::KEY_UP][p] = true;
      }
      else {
        if (_lib->_keysHeld[Lib::KEY_UP][p]) {
          _lib->_keysReleased[Lib::KEY_UP][p] = true;
        }
        _lib->_keysHeld[Lib::KEY_UP][p] = false;
      }
      if (d & OIS::Pov::South) {
        _lib->_keysPressed[Lib::KEY_DOWN][p] = true;
        _lib->_keysHeld[Lib::KEY_DOWN][p] = true;
      }
      else {
        if (_lib->_keysHeld[Lib::KEY_DOWN][p]) {
          _lib->_keysReleased[Lib::KEY_DOWN][p] = true;
        }
        _lib->_keysHeld[Lib::KEY_DOWN][p] = false;
      }
      if (d & OIS::Pov::West) {
        _lib->_keysPressed[Lib::KEY_LEFT][p] = true;
        _lib->_keysHeld[Lib::KEY_LEFT][p] = true;
      }
      else {
        if (_lib->_keysHeld[Lib::KEY_LEFT][p]) {
          _lib->_keysReleased[Lib::KEY_LEFT][p] = true;
        }
        _lib->_keysHeld[Lib::KEY_LEFT][p] = false;
      }
      if (d & OIS::Pov::East) {
        _lib->_keysPressed[Lib::KEY_RIGHT][p] = true;
        _lib->_keysHeld[Lib::KEY_RIGHT][p] = true;
      }
      else {
        if (_lib->_keysHeld[Lib::KEY_RIGHT][p]) {
          _lib->_keysReleased[Lib::KEY_RIGHT][p] = true;
        }
        _lib->_keysHeld[Lib::KEY_RIGHT][p] = false;
      }
    }
  }
  for (std::size_t i = 0; i < config._aimDpads.size(); ++i) {
    if (config._aimDpads[i] == pov) {
      _lib->_internals->_padAimDpads[p] = d;
    }
  }
  return true;
}
#else
struct Internals {};
#endif

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

Lib::Lib()
  : _recording(false)
  , _frameCount(1)
  , _scoreFrame(0)
  , _cycle(0)
  , _replay()
  , _recordSeed(0)
  , _players(1)
  , _record(std::ios::in | std::ios::out | std::ios::binary)
  , _exit(false)
  , _cwd(0)
  , _captureMouse(false)
  , _mousePosX(0)
  , _mousePosY(0)
  , _mouseMoving(true)
{
#ifndef PLATFORM_SCORE
  _internals.reset(new Internals());
#ifdef PLATFORM_LINUX
  _cwd = get_current_dir_name();
  char temp[256];
  sprintf(temp, "/proc/%d/exe", getpid());
  _exe.resize(256);
  std::size_t bytes = 0;
  while ((bytes = readlink(temp, &_exe[0], _exe.size())) == _exe.size()) {
    _exe.resize(_exe.size() * 2);
  }
  if (bytes >= 0) {
    while (_exe[bytes] != '/') {
      --bytes;
    }
    _exe[bytes] = '\0';
    if (chdir(&_exe[0]) == 0) {
      mkdir("replays", 0777);
    }
  }
#else
  DWORD length = MAX_PATH;
  _cwd = new char[MAX_PATH + 1];
  GetCurrentDirectory(length, _cwd);
  _cwd[MAX_PATH + 1] = '\0';

  _exe.resize(MAX_PATH);
  DWORD result =
      GetModuleFileName(0, &_exe[0], static_cast< DWORD >(_exe.size()));
  while (result == _exe.size()) {
    _exe.resize(_exe.size() * 2);
    result = GetModuleFileName(0, &_exe[0], static_cast< DWORD >(_exe.size()));
  }

  if (result != 0) {
    --result;
    while (_exe[result] != '\\') {
      --result;
    }
    _exe[result] = '\0';
    SetCurrentDirectory(&_exe[0]);
  }
  CreateDirectory("replays", 0);
#endif

  _internals->_manager = OIS::InputManager::createInputSystem(0);
  _internals->_padCount =
      std::min(4, _internals->_manager->getNumberOfDevices(OIS::OISJoyStick));
  OIS::JoyStick* tpads[4];
  tpads[0] = tpads[1] = tpads[2] = tpads[3] = 0;
  _internals->_pads[0] = _internals->_pads[1] =
      _internals->_pads[2] = _internals->_pads[3] = 0;
  for (int32_t i = 0; i < _internals->_padCount; ++i) {
    try {
      tpads[i] = (OIS::JoyStick*)
          _internals->_manager->createInputObject(OIS::OISJoyStick, true);
      tpads[i]->setEventCallback(&_internals->_padHandler);
    }
    catch (const std::exception& e) {
      tpads[i] = 0;
      (void) e;
    }
  }

  _internals->_padHandler.SetLib(this);
  int configs = 0;
  std::ifstream f("gamepads.txt");
  std::string line;
  getline(f, line);
  std::stringstream ss(line);
  ss >> configs;
  for (int i = 0; i < PLAYERS; ++i) {
    if (i >= configs) {
      _internals->_padConfig[i].SetDefault();
      for (int p = 0; p < PLAYERS; ++p) {
        if (!tpads[p]) {
          continue;
        }
        _internals->_pads[i] = tpads[p];
        tpads[p] = 0;
        break;
      }
      continue;
    }
    getline(f, line);
    bool assigned = false;
    for (int p = 0; p < PLAYERS; ++p) {
      if (!tpads[p]) {
        continue;
      }

      std::string name = tpads[p]->vendor();
      for (std::size_t j = 0; j < name.length(); ++j) {
        if (name[j] == '\n' || name[j] == '\r') {
          name[j] = ' ';
        }
      }

      if (name.compare(line) == 0) {
        _internals->_pads[i] = tpads[p];
        tpads[p] = 0;
        _internals->_padConfig[i].Read(f);
        assigned = true;
        break;
      }
    }

    if (!assigned) {
      PadConfig t;
      t.Read(f);
      --i;
      --configs;
    }
  }
  f.close();

  for (int i = 0; i < PLAYERS; ++i) {
    if (!_internals->_pads[i]) {
      continue;
    }
    else {
      _internals->_ff[i] = 0;
    }
    _internals->_ff[i] = (OIS::ForceFeedback*)
        _internals->_pads[i]->queryInterface(OIS::Interface::ForceFeedback);

    /*if (_ff[i]) {
      OIS::Effect* e =
          new OIS::Effect(OIS::Effect::ConstantForce, OIS::Effect::Constant);
      e->setNumAxes(1);
      ((OIS::ConstantEffect*) e->getForceEffect())->level = 5000;
      e->direction = OIS::Effect::North;
      e->trigger_button = 1;
      e->trigger_interval = 1000;
      e->replay_length = 5000;
      e->replay_delay = 0;
      _ff[i]->upload(e);
    }*/
  }

  for (int i = int(sf::VideoMode::GetModesCount()) - 1;
       i >= 0 && !LoadSaveSettings()._windowed; --i) {
    sf::VideoMode m = sf::VideoMode::GetMode(i);
    if (m.Width >= unsigned(Lib::WIDTH) &&
        m.Height >= unsigned(Lib::HEIGHT) && m.BitsPerPixel == 32) {
      _internals->_window.Create(
          m, "WiiSPACE", sf::Style::Fullscreen, sf::WindowSettings(0, 0, 0));
      _extraX = (m.Width - Lib::WIDTH) / 2;
      _extraY = (m.Height - Lib::HEIGHT) / 2;
      return;
    }
  }
  _internals->_window.Create(sf::VideoMode(640, 480, 32), "WiiSPACE",
                             sf::Style::Close | sf::Style::Titlebar);
  _extraX = 0;
  _extraY = 0;
#endif
}

Lib::~Lib()
{
#ifndef PLATFORM_SCORE
#ifdef PLATFORM_LINUX
  free(_cwd);
#else
  delete _cwd;
#endif
  OIS::InputManager::destroyInputSystem(_internals->_manager);
  for (int i = 0; i < SOUND_MAX; ++i) {
    delete _internals->_sounds[i].second.second;
  }
#endif
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
  file.open(SAVE_PATH, std::ios::binary);

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
  file.open(SAVE_PATH, std::ios::binary);
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
  settings._windowed = 0;
  settings._volume = 100;
  std::ifstream file;
  file.open(SETTINGS_PATH);

  if (!file) {
    std::ofstream out;
    out.open(SETTINGS_PATH);
    out <<
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
  return settings;
}

void Lib::SaveSaveSettings(const Settings& settings)
{
  std::ofstream out;
  out.open(SETTINGS_PATH);
  out <<
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
  z::seed(_recordSeed);

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
  file << Crypt(z::compress_string(_record.str()), SUPER_ENCRYPTION_KEY);
  file.close();
}

void Lib::OnScore(
    long seed, int32_t players, bool bossMode, uint64_t score,
    bool hardMode, bool fastMode, bool whatMode)
{
#ifdef PLATFORM_SCORE
  std::cout << seed << "\n" << players << "\n" << bossMode << "\n" <<
      hardMode << "\n" << fastMode << "\n" << whatMode << "\n" <<
      score << "\n" << std::flush;
  throw score_finished{};
#endif
}

void Lib::Record(vec2 velocity, const vec2& target, int keys)
{
  char k = keys;
  std::stringstream binary(std::ios::in | std::ios::out | std::ios::binary);
  fixed_write(binary, velocity.x);
  fixed_write(binary, velocity.y);
  fixed_write(binary, target.x);
  fixed_write(binary, target.y);
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
    temp = z::decompress_string(Crypt(b.str(), SUPER_ENCRYPTION_KEY));
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
  z::seed(_replay._seed);
  _replay._players = std::max(std::min(4, _replay._players), 1);

  _replay._playerFrames.clear();
  while (!decrypted.eof()){
    PlayerFrame pf;
    char k;
    fixed_read(pf._velocity.x, decrypted);
    fixed_read(pf._velocity.y, decrypted);
    fixed_read(pf._target.x, decrypted);
    fixed_read(pf._target.y, decrypted);
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

colour Lib::Cycle(colour rgb) const
{
  return z::colour_cycle(rgb, _cycle);
}

void Lib::SetWorkingDirectory(bool original)
{
#ifndef PLATFORM_SCORE
  if (original) {
#ifdef PLATFORM_LINUX
    if (chdir(_cwd) == 0) {
      (void) _cwd;
    }
#else
    SetCurrentDirectory(_cwd);
#endif
  }
  else {
#ifdef PLATFORM_LINUX
    if (chdir(&_exe[0]) == 0) {
      (void) 0;
    }
#else
    SetCurrentDirectory(&_exe[0]);
#endif
  }
#endif
}

// General
//------------------------------
void Lib::Init()
{
#ifndef PLATFORM_SCORE
  _settings = LoadSaveSettings();
  _internals->_image.LoadFromFile("console.png");
  _internals->_image.CreateMaskFromColor(RgbaToColor(0x000000ff));
  _internals->_image.SetSmooth(false);
  _internals->_font = sf::Sprite(_internals->_image);
  LoadSounds();

  ClearScreen();
  _internals->_window.Show(true);
  _internals->_window.UseVerticalSync(true);
  _internals->_window.SetFramerateLimit(50);
  _internals->_window.ShowMouseCursor(false);

  glClearColor(0.f, 0.f, 0.f, 0.f);
  glDisable(GL_DEPTH_TEST);
  glLineWidth(1);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  for (int i = 0; i < KEY_MAX; ++i) {
    std::vector< bool > f;
    for (int j = 0; j < PLAYERS; ++j) {
      f.push_back(false);
    }
    _keysPressed.push_back(f);
    _keysHeld.push_back(f);
    _keysReleased.push_back(f);
  }
  for (int j = 0; j < PLAYERS; ++j) {
    _internals->_padMoveVAxes[j] = 0;
    _internals->_padMoveHAxes[j] = 0;
    _internals->_padAimVAxes[j] = 0;
    _internals->_padAimHAxes[j] = 0;
    _internals->_padMoveDpads[j] = OIS::Pov::Centered;
    _internals->_padAimDpads[j] = OIS::Pov::Centered;
  }
  sf::Listener::SetGlobalVolume(
      std::max(0.f, std::min(100.f, _settings._volume.to_float())));
  for (int i = 0; i < SOUND_MAX; ++i) {
    _internals->_voices.push_back(new sf::Sound());
  }
#endif
}

void Lib::BeginFrame()
{
#ifndef PLATFORM_SCORE
  int t = _mousePosX;
  int u = _mousePosY;
  sf::Event e;
  int kp = GetPlayerCount() <= _internals->_padCount ?
      GetPlayerCount() - 1 : _internals->_padCount;
  while (_internals->_window.GetEvent(e))
  {
    if (e.Type == sf::Event::Closed) {
      _exit = true;
      return;
    }

    for (int i = 0; i < KEY_MAX; ++i) {
      bool kd = e.Type == sf::Event::KeyPressed;
      bool ku = e.Type == sf::Event::KeyReleased;
      bool md = e.Type == sf::Event::MouseButtonPressed;
      bool mu = e.Type == sf::Event::MouseButtonReleased;

      bool b = false;
      b |= (kd || ku) && SfkToKey(e.Key.Code, Lib::Key(i));
      b |= (md || mu) && SfmToKey(e.MouseButton.Button, Lib::Key(i));
      if (!b) {
        continue;
      }

      if (kd || md) {
        _keysPressed[i][kp] = true;
        _keysHeld[i][kp] = true;
      }
      if (ku || mu) {
        _keysReleased[i][kp] = true;
        _keysHeld[i][kp] = false;
      }
    }

    if (e.Type == sf::Event::MouseMoved) {
      _mousePosX = e.MouseMove.X;
      _mousePosY = e.MouseMove.Y;
    }
  }

  for (std::size_t i = 0; i < _internals->_sounds.size(); ++i) {
    if (_internals->_sounds[i].first > 0) {
      _internals->_sounds[i].first--;
    }
  }

  _mousePosX = std::max(0, std::min(Lib::WIDTH - 1, _mousePosX));
  _mousePosY = std::max(0, std::min(Lib::HEIGHT - 1, _mousePosY));
  if (t != _mousePosX || u != _mousePosY) {
    _mouseMoving = true;
  }
  if (_captureMouse) {
    _internals->_window.SetCursorPosition(_mousePosX, _mousePosY);
  }

  for (int32_t i = 0; i < _internals->_padCount; ++i) {
    _internals->_pads[i]->capture();
    const OIS::JoyStickState& s = _internals->_pads[i]->getJoyStickState();
    for (std::size_t j = 0; j < s.mAxes.size(); ++j) {
      OIS::JoyStickEvent arg(_internals->_pads[i], s);
      _internals->_padHandler.axisMoved(arg, int(j));
    }
  }
#else
  if (_scoreFrame < 5) {
    ++_scoreFrame;
  }
  SetFrameCount(16384);
#endif
}

void Lib::EndFrame()
{
#ifndef PLATFORM_SCORE
  if (!_internals->_window.IsOpened()) {
    _exit = true;
  }

  for (int i = 0; i < KEY_MAX; ++i) {
    for (int j = 0; j < PLAYERS; ++j) {
      _keysPressed[i][j] = false;
      _keysReleased[i][j] = false;
    }
  }
#else
  SetFrameCount(16384);
#endif
  /*for (int i = 0; i < PLAYERS; i++) {
    if (_rumble[i]) {
      _rumble[i]--;
      if (!_rumble[i]) {
      WPAD_Rumble(i, false);
      PAD_ControlMotor(i, PAD_MOTOR_STOP);
    }
  }*/
}

void Lib::CaptureMouse(bool enabled)
{
  _captureMouse = enabled;
}

void Lib::NewGame()
{
  for (int i = 0; i < KEY_MAX; ++i) {
    for (int j = 0; j < PLAYERS; ++j) {
      _keysPressed[i][j] = false;
      _keysReleased[i][j] = false;
      _keysHeld[i][j] = false;
    }
  }
}

void Lib::Exit(bool exit)
{
#ifndef PLATFORM_SCORE
  if (!LoadSaveSettings()._windowed) {
    _internals->_window.Create(
        sf::VideoMode(640, 480, 32), "WiiSPACE",
        sf::Style::Close | sf::Style::Titlebar);
  }
#endif
  _exit = exit;
}

bool Lib::Exit() const
{
  return _exit;
}

void Lib::TakeScreenShot()
{
#ifndef PLATFORM_SCORE
  sf::Image image = _internals->_window.Capture();
  for (unsigned int x = 0; x < image.GetWidth(); ++x) {
    for (unsigned int y = 0; y < image.GetHeight(); ++y) {
      sf::Color c = image.GetPixel(x, y);
      c.a = 0xff;
      image.SetPixel(x, y, c);
    }
  }

  std::stringstream ss;
  ss << "screenshot" << time(0) % 10000000 << ".png";
  image.SaveToFile(ss.str());
#endif
}

Lib::Settings Lib::LoadSettings() const
{
#ifndef PLATFORM_SCORE
  return _settings;
#else
  return Settings();
#endif
}

void Lib::SetVolume(int volume)
{
#ifndef PLATFORM_SCORE
  _settings._volume = fixed(std::max(0, std::min(100, volume)));
  sf::Listener::SetGlobalVolume(_settings._volume.to_float());
  SaveSaveSettings(_settings);
#endif
}

// Input
//------------------------------
Lib::PadType Lib::IsPadConnected(int32_t player) const
{
#ifndef PLATFORM_SCORE
  PadType r = PAD_NONE;
  if (player < _internals->_padCount) {
    r = PAD_GAMEPAD;
  }
  if ((GetPlayerCount() <= _internals->_padCount &&
       player == GetPlayerCount() - 1) ||
      (GetPlayerCount() > _internals->_padCount &&
       player == _internals->_padCount)) {
    r = PadType(r | PAD_KEYMOUSE);
  }
  return r;
#else
  return PAD_NONE;
#endif
}

bool Lib::IsKeyPressed(int32_t player, Key k) const
{
#ifndef PLATFORM_SCORE
  return _keysPressed[k][player];
#else
  if (player != 0) {
    return false;
  }
  if (k == KEY_DOWN && _scoreFrame < 4) {
    return true;
  }
  if (k == KEY_ACCEPT && _scoreFrame == 5) {
    return true;
  }
  return false;
#endif
}

bool Lib::IsKeyReleased(int32_t player, Key k) const
{
#ifndef PLATFORM_SCORE
  return _keysReleased[k][player];
#else
  return false;
#endif
}

bool Lib::IsKeyHeld(int32_t player, Key k) const
{
#ifndef PLATFORM_SCORE
  vec2 v(_internals->_padAimHAxes[player], _internals->_padAimVAxes[player]);
  if (k == KEY_FIRE &&
      (_internals->_padAimDpads[player] != OIS::Pov::Centered ||
       v.length() >= fixed::tenth * 2)) {
    return true;
  }
  return _keysHeld[k][player];
#else
  return false;
#endif
}

vec2 Lib::GetMoveVelocity(int32_t player) const
{
#ifndef PLATFORM_SCORE
  bool kU = IsKeyHeld(player, KEY_UP) ||
      _internals->_padMoveDpads[player] & OIS::Pov::North;
  bool kD = IsKeyHeld(player, KEY_DOWN) ||
      _internals->_padMoveDpads[player] & OIS::Pov::South;
  bool kL = IsKeyHeld(player, KEY_LEFT) ||
      _internals->_padMoveDpads[player] & OIS::Pov::West;
  bool kR = IsKeyHeld(player, KEY_RIGHT) ||
      _internals->_padMoveDpads[player] & OIS::Pov::East;

  vec2 v;
  if (kU) {
    v += vec2(0, -1);
  }
  if (kD) {
    v += vec2(0, 1);
  }
  if (kL) {
    v += vec2(-1, 0);
  }
  if (kR) {
    v += vec2(1, 0);
  }
  if (v != vec2()) {
    return v;
  }

  v = vec2(_internals->_padMoveHAxes[player],
           _internals->_padMoveVAxes[player]);
  if (v.length() < fixed::tenth * 2) {
    return vec2();
  }
  return v;
#else
  return vec2();
#endif
}

vec2 Lib::GetFireTarget(int32_t player, const vec2& position) const
{
#ifndef PLATFORM_SCORE
  bool kp = player ==
      (GetPlayerCount() <= _internals->_padCount ?
       GetPlayerCount() - 1 : _internals->_padCount);
  vec2 v(_internals->_padAimHAxes[player], _internals->_padAimVAxes[player]);

  if (v.length() >= fixed::tenth * 2) {
    v.normalise();
    v *= 48;
    if (kp) {
      _mouseMoving = false;
    }
    _internals->_padLastAim[player] = v;
    return v + position;
  }

  if (_internals->_padAimDpads[player] != OIS::Pov::Centered) {
    bool kU = (_internals->_padAimDpads[player] & OIS::Pov::North) != 0;
    bool kD = (_internals->_padAimDpads[player] & OIS::Pov::South) != 0;
    bool kL = (_internals->_padAimDpads[player] & OIS::Pov::West) != 0;
    bool kR = (_internals->_padAimDpads[player] & OIS::Pov::East) != 0;

    vec2 v;
    if (kU) {
      v += vec2(0, -1);
    }
    if (kD) {
      v += vec2(0, 1);
    }
    if (kL) {
      v += vec2(-1, 0);
    }
    if (kR) {
      v += vec2(1, 0);
    }

    if (v != vec2()) {
      v.normalise();
      v *= 48;
      if (kp) {
        _mouseMoving = false;
      }
      _internals->_padLastAim[player] = v;
      return v + position;
    }
  }

  if (_mouseMoving && kp) {
    return vec2(fixed(_mousePosX), fixed(_mousePosY));
  }
  if (_internals->_padLastAim[player] != vec2()) {
    return position + _internals->_padLastAim[player];
  }
  return position + vec2(48, 0);
#else
  return vec2();
#endif
}

// Output
//------------------------------
void Lib::ClearScreen() const
{
#ifndef PLATFORM_SCORE
  glClear(GL_COLOR_BUFFER_BIT);
  _internals->_window.Clear(RgbaToColor(0x000000ff));
  _internals->_window.Draw(sf::Shape::Rectangle(0, 0, 0, 0, sf::Color()));
#endif
}

void Lib::RenderLine(const flvec2& a, const flvec2& b, colour c) const
{
#ifndef PLATFORM_SCORE
  c = Cycle(c);
  glBegin(GL_LINES);
  glColor4ub(c >> 24, c >> 16, c >> 8, c);
  glVertex3f(a.x + _extraX, a.y + _extraY, 0);
  glVertex3f(b.x + _extraX, b.y + _extraY, 0);
  glColor4ub(c >> 24, c >> 16, c >> 8, c & 0xffffff33);
  glVertex3f(a.x + _extraX + 1, a.y + _extraY, 0);
  glVertex3f(b.x + _extraX + 1, b.y + _extraY, 0);
  glVertex3f(a.x + _extraX - 1, a.y + _extraY, 0);
  glVertex3f(b.x + _extraX - 1, b.y + _extraY, 0);
  glVertex3f(a.x + _extraX, a.y + _extraY + 1, 0);
  glVertex3f(b.x + _extraX, b.y + _extraY + 1, 0);
  glVertex3f(a.x + _extraX, a.y + _extraY - 1, 0);
  glVertex3f(b.x + _extraX, b.y + _extraY - 1, 0);
  glEnd();
#endif
}

void Lib::RenderText(const flvec2& v, const std::string& text, colour c) const
{
#ifndef PLATFORM_SCORE
  _internals->_font.SetColor(RgbaToColor(Cycle(c)));
  for (std::size_t i = 0; i < text.length(); ++i) {
    _internals->_font.SetPosition(
        (int(i) + v.x) * TEXT_WIDTH + _extraX, v.y * TEXT_HEIGHT + _extraY);
    _internals->_font.SetSubRect(sf::IntRect(TEXT_WIDTH * text[i], 0,
                                 TEXT_WIDTH * (1 + text[i]), TEXT_HEIGHT));
    _internals->_window.Draw(_internals->_font);
  }
  _internals->_window.Draw(sf::Shape::Rectangle(0, 0, 0, 0, sf::Color()));
#endif
}

void Lib::RenderRect(
    const flvec2& low, const flvec2& hi, colour c, int lineWidth) const
{
#ifndef PLATFORM_SCORE
  c = Cycle(c);
  flvec2 ab(low.x, hi.y);
  flvec2 ba(hi.x, low.y);
  const flvec2* list[4];
  flvec2 normals[4];
  list[0] = &low;
  list[1] = &ab;
  list[2] = &hi;
  list[3] = &ba;

  flvec2 centre = (low + hi) / 2;
  for (std::size_t i = 0; i < 4; ++i) {
    const flvec2& v0 = *list[(i + 3) % 4];
    const flvec2& v1 = *list[i];
    const flvec2& v2 = *list[(i + 1) % 4];

    flvec2 n1(v0.y - v1.y, v1.x - v0.x);
    flvec2 n2(v1.y - v2.y, v2.x - v1.x);

    n1.normalise();
    n2.normalise();

    float f = 1 + n1.x * n2.x + n1.y * n2.y;
    normals[i] = (n1 + n2) / f;
    float dot =
        (v1.x - centre.x) * normals[i].x +
        (v1.y - centre.y) * normals[i].y;

    if (dot < 0) {
      normals[i] = flvec2() - normals[i];
    }
  }

  glBegin(GL_TRIANGLE_STRIP);
  glColor4ub(c >> 24, c >> 16, c >> 8, c);
  for (std::size_t i = 0; i < 5; ++i)
  {
    glVertex3f(list[i % 4]->x, list[i % 4]->y, 0);
    glVertex3f(list[i % 4]->x + normals[i % 4].x,
               list[i % 4]->y + normals[i % 4].y, 0);
  }
  glEnd();

  if (lineWidth > 1) {
    RenderRect(low + flvec2(1.f, 1.f),
               hi - flvec2(1.f, 1.f), Cycle(c), lineWidth - 1);
  }
#endif
}

void Lib::Render() const
{
#ifndef PLATFORM_SCORE
  _internals->_window.Draw(sf::Shape::Rectangle(
      0.f, 0.f,
      float(_extraX), float(Lib::HEIGHT + _extraY * 2),
      sf::Color(0, 0, 0, 255)));
  _internals->_window.Draw(sf::Shape::Rectangle(
      0.f, 0.f,
      float(Lib::WIDTH + _extraX * 2), float(_extraY),
      sf::Color(0, 0, 0, 255)));
  _internals->_window.Draw(sf::Shape::Rectangle(
      float(Lib::WIDTH + _extraX), 0,
      float(Lib::WIDTH + _extraX * 2), float(Lib::HEIGHT + _extraY * 2),
      sf::Color(0, 0, 0, 255)));
  _internals->_window.Draw(sf::Shape::Rectangle(
      0.f, float(Lib::HEIGHT + _extraY),
      float(Lib::WIDTH + _extraX * 2), float(Lib::HEIGHT + _extraY * 2),
      sf::Color(0, 0, 0, 255)));
  _internals->_window.Draw(sf::Shape::Rectangle(
      0.f, 0.f,
      float(_extraX - 2), float(Lib::HEIGHT + _extraY * 2),
      sf::Color(32, 32, 32, 255)));
  _internals->_window.Draw(sf::Shape::Rectangle(
      0.f, 0.f,
      float(Lib::WIDTH + _extraX * 2), float(_extraY - 2),
      sf::Color(32, 32, 32, 255)));
  _internals->_window.Draw(sf::Shape::Rectangle(
      float(Lib::WIDTH + _extraX + 2), float(_extraY - 4),
      float(Lib::WIDTH + _extraX * 2), float(Lib::HEIGHT + _extraY * 2),
      sf::Color(32, 32, 32, 255)));
  _internals->_window.Draw(sf::Shape::Rectangle(
      float(_extraX - 2), float(Lib::HEIGHT + _extraY + 2),
      float(Lib::WIDTH + _extraX * 2), float(Lib::HEIGHT + _extraY * 2),
      sf::Color(32, 32, 32, 255)));
  _internals->_window.Draw(sf::Shape::Rectangle(
      float(_extraX - 4), float(_extraY - 4),
      float(_extraX - 2), float(Lib::HEIGHT + _extraY + 4),
      sf::Color(128, 128, 128, 255)));
  _internals->_window.Draw(sf::Shape::Rectangle(
      float(_extraX - 4), float(_extraY - 4),
      float(Lib::WIDTH + _extraX + 4), float(_extraY - 2),
      sf::Color(128, 128, 128, 255)));
  _internals->_window.Draw(sf::Shape::Rectangle(
      float(Lib::WIDTH + _extraX + 2), float(_extraY - 4),
      float(Lib::WIDTH + _extraX + 4), float(Lib::HEIGHT + _extraY + 4),
      sf::Color(128, 128, 128, 255)));
  _internals->_window.Draw(sf::Shape::Rectangle(
      float(_extraX - 2), float(Lib::HEIGHT + _extraY + 2),
      float(Lib::WIDTH + _extraX + 4), float(Lib::HEIGHT + _extraY + 4),
      sf::Color(128, 128, 128, 255)));
  _internals->_window.Display();
#endif
}

void Lib::Rumble(int player, int time)
{
  /*if (player < 0 || player >= PLAYERS) {
    return;
  }
  _rumble[player] = std::max(_rumble[player], time);
  if (_rumble[player]) {
    WPAD_Rumble(player, true);
    PAD_ControlMotor(player, PAD_MOTOR_RUMBLE);
  }*/
}

void Lib::StopRumble()
{
  /*for (int i = 0; i < PLAYERS; i++) {
    _rumble[i] = 0;
    WPAD_Rumble(i, false);
    PAD_ControlMotor(i, PAD_MOTOR_STOP);
  }*/
}

bool Lib::PlaySound(Sound sound, float volume, float pan, float repitch)
{
#ifndef PLATFORM_SCORE
  if (volume < 0) {
    volume = 0;
  }
  if (volume > 1) {
    volume = 1;
  }
  if (pan < -1) {
    pan = -1;
  }
  if (pan > 1) {
    pan = 1;
  }

  const sf::SoundBuffer* buffer = 0;
  for (std::size_t i = 0; i < _internals->_sounds.size(); i++) {
    if (sound == _internals->_sounds[i].second.first) {
      if (_internals->_sounds[i].first <= 0) {
        buffer = _internals->_sounds[i].second.second;
        _internals->_sounds[i].first = SOUND_TIMER;
      }
      break;
    }
  }

  if (!buffer) {
    return false;
  }
  for (int i = 0; i < SOUND_MAX; ++i) {
    if (_internals->_voices[i]->GetStatus() != sf::Sound::Playing) {
      _internals->_voices[i]->SetAttenuation(0.f);
      _internals->_voices[i]->SetLoop(false);
      _internals->_voices[i]->SetMinDistance(100.f);
      _internals->_voices[i]->SetBuffer(*buffer);
      _internals->_voices[i]->SetVolume(100.f * volume);
      _internals->_voices[i]->SetPitch(pow(2.f, repitch));
      _internals->_voices[i]->SetPosition(pan, 0, -1);
      _internals->_voices[i]->Play();
      return true;
    }
  }
#endif
  return false;
}

// Sounds
//------------------------------
#ifndef PLATFORM_SCORE
#define USE_SOUND(sound, data)\
    sf::SoundBuffer* t_##sound = new sf::SoundBuffer(); \
    t_##sound->LoadFromFile(data); \
    _internals->_sounds.push_back(\
        Internals::SoundResource(0, Internals::NamedSound(sound, t_##sound)));
#endif

void Lib::LoadSounds()
{
#ifndef PLATFORM_SCORE
  USE_SOUND(SOUND_PLAYER_FIRE, "PlayerFire.wav");
  USE_SOUND(SOUND_MENU_CLICK, "MenuClick.wav");
  USE_SOUND(SOUND_MENU_ACCEPT, "MenuAccept.wav");
  USE_SOUND(SOUND_POWERUP_LIFE, "PowerupLife.wav");
  USE_SOUND(SOUND_POWERUP_OTHER, "PowerupOther.wav");
  USE_SOUND(SOUND_ENEMY_HIT, "EnemyHit.wav");
  USE_SOUND(SOUND_ENEMY_DESTROY, "EnemyDestroy.wav");
  USE_SOUND(SOUND_ENEMY_SHATTER, "EnemyShatter.wav");
  USE_SOUND(SOUND_ENEMY_SPAWN, "EnemySpawn.wav");
  USE_SOUND(SOUND_BOSS_ATTACK, "BossAttack.wav");
  USE_SOUND(SOUND_BOSS_FIRE, "BossFire.wav");
  USE_SOUND(SOUND_PLAYER_RESPAWN, "PlayerRespawn.wav");
  USE_SOUND(SOUND_PLAYER_DESTROY, "PlayerDestroy.wav");
  USE_SOUND(SOUND_PLAYER_SHIELD, "PlayerShield.wav");
  USE_SOUND(SOUND_EXPLOSION, "Explosion.wav");
#endif
}