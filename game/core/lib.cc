#include "game/core/lib.h"
#include "game/core/save.h"
#include <algorithm>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

const std::string Lib::kSuperEncryptionKey = "<>";

#ifdef PLATFORM_LINUX
#include <sys/stat.h>
#include <unistd.h>
#endif

#ifndef PLATFORM_SCORE
#include "game/util/gamepad.h"
#include <OISForceFeedback.h>
#include <OISInputManager.h>
#include <OISJoyStick.h>
#include <SFML/Audio.hpp>
#include <SFML/Graphics.hpp>
#include <SFML/OpenGL.hpp>

#define RgbaToColor(colour) (sf::Color((colour) >> 24, (colour) >> 16, (colour) >> 8, (colour)))

sf::RectangleShape MakeRectangle(float x, float y, float w, float h, sf::Color color) {
  sf::RectangleShape r{{w, h}};
  r.setPosition(x, y);
  r.setOutlineColor(color);
  return r;
}

bool SfkToKey(sf::Keyboard::Key code, Lib::key key) {
  if ((code == sf::Keyboard::W || code == sf::Keyboard::Up) && key == Lib::key::kUp) {
    return true;
  }
  if ((code == sf::Keyboard::A || code == sf::Keyboard::Left) && key == Lib::key::kLeft) {
    return true;
  }
  if ((code == sf::Keyboard::S || code == sf::Keyboard::Down) && key == Lib::key::kDown) {
    return true;
  }
  if ((code == sf::Keyboard::D || code == sf::Keyboard::Right) && key == Lib::key::kRight) {
    return true;
  }
  if ((code == sf::Keyboard::Escape || code == sf::Keyboard::Return) && key == Lib::key::kMenu) {
    return true;
  }
  if ((code == sf::Keyboard::Z || code == sf::Keyboard::LControl ||
       code == sf::Keyboard::RControl) &&
      key == Lib::key::kFire) {
    return true;
  }
  if ((code == sf::Keyboard::X || code == sf::Keyboard::Space) && key == Lib::key::kBomb) {
    return true;
  }
  if ((code == sf::Keyboard::Z || code == sf::Keyboard::Space || code == sf::Keyboard::LControl ||
       code == sf::Keyboard::RControl) &&
      key == Lib::key::kAccept) {
    return true;
  }
  if ((code == sf::Keyboard::X || code == sf::Keyboard::Escape) && key == Lib::key::kCancel) {
    return true;
  }
  return false;
}

bool SfmToKey(sf::Mouse::Button code, Lib::key key) {
  if (code == sf::Mouse::Left && (key == Lib::key::kFire || key == Lib::key::kAccept)) {
    return true;
  }
  if (code == sf::Mouse::Right && (key == Lib::key::kBomb || key == Lib::key::kCancel)) {
    return true;
  }
  return false;
}

bool PadToKey(PadConfig config, std::int32_t button, Lib::key key) {
  PadConfig::Buttons none;
  const PadConfig::Buttons& buttons = key == Lib::key::kMenu ? config._startButtons
      : key == Lib::key::kFire || key == Lib::key::kAccept   ? config._fireButtons
      : key == Lib::key::kBomb || key == Lib::key::kCancel   ? config._bombButtons
      : key == Lib::key::kUp                                 ? config._moveUpButtons
      : key == Lib::key::kDown                               ? config._moveDownButtons
      : key == Lib::key::kLeft                               ? config._moveLeftButtons
      : key == Lib::key::kRight                              ? config._moveRightButtons
                                                             : none;

  for (std::size_t i = 0; i < buttons.size(); ++i) {
    if (buttons[i] == button) {
      return true;
    }
  }
  return false;
}

class Handler : public OIS::JoyStickListener {
public:
  void SetLib(Lib* lib) {
    _lib = lib;
  }

  bool buttonPressed(const OIS::JoyStickEvent& arg, std::int32_t button) override;
  bool buttonReleased(const OIS::JoyStickEvent& arg, std::int32_t button) override;
  bool axisMoved(const OIS::JoyStickEvent& arg, std::int32_t axis) override;
  bool povMoved(const OIS::JoyStickEvent& arg, std::int32_t pov) override;

private:
  Lib* _lib;
};

struct Internals {
  mutable sf::RenderWindow window;
  sf::Texture texture;
  mutable sf::Sprite font;

  OIS::InputManager* manager;
  Handler pad_handler;
  std::int32_t pad_count;
  PadConfig pad_config[kPlayers];
  OIS::JoyStick* pads[kPlayers];
  OIS::ForceFeedback* ff[kPlayers];
  fixed pad_move_vaxes[kPlayers];
  fixed pad_move_haxes[kPlayers];
  fixed pad_aim_vaxes[kPlayers];
  fixed pad_aim_haxes[kPlayers];
  std::int32_t pad_move_dpads[kPlayers];
  std::int32_t pad_aim_dpads[kPlayers];
  mutable vec2 pad_last_aim[kPlayers];

  typedef std::pair<Lib::sound, std::unique_ptr<sf::SoundBuffer>> named_sound;
  typedef std::pair<std::int32_t, named_sound> sound_resource;
  std::vector<sound_resource> sounds;
  std::vector<sf::Sound> voices;
};

bool Handler::buttonPressed(const OIS::JoyStickEvent& arg, std::int32_t button) {
  std::int32_t p = -1;
  for (std::int32_t i = 0; i < kPlayers && i < _lib->_internals->pad_count; ++i) {
    if (_lib->_internals->pads[i] == arg.device) {
      p = i;
    }
  }
  if (p < 0) {
    return true;
  }

  for (std::int32_t i = 0; i < static_cast<std::int32_t>(Lib::key::kMax); ++i) {
    if (PadToKey(_lib->_internals->pad_config[p], button, static_cast<Lib::key>(i))) {
      _lib->_keys_pressed[i][p] = true;
      _lib->_keys_held[i][p] = true;
    }
  }
  return true;
}

bool Handler::buttonReleased(const OIS::JoyStickEvent& arg, std::int32_t button) {
  std::int32_t p = -1;
  for (std::int32_t i = 0; i < kPlayers && i < _lib->_internals->pad_count; ++i) {
    if (_lib->_internals->pads[i] == arg.device) {
      p = i;
    }
  }
  if (p < 0) {
    return true;
  }

  for (std::int32_t i = 0; i < static_cast<std::int32_t>(Lib::key::kMax); ++i) {
    if (PadToKey(_lib->_internals->pad_config[p], button, static_cast<Lib::key>(i))) {
      _lib->_keys_released[i][p] = true;
      _lib->_keys_held[i][p] = false;
    }
  }
  return true;
}

bool Handler::axisMoved(const OIS::JoyStickEvent& arg, std::int32_t axis) {
  std::int32_t p = -1;
  for (std::int32_t i = 0; i < kPlayers && i < _lib->_internals->pad_count; ++i) {
    if (_lib->_internals->pads[i] == arg.device) {
      p = i;
    }
  }
  if (p < 0) {
    return true;
  }

  fixed v = std::max(
      -fixed(1), std::min(fixed(1), fixed(arg.state.mAxes[axis].abs) / OIS::JoyStick::MAX_AXIS));
  PadConfig& config = _lib->_internals->pad_config[p];

  for (std::size_t i = 0; i < config._moveSticks.size(); ++i) {
    if (config._moveSticks[i]._axis1 == axis) {
      _lib->_internals->pad_move_vaxes[p] = config._moveSticks[i]._axis1r ? -v : v;
    }
    if (config._moveSticks[i]._axis2 == axis) {
      _lib->_internals->pad_move_haxes[p] = config._moveSticks[i]._axis2r ? -v : v;
    }
  }
  for (std::size_t i = 0; i < config._aimSticks.size(); ++i) {
    if (config._aimSticks[i]._axis1 == axis) {
      _lib->_internals->pad_aim_vaxes[p] = config._aimSticks[i]._axis1r ? -v : v;
    }
    if (config._aimSticks[i]._axis2 == axis) {
      _lib->_internals->pad_aim_haxes[p] = config._aimSticks[i]._axis2r ? -v : v;
    }
  }
  return true;
}

bool Handler::povMoved(const OIS::JoyStickEvent& arg, std::int32_t pov) {
  std::int32_t p = -1;
  for (std::int32_t i = 0; i < kPlayers && i < _lib->_internals->pad_count; ++i) {
    if (_lib->_internals->pads[i] == arg.device) {
      p = i;
    }
  }
  if (p < 0) {
    return true;
  }

  auto k = [](Lib::key key) { return static_cast<std::size_t>(key); };

  PadConfig& config = _lib->_internals->pad_config[p];
  std::int32_t d = arg.state.mPOV[pov].direction;
  for (std::size_t i = 0; i < config._moveDpads.size(); ++i) {
    if (config._moveDpads[i] == pov) {
      _lib->_internals->pad_move_dpads[p] = d;
      if (d & OIS::Pov::North) {
        _lib->_keys_pressed[k(Lib::key::kUp)][p] = true;
        _lib->_keys_held[k(Lib::key::kUp)][p] = true;
      } else {
        if (_lib->_keys_held[k(Lib::key::kUp)][p]) {
          _lib->_keys_released[k(Lib::key::kUp)][p] = true;
        }
        _lib->_keys_held[k(Lib::key::kUp)][p] = false;
      }
      if (d & OIS::Pov::South) {
        _lib->_keys_pressed[k(Lib::key::kDown)][p] = true;
        _lib->_keys_held[k(Lib::key::kDown)][p] = true;
      } else {
        if (_lib->_keys_held[k(Lib::key::kDown)][p]) {
          _lib->_keys_released[k(Lib::key::kDown)][p] = true;
        }
        _lib->_keys_held[k(Lib::key::kDown)][p] = false;
      }
      if (d & OIS::Pov::West) {
        _lib->_keys_pressed[k(Lib::key::kLeft)][p] = true;
        _lib->_keys_held[k(Lib::key::kLeft)][p] = true;
      } else {
        if (_lib->_keys_held[k(Lib::key::kLeft)][p]) {
          _lib->_keys_released[k(Lib::key::kLeft)][p] = true;
        }
        _lib->_keys_held[k(Lib::key::kLeft)][p] = false;
      }
      if (d & OIS::Pov::East) {
        _lib->_keys_pressed[k(Lib::key::kRight)][p] = true;
        _lib->_keys_held[k(Lib::key::kRight)][p] = true;
      } else {
        if (_lib->_keys_held[k(Lib::key::kRight)][p]) {
          _lib->_keys_released[k(Lib::key::kRight)][p] = true;
        }
        _lib->_keys_held[k(Lib::key::kRight)][p] = false;
      }
    }
  }
  for (std::size_t i = 0; i < config._aimDpads.size(); ++i) {
    if (config._aimDpads[i] == pov) {
      _lib->_internals->pad_aim_dpads[p] = d;
    }
  }
  return true;
}
#else
struct Internals {};
#endif

Lib::Lib() : _cycle(0), _players(1), _capture_mouse(false), _mouse_moving(true), _score_frame(0) {
#ifndef PLATFORM_SCORE
  set_working_directory(false);
#ifdef PLATFORM_LINUX
  mkdir("replays", 0777);
#else
  CreateDirectory("replays", 0);
#endif
  _internals.reset(new Internals());

  bool created = false;
  for (int i = sf::VideoMode::getFullscreenModes().size() - 1; i >= 0 && !Settings().windowed;
       --i) {
    sf::VideoMode m = sf::VideoMode::getFullscreenModes()[i];
    if (m.width >= unsigned(Lib::kWidth) && m.height >= unsigned(Lib::kHeight) &&
        m.bitsPerPixel == 32) {
      _internals->window.create(m, "WiiSPACE", sf::Style::Close | sf::Style::Titlebar,
                                sf::ContextSettings(0, 0, 0));
      _extra.x = (m.width - Lib::kWidth) / 2;
      _extra.y = (m.height - Lib::kHeight) / 2;
      created = true;
      break;
    }
  }
  if (!created) {
    _internals->window.create(sf::VideoMode(640, 480, 32), "WiiSPACE",
                              sf::Style::Close | sf::Style::Titlebar);
  }

  _internals->manager = OIS::InputManager::createInputSystem(
      reinterpret_cast<std::size_t>(_internals->window.getSystemHandle()));
  _internals->pad_count = std::min(4, _internals->manager->getNumberOfDevices(OIS::OISJoyStick));
  OIS::JoyStick* tpads[4];
  tpads[0] = tpads[1] = tpads[2] = tpads[3] = 0;
  _internals->pads[0] = _internals->pads[1] = _internals->pads[2] = _internals->pads[3] = 0;
  for (std::int32_t i = 0; i < _internals->pad_count; ++i) {
    try {
      tpads[i] = (OIS::JoyStick*)_internals->manager->createInputObject(OIS::OISJoyStick, true);
      tpads[i]->setEventCallback(&_internals->pad_handler);
    } catch (const std::exception& e) {
      tpads[i] = 0;
      (void)e;
    }
  }

  _internals->pad_handler.SetLib(this);
  std::int32_t configs = 0;
  std::ifstream f("gamepads.txt");
  std::string line;
  getline(f, line);
  std::stringstream ss(line);
  ss >> configs;
  for (std::int32_t i = 0; i < kPlayers; ++i) {
    if (i >= configs) {
      _internals->pad_config[i].SetDefault();
      for (std::int32_t p = 0; p < kPlayers; ++p) {
        if (!tpads[p]) {
          continue;
        }
        _internals->pads[i] = tpads[p];
        tpads[p] = 0;
        break;
      }
      continue;
    }
    getline(f, line);
    bool assigned = false;
    for (std::int32_t p = 0; p < kPlayers; ++p) {
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
        _internals->pads[i] = tpads[p];
        tpads[p] = 0;
        _internals->pad_config[i].Read(f);
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

  for (std::int32_t i = 0; i < kPlayers; ++i) {
    if (!_internals->pads[i]) {
      continue;
    } else {
      _internals->ff[i] = 0;
    }
    _internals->ff[i] =
        (OIS::ForceFeedback*)_internals->pads[i]->queryInterface(OIS::Interface::ForceFeedback);

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

  sf::Image image;
  image.loadFromFile("console.png");
  image.createMaskFromColor(RgbaToColor(0x000000ff));
  _internals->texture.loadFromImage(image);
  _internals->texture.setSmooth(false);
  _internals->font = sf::Sprite(_internals->texture);
  load_sounds();

  clear_screen();
  _internals->window.setVisible(true);
  _internals->window.setVerticalSyncEnabled(true);
  _internals->window.setFramerateLimit(50);
  _internals->window.setMouseCursorVisible(false);

  glClearColor(0.f, 0.f, 0.f, 0.f);
  glDisable(GL_DEPTH_TEST);
  glLineWidth(1);
  glEnable(GL_BLEND);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

  for (std::int32_t i = 0; i < static_cast<std::int32_t>(key::kMax); ++i) {
    std::vector<bool> f;
    for (std::int32_t j = 0; j < kPlayers; ++j) {
      f.push_back(false);
    }
    _keys_pressed.push_back(f);
    _keys_held.push_back(f);
    _keys_released.push_back(f);
  }
  for (std::int32_t j = 0; j < kPlayers; ++j) {
    _internals->pad_move_vaxes[j] = 0;
    _internals->pad_move_haxes[j] = 0;
    _internals->pad_aim_vaxes[j] = 0;
    _internals->pad_aim_haxes[j] = 0;
    _internals->pad_move_dpads[j] = OIS::Pov::Centered;
    _internals->pad_aim_dpads[j] = OIS::Pov::Centered;
  }
  for (std::int32_t i = 0; i < static_cast<std::int32_t>(sound::kMax); ++i) {
    _internals->voices.emplace_back();
  }
#endif
}

Lib::~Lib() {
#ifndef PLATFORM_SCORE
  OIS::InputManager::destroyInputSystem(_internals->manager);
#endif
}

bool Lib::is_key_pressed(key k) const {
  for (std::int32_t i = 0; i < kPlayers; i++)
    if (is_key_pressed(i, k)) {
      return true;
    }
  return false;
}

bool Lib::is_key_released(key k) const {
  for (std::int32_t i = 0; i < kPlayers; i++)
    if (is_key_released(i, k)) {
      return true;
    }
  return false;
}

bool Lib::is_key_held(key k) const {
  for (std::int32_t i = 0; i < kPlayers; i++)
    if (is_key_held(i, k)) {
      return true;
    }
  return false;
}

void Lib::set_colour_cycle(std::int32_t cycle) {
  _cycle = cycle % 256;
}

std::int32_t Lib::get_colour_cycle() const {
  return _cycle;
}

void Lib::set_working_directory(bool original) {
#ifndef PLATFORM_SCORE
  static bool initialised = false;
  static char* cwd = nullptr;
  static std::vector<char> exe;

  if (!initialised) {
#ifdef PLATFORM_LINUX
    cwd = get_current_dir_name();
    char temp[256];
    sprintf(temp, "/proc/%d/exe", getpid());
    exe.resize(256);
    std::size_t bytes = 0;
    while ((bytes = readlink(temp, &exe[0], exe.size())) == exe.size()) {
      exe.resize(exe.size() * 2);
    }
    if (bytes >= 0) {
      while (exe[bytes] != '/') {
        --bytes;
      }
      exe[bytes] = '\0';
    }
#else
    DWORD length = MAX_PATH;
    cwd = new char[MAX_PATH + 1];
    GetCurrentDirectory(length, cwd);
    cwd[MAX_PATH + 1] = '\0';

    exe.resize(MAX_PATH);
    DWORD result = GetModuleFileName(0, &exe[0], (DWORD)exe.size());
    while (result == exe.size()) {
      exe.resize(exe.size() * 2);
      result = GetModuleFileName(0, &exe[0], (DWORD)exe.size());
    }

    if (result != 0) {
      --result;
      while (exe[result] != '\\') {
        --result;
      }
      exe[result] = '\0';
    }
#endif
    initialised = true;
  }

#ifdef PLATFORM_LINUX
  [](int) {}(chdir(original ? cwd : exe.data()));
#else
  SetCurrentDirectory(original ? cwd : exe.data());
#endif
#endif
}

bool Lib::begin_frame() {
#ifndef PLATFORM_SCORE
  ivec2 t = _mouse;
  sf::Event e;
  std::int32_t kp = _players <= _internals->pad_count ? _players - 1 : _internals->pad_count;
  while (_internals->window.pollEvent(e)) {
    if (e.type == sf::Event::Closed || !_internals->window.isOpen()) {
      return true;
    }

    for (std::int32_t i = 0; i < static_cast<std::int32_t>(key::kMax); ++i) {
      bool kd = e.type == sf::Event::KeyPressed;
      bool ku = e.type == sf::Event::KeyReleased;
      bool md = e.type == sf::Event::MouseButtonPressed;
      bool mu = e.type == sf::Event::MouseButtonReleased;

      bool b = false;
      b |= (kd || ku) && SfkToKey(e.key.code, static_cast<key>(i));
      b |= (md || mu) && SfmToKey(e.mouseButton.button, static_cast<key>(i));
      if (!b) {
        continue;
      }

      if (kd || md) {
        _keys_pressed[i][kp] = true;
        _keys_held[i][kp] = true;
      }
      if (ku || mu) {
        _keys_released[i][kp] = true;
        _keys_held[i][kp] = false;
      }
    }

    if (e.type == sf::Event::MouseMoved) {
      _mouse.x = e.mouseMove.x;
      _mouse.y = e.mouseMove.y;
    }
  }

  for (std::size_t i = 0; i < _internals->sounds.size(); ++i) {
    if (_internals->sounds[i].first > 0) {
      _internals->sounds[i].first--;
    }
  }

  _mouse = std::max(ivec2{}, std::min(ivec2{Lib::kWidth - 1, Lib::kHeight - 1}, _mouse));
  if (t != _mouse) {
    _mouse_moving = true;
  }
  if (_capture_mouse) {
    sf::Mouse::setPosition({_mouse.x, _mouse.y}, _internals->window);
  }

  for (std::int32_t i = 0; i < _internals->pad_count; ++i) {
    _internals->pads[i]->capture();
    const OIS::JoyStickState& s = _internals->pads[i]->getJoyStickState();
    for (std::size_t j = 0; j < s.mAxes.size(); ++j) {
      OIS::JoyStickEvent arg(_internals->pads[i], s);
      _internals->pad_handler.axisMoved(arg, j);
    }
  }
#else
  if (_score_frame < 5) {
    ++_score_frame;
  }
#endif
  return false;
}

void Lib::end_frame() {
#ifndef PLATFORM_SCORE
  for (std::int32_t i = 0; i < static_cast<std::int32_t>(key::kMax); ++i) {
    for (std::int32_t j = 0; j < kPlayers; ++j) {
      _keys_pressed[i][j] = false;
      _keys_released[i][j] = false;
    }
  }
#endif
  /*for (std::int32_t i = 0; i < kPlayers; i++) {
    if (_rumble[i]) {
      _rumble[i]--;
      if (!_rumble[i]) {
      WPAD_Rumble(i, false);
      PAD_ControlMotor(i, PAD_MOTOR_STOP);
    }
  }*/
}

void Lib::capture_mouse(bool enabled) {
  _capture_mouse = enabled;
}

void Lib::new_game() {
#ifndef PLATFORM_SCORE
  for (std::int32_t i = 0; i < static_cast<std::int32_t>(key::kMax); ++i) {
    for (std::int32_t j = 0; j < kPlayers; ++j) {
      _keys_pressed[i][j] = false;
      _keys_released[i][j] = false;
      _keys_held[i][j] = false;
    }
  }
#endif
}

void Lib::take_screenshot() {
#ifndef PLATFORM_SCORE
  sf::Texture texture;
  texture.create(_internals->window.getSize().x, _internals->window.getSize().y);
  texture.update(_internals->window);

  std::stringstream ss;
  ss << "screenshot" << time(0) % 10000000 << ".png";
  texture.copyToImage().saveToFile(ss.str());
#endif
}

// Input
//------------------------------
Lib::pad_type Lib::get_pad_type(std::int32_t player) const {
#ifndef PLATFORM_SCORE
  pad_type result = pad_type::kPadNone;
  if (player < _internals->pad_count) {
    result = pad_type::kPadGamepad;
  }
  if ((_players <= _internals->pad_count && player == _players - 1) ||
      (_players > _internals->pad_count && player == _internals->pad_count)) {
    result = static_cast<pad_type>(result | pad_type::kPadKeyboardMouse);
  }
  return result;
#else
  return pad_type::kPadNone;
#endif
}

bool Lib::is_key_pressed(std::int32_t player, key k) const {
#ifndef PLATFORM_SCORE
  return _keys_pressed[static_cast<std::size_t>(k)][player];
#else
  if (player != 0) {
    return false;
  }
  if (k == key::kDown && _score_frame < 4) {
    return true;
  }
  if (k == key::kAccept && _score_frame == 5) {
    return true;
  }
  return false;
#endif
}

bool Lib::is_key_released(std::int32_t player, key k) const {
#ifndef PLATFORM_SCORE
  return _keys_released[static_cast<std::size_t>(k)][player];
#else
  return false;
#endif
}

bool Lib::is_key_held(std::int32_t player, key k) const {
#ifndef PLATFORM_SCORE
  vec2 v(_internals->pad_aim_haxes[player], _internals->pad_aim_vaxes[player]);
  if (k == key::kFire &&
      (_internals->pad_aim_dpads[player] != OIS::Pov::Centered ||
       v.length() >= fixed_c::tenth * 2)) {
    return true;
  }
  return _keys_held[static_cast<std::size_t>(k)][player];
#else
  return false;
#endif
}

vec2 Lib::get_move_velocity(std::int32_t player) const {
#ifndef PLATFORM_SCORE
  bool ku = is_key_held(player, key::kUp) || _internals->pad_move_dpads[player] & OIS::Pov::North;
  bool kd = is_key_held(player, key::kDown) || _internals->pad_move_dpads[player] & OIS::Pov::South;
  bool kl = is_key_held(player, key::kLeft) || _internals->pad_move_dpads[player] & OIS::Pov::West;
  bool kr = is_key_held(player, key::kRight) || _internals->pad_move_dpads[player] & OIS::Pov::East;

  vec2 v = vec2(0, -1) * ku + vec2(0, 1) * kd + vec2(-1, 0) * kl + vec2(1, 0) * kr;
  if (v != vec2()) {
    return v;
  }

  v = vec2(_internals->pad_move_haxes[player], _internals->pad_move_vaxes[player]);
  if (v.length() < fixed_c::tenth * 2) {
    return vec2();
  }
  return v;
#else
  return vec2();
#endif
}

vec2 Lib::get_fire_target(std::int32_t player, const vec2& position) const {
#ifndef PLATFORM_SCORE
  bool kp = player == (_players <= _internals->pad_count ? _players - 1 : _internals->pad_count);
  vec2 v(_internals->pad_aim_haxes[player], _internals->pad_aim_vaxes[player]);

  if (v.length() >= fixed_c::tenth * 2) {
    v = v.normalised() * 48;
    if (kp) {
      _mouse_moving = false;
    }
    _internals->pad_last_aim[player] = v;
    return v + position;
  }

  if (_internals->pad_aim_dpads[player] != OIS::Pov::Centered) {
    bool ku = (_internals->pad_aim_dpads[player] & OIS::Pov::North) != 0;
    bool kd = (_internals->pad_aim_dpads[player] & OIS::Pov::South) != 0;
    bool kl = (_internals->pad_aim_dpads[player] & OIS::Pov::West) != 0;
    bool kr = (_internals->pad_aim_dpads[player] & OIS::Pov::East) != 0;

    vec2 v = vec2(0, -1) * ku + vec2(0, 1) * kd + vec2(-1, 0) * kl + vec2(1, 0) * kr;
    if (v != vec2()) {
      v = v.normalised() * 48;
      if (kp) {
        _mouse_moving = false;
      }
      _internals->pad_last_aim[player] = v;
      return v + position;
    }
  }

  if (_mouse_moving && kp) {
    return vec2(_mouse.x, _mouse.y);
  }
  if (_internals->pad_last_aim[player] != vec2()) {
    return position + _internals->pad_last_aim[player];
  }
  return position + vec2(48, 0);
#else
  return vec2();
#endif
}

// Output
//------------------------------
void Lib::clear_screen() const {
#ifndef PLATFORM_SCORE
  glClear(GL_COLOR_BUFFER_BIT);
  _internals->window.clear(RgbaToColor(0x000000ff));
  _internals->window.draw(MakeRectangle(0, 0, 0, 0, sf::Color()));
#endif
}

void Lib::render_line(const fvec2& a, const fvec2& b, colour_t c) const {
#ifndef PLATFORM_SCORE
  c = z::colour_cycle(c, _cycle);
  glBegin(GL_LINES);
  glColor4ub(c >> 24, c >> 16, c >> 8, c);
  glVertex3f(a.x + _extra.x, a.y + _extra.y, 0);
  glVertex3f(b.x + _extra.x, b.y + _extra.y, 0);
  glColor4ub(c >> 24, c >> 16, c >> 8, c & 0xffffff33);
  glVertex3f(a.x + _extra.x + 1, a.y + _extra.y, 0);
  glVertex3f(b.x + _extra.x + 1, b.y + _extra.y, 0);
  glVertex3f(a.x + _extra.x - 1, a.y + _extra.y, 0);
  glVertex3f(b.x + _extra.x - 1, b.y + _extra.y, 0);
  glVertex3f(a.x + _extra.x, a.y + _extra.y + 1, 0);
  glVertex3f(b.x + _extra.x, b.y + _extra.y + 1, 0);
  glVertex3f(a.x + _extra.x, a.y + _extra.y - 1, 0);
  glVertex3f(b.x + _extra.x, b.y + _extra.y - 1, 0);
  glEnd();
#endif
}

void Lib::render_text(const fvec2& v, const std::string& text, colour_t c) const {
#ifndef PLATFORM_SCORE
  _internals->font.setColor(RgbaToColor(z::colour_cycle(c, _cycle)));
  for (std::size_t i = 0; i < text.length(); ++i) {
    _internals->font.setPosition((std::int32_t(i) + v.x) * kTextWidth + _extra.x,
                                 v.y * kTextHeight + _extra.y);
    _internals->font.setTextureRect(sf::IntRect(kTextWidth * text[i], 0, kTextWidth, kTextHeight));
    _internals->window.draw(_internals->font);
  }
  _internals->window.draw(MakeRectangle(0, 0, 0, 0, sf::Color()));
#endif
}

void Lib::render_rect(const fvec2& low, const fvec2& hi, colour_t c,
                      std::int32_t line_width) const {
#ifndef PLATFORM_SCORE
  c = z::colour_cycle(c, _cycle);
  fvec2 ab(low.x, hi.y);
  fvec2 ba(hi.x, low.y);
  const fvec2* list[4];
  fvec2 normals[4];
  list[0] = &low;
  list[1] = &ab;
  list[2] = &hi;
  list[3] = &ba;

  fvec2 centre = (low + hi) / 2;
  for (std::size_t i = 0; i < 4; ++i) {
    const fvec2& v0 = *list[(i + 3) % 4];
    const fvec2& v1 = *list[i];
    const fvec2& v2 = *list[(i + 1) % 4];

    fvec2 n1 = fvec2(v0.y - v1.y, v1.x - v0.x).normalised();
    fvec2 n2 = fvec2(v1.y - v2.y, v2.x - v1.x).normalised();

    float f = 1 + n1.x * n2.x + n1.y * n2.y;
    normals[i] = (n1 + n2) / f;
    float dot = (v1.x - centre.x) * normals[i].x + (v1.y - centre.y) * normals[i].y;

    if (dot < 0) {
      normals[i] = fvec2() - normals[i];
    }
  }

  glBegin(GL_TRIANGLE_STRIP);
  glColor4ub(c >> 24, c >> 16, c >> 8, c);
  for (std::size_t i = 0; i < 5; ++i) {
    glVertex3f(list[i % 4]->x, list[i % 4]->y, 0);
    glVertex3f(list[i % 4]->x + normals[i % 4].x, list[i % 4]->y + normals[i % 4].y, 0);
  }
  glEnd();

  if (line_width > 1) {
    render_rect(low + fvec2(1.f, 1.f), hi - fvec2(1.f, 1.f), z::colour_cycle(c, _cycle),
                line_width - 1);
  }
#endif
}

void Lib::render() const {
#ifndef PLATFORM_SCORE
  _internals->window.draw(MakeRectangle(
      0.f, 0.f, float(_extra.x), float(Lib::kHeight + _extra.y * 2), sf::Color(0, 0, 0, 255)));
  _internals->window.draw(MakeRectangle(0.f, 0.f, float(Lib::kWidth + _extra.x * 2),
                                        float(_extra.y), sf::Color(0, 0, 0, 255)));
  _internals->window.draw(
      MakeRectangle(float(Lib::kWidth + _extra.x), 0, float(Lib::kWidth + _extra.x * 2),
                    float(Lib::kHeight + _extra.y * 2), sf::Color(0, 0, 0, 255)));
  _internals->window.draw(
      MakeRectangle(0.f, float(Lib::kHeight + _extra.y), float(Lib::kWidth + _extra.x * 2),
                    float(Lib::kHeight + _extra.y * 2), sf::Color(0, 0, 0, 255)));
  _internals->window.draw(MakeRectangle(0.f, 0.f, float(_extra.x - 2),
                                        float(Lib::kHeight + _extra.y * 2),
                                        sf::Color(32, 32, 32, 255)));
  _internals->window.draw(MakeRectangle(0.f, 0.f, float(Lib::kWidth + _extra.x * 2),
                                        float(_extra.y - 2), sf::Color(32, 32, 32, 255)));
  _internals->window.draw(MakeRectangle(
      float(Lib::kWidth + _extra.x + 2), float(_extra.y - 4), float(Lib::kWidth + _extra.x * 2),
      float(Lib::kHeight + _extra.y * 2), sf::Color(32, 32, 32, 255)));
  _internals->window.draw(MakeRectangle(
      float(_extra.x - 2), float(Lib::kHeight + _extra.y + 2), float(Lib::kWidth + _extra.x * 2),
      float(Lib::kHeight + _extra.y * 2), sf::Color(32, 32, 32, 255)));
  _internals->window.draw(MakeRectangle(float(_extra.x - 4), float(_extra.y - 4),
                                        float(_extra.x - 2), float(Lib::kHeight + _extra.y + 4),
                                        sf::Color(128, 128, 128, 255)));
  _internals->window.draw(MakeRectangle(float(_extra.x - 4), float(_extra.y - 4),
                                        float(Lib::kWidth + _extra.x + 4), float(_extra.y - 2),
                                        sf::Color(128, 128, 128, 255)));
  _internals->window.draw(MakeRectangle(
      float(Lib::kWidth + _extra.x + 2), float(_extra.y - 4), float(Lib::kWidth + _extra.x + 4),
      float(Lib::kHeight + _extra.y + 4), sf::Color(128, 128, 128, 255)));
  _internals->window.draw(MakeRectangle(
      float(_extra.x - 2), float(Lib::kHeight + _extra.y + 2), float(Lib::kWidth + _extra.x + 4),
      float(Lib::kHeight + _extra.y + 4), sf::Color(128, 128, 128, 255)));
  _internals->window.display();
#endif
}

void Lib::rumble(std::int32_t player, std::int32_t time) {
  /*if (player < 0 || player >= kPlayers) {
    return;
  }
  _rumble[player] = std::max(_rumble[player], time);
  if (_rumble[player]) {
    WPAD_Rumble(player, true);
    PAD_ControlMotor(player, PAD_MOTOR_RUMBLE);
  }*/
}

void Lib::stop_rumble() {
  /*for (std::int32_t i = 0; i < kPlayers; i++) {
    _rumble[i] = 0;
    WPAD_Rumble(i, false);
    PAD_ControlMotor(i, PAD_MOTOR_STOP);
  }*/
}

bool Lib::play_sound(sound s, float volume, float pan, float repitch) {
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
  for (std::size_t i = 0; i < _internals->sounds.size(); i++) {
    if (s == _internals->sounds[i].second.first) {
      if (_internals->sounds[i].first <= 0) {
        buffer = _internals->sounds[i].second.second.get();
        _internals->sounds[i].first = kSoundTimer;
      }
      break;
    }
  }

  if (!buffer) {
    return false;
  }
  for (std::int32_t i = 0; i < static_cast<std::int32_t>(sound::kMax); ++i) {
    if (_internals->voices[i].getStatus() != sf::Sound::Playing) {
      _internals->voices[i].setAttenuation(0.f);
      _internals->voices[i].setLoop(false);
      _internals->voices[i].setMinDistance(100.f);
      _internals->voices[i].setBuffer(*buffer);
      _internals->voices[i].setVolume(100.f * volume);
      _internals->voices[i].setPitch(pow(2.f, repitch));
      _internals->voices[i].setPosition(pan, 0, -1);
      _internals->voices[i].play();
      return true;
    }
  }
#endif
  return false;
}

void Lib::set_volume(std::int32_t volume) {
#ifndef PLATFORM_SCORE
  sf::Listener::setGlobalVolume(float(std::max(0, std::min(100, volume))));
#endif
}

void Lib::load_sounds() {
#ifndef PLATFORM_SCORE
  auto use_sound = [&](sound s, const std::string& filename) {
    sf::SoundBuffer* buffer = new sf::SoundBuffer();
    buffer->loadFromFile(filename);
    _internals->sounds.emplace_back(0, Internals::named_sound{s, nullptr});
    _internals->sounds.back().second.second.reset(buffer);
  };
  use_sound(sound::kPlayerFire, "PlayerFire.wav");
  use_sound(sound::kMenuClick, "MenuClick.wav");
  use_sound(sound::kMenuAccept, "MenuAccept.wav");
  use_sound(sound::kPowerupLife, "PowerupLife.wav");
  use_sound(sound::kPowerupOther, "PowerupOther.wav");
  use_sound(sound::kEnemyHit, "EnemyHit.wav");
  use_sound(sound::kEnemyDestroy, "EnemyDestroy.wav");
  use_sound(sound::kEnemyShatter, "EnemyShatter.wav");
  use_sound(sound::kEnemySpawn, "EnemySpawn.wav");
  use_sound(sound::kBossAttack, "BossAttack.wav");
  use_sound(sound::kBossFire, "BossFire.wav");
  use_sound(sound::kPlayerRespawn, "PlayerRespawn.wav");
  use_sound(sound::kPlayerDestroy, "PlayerDestroy.wav");
  use_sound(sound::kPlayerShield, "PlayerShield.wav");
  use_sound(sound::kExplosion, "Explosion.wav");
#endif
}
