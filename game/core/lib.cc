#include "game/core/lib.h"
#include "game/core/save.h"
#include "game/io/io.h"
#include "game/mixer/mixer.h"
#include "game/render/renderer.h"
#include <nonstd/span.hpp>
#include <algorithm>
#include <iostream>
#include <limits>

// TODO: fuck this right off.
#ifdef PLATFORM_LINUX
#include <sys/stat.h>
#include <unistd.h>
#else
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

const std::string Lib::kSuperEncryptionKey = "<>";

namespace {

glm::vec4 convert_colour(colour_t c) {
  return {((c >> 24) & 0xff) / 255.f, ((c >> 16) & 0xff) / 255.f, ((c >> 8) & 0xff) / 255.f,
          (c & 0xff) / 255.f};
}

glm::vec2 convert_vec(fvec2 v) {
  return {v.x, v.y};
}

nonstd::span<const ii::io::keyboard::key> keys_for(Lib::key k) {
  using type = ii::io::keyboard::key;
  static constexpr type fire[] = {type::kZ, type::kLCtrl, type::kRCtrl};
  static constexpr type bomb[] = {type::kX, type::kSpacebar};
  static constexpr type accept[] = {type::kZ, type::kSpacebar, type::kLCtrl, type::kRCtrl};
  static constexpr type cancel[] = {type::kX, type::kEscape};
  static constexpr type menu[] = {type::kEscape, type::kReturn};
  static constexpr type up[] = {type::kW, type::kUArrow};
  static constexpr type down[] = {type::kS, type::kDArrow};
  static constexpr type left[] = {type::kA, type::kLArrow};
  static constexpr type right[] = {type::kD, type::kRArrow};

  switch (k) {
  case Lib::key::kFire:
    return fire;
  case Lib::key::kBomb:
    return bomb;
  case Lib::key::kAccept:
    return accept;
  case Lib::key::kCancel:
    return cancel;
  case Lib::key::kMenu:
    return menu;
  case Lib::key::kUp:
    return up;
  case Lib::key::kDown:
    return down;
  case Lib::key::kLeft:
    return left;
  case Lib::key::kRight:
    return right;
  default:
    return {};
  }
}

nonstd::span<const ii::io::mouse::button> mouse_buttons_for(Lib::key k) {
  using type = ii::io::mouse::button;
  static constexpr type fire[] = {type::kL};
  static constexpr type bomb[] = {type::kR};
  static constexpr type accept[] = {type::kL};
  static constexpr type cancel[] = {type::kR};

  switch (k) {
  case Lib::key::kFire:
    return fire;
  case Lib::key::kBomb:
    return bomb;
  case Lib::key::kAccept:
    return accept;
  case Lib::key::kCancel:
    return cancel;
  default:
    return {};
  }
}

nonstd::span<const ii::io::controller::button> controller_buttons_for(Lib::key k) {
  using type = ii::io::controller::button;
  static constexpr type fire[] = {type::kA, type::kRShoulder};
  static constexpr type bomb[] = {type::kB, type::kLShoulder};
  static constexpr type accept[] = {type::kA, type::kRShoulder};
  static constexpr type cancel[] = {type::kB, type::kLShoulder};
  static constexpr type menu[] = {type::kStart, type::kGuide};
  static constexpr type up[] = {type::kDpadUp};
  static constexpr type down[] = {type::kDpadDown};
  static constexpr type left[] = {type::kDpadLeft};
  static constexpr type right[] = {type::kDpadRight};

  switch (k) {
  case Lib::key::kFire:
    return fire;
  case Lib::key::kBomb:
    return bomb;
  case Lib::key::kAccept:
    return accept;
  case Lib::key::kCancel:
    return cancel;
  case Lib::key::kMenu:
    return menu;
  case Lib::key::kUp:
    return up;
  case Lib::key::kDown:
    return down;
  case Lib::key::kLeft:
    return left;
  case Lib::key::kRight:
    return right;
  default:
    return {};
  }
}

vec2 controller_stick(std::int16_t x, std::int16_t y) {
  vec2 v{fixed::from_internal(static_cast<std::int64_t>(x) << 17),
         fixed::from_internal(static_cast<std::int64_t>(y) << 17)};
  if (v.length() < fixed_c::tenth * 2) {
    return vec2{};
  }
  if (v.length() > 1 - fixed_c::tenth * 2) {
    return v.normalised();
  }
  return v;
}

template <typename T>
bool contains(nonstd::span<const T> range, T value) {
  return std::find(range.begin(), range.end(), value) != range.end();
}

}  // namespace

struct Lib::Internals {
  Internals(bool headless) : mixer{ii::io::kAudioSampleRate, /* enabled */ !headless} {
    last_aim.resize(kPlayers);
  }

  ii::io::keyboard::frame keyboard_frame;
  ii::io::mouse::frame mouse_frame;
  std::vector<ii::io::controller::info> controller_info;
  std::vector<ii::io::controller::frame> controller_frames;
  std::vector<vec2> last_aim;
  bool mouse_moving = false;

  struct player_input {
    bool kbm = false;
    ii::io::controller::frame* pad = nullptr;
  };

  ii::Mixer mixer;
  typedef std::pair<std::int32_t, Lib::sound> sound_resource;
  std::vector<sound_resource> sounds;

  player_input assign_input(std::uint32_t player_number, std::uint32_t player_count) {
    player_input input;
    if (player_number < controller_frames.size()) {
      input.pad = &controller_frames[player_number];
    }
    input.kbm = (player_count <= controller_frames.size() && 1 + player_number == player_count) ||
        (player_count > controller_frames.size() && player_number == controller_frames.size());
    return input;
  }

  bool is_pressed(player_input input, key k) {
    if (input.kbm) {
      for (const auto& e : keyboard_frame.key_events) {
        if (e.down && contains(keys_for(k), e.key)) {
          return true;
        }
      }
      for (const auto& e : mouse_frame.button_events) {
        if (e.down && contains(mouse_buttons_for(k), e.button)) {
          return true;
        }
      }
    }
    if (input.pad) {
      for (const auto& e : input.pad->button_events) {
        if (e.down && contains(controller_buttons_for(k), e.button)) {
          return true;
        }
      }
    }
    return false;
  }

  bool is_held(player_input input, key k) {
    if (input.kbm) {
      for (auto x : keys_for(k)) {
        if (keyboard_frame.key(x)) {
          return true;
        }
      }
      for (auto x : mouse_buttons_for(k)) {
        if (mouse_frame.button(x)) {
          return true;
        }
      }
    }
    if (input.pad) {
      for (auto x : controller_buttons_for(k)) {
        if (input.pad->button(x)) {
          return true;
        }
      }
    }
    return false;
  }
};

Lib::Lib(bool headless, ii::io::IoLayer& io_layer, ii::Renderer& renderer)
: headless_{headless}, io_layer_{io_layer}, renderer_{renderer} {
#ifndef PLATFORM_SCORE
  set_working_directory(false);
#ifdef PLATFORM_LINUX
  mkdir("replays", 0777);
#else
  CreateDirectory("replays", 0);
#endif
#endif
  internals_ = std::make_unique<Internals>(headless);
  io_layer_.set_audio_callback([mixer = &internals_->mixer](std::uint8_t* p, std::size_t k) {
    mixer->audio_callback(p, k);
  });

  auto use_sound = [&](sound s, const std::string& filename) {
    internals_->sounds.emplace_back(0, s);
    auto result =
        internals_->mixer.load_wav_file(filename, static_cast<ii::Mixer::audio_handle_t>(s));
    if (!result) {
      std::cerr << "Couldn't load sound " + filename + ": " << result.error() << std::endl;
    }
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

  clear_screen();
}

Lib::~Lib() {}

bool Lib::is_key_pressed(key k) const {
  for (std::int32_t i = 0; i < kPlayers; ++i)
    if (is_key_pressed(i, k)) {
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
  colour_cycle_ = cycle % 256;
}

std::int32_t Lib::get_colour_cycle() const {
  return colour_cycle_;
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
  bool audio_change = false;
  bool controller_change = false;
  while (true) {
    auto event = io_layer_.poll();
    if (!event) {
      break;
    }
    if (*event == ii::io::event_type::kClose) {
      return true;
    } else if (*event == ii::io::event_type::kAudioDeviceChange) {
      audio_change = true;
    } else if (*event == ii::io::event_type::kControllerChange) {
      controller_change = true;
    }
  }

  if (audio_change) {
    auto result = io_layer_.open_audio_device(std::nullopt);
    if (!result) {
      std::cerr << "Error opening audio device: " << result.error() << std::endl;
    }
  }

  if (controller_change) {
    internals_->controller_info = io_layer_.controller_info();
    internals_->controller_frames.clear();
    internals_->controller_frames.resize(internals_->controller_info.size());
  }

  internals_->keyboard_frame = io_layer_.keyboard_frame();
  internals_->mouse_frame = io_layer_.mouse_frame();
  for (std::size_t i = 0; i < internals_->controller_info.size(); ++i) {
    internals_->controller_frames[i] = io_layer_.controller_frame(i);
  }

  for (auto& s : internals_->sounds) {
    if (s.first > 0) {
      --s.first;
    }
  }

  renderer_.set_dimensions(io_layer_.dimensions(), glm::uvec2{kWidth, kHeight});
  if (headless_ && score_frame_ < 5) {
    ++score_frame_;
  }
  return false;
}

void Lib::end_frame() {
  io_layer_.input_frame_clear();
}

void Lib::capture_mouse(bool enabled) {
  io_layer_.capture_mouse(enabled);
}

void Lib::new_game() {
  io_layer_.input_frame_clear();
}

Lib::pad_type Lib::get_pad_type(std::int32_t player) const {
  pad_type result = pad_type::kPadNone;
  auto input = internals_->assign_input(player, players_);
  if (input.pad) {
    result = pad_type::kPadGamepad;
  }
  if (input.kbm) {
    result = static_cast<pad_type>(result | pad_type::kPadKeyboardMouse);
  }
  return result;
}

bool Lib::is_key_pressed(std::int32_t player, key k) const {
  if (!headless_) {
    return internals_->is_pressed(internals_->assign_input(player, players_), k);
  }
  if (player != 0) {
    return false;
  }
  // TODO: just don't do whatever menus this is for in headless mode.
  if (k == key::kDown && score_frame_ < 4) {
    return true;
  }
  if (k == key::kAccept && score_frame_ == 5) {
    return true;
  }
  return false;
}

bool Lib::is_key_held(std::int32_t player, key k) const {
  if (!headless_) {
    return internals_->is_held(internals_->assign_input(player, players_), k);
  }
  return false;
}

vec2 Lib::get_move_velocity(std::int32_t player) const {
  if (!headless_) {
    bool ku = is_key_held(player, key::kUp);
    bool kd = is_key_held(player, key::kDown);
    bool kl = is_key_held(player, key::kLeft);
    bool kr = is_key_held(player, key::kRight);

    vec2 v = vec2{0, -1} * ku + vec2{0, 1} * kd + vec2{-1, 0} * kl + vec2{1, 0} * kr;
    if (v != vec2{}) {
      return v;
    }

    if (auto input = internals_->assign_input(player, players_); input.pad) {
      return controller_stick(input.pad->axis(ii::io::controller::axis::kLX),
                              input.pad->axis(ii::io::controller::axis::kLY));
    }
    return v;
  }
  return {};
}

vec2 Lib::get_fire_target(std::int32_t player, const vec2& position) const {
  if (!headless_) {
    auto input = internals_->assign_input(player, players_);
    if (input.pad) {
      auto v = controller_stick(input.pad->axis(ii::io::controller::axis::kRX),
                                input.pad->axis(ii::io::controller::axis::kRY));
      if (v != vec2{}) {
        if (input.kbm) {
          internals_->mouse_moving = false;
        }
        internals_->last_aim[player] = v.normalised() * 48;
        return position + internals_->last_aim[player];
      }
    }
    if (internals_->mouse_frame.cursor_delta != glm::ivec2{0, 0}) {
      internals_->mouse_moving = true;
    }
    if (input.kbm && internals_->mouse_moving) {
      auto c = static_cast<glm::vec2>(internals_->mouse_frame.cursor);
      auto r = renderer_.legacy_render_scale();
      c /= static_cast<glm::vec2>(io_layer_.dimensions());
      c -= (glm::vec2{1.f, 1.f} - r) / 2.f;
      c /= r;
      c *= glm::vec2{Lib::kWidth, Lib::kHeight};
      c.x = std::max<float>(0, std::min<float>(Lib::kWidth, c.x));
      c.y = std::max<float>(0, std::min<float>(Lib::kHeight, c.y));
      return vec2{c.x, c.y};
    } else if (input.pad) {
      return position + internals_->last_aim[player];
    }
  }
  return position + vec2{48, 0};
}

void Lib::clear_screen() const {
  renderer_.clear_screen();
}

void Lib::render_line(const fvec2& a, const fvec2& b, colour_t c) const {
  c = z::colour_cycle(c, colour_cycle_);
  renderer_.render_legacy_line(convert_vec(a), convert_vec(b), convert_colour(c));
}

void Lib::render_text(const fvec2& v, const std::string& text, colour_t c) const {
  renderer_.render_legacy_text(static_cast<glm::ivec2>(convert_vec(v)), convert_colour(c), text);
}

void Lib::render_rect(const fvec2& low, const fvec2& hi, colour_t c,
                      std::int32_t line_width) const {
  // TODO: this is incorrect now for ingame rect rendering.
  c = z::colour_cycle(c, colour_cycle_);
  renderer_.render_legacy_rect(static_cast<glm::ivec2>(convert_vec(low)),
                               static_cast<glm::ivec2>(convert_vec(hi)), line_width,
                               convert_colour(c));
}

void Lib::render() const {
  // TODO: goes too fast. Old was 50FPS.
  io_layer_.swap_buffers();
}

void Lib::rumble(std::int32_t player, std::int32_t time) {
  // TODO.
}

void Lib::stop_rumble() {
  // TODO.
}

bool Lib::play_sound(sound s, float volume, float pan, float repitch) {
  // TODO: can this (sound resource/timing) all be removed?
#ifdef PLATFORM_SCORE
  return false;
#else
  bool timing_ok = false;
  for (std::size_t i = 0; i < internals_->sounds.size(); ++i) {
    if (s == internals_->sounds[i].second) {
      if (internals_->sounds[i].first <= 0) {
        timing_ok = true;
        internals_->sounds[i].first = kSoundTimer;
      }
      break;
    }
  }

  if (!timing_ok) {
    return false;
  }
  internals_->mixer.play(static_cast<ii::Mixer::audio_handle_t>(s), volume, pan,
                         std::pow(2.f, repitch));
#endif
  return false;
}

void Lib::set_volume(std::int32_t volume) {
  internals_->mixer.set_master_volume(std::max(0, std::min(100, volume)) / 100.f);
}
