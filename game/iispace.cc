#include "game/core/ui_layer.h"
#include "game/core/z0_game.h"
#include "game/flags.h"
#include "game/io/file/std_filesystem.h"
#include "game/io/sdl_io.h"
#include "game/mixer/mixer.h"
#include "game/mixer/sound.h"
#include "game/render/gl_renderer.h"
#include <chrono>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

namespace ii {
namespace {

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

struct options_t {
  std::uint32_t ai_count = 0;
};

bool run(const std::vector<std::string>& args, const game_options_t& options) {
  static constexpr char kGlMajor = 4;
  static constexpr char kGlMinor = 6;

  auto io_layer_result = io::SdlIoLayer::create("iispace", kGlMajor, kGlMinor);
  if (!io_layer_result) {
    std::cerr << "Error initialising IO: " << io_layer_result.error() << std::endl;
    return false;
  }
  io::StdFilesystem fs{"assets", "savedata", "savedata/replays"};
  auto renderer_result = render::GlRenderer::create();
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
  ModalStack modal_stack;
  auto* game = modal_stack.add(std::make_unique<z0Game>(options));

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
    modal_stack.add(std::make_unique<GameModal>(std::move(*reader)));
  }

  using counter_t = std::chrono::duration<double>;
  static constexpr std::uint32_t kFps = 50;
  static constexpr counter_t time_per_frame{1. / kFps};
  auto last_time = std::chrono::steady_clock::now();
  auto elapsed_time = [&last_time] {
    auto now = std::chrono::steady_clock::now();
    auto r = std::chrono::duration_cast<counter_t>(now - last_time);
    return r;
  };

  bool exit = false;
  while (!exit) {
    io_layer->input_frame_clear();
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
    modal_stack.update(ui_layer);
    mixer.commit();
    exit |= modal_stack.empty();

    renderer->clear_screen();
    modal_stack.render(ui_layer, *renderer);
    io_layer->swap_buffers();

    auto render_status = renderer->status();
    if (!render_status) {
      std::cerr << render_status.error() << std::endl;
    }

    auto elapsed = elapsed_time();
    if (time_per_frame > elapsed) {
      std::this_thread::sleep_for(time_per_frame - elapsed);
    }
    last_time = std::chrono::steady_clock::now();
  }
  return true;
}

}  // namespace
}  // namespace ii

int main(int argc, char** argv) {
  auto args = ii::args_init(argc, argv);
  ii::game_options_t options;
  if (auto result = ii::flag_parse<std::uint32_t>(args, "ai_count", options.ai_count, 0u);
      !result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }
  if (auto result = ii::args_finish(args); !result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }
  return ii::run(args, options) ? 0 : 1;
}
