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

template <typename T>
bool contains(nonstd::span<const T> range, T value) {
  return std::find(range.begin(), range.end(), value) != range.end();
}

}  // namespace

struct Lib::Internals {
  Internals() : mixer{ii::io::kAudioSampleRate} {}
  ii::Mixer mixer;

  struct sound_aggregation {
    std::size_t count = 0;
    float volume = 0.f;
    float pan = 0.f;
    float pitch = 0.f;
  };
  std::unordered_map<ii::sound, sound_aggregation> sounds;
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
}

Lib::~Lib() {}

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

  internals_->sounds.clear();
  renderer_.set_dimensions(io_layer_.dimensions(), glm::uvec2{kWidth, kHeight});
  return false;
}

void Lib::end_frame() {
  io_layer_.input_frame_clear();
  internals_->mixer.commit();
}

void Lib::new_game() {
  io_layer_.input_frame_clear();
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
  sim.clear_output();
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

void Lib::rumble(std::int32_t player, std::int32_t time) {
  // TODO
}

void Lib::play_sound(ii::sound s) {
  internals_->mixer.play(static_cast<ii::Mixer::audio_handle_t>(s), 1.f, 0.f, 1.f);
}

void Lib::set_volume(std::int32_t volume) {
  internals_->mixer.set_master_volume(std::max(0, std::min(100, volume)) / 100.f);
}