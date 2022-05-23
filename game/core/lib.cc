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
    lib_ = lib;
  }

  bool buttonPressed(const OIS::JoyStickEvent& arg, std::int32_t button) override;
  bool buttonReleased(const OIS::JoyStickEvent& arg, std::int32_t button) override;
  bool axisMoved(const OIS::JoyStickEvent& arg, std::int32_t axis) override;
  bool povMoved(const OIS::JoyStickEvent& arg, std::int32_t pov) override;

private:
  Lib* lib_;
};

struct Internals {
  mutable sf::RenderWindow window;
  sf::Texture texture;
  mutable sf::Sprite font;

  OIS::InputManager* manager = nullptr;
  Handler pad_handler;
  std::int32_t pad_count = 0;
  PadConfig pad_config[kPlayers] = {{}};
  OIS::JoyStick* pads[kPlayers] = {nullptr};
  OIS::ForceFeedback* ff[kPlayers] = {nullptr};
  fixed pad_move_vaxes[kPlayers] = {0};
  fixed pad_move_haxes[kPlayers] = {0};
  fixed pad_aim_vaxes[kPlayers] = {0};
  fixed pad_aim_haxes[kPlayers] = {0};
  std::int32_t pad_move_dpads[kPlayers] = {0};
  std::int32_t pad_aim_dpads[kPlayers] = {0};
  mutable vec2 pad_last_aim[kPlayers] = {{0, 0}};

  typedef std::pair<Lib::sound, std::unique_ptr<sf::SoundBuffer>> named_sound;
  typedef std::pair<std::int32_t, named_sound> sound_resource;
  std::vector<sound_resource> sounds;
  std::vector<sf::Sound> voices;
};

bool Handler::buttonPressed(const OIS::JoyStickEvent& arg, std::int32_t button) {
  std::int32_t p = -1;
  for (std::int32_t i = 0; i < kPlayers && i < lib_->internals_->pad_count; ++i) {
    if (lib_->internals_->pads[i] == arg.device) {
      p = i;
    }
  }
  if (p < 0) {
    return true;
  }

  for (std::int32_t i = 0; i < static_cast<std::int32_t>(Lib::key::kMax); ++i) {
    if (PadToKey(lib_->internals_->pad_config[p], button, static_cast<Lib::key>(i))) {
      lib_->keys_pressed_[i][p] = true;
      lib_->keys_held_[i][p] = true;
    }
  }
  return true;
}

bool Handler::buttonReleased(const OIS::JoyStickEvent& arg, std::int32_t button) {
  std::int32_t p = -1;
  for (std::int32_t i = 0; i < kPlayers && i < lib_->internals_->pad_count; ++i) {
    if (lib_->internals_->pads[i] == arg.device) {
      p = i;
    }
  }
  if (p < 0) {
    return true;
  }

  for (std::int32_t i = 0; i < static_cast<std::int32_t>(Lib::key::kMax); ++i) {
    if (PadToKey(lib_->internals_->pad_config[p], button, static_cast<Lib::key>(i))) {
      lib_->keys_released_[i][p] = true;
      lib_->keys_held_[i][p] = false;
    }
  }
  return true;
}

bool Handler::axisMoved(const OIS::JoyStickEvent& arg, std::int32_t axis) {
  std::int32_t p = -1;
  for (std::int32_t i = 0; i < kPlayers && i < lib_->internals_->pad_count; ++i) {
    if (lib_->internals_->pads[i] == arg.device) {
      p = i;
    }
  }
  if (p < 0) {
    return true;
  }

  fixed v = std::max(
      -fixed{1}, std::min(fixed{1}, fixed{arg.state.mAxes[axis].abs} / OIS::JoyStick::MAX_AXIS));
  PadConfig& config = lib_->internals_->pad_config[p];

  for (std::size_t i = 0; i < config._moveSticks.size(); ++i) {
    if (config._moveSticks[i].axis1_ == axis) {
      lib_->internals_->pad_move_vaxes[p] = config._moveSticks[i].axis1r_ ? -v : v;
    }
    if (config._moveSticks[i].axis2_ == axis) {
      lib_->internals_->pad_move_haxes[p] = config._moveSticks[i].axis2r_ ? -v : v;
    }
  }
  for (std::size_t i = 0; i < config._aimSticks.size(); ++i) {
    if (config._aimSticks[i].axis1_ == axis) {
      lib_->internals_->pad_aim_vaxes[p] = config._aimSticks[i].axis1r_ ? -v : v;
    }
    if (config._aimSticks[i].axis2_ == axis) {
      lib_->internals_->pad_aim_haxes[p] = config._aimSticks[i].axis2r_ ? -v : v;
    }
  }
  return true;
}

bool Handler::povMoved(const OIS::JoyStickEvent& arg, std::int32_t pov) {
  std::int32_t p = -1;
  for (std::int32_t i = 0; i < kPlayers && i < lib_->internals_->pad_count; ++i) {
    if (lib_->internals_->pads[i] == arg.device) {
      p = i;
    }
  }
  if (p < 0) {
    return true;
  }

  auto k = [](Lib::key key) { return static_cast<std::size_t>(key); };

  PadConfig& config = lib_->internals_->pad_config[p];
  std::int32_t d = arg.state.mPOV[pov].direction;
  for (std::size_t i = 0; i < config._moveDpads.size(); ++i) {
    if (config._moveDpads[i] == pov) {
      lib_->internals_->pad_move_dpads[p] = d;
      if (d & OIS::Pov::North) {
        lib_->keys_pressed_[k(Lib::key::kUp)][p] = true;
        lib_->keys_held_[k(Lib::key::kUp)][p] = true;
      } else {
        if (lib_->keys_held_[k(Lib::key::kUp)][p]) {
          lib_->keys_released_[k(Lib::key::kUp)][p] = true;
        }
        lib_->keys_held_[k(Lib::key::kUp)][p] = false;
      }
      if (d & OIS::Pov::South) {
        lib_->keys_pressed_[k(Lib::key::kDown)][p] = true;
        lib_->keys_held_[k(Lib::key::kDown)][p] = true;
      } else {
        if (lib_->keys_held_[k(Lib::key::kDown)][p]) {
          lib_->keys_released_[k(Lib::key::kDown)][p] = true;
        }
        lib_->keys_held_[k(Lib::key::kDown)][p] = false;
      }
      if (d & OIS::Pov::West) {
        lib_->keys_pressed_[k(Lib::key::kLeft)][p] = true;
        lib_->keys_held_[k(Lib::key::kLeft)][p] = true;
      } else {
        if (lib_->keys_held_[k(Lib::key::kLeft)][p]) {
          lib_->keys_released_[k(Lib::key::kLeft)][p] = true;
        }
        lib_->keys_held_[k(Lib::key::kLeft)][p] = false;
      }
      if (d & OIS::Pov::East) {
        lib_->keys_pressed_[k(Lib::key::kRight)][p] = true;
        lib_->keys_held_[k(Lib::key::kRight)][p] = true;
      } else {
        if (lib_->keys_held_[k(Lib::key::kRight)][p]) {
          lib_->keys_released_[k(Lib::key::kRight)][p] = true;
        }
        lib_->keys_held_[k(Lib::key::kRight)][p] = false;
      }
    }
  }
  for (std::size_t i = 0; i < config._aimDpads.size(); ++i) {
    if (config._aimDpads[i] == pov) {
      lib_->internals_->pad_aim_dpads[p] = d;
    }
  }
  return true;
}
#else
struct Internals {};
#endif

Lib::Lib() {
#ifndef PLATFORM_SCORE
  set_working_directory(false);
#ifdef PLATFORM_LINUX
  mkdir("replays", 0777);
#else
  CreateDirectory("replays", 0);
#endif
  internals_ = std::make_unique<Internals>();

  bool created = false;
  for (int i = sf::VideoMode::getFullscreenModes().size() - 1; i >= 0 && !Settings().windowed;
       --i) {
    sf::VideoMode m = sf::VideoMode::getFullscreenModes()[i];
    if (m.width >= unsigned(Lib::kWidth) && m.height >= unsigned(Lib::kHeight) &&
        m.bitsPerPixel == 32) {
      internals_->window.create(m, "WiiSPACE", sf::Style::Close | sf::Style::Titlebar,
                                sf::ContextSettings{0, 0, 0});
      extra_.x = (m.width - Lib::kWidth) / 2;
      extra_.y = (m.height - Lib::kHeight) / 2;
      created = true;
      break;
    }
  }
  if (!created) {
    internals_->window.create(sf::VideoMode{640, 480, 32}, "WiiSPACE",
                              sf::Style::Close | sf::Style::Titlebar);
  }

  internals_->manager = OIS::InputManager::createInputSystem(
      reinterpret_cast<std::size_t>(internals_->window.getSystemHandle()));
  internals_->pad_count = std::min(4, internals_->manager->getNumberOfDevices(OIS::OISJoyStick));
  OIS::JoyStick* tpads[4] = {nullptr};
  for (std::int32_t i = 0; i < internals_->pad_count; ++i) {
    try {
      tpads[i] = (OIS::JoyStick*)internals_->manager->createInputObject(OIS::OISJoyStick, true);
      tpads[i]->setEventCallback(&internals_->pad_handler);
    } catch (const std::exception& e) {
      tpads[i] = 0;
      (void)e;
    }
  }

  internals_->pad_handler.SetLib(this);
  std::int32_t configs = 0;
  std::ifstream f{"gamepads.txt"};
  std::string line;
  getline(f, line);
  std::stringstream ss{line};
  ss >> configs;
  for (std::int32_t i = 0; i < kPlayers; ++i) {
    if (i >= configs) {
      internals_->pad_config[i].SetDefault();
      for (std::int32_t p = 0; p < kPlayers; ++p) {
        if (!tpads[p]) {
          continue;
        }
        internals_->pads[i] = tpads[p];
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
        internals_->pads[i] = tpads[p];
        tpads[p] = 0;
        internals_->pad_config[i].Read(f);
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
    if (!internals_->pads[i]) {
      continue;
    } else {
      internals_->ff[i] = 0;
    }
    internals_->ff[i] =
        (OIS::ForceFeedback*)internals_->pads[i]->queryInterface(OIS::Interface::ForceFeedback);

    /*if (ff_[i]) {
      OIS::Effect* e =
          new OIS::Effect(OIS::Effect::ConstantForce, OIS::Effect::Constant);
      e->setNumAxes(1);
      ((OIS::ConstantEffect*) e->getForceEffect())->level = 5000;
      e->direction = OIS::Effect::North;
      e->trigger_button = 1;
      e->trigger_interval = 1000;
      e->replay_length = 5000;
      e->replay_delay = 0;
      ff_[i]->upload(e);
    }*/
  }

  sf::Image image;
  image.loadFromFile("console.png");
  image.createMaskFromColor(RgbaToColor(0x000000ff));
  internals_->texture.loadFromImage(image);
  internals_->texture.setSmooth(false);
  internals_->font = sf::Sprite(internals_->texture);
  load_sounds();

  clear_screen();
  internals_->window.setVisible(true);
  internals_->window.setVerticalSyncEnabled(true);
  internals_->window.setFramerateLimit(50);
  internals_->window.setMouseCursorVisible(false);

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
    keys_pressed_.push_back(f);
    keys_held_.push_back(f);
    keys_released_.push_back(f);
  }
  for (std::int32_t j = 0; j < kPlayers; ++j) {
    internals_->pad_move_vaxes[j] = 0;
    internals_->pad_move_haxes[j] = 0;
    internals_->pad_aim_vaxes[j] = 0;
    internals_->pad_aim_haxes[j] = 0;
    internals_->pad_move_dpads[j] = OIS::Pov::Centered;
    internals_->pad_aim_dpads[j] = OIS::Pov::Centered;
  }
  for (std::int32_t i = 0; i < static_cast<std::int32_t>(sound::kMax); ++i) {
    internals_->voices.emplace_back();
  }
#endif
}

Lib::~Lib() {
#ifndef PLATFORM_SCORE
  OIS::InputManager::destroyInputSystem(internals_->manager);
#endif
}

bool Lib::is_key_pressed(key k) const {
  for (std::int32_t i = 0; i < kPlayers; ++i)
    if (is_key_pressed(i, k)) {
      return true;
    }
  return false;
}

bool Lib::is_key_released(key k) const {
  for (std::int32_t i = 0; i < kPlayers; ++i)
    if (is_key_released(i, k)) {
      return true;
    }
  return false;
}

bool Lib::is_key_held(key k) const {
  for (std::int32_t i = 0; i < kPlayers; ++i)
    if (is_key_held(i, k)) {
      return true;
    }
  return false;
}

void Lib::set_colour_cycle(std::int32_t cycle) {
  cycle_ = cycle % 256;
}

std::int32_t Lib::get_colour_cycle() const {
  return cycle_;
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
  ivec2 t = mouse_;
  sf::Event e;
  std::int32_t kp = players_ <= internals_->pad_count ? players_ - 1 : internals_->pad_count;
  while (internals_->window.pollEvent(e)) {
    if (e.type == sf::Event::Closed || !internals_->window.isOpen()) {
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
        keys_pressed_[i][kp] = true;
        keys_held_[i][kp] = true;
      }
      if (ku || mu) {
        keys_released_[i][kp] = true;
        keys_held_[i][kp] = false;
      }
    }

    if (e.type == sf::Event::MouseMoved) {
      mouse_.x = e.mouseMove.x;
      mouse_.y = e.mouseMove.y;
    }
  }

  for (std::size_t i = 0; i < internals_->sounds.size(); ++i) {
    if (internals_->sounds[i].first > 0) {
      internals_->sounds[i].first--;
    }
  }

  mouse_ = std::max(ivec2{}, std::min(ivec2{Lib::kWidth - 1, Lib::kHeight - 1}, mouse_));
  if (t != mouse_) {
    mouse_moving_ = true;
  }
  if (capture_mouse_) {
    sf::Mouse::setPosition({mouse_.x, mouse_.y}, internals_->window);
  }

  for (std::int32_t i = 0; i < internals_->pad_count; ++i) {
    internals_->pads[i]->capture();
    const OIS::JoyStickState& s = internals_->pads[i]->getJoyStickState();
    for (std::size_t j = 0; j < s.mAxes.size(); ++j) {
      OIS::JoyStickEvent arg(internals_->pads[i], s);
      internals_->pad_handler.axisMoved(arg, j);
    }
  }
#else
  if (score_frame_ < 5) {
    ++score_frame_;
  }
#endif
  return false;
}

void Lib::end_frame() {
#ifndef PLATFORM_SCORE
  for (std::int32_t i = 0; i < static_cast<std::int32_t>(key::kMax); ++i) {
    for (std::int32_t j = 0; j < kPlayers; ++j) {
      keys_pressed_[i][j] = false;
      keys_released_[i][j] = false;
    }
  }
#endif
  /*for (std::int32_t i = 0; i < kPlayers; ++i) {
    if (rumble_[i]) {
      rumble_[i]--;
      if (!rumble_[i]) {
      WPAD_Rumble(i, false);
      PAD_ControlMotor(i, PAD_MOTOR_STOP);
    }
  }*/
}

void Lib::capture_mouse(bool enabled) {
  capture_mouse_ = enabled;
}

void Lib::new_game() {
#ifndef PLATFORM_SCORE
  for (std::int32_t i = 0; i < static_cast<std::int32_t>(key::kMax); ++i) {
    for (std::int32_t j = 0; j < kPlayers; ++j) {
      keys_pressed_[i][j] = false;
      keys_released_[i][j] = false;
      keys_held_[i][j] = false;
    }
  }
#endif
}

// Input
//------------------------------
Lib::pad_type Lib::get_pad_type(std::int32_t player) const {
#ifndef PLATFORM_SCORE
  pad_type result = pad_type::kPadNone;
  if (player < internals_->pad_count) {
    result = pad_type::kPadGamepad;
  }
  if ((players_ <= internals_->pad_count && player == players_ - 1) ||
      (players_ > internals_->pad_count && player == internals_->pad_count)) {
    result = static_cast<pad_type>(result | pad_type::kPadKeyboardMouse);
  }
  return result;
#else
  return pad_type::kPadNone;
#endif
}

bool Lib::is_key_pressed(std::int32_t player, key k) const {
#ifndef PLATFORM_SCORE
  return keys_pressed_[static_cast<std::size_t>(k)][player];
#else
  if (player != 0) {
    return false;
  }
  if (k == key::kDown && score_frame_ < 4) {
    return true;
  }
  if (k == key::kAccept && score_frame_ == 5) {
    return true;
  }
  return false;
#endif
}

bool Lib::is_key_released(std::int32_t player, key k) const {
#ifndef PLATFORM_SCORE
  return keys_released_[static_cast<std::size_t>(k)][player];
#else
  return false;
#endif
}

bool Lib::is_key_held(std::int32_t player, key k) const {
#ifndef PLATFORM_SCORE
  vec2 v(internals_->pad_aim_haxes[player], internals_->pad_aim_vaxes[player]);
  if (k == key::kFire &&
      (internals_->pad_aim_dpads[player] != OIS::Pov::Centered ||
       v.length() >= fixed_c::tenth * 2)) {
    return true;
  }
  return keys_held_[static_cast<std::size_t>(k)][player];
#else
  return false;
#endif
}

vec2 Lib::get_move_velocity(std::int32_t player) const {
#ifndef PLATFORM_SCORE
  bool ku = is_key_held(player, key::kUp) || internals_->pad_move_dpads[player] & OIS::Pov::North;
  bool kd = is_key_held(player, key::kDown) || internals_->pad_move_dpads[player] & OIS::Pov::South;
  bool kl = is_key_held(player, key::kLeft) || internals_->pad_move_dpads[player] & OIS::Pov::West;
  bool kr = is_key_held(player, key::kRight) || internals_->pad_move_dpads[player] & OIS::Pov::East;

  vec2 v = vec2{0, -1} * ku + vec2{0, 1} * kd + vec2{-1, 0} * kl + vec2{1, 0} * kr;
  if (v != vec2{}) {
    return v;
  }

  v = vec2{internals_->pad_move_haxes[player], internals_->pad_move_vaxes[player]};
  if (v.length() < fixed_c::tenth * 2) {
    return {};
  }
  return v;
#else
  return {};
#endif
}

vec2 Lib::get_fire_target(std::int32_t player, const vec2& position) const {
#ifndef PLATFORM_SCORE
  bool kp = player == (players_ <= internals_->pad_count ? players_ - 1 : internals_->pad_count);
  vec2 v(internals_->pad_aim_haxes[player], internals_->pad_aim_vaxes[player]);

  if (v.length() >= fixed_c::tenth * 2) {
    v = v.normalised() * 48;
    if (kp) {
      mouse_moving_ = false;
    }
    internals_->pad_last_aim[player] = v;
    return v + position;
  }

  if (internals_->pad_aim_dpads[player] != OIS::Pov::Centered) {
    bool ku = (internals_->pad_aim_dpads[player] & OIS::Pov::North) != 0;
    bool kd = (internals_->pad_aim_dpads[player] & OIS::Pov::South) != 0;
    bool kl = (internals_->pad_aim_dpads[player] & OIS::Pov::West) != 0;
    bool kr = (internals_->pad_aim_dpads[player] & OIS::Pov::East) != 0;

    vec2 v = vec2{0, -1} * ku + vec2{0, 1} * kd + vec2{-1, 0} * kl + vec2{1, 0} * kr;
    if (v != vec2{}) {
      v = v.normalised() * 48;
      if (kp) {
        mouse_moving_ = false;
      }
      internals_->pad_last_aim[player] = v;
      return v + position;
    }
  }

  if (mouse_moving_ && kp) {
    return vec2{mouse_.x, mouse_.y};
  }
  if (internals_->pad_last_aim[player] != vec2{}) {
    return position + internals_->pad_last_aim[player];
  }
  return position + vec2{48, 0};
#else
  return {};
#endif
}

// Output
//------------------------------
void Lib::clear_screen() const {
#ifndef PLATFORM_SCORE
  glClear(GL_COLOR_BUFFER_BIT);
  internals_->window.clear(RgbaToColor(0x000000ff));
  internals_->window.draw(MakeRectangle(0, 0, 0, 0, {}));
#endif
}

void Lib::render_line(const fvec2& a, const fvec2& b, colour_t c) const {
#ifndef PLATFORM_SCORE
  c = z::colour_cycle(c, cycle_);
  glBegin(GL_LINES);
  glColor4ub(c >> 24, c >> 16, c >> 8, c);
  glVertex3f(a.x + extra_.x, a.y + extra_.y, 0);
  glVertex3f(b.x + extra_.x, b.y + extra_.y, 0);
  glColor4ub(c >> 24, c >> 16, c >> 8, c & 0xffffff33);
  glVertex3f(a.x + extra_.x + 1, a.y + extra_.y, 0);
  glVertex3f(b.x + extra_.x + 1, b.y + extra_.y, 0);
  glVertex3f(a.x + extra_.x - 1, a.y + extra_.y, 0);
  glVertex3f(b.x + extra_.x - 1, b.y + extra_.y, 0);
  glVertex3f(a.x + extra_.x, a.y + extra_.y + 1, 0);
  glVertex3f(b.x + extra_.x, b.y + extra_.y + 1, 0);
  glVertex3f(a.x + extra_.x, a.y + extra_.y - 1, 0);
  glVertex3f(b.x + extra_.x, b.y + extra_.y - 1, 0);
  glEnd();
#endif
}

void Lib::render_text(const fvec2& v, const std::string& text, colour_t c) const {
#ifndef PLATFORM_SCORE
  internals_->font.setColor(RgbaToColor(z::colour_cycle(c, cycle_)));
  for (std::size_t i = 0; i < text.length(); ++i) {
    internals_->font.setPosition((static_cast<std::int32_t>(i) + v.x) * kTextWidth + extra_.x,
                                 v.y * kTextHeight + extra_.y);
    internals_->font.setTextureRect(sf::IntRect{kTextWidth * text[i], 0, kTextWidth, kTextHeight});
    internals_->window.draw(internals_->font);
  }
  internals_->window.draw(MakeRectangle(0, 0, 0, 0, {}));
#endif
}

void Lib::render_rect(const fvec2& low, const fvec2& hi, colour_t c,
                      std::int32_t line_width) const {
#ifndef PLATFORM_SCORE
  c = z::colour_cycle(c, cycle_);
  fvec2 ab{low.x, hi.y};
  fvec2 ba{hi.x, low.y};
  const fvec2* list[4] = {&low, &ab, &hi, &ba};
  fvec2 normals[4] = {{}};

  auto centre = (low + hi) / 2;
  for (std::size_t i = 0; i < 4; ++i) {
    const auto& v0 = *list[(i + 3) % 4];
    const auto& v1 = *list[i];
    const auto& v2 = *list[(i + 1) % 4];

    auto n1 = fvec2{v0.y - v1.y, v1.x - v0.x}.normalised();
    auto n2 = fvec2{v1.y - v2.y, v2.x - v1.x}.normalised();

    auto f = 1.f + n1.x * n2.x + n1.y * n2.y;
    normals[i] = (n1 + n2) / f;
    auto dot = (v1.x - centre.x) * normals[i].x + (v1.y - centre.y) * normals[i].y;

    if (dot < 0) {
      normals[i] = fvec2{} - normals[i];
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
    render_rect(low + fvec2{1.f, 1.f}, hi - fvec2{1.f, 1.f}, z::colour_cycle(c, cycle_),
                line_width - 1);
  }
#endif
}

void Lib::render() const {
#ifndef PLATFORM_SCORE
  internals_->window.draw(
      MakeRectangle(0.f, 0.f, extra_.x, Lib::kHeight + extra_.y * 2, sf::Color(0, 0, 0, 255)));
  internals_->window.draw(
      MakeRectangle(0.f, 0.f, Lib::kWidth + extra_.x * 2, extra_.y, sf::Color(0, 0, 0, 255)));
  internals_->window.draw(MakeRectangle(Lib::kWidth + extra_.x, 0, Lib::kWidth + extra_.x * 2,
                                        Lib::kHeight + extra_.y * 2, sf::Color(0, 0, 0, 255)));
  internals_->window.draw(MakeRectangle(0.f, Lib::kHeight + extra_.y, Lib::kWidth + extra_.x * 2,
                                        Lib::kHeight + extra_.y * 2, sf::Color(0, 0, 0, 255)));
  internals_->window.draw(MakeRectangle(0.f, 0.f, extra_.x - 2, Lib::kHeight + extra_.y * 2,
                                        sf::Color(32, 32, 32, 255)));
  internals_->window.draw(MakeRectangle(0.f, 0.f, Lib::kWidth + extra_.x * 2, extra_.y - 2,
                                        sf::Color(32, 32, 32, 255)));
  internals_->window.draw(MakeRectangle(Lib::kWidth + extra_.x + 2, extra_.y - 4,
                                        Lib::kWidth + extra_.x * 2, Lib::kHeight + extra_.y * 2,
                                        sf::Color(32, 32, 32, 255)));
  internals_->window.draw(MakeRectangle(extra_.x - 2, Lib::kHeight + extra_.y + 2,
                                        Lib::kWidth + extra_.x * 2, Lib::kHeight + extra_.y * 2,
                                        sf::Color(32, 32, 32, 255)));
  internals_->window.draw(MakeRectangle(extra_.x - 4, extra_.y - 4, extra_.x - 2,
                                        Lib::kHeight + extra_.y + 4,
                                        sf::Color(128, 128, 128, 255)));
  internals_->window.draw(MakeRectangle(extra_.x - 4, extra_.y - 4, Lib::kWidth + extra_.x + 4,
                                        extra_.y - 2, sf::Color(128, 128, 128, 255)));
  internals_->window.draw(MakeRectangle(Lib::kWidth + extra_.x + 2, extra_.y - 4,
                                        Lib::kWidth + extra_.x + 4, Lib::kHeight + extra_.y + 4,
                                        sf::Color(128, 128, 128, 255)));
  internals_->window.draw(MakeRectangle(extra_.x - 2, Lib::kHeight + extra_.y + 2,
                                        Lib::kWidth + extra_.x + 4, Lib::kHeight + extra_.y + 4,
                                        sf::Color(128, 128, 128, 255)));
  internals_->window.display();
#endif
}

void Lib::rumble(std::int32_t player, std::int32_t time) {
  /*if (player < 0 || player >= kPlayers) {
    return;
  }
  rumble_[player] = std::max(rumble_[player], time);
  if (rumble_[player]) {
    WPAD_Rumble(player, true);
    PAD_ControlMotor(player, PAD_MOTOR_RUMBLE);
  }*/
}

void Lib::stop_rumble() {
  /*for (std::int32_t i = 0; i < kPlayers; ++i) {
    rumble_[i] = 0;
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
  for (std::size_t i = 0; i < internals_->sounds.size(); ++i) {
    if (s == internals_->sounds[i].second.first) {
      if (internals_->sounds[i].first <= 0) {
        buffer = internals_->sounds[i].second.second.get();
        internals_->sounds[i].first = kSoundTimer;
      }
      break;
    }
  }

  if (!buffer) {
    return false;
  }
  for (std::int32_t i = 0; i < static_cast<std::int32_t>(sound::kMax); ++i) {
    if (internals_->voices[i].getStatus() != sf::Sound::Playing) {
      internals_->voices[i].setAttenuation(0.f);
      internals_->voices[i].setLoop(false);
      internals_->voices[i].setMinDistance(100.f);
      internals_->voices[i].setBuffer(*buffer);
      internals_->voices[i].setVolume(100.f * volume);
      internals_->voices[i].setPitch(std::pow(2.f, repitch));
      internals_->voices[i].setPosition(pan, 0, -1);
      internals_->voices[i].play();
      return true;
    }
  }
#endif
  return false;
}

void Lib::set_volume(std::int32_t volume) {
#ifndef PLATFORM_SCORE
  sf::Listener::setGlobalVolume(static_cast<float>(std::max(0, std::min(100, volume))));
#endif
}

void Lib::load_sounds() {
#ifndef PLATFORM_SCORE
  auto use_sound = [&](sound s, const std::string& filename) {
    auto buffer = std::make_unique<sf::SoundBuffer>();
    buffer->loadFromFile(filename);
    internals_->sounds.emplace_back(0, Internals::named_sound{s, nullptr});
    internals_->sounds.back().second.second = std::move(buffer);
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
