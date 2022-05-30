#include "game/core/lib.h"
#include "game/core/z0_game.h"
#include "game/io/file/filesystem.h"
#include "game/io/io.h"
#include "game/logic/sim/sim_state.h"
#include "game/mixer/mixer.h"
#include "game/render/gl_renderer.h"
#include <nonstd/span.hpp>
#include <algorithm>
#include <iostream>
#include <limits>
#include <sstream>
#include <unordered_map>

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
  Internals() : mixer{ii::io::kAudioSampleRate} {
    last_aim.resize(kPlayers);
  }

  ii::Mixer mixer;
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

  struct sound_aggregation {
    std::size_t count = 0;
    float volume = 0.f;
    float pan = 0.f;
    float pitch = 0.f;
  };
  std::unordered_map<ii::sound, sound_aggregation> sounds;

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

Lib::Lib(ii::io::Filesystem& fs, ii::io::IoLayer& io_layer, ii::render::GlRenderer& renderer)
: fs_{fs}, io_layer_{io_layer}, renderer_{renderer} {
  internals_ = std::make_unique<Internals>();
  io_layer_.set_audio_callback([mixer = &internals_->mixer](std::uint8_t* p, std::size_t k) {
    mixer->audio_callback(p, k);
  });

  auto use_sound = [&](ii::sound s, const std::string& filename) {
    auto bytes = fs.read_asset(filename);
    if (!bytes) {
      std::cerr << bytes.error() << std::endl;
      return;
    }
    auto result =
        internals_->mixer.load_wav_memory(*bytes, static_cast<ii::Mixer::audio_handle_t>(s));
    if (!result) {
      std::cerr << "Couldn't load sound " + filename + ": " << result.error() << std::endl;
    }
  };
  use_sound(ii::sound::kPlayerFire, "PlayerFire.wav");
  use_sound(ii::sound::kMenuClick, "MenuClick.wav");
  use_sound(ii::sound::kMenuAccept, "MenuAccept.wav");
  use_sound(ii::sound::kPowerupLife, "PowerupLife.wav");
  use_sound(ii::sound::kPowerupOther, "PowerupOther.wav");
  use_sound(ii::sound::kEnemyHit, "EnemyHit.wav");
  use_sound(ii::sound::kEnemyDestroy, "EnemyDestroy.wav");
  use_sound(ii::sound::kEnemyShatter, "EnemyShatter.wav");
  use_sound(ii::sound::kEnemySpawn, "EnemySpawn.wav");
  use_sound(ii::sound::kBossAttack, "BossAttack.wav");
  use_sound(ii::sound::kBossFire, "BossFire.wav");
  use_sound(ii::sound::kPlayerRespawn, "PlayerRespawn.wav");
  use_sound(ii::sound::kPlayerDestroy, "PlayerDestroy.wav");
  use_sound(ii::sound::kPlayerShield, "PlayerShield.wav");
  use_sound(ii::sound::kExplosion, "Explosion.wav");

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
  internals_->sounds.clear();

  renderer_.set_dimensions(io_layer_.dimensions(), glm::uvec2{kWidth, kHeight});
  return false;
}

void Lib::end_frame() {
  io_layer_.input_frame_clear();
  internals_->mixer.commit();
}

void Lib::post_update(ii::SimState& sim) {
  for (const auto& pair : sim.get_sound_output()) {
    auto& s = pair.second;
    internals_->mixer.play(static_cast<ii::Mixer::audio_handle_t>(pair.first), s.volume, s.pan,
                           s.pitch);
  }
  for (const auto& pair : sim.get_rumble_output()) {
    rumble(pair.first, pair.second);
  }
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
  return internals_->is_pressed(internals_->assign_input(player, players_), k);
}

bool Lib::is_key_held(std::int32_t player, key k) const {
  return internals_->is_held(internals_->assign_input(player, players_), k);
}

vec2 Lib::get_move_velocity(std::int32_t player) const {
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

std::pair<bool, vec2> Lib::get_fire_target(std::int32_t player) const {
  auto input = internals_->assign_input(player, players_);
  if (input.pad) {
    auto v = controller_stick(input.pad->axis(ii::io::controller::axis::kRX),
                              input.pad->axis(ii::io::controller::axis::kRY));
    if (v != vec2{}) {
      if (input.kbm) {
        internals_->mouse_moving = false;
      }
      internals_->last_aim[player] = v.normalised() * 48;
      return {true, internals_->last_aim[player]};
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
    return {false, vec2{c.x, c.y}};
  } else if (input.pad && internals_->last_aim[player] != vec2{}) {
    return {true, internals_->last_aim[player]};
  }
  return {true, vec2{48, 0}};
}

void Lib::clear_screen() const {
  renderer_.clear_screen();
}

void Lib::render_line(const fvec2& a, const fvec2& b, colour_t c) const {
  c = z::colour_cycle(c, colour_cycle_);
  renderer_.render_legacy_line(convert_vec(a), convert_vec(b), convert_colour(c));
}

void Lib::render_lines(const nonstd::span<ii::render_output::line_t>& lines) const {
  std::vector<ii::render::line_t> render;
  for (const auto& line : lines) {
    auto& rl = render.emplace_back();
    rl.a = convert_vec(line.a);
    rl.b = convert_vec(line.b);
    rl.colour = convert_colour(z::colour_cycle(line.c, colour_cycle_));
  }
  renderer_.render_legacy_lines(render);
}

void Lib::render_text(const fvec2& v, const std::string& text, colour_t c) const {
  renderer_.render_legacy_text(static_cast<glm::ivec2>(convert_vec(v)), convert_colour(c), text);
}

void Lib::render_rect(const fvec2& lo, const fvec2& hi, colour_t c, std::int32_t line_width) const {
  c = z::colour_cycle(c, colour_cycle_);
  renderer_.render_legacy_rect(static_cast<glm::ivec2>(convert_vec(lo)),
                               static_cast<glm::ivec2>(convert_vec(hi)), line_width,
                               convert_colour(c));
}

void Lib::render() const {
  // TODO: goes too fast. Old was 50FPS.
  io_layer_.swap_buffers();
}

void Lib::rumble(std::int32_t player, std::int32_t time) {
  // TODO
}

void Lib::play_sound(ii::sound s) {
  internals_->mixer.play(static_cast<ii::Mixer::audio_handle_t>(s), 1.f, 0.f, 1.f);
}

void Lib::set_volume(std::int32_t volume) {
  internals_->mixer.set_master_volume(std::max(0, std::min(100, volume)) / 100.f);
}

LibInputAdapter::LibInputAdapter(Lib& lib, ii::ReplayWriter& writer) : lib_{lib}, writer_{writer} {}

std::vector<ii::input_frame> LibInputAdapter::get() {
  std::vector<ii::input_frame> result;
  for (std::int32_t i = 0; i < lib_.player_count(); ++i) {
    auto& f = result.emplace_back();
    f.velocity = lib_.get_move_velocity(i);
    auto target = lib_.get_fire_target(i);
    if (target.first) {
      f.target_relative = target.second;
    } else {
      f.target_absolute = target.second;
    }
    f.keys = static_cast<std::int32_t>(lib_.is_key_held(i, Lib::key::kFire)) |
        (lib_.is_key_pressed(i, Lib::key::kBomb) << 1);
    writer_.add_input_frame(f);
  }
  return result;
}