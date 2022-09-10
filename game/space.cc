#include "game/core/game_options.h"
#include "game/core/layers/main_menu.h"
#include "game/core/sim/replay_viewer.h"
#include "game/core/ui/game_stack.h"
#include "game/flags.h"
#include "game/io/file/std_filesystem.h"
#include "game/io/sdl_io.h"
#include "game/mixer/mixer.h"
#include "game/mixer/sound.h"
#include "game/mode_flags.h"
#include "game/render/gl_renderer.h"
#include "game/system/system.h"
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
    auto result = mixer.load_wav_memory(*bytes, static_cast<Mixer::audio_handle_t>(s));
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

bool run(System& system, const std::vector<std::string>& args, const game_options_t& options) {
  static constexpr const char* kTitle = "space";
  static constexpr char kGlMajor = 4;
  static constexpr char kGlMinor = 6;
  static constexpr std::uint32_t kGlslVersion = 460;

  auto io_layer_result = io::SdlIoLayer::create(kTitle, kGlMajor, kGlMinor);
  if (!io_layer_result) {
    std::cerr << "Error initialising IO: " << io_layer_result.error() << std::endl;
    return false;
  }
  io::StdFilesystem fs{"assets", "savedata", "savedata/replays"};
  auto renderer_result = render::GlRenderer::create(kGlslVersion);
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

  ui::GameStack stack{fs, *io_layer, mixer, options};
  stack.add<MainMenuLayer>();

  if (!args.empty()) {
    auto replay_data = fs.read(args[0]);
    if (!replay_data) {
      std::cerr << replay_data.error() << std::endl;
      return false;
    }
    auto reader = data::ReplayReader::create(*replay_data);
    if (!reader) {
      std::cerr << reader.error() << std::endl;
      return false;
    }
    stack.add<ReplayViewer>(std::move(*reader));
  }

  using counter_t = std::chrono::duration<double>;
  auto start_time = std::chrono::steady_clock::now();
  auto target_time = counter_t{0.};
  auto elapsed_time = [&] {
    auto now = std::chrono::steady_clock::now();
    return std::chrono::duration_cast<counter_t>(now - start_time);
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
      }
      if (*event == io::event_type::kClose) {
        exit = true;
        break;
      }
      if (*event == io::event_type::kAudioDeviceChange) {
        audio_change = true;
      } else if (*event == io::event_type::kControllerChange) {
        controller_change = true;
      }
    }

    if (audio_change) {
      auto result = io_layer->open_audio_device(std::nullopt);
      if (!result) {
        std::cerr << "Error opening audio device: " << result.error() << std::endl;
      }
    }

    stack.update(controller_change);
    mixer.commit();
    exit |= stack.empty();

    renderer->clear_screen();
    stack.render(*renderer);
    io_layer->swap_buffers();

    auto render_status = renderer->status();
    if (!render_status) {
      std::cerr << render_status.error() << std::endl;
    }

    counter_t time_per_frame{1. / stack.fps()};
    target_time += time_per_frame;
    auto actual_time = elapsed_time();
    if (actual_time >= target_time) {
      target_time = actual_time;
    } else {
      std::this_thread::sleep_for(target_time - actual_time);
    }
  }
  return true;
}

result<std::vector<std::uint32_t>>
parse_player_index_list(const std::optional<std::string>& index_string) {
  std::vector<std::uint32_t> result;
  if (!index_string) {
    return result;
  }
  for (char c : *index_string) {
    if (c >= '0' && c <= '9') {
      result.emplace_back(static_cast<std::uint32_t>(c - '0'));
    } else {
      return unexpected("invalid player index list");
    }
  }
  return result;
}

result<game_options_t> parse_args(std::vector<std::string>& args) {
  game_options_t options;

  if (auto r = parse_compatibility_level(args, options.compatibility); !r) {
    return unexpected(r.error());
  }

  std::optional<std::string> ai_players;
  if (auto r = flag_parse(args, "ai_players", ai_players); !r) {
    return unexpected(r.error());
  }
  auto index_list = parse_player_index_list(ai_players);
  if (!index_list) {
    return unexpected(index_list.error());
  }
  options.ai_players = std::move(*index_list);

  std::optional<std::string> replay_remote_players;
  if (auto r = flag_parse(args, "replay_remote_players", replay_remote_players); !r) {
    return unexpected(r.error());
  }
  index_list = parse_player_index_list(replay_remote_players);
  if (!index_list) {
    return unexpected(index_list.error());
  }
  options.replay_remote_players = std::move(*index_list);

  if (auto r = flag_parse<std::uint64_t>(args, "replay_min_tick_delivery_delay",
                                         options.replay_min_tick_delivery_delay, 0u);
      !r) {
    return unexpected(r.error());
  }

  if (auto r = flag_parse<std::uint64_t>(args, "replay_max_tick_delivery_delay",
                                         options.replay_max_tick_delivery_delay, 8u);
      !r) {
    return unexpected(r.error());
  }

  return {std::move(options)};
}

}  // namespace
}  // namespace ii

int main(int argc, const char** argv) {
  auto system = ii::create_system();
  auto init = system->init();
  if (!init) {
    std::cerr << init.error() << std::endl;
    return 1;
  }

  std::vector<std::string> args = std::move(*init);
  ii::args_init(args, argc, argv);
  auto options = ii::parse_args(args);
  if (!options) {
    std::cerr << options.error() << std::endl;
    return 1;
  }
  if (auto result = ii::args_finish(args); !result) {
    std::cerr << result.error() << std::endl;
    return 1;
  }
  return ii::run(*system, args, *options) ? 0 : 1;
}
