#include "z0.h"
#ifdef PLATFORM_IISPACE

#include <iostream>
#include <fstream>
#include "lib_ii.h"
#define SOUND_MAX 16

#ifdef PLATFORM_LINUX
#include <sys/stat.h>
#endif

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

bool LibWin::Handler::buttonPressed(const OIS::JoyStickEvent& arg, int button)
{
  int p = -1;
  for (int i = 0; i < PLAYERS && i < _lib->_padCount; ++i) {
    if (_lib->_pads[i] == arg.device) {
      p = i;
    }
  }
  if (p < 0) {
    return true;
  }

  for (int i = 0; i < KEY_MAX; ++i) {
    if (PadToKey(_lib->_padConfig[p], button, Key(i))) {
      _lib->_keysPressed[i][p] = true;
      _lib->_keysHeld[i][p] = true;
    }
  }
  return true;
}

bool LibWin::Handler::buttonReleased(const OIS::JoyStickEvent& arg, int button)
{
  int p = -1;
  for (int i = 0; i < PLAYERS && i < _lib->_padCount; ++i) {
    if (_lib->_pads[i] == arg.device) {
      p = i;
    }
  }
  if (p < 0) {
    return true;
  }

  for (int i = 0; i < KEY_MAX; ++i) {
    if (PadToKey(_lib->_padConfig[p], button, Key(i))) {
      _lib->_keysReleased[i][p] = true;
      _lib->_keysHeld[i][p] = false;
    }
  }
  return true;
}

bool LibWin::Handler::axisMoved(const OIS::JoyStickEvent& arg, int axis)
{
  int p = -1;
  for (int i = 0; i < PLAYERS && i < _lib->_padCount; ++i) {
    if (_lib->_pads[i] == arg.device) {
      p = i;
    }
  }
  if (p < 0) {
    return true;
  }

  fixed v = std::max(-M_ONE, std::min(
      M_ONE, fixed(arg.state.mAxes[axis].abs) / OIS::JoyStick::MAX_AXIS));
  PadConfig& config = _lib->_padConfig[p];

  for (std::size_t i = 0; i < config._moveSticks.size(); ++i) {
    if (config._moveSticks[i]._axis1 == axis) {
      _lib->_padMoveVAxes[p] = config._moveSticks[i]._axis1r ? -v : v;
    }
    if (config._moveSticks[i]._axis2 == axis) {
      _lib->_padMoveHAxes[p] = config._moveSticks[i]._axis2r ? -v : v;
    }
  }
  for (std::size_t i = 0; i < config._aimSticks.size(); ++i) {
    if (config._aimSticks[i]._axis1 == axis) {
      _lib->_padAimVAxes[p] = config._aimSticks[i]._axis1r ? -v : v;
    }
    if (config._aimSticks[i]._axis2 == axis) {
      _lib->_padAimHAxes[p] = config._aimSticks[i]._axis2r ? -v : v;
    }
  }
  return true;
}

bool LibWin::Handler::povMoved(const OIS::JoyStickEvent& arg, int pov)
{
  int p = -1;
  for (int i = 0; i < PLAYERS && i < _lib->_padCount; ++i) {
    if (_lib->_pads[i] == arg.device) {
      p = i;
    }
  }
  if (p < 0) {
    return true;
  }

  PadConfig& config = _lib->_padConfig[p];
  int d = arg.state.mPOV[pov].direction;
  for (std::size_t i = 0; i < config._moveDpads.size(); ++i) {
    if (config._moveDpads[i] == pov) {
      _lib->_padMoveDpads[p] = d;
      if (d & OIS::Pov::North) {
        _lib->_keysPressed[KEY_UP][p] = true;
        _lib->_keysHeld[KEY_UP][p] = true;
      }
      else {
        if (_lib->_keysHeld[KEY_UP][p]) {
          _lib->_keysReleased[KEY_UP][p] = true;
        }
        _lib->_keysHeld[KEY_UP][p] = false;
      }
      if (d & OIS::Pov::South) {
        _lib->_keysPressed[KEY_DOWN][p] = true;
        _lib->_keysHeld[KEY_DOWN][p] = true;
      }
      else {
        if (_lib->_keysHeld[KEY_DOWN][p]) {
          _lib->_keysReleased[KEY_DOWN][p] = true;
        }
        _lib->_keysHeld[KEY_DOWN][p] = false;
      }
      if (d & OIS::Pov::West) {
        _lib->_keysPressed[KEY_LEFT][p] = true;
        _lib->_keysHeld[KEY_LEFT][p] = true;
      }
      else {
        if (_lib->_keysHeld[KEY_LEFT][p]) {
          _lib->_keysReleased[KEY_LEFT][p] = true;
        }
        _lib->_keysHeld[KEY_LEFT][p] = false;
      }
      if (d & OIS::Pov::East) {
        _lib->_keysPressed[KEY_RIGHT][p] = true;
        _lib->_keysHeld[KEY_RIGHT][p] = true;
      }
      else {
        if (_lib->_keysHeld[KEY_RIGHT][p]) {
          _lib->_keysReleased[KEY_RIGHT][p] = true;
        }
        _lib->_keysHeld[KEY_RIGHT][p] = false;
      }
    }
  }
  for (std::size_t i = 0; i < config._aimDpads.size(); ++i) {
    if (config._aimDpads[i] == pov) {
      _lib->_padAimDpads[p] = d;
    }
  }
  return true;
}

LibWin::LibWin()
  : _exitType(NO_EXIT)
  , _cwd(0)
  , _captureMouse(false)
  , _mousePosX(0)
  , _mousePosY(0)
  , _mouseMoving(true)
{
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

  _manager = OIS::InputManager::createInputSystem(0);
  _padCount = std::min(4, _manager->getNumberOfDevices(OIS::OISJoyStick));
  OIS::JoyStick* tpads[4];
  tpads[0] = tpads[1] = tpads[2] = tpads[3] = 0;
  _pads[0] = _pads[1] = _pads[2] = _pads[3] = 0;
  for (int i = 0; i < _padCount; ++i) {
    try {
      tpads[i] = (OIS::JoyStick*)
          _manager->createInputObject(OIS::OISJoyStick, true);
      tpads[i]->setEventCallback(&_padHandler);
    }
    catch (const std::exception& e) {
      tpads[i] = 0;
      (void) e;
    }
  }

  _padHandler.SetLib(this);
  int configs = 0;
  std::ifstream f("gamepads.txt");
  std::string line;
  getline(f, line);
  std::stringstream ss(line);
  ss >> configs;
  for (int i = 0; i < PLAYERS; ++i) {
    if (i >= configs) {
      _padConfig[i].SetDefault();
      for (int p = 0; p < PLAYERS; ++p) {
        if (!tpads[p]) {
          continue;
        }
        _pads[i] = tpads[p];
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
        _pads[i] = tpads[p];
        tpads[p] = 0;
        _padConfig[i].Read(f);
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
    if (!_pads[i]) {
      continue;
    }
    else {
      _ff[i] = 0;
    }
    _ff[i] = (OIS::ForceFeedback*)
        _pads[i]->queryInterface(OIS::Interface::ForceFeedback);

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
      _window.Create(
          m, "WiiSPACE", sf::Style::Fullscreen, sf::WindowSettings(0, 0, 0));
      _extraX = (m.Width - Lib::WIDTH) / 2;
      _extraY = (m.Height - Lib::HEIGHT) / 2;
      return;
    }
  }
  _window.Create(sf::VideoMode(640, 480, 32), "WiiSPACE",
                 sf::Style::Close | sf::Style::Titlebar);
  _extraX = 0;
  _extraY = 0;
}

void LibWin::SetWorkingDirectory(bool original)
{
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
}

LibWin::~LibWin()
{
#ifdef PLATFORM_LINUX
  free(_cwd);
#else
  delete _cwd;
#endif
  OIS::InputManager::destroyInputSystem(_manager);
  for (int i = 0; i < SOUND_MAX; ++i) {
    delete _sounds[i].second.second;
  }
}

// General
//------------------------------
void LibWin::Init()
{
  _settings = LoadSaveSettings();
  _image.LoadFromFile("console.png");
  _image.CreateMaskFromColor(RgbaToColor(0x000000ff));
  _image.SetSmooth(false);
  _font = sf::Sprite(_image);
  LoadSounds();

  ClearScreen();
  _window.Show(true);
  _window.UseVerticalSync(true);
  _window.SetFramerateLimit(50);
  _window.ShowMouseCursor(false);

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
    _padMoveVAxes[j] = 0;
    _padMoveHAxes[j] = 0;
    _padAimVAxes[j] = 0;
    _padAimHAxes[j] = 0;
    _padMoveDpads[j] = OIS::Pov::Centered;
    _padAimDpads[j] = OIS::Pov::Centered;
  }
  sf::Listener::SetGlobalVolume(
      std::max(0.f, std::min(100.f, z_float(_settings._volume))));
  for (int i = 0; i < SOUND_MAX; ++i) {
    _voices.push_back(new sf::Sound());
  }
}

void LibWin::BeginFrame()
{
  int t = _mousePosX;
  int u = _mousePosY;
  sf::Event e;
  int kp = GetPlayerCount() <= _padCount ? GetPlayerCount() - 1 : _padCount;
  while (_window.GetEvent(e))
  {
    if (e.Type == sf::Event::Closed) {
      _exitType = EXIT_TO_LOADER;
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

  for (std::size_t i = 0; i < _sounds.size(); ++i) {
    if (_sounds[i].first > 0) {
      _sounds[i].first--;
    }
  }

  _mousePosX = std::max(0, std::min(Lib::WIDTH - 1, _mousePosX));
  _mousePosY = std::max(0, std::min(Lib::HEIGHT - 1, _mousePosY));
  if (t != _mousePosX || u != _mousePosY) {
    _mouseMoving = true;
  }
  if (_captureMouse) {
    _window.SetCursorPosition(_mousePosX, _mousePosY);
  }

  for (int i = 0; i < _padCount; ++i) {
    _pads[i]->capture();
    const OIS::JoyStickState& s = _pads[i]->getJoyStickState();
    for (std::size_t j = 0; j < s.mAxes.size(); ++j) {
      OIS::JoyStickEvent arg(_pads[i], s);
      _padHandler.axisMoved(arg, int(j));
    }
  }
}

void LibWin::EndFrame()
{
  if (!_window.IsOpened()) {
    _exitType = EXIT_TO_SYSTEM;
  }

  for (int i = 0; i < KEY_MAX; ++i) {
    for (int j = 0; j < PLAYERS; ++j) {
      _keysPressed[i][j] = false;
      _keysReleased[i][j] = false;
    }
  }
  /*for (int i = 0; i < PLAYERS; i++) {
    if (_rumble[i]) {
      _rumble[i]--;
      if (!_rumble[i]) {
      WPAD_Rumble(i, false);
      PAD_ControlMotor(i, PAD_MOTOR_STOP);
    }
  }*/
}

void LibWin::CaptureMouse(bool enabled)
{
  _captureMouse = enabled;
}

void LibWin::NewGame()
{
  for (int i = 0; i < KEY_MAX; ++i) {
    for (int j = 0; j < PLAYERS; ++j) {
      _keysPressed[i][j] = false;
      _keysReleased[i][j] = false;
      _keysHeld[i][j] = false;
    }
  }
}

void LibWin::Exit(ExitType t)
{
  if (!LoadSaveSettings()._windowed) {
    _window.Create(sf::VideoMode(640, 480, 32), "WiiSPACE",
                   sf::Style::Close | sf::Style::Titlebar);
  }
  _exitType = t;
}

Lib::ExitType LibWin::GetExitType() const
{
  return _exitType;
}

void LibWin::SystemExit(bool powerOff) const
{
}

int LibWin::RandInt(int lessThan)
{
  return z_rand() % lessThan;
}

fixed LibWin::RandFloat()
{
  return fixed(z_rand()) / fixed(Z_RAND_MAX);
}

void LibWin::TakeScreenShot()
{
  sf::Image image = _window.Capture();
  for (unsigned int x = 0; x < image.GetWidth(); ++x) {
    for (unsigned int y = 0; y < image.GetHeight(); ++y) {
      sf::Color c = image.GetPixel(x, y);
      c.a = 0xff;
      image.SetPixel(x, y, c);
    }
  }
  image.SaveToFile(ScreenshotPath());
}

LibWin::Settings LibWin::LoadSettings() const
{
  return _settings;
}

void LibWin::SetVolume(int volume)
{
  _settings._volume = fixed(std::max(0, std::min(100, volume)));
  sf::Listener::SetGlobalVolume(z_float(_settings._volume));
  SaveSaveSettings(_settings);
}

// Input
//------------------------------
LibWin::PadType LibWin::IsPadConnected(int player) const
{
  PadType r = PAD_NONE;
  if (player < _padCount) {
    r = PAD_GAMEPAD;
  }
  if ((GetPlayerCount() <= _padCount && player == GetPlayerCount() - 1) ||
      (GetPlayerCount() > _padCount && player == _padCount)) {
    r = PadType(r | PAD_KEYMOUSE);
  }
  return r;
}

bool LibWin::IsKeyPressed(int player, Key k) const
{
  return _keysPressed[k][player];
}

bool LibWin::IsKeyReleased(int player, Key k) const
{
  return _keysReleased[k][player];
}

bool LibWin::IsKeyHeld(int player, Key k) const
{
  Vec2 v(_padAimHAxes[player], _padAimVAxes[player]);
  if (k == KEY_FIRE && (_padAimDpads[player] != OIS::Pov::Centered ||
                        v.Length() >= M_PT_ONE * 2)) {
    return true;
  }
  return _keysHeld[k][player];
}

Vec2 LibWin::GetMoveVelocity(int player) const
{
  bool kU = IsKeyHeld(player, KEY_UP) ||
      _padMoveDpads[player] & OIS::Pov::North;
  bool kD = IsKeyHeld(player, KEY_DOWN) ||
      _padMoveDpads[player] & OIS::Pov::South;
  bool kL = IsKeyHeld(player, KEY_LEFT) ||
      _padMoveDpads[player] & OIS::Pov::West;
  bool kR = IsKeyHeld(player, KEY_RIGHT) ||
      _padMoveDpads[player] & OIS::Pov::East;

  Vec2 v;
  if (kU) {
    v += Vec2(0, -1);
  }
  if (kD) {
    v += Vec2(0, 1);
  }
  if (kL) {
    v += Vec2(-1, 0);
  }
  if (kR) {
    v += Vec2(1, 0);
  }
  if (v != Vec2()) {
    return v;
  }

  v = Vec2(_padMoveHAxes[player], _padMoveVAxes[player]);
  if (v.Length() < M_PT_ONE * 2) {
    return Vec2();
  }
  return v;
}

Vec2 LibWin::GetFireTarget(int player, const Vec2& position) const
{
  bool kp = player ==
      (GetPlayerCount() <= _padCount ? GetPlayerCount() - 1 : _padCount);
  Vec2 v(_padAimHAxes[player], _padAimVAxes[player]);

  if (v.Length() >= M_PT_ONE * 2) {
    v.Normalise();
    v *= 48;
    if (kp) {
      _mouseMoving = false;
    }
    _padLastAim[player] = v;
    return v + position;
  }

  if (_padAimDpads[player] != OIS::Pov::Centered) {
    bool kU = (_padAimDpads[player] & OIS::Pov::North) != 0;
    bool kD = (_padAimDpads[player] & OIS::Pov::South) != 0;
    bool kL = (_padAimDpads[player] & OIS::Pov::West) != 0;
    bool kR = (_padAimDpads[player] & OIS::Pov::East) != 0;

    Vec2 v;
    if (kU) {
      v += Vec2(0, -1);
    }
    if (kD) {
      v += Vec2(0, 1);
    }
    if (kL) {
      v += Vec2(-1, 0);
    }
    if (kR) {
      v += Vec2(1, 0);
    }

    if (v != Vec2()) {
      v.Normalise();
      v *= 48;
      if (kp) {
        _mouseMoving = false;
      }
      _padLastAim[player] = v;
      return v + position;
    }
  }

  if (_mouseMoving && kp) {
    return Vec2(fixed(_mousePosX), fixed(_mousePosY));
  }
  if (_padLastAim[player] != Vec2()) {
    return position + _padLastAim[player];
  }
  return position + Vec2(48, 0);
}

// Output
//------------------------------
void LibWin::ClearScreen() const
{
  glClear(GL_COLOR_BUFFER_BIT);
  _window.Clear(RgbaToColor(0x000000ff));
  _window.Draw(sf::Shape::Rectangle(0, 0, 0, 0, sf::Color()));
}

void LibWin::RenderLine(const Vec2f& a, const Vec2f& b, Colour c) const
{
  c = Cycle(c);
  glBegin(GL_LINES);
  glColor4ub(c >> 24, c >> 16, c >> 8, c);
  glVertex3f(a._x + _extraX, a._y + _extraY, 0);
  glVertex3f(b._x + _extraX, b._y + _extraY, 0);
  glColor4ub(c >> 24, c >> 16, c >> 8, c & 0xffffff33);
  glVertex3f(a._x + _extraX + 1, a._y + _extraY, 0);
  glVertex3f(b._x + _extraX + 1, b._y + _extraY, 0);
  glVertex3f(a._x + _extraX - 1, a._y + _extraY, 0);
  glVertex3f(b._x + _extraX - 1, b._y + _extraY, 0);
  glVertex3f(a._x + _extraX, a._y + _extraY + 1, 0);
  glVertex3f(b._x + _extraX, b._y + _extraY + 1, 0);
  glVertex3f(a._x + _extraX, a._y + _extraY - 1, 0);
  glVertex3f(b._x + _extraX, b._y + _extraY - 1, 0);
  glEnd();
}

void LibWin::RenderText(const Vec2f& v, const std::string& text, Colour c) const
{
  _font.SetColor(RgbaToColor(Cycle(c)));
  for (std::size_t i = 0; i < text.length(); ++i) {
    _font.SetPosition(
        (int(i) + v._x) * TEXT_WIDTH + _extraX, v._y * TEXT_HEIGHT + _extraY);
    _font.SetSubRect(sf::IntRect(TEXT_WIDTH * text[i], 0,
                                 TEXT_WIDTH * (1 + text[i]), TEXT_HEIGHT));
    _window.Draw(_font);
  }
  _window.Draw(sf::Shape::Rectangle(0, 0, 0, 0, sf::Color()));
}

void LibWin::RenderRect(
    const Vec2f& low, const Vec2f& hi, Colour c, int lineWidth) const
{
  c = Cycle(c);
  Vec2f ab(low._x, hi._y);
  Vec2f ba(hi._x, low._y);
  const Vec2f* list[4];
  Vec2f normals[4];
  list[0] = &low;
  list[1] = &ab;
  list[2] = &hi;
  list[3] = &ba;

  Vec2f centre = (low + hi) / 2;
  for (std::size_t i = 0; i < 4; ++i) {
    const Vec2f& v0 = *list[(i + 3) % 4];
    const Vec2f& v1 = *list[i];
    const Vec2f& v2 = *list[(i + 1) % 4];

    Vec2f n1(v0._y - v1._y, v1._x - v0._x);
    Vec2f n2(v1._y - v2._y, v2._x - v1._x);

    n1.Normalise();
    n2.Normalise();

    float f = 1 + n1._x * n2._x + n1._y * n2._y;
    normals[i] = (n1 + n2) / f;
    float dot =
        (v1._x - centre._x) * normals[i]._x +
        (v1._y - centre._y) * normals[i]._y;

    if (dot < 0) {
      normals[i] = Vec2f() - normals[i];
    }
  }

  glBegin(GL_TRIANGLE_STRIP);
  glColor4ub(c >> 24, c >> 16, c >> 8, c);
  for (std::size_t i = 0; i < 5; ++i)
  {
    glVertex3f(list[i % 4]->_x, list[i % 4]->_y, 0);
    glVertex3f(list[i % 4]->_x + normals[i % 4]._x,
               list[i % 4]->_y + normals[i % 4]._y, 0);
  }
  glEnd();

  if (lineWidth > 1) {
    RenderRect(low + Vec2f(1.f, 1.f),
               hi - Vec2f(1.f, 1.f), Cycle(c), lineWidth - 1);
  }
}

void LibWin::Render() const
{
  _window.Draw(sf::Shape::Rectangle(
      0.f, 0.f,
      float(_extraX), float(Lib::HEIGHT + _extraY * 2),
      sf::Color(0, 0, 0, 255)));
  _window.Draw(sf::Shape::Rectangle(
      0.f, 0.f,
      float(Lib::WIDTH + _extraX * 2), float(_extraY),
      sf::Color(0, 0, 0, 255)));
  _window.Draw(sf::Shape::Rectangle(
      float(Lib::WIDTH + _extraX), 0,
      float(Lib::WIDTH + _extraX * 2), float(Lib::HEIGHT + _extraY * 2),
      sf::Color(0, 0, 0, 255)));
  _window.Draw(sf::Shape::Rectangle(
      0.f, float(Lib::HEIGHT + _extraY),
      float(Lib::WIDTH + _extraX * 2), float(Lib::HEIGHT + _extraY * 2),
      sf::Color(0, 0, 0, 255)));
  _window.Draw(sf::Shape::Rectangle(
      0.f, 0.f,
      float(_extraX - 2), float(Lib::HEIGHT + _extraY * 2),
      sf::Color(32, 32, 32, 255)));
  _window.Draw(sf::Shape::Rectangle(
      0.f, 0.f,
      float(Lib::WIDTH + _extraX * 2), float(_extraY - 2),
      sf::Color(32, 32, 32, 255)));
  _window.Draw(sf::Shape::Rectangle(
      float(Lib::WIDTH + _extraX + 2), float(_extraY - 4),
      float(Lib::WIDTH + _extraX * 2), float(Lib::HEIGHT + _extraY * 2),
      sf::Color(32, 32, 32, 255)));
  _window.Draw(sf::Shape::Rectangle(
      float(_extraX - 2), float(Lib::HEIGHT + _extraY + 2),
      float(Lib::WIDTH + _extraX * 2), float(Lib::HEIGHT + _extraY * 2),
      sf::Color(32, 32, 32, 255)));
  _window.Draw(sf::Shape::Rectangle(
      float(_extraX - 4), float(_extraY - 4),
      float(_extraX - 2), float(Lib::HEIGHT + _extraY + 4),
      sf::Color(128, 128, 128, 255)));
  _window.Draw(sf::Shape::Rectangle(
      float(_extraX - 4), float(_extraY - 4),
      float(Lib::WIDTH + _extraX + 4), float(_extraY - 2),
      sf::Color(128, 128, 128, 255)));
  _window.Draw(sf::Shape::Rectangle(
      float(Lib::WIDTH + _extraX + 2), float(_extraY - 4), 
      float(Lib::WIDTH + _extraX + 4), float(Lib::HEIGHT + _extraY + 4),
      sf::Color(128, 128, 128, 255)));
  _window.Draw(sf::Shape::Rectangle(
      float(_extraX - 2), float(Lib::HEIGHT + _extraY + 2),
      float(Lib::WIDTH + _extraX + 4), float(Lib::HEIGHT + _extraY + 4),
      sf::Color(128, 128, 128, 255)));
  _window.Display();
}

void LibWin::Rumble(int player, int time)
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

void LibWin::StopRumble()
{
  /*for (int i = 0; i < PLAYERS; i++) {
    _rumble[i] = 0;
    WPAD_Rumble(i, false);
    PAD_ControlMotor(i, PAD_MOTOR_STOP);
  }*/
}

bool LibWin::PlaySound(Sound sound, float volume, float pan, float repitch)
{
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
  for (std::size_t i = 0; i < _sounds.size(); i++) {
    if (sound == _sounds[i].second.first) {
      if (_sounds[i].first <= 0) {
        buffer = _sounds[i].second.second;
        _sounds[i].first = SOUND_TIMER;
      }
      break;
    }
  }

  if (!buffer) {
    return false;
  }
  for (int i = 0; i < SOUND_MAX; ++i) {
    if (_voices[i]->GetStatus() != sf::Sound::Playing) {
      _voices[i]->SetAttenuation(0.f);
      _voices[i]->SetLoop(false);
      _voices[i]->SetMinDistance(100.f);
      _voices[i]->SetBuffer(*buffer);
      _voices[i]->SetVolume(100.f * volume);
      _voices[i]->SetPitch(pow(2.f, repitch));
      _voices[i]->SetPosition(pan, 0, -1);
      _voices[i]->Play();
      return true;
    }
  }
  return false;
}

// Sounds
//------------------------------
#define USE_SOUND(sound, data)\
    sf::SoundBuffer* t_##sound = new sf::SoundBuffer();\
    t_##sound->LoadFromFile(data);\
    _sounds.push_back(SoundResource(0, NamedSound(sound, t_##sound)));

void LibWin::LoadSounds()
{
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
}

#endif
