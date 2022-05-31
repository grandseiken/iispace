#include "game/core/ui_layer.h"
#include "game/core/z0_game.h"
#include "game/io/file/std_filesystem.h"
#include "game/io/sdl_io.h"
#include "game/mixer/mixer.h"
#include "game/mixer/sound.h"
#include "game/render/gl_renderer.h"
#include <iostream>
#include <string>
#include <vector>

namespace ii {

void load_sounds(const io::Filesystem& fs, Mixer& mixer) {
  auto load_sound = [&](sound s, const std::string& filename) {
    auto bytes = fs.read_asset(filename);
    if (!bytes) {
      std::cerr << bytes.error() << std::endl;
      return;
    }
    auto result = mixer.load_wav_memory(*bytes, static_cast<ii::Mixer::audio_handle_t>(s));
    if (!result) {
      std::cerr << "Couldn't load sound " + filename + ": " << result.error() << std::endl;
    }
  };
  load_sound(sound::kPlayerFire, "PlayerFire.wav");
  load_sound(sound::kMenuClick, "MenuClick.wav");
  load_sound(sound::kMenuAccept, "MenuAccept.wav");
  load_sound(sound::kPowerupLife, "PowerupLife.wav");
  load_sound(sound::kPowerupOther, "PowerupOther.wav");
  load_sound(sound::kEnemyHit, "EnemyHit.wav");
  load_sound(sound::kEnemyDestroy, "EnemyDestroy.wav");
  load_sound(sound::kEnemyShatter, "EnemyShatter.wav");
  load_sound(sound::kEnemySpawn, "EnemySpawn.wav");
  load_sound(sound::kBossAttack, "BossAttack.wav");
  load_sound(sound::kBossFire, "BossFire.wav");
  load_sound(sound::kPlayerRespawn, "PlayerRespawn.wav");
  load_sound(sound::kPlayerDestroy, "PlayerDestroy.wav");
  load_sound(sound::kPlayerShield, "PlayerShield.wav");
  load_sound(sound::kExplosion, "Explosion.wav");
}

bool run(const std::vector<std::string>& args) {
  static constexpr char kGlMajor = 4;
  static constexpr char kGlMinor = 6;

  auto io_layer_result = io::SdlIoLayer::create("iispace", kGlMajor, kGlMinor);
  if (!io_layer_result) {
    std::cerr << "Error initialising IO: " << io_layer_result.error() << std::endl;
    return false;
  }
  io::StdFilesystem fs{"assets", "savedata", "savedata/replays"};
  auto renderer_result = render::GlRenderer::create(fs);
  if (!renderer_result) {
    std::cerr << "Error initialising renderer: " << renderer_result.error() << std::endl;
    return false;
  }
  Mixer mixer{io::kAudioSampleRate};
  auto io_layer = std::move(*io_layer_result);
  auto renderer = std::move(*renderer_result);
  io_layer->set_audio_callback(
      [&mixer](std::uint8_t* p, std::size_t k) { mixer.audio_callback(p, k); });
  load_sounds(fs, mixer);

  ui::UiLayer ui_layer{fs, *io_layer, mixer};

  std::optional<ReplayReader> replay;
  if (!args.empty()) {
    auto replay_data = fs.read_replay(args[0]);
    if (!replay_data) {
      std::cerr << replay_data.error() << std::endl;
      return false;
    }
    auto reader = ii::ReplayReader::create(*replay_data);
    if (!reader) {
      std::cerr << reader.error() << std::endl;
      return false;
    }
    replay = std::move(*reader);
  }
  z0Game game{std::move(replay)};

  bool exit = false;
  while (!exit) {
    bool audio_change = false;
    bool controller_change = false;
    while (true) {
      auto event = io_layer->poll();
      if (!event) {
        break;
      } else if (*event == ii::io::event_type::kClose) {
        exit = true;
        break;
      } else if (*event == ii::io::event_type::kAudioDeviceChange) {
        audio_change = true;
      } else if (*event == ii::io::event_type::kControllerChange) {
        controller_change = true;
      }
    }

    if (audio_change) {
      auto result = io_layer->open_audio_device(std::nullopt);
      if (!result) {
        std::cerr << "Error opening audio device: " << result.error() << std::endl;
      }
    }

    ui_layer.compute_input_frame(controller_change);
    if (game.update(ui_layer)) {
      exit = true;
    }
    io_layer->input_frame_clear();
    mixer.commit();

    renderer->clear_screen();
    game.render(ui_layer, *renderer);
    // TODO: too fast. Was 50FPS, now 60FPS, need timing logic.
    io_layer->swap_buffers();
  }
  return true;
}

}  // namespace ii

int main(int argc, char** argv) {
  std::vector<std::string> args;
  for (int i = 1; i < argc; ++i) {
    args.emplace_back(argv[i]);
  }
  return ii::run(args) ? 0 : 1;
}
