#ifndef II_GAME_CORE_GAME_STACK_H
#define II_GAME_CORE_GAME_STACK_H
#include "game/data/config.h"
#include "game/data/replay.h"
#include "game/data/save.h"
#include "game/mixer/sound.h"
#include <array>

namespace ii::io {
class Filesystem;
class IoLayer;
}  // namespace ii::io
namespace ii {
class Mixer;
}  // namespace ii

namespace ii::ui {
enum class key {
  kAccept,
  kCancel,
  kMenu,
  kUp,
  kDown,
  kLeft,
  kRight,
  kMax,
};

struct input_frame {
  bool controller_change = false;
  std::array<bool, static_cast<std::size_t>(key::kMax)> key_pressed = {false};
  std::array<bool, static_cast<std::size_t>(key::kMax)> key_held = {false};

  bool pressed(key k) const {
    return key_pressed[static_cast<std::size_t>(k)];
  }

  bool& pressed(key k) {
    return key_pressed[static_cast<std::size_t>(k)];
  }

  bool held(key k) const {
    return key_held[static_cast<std::size_t>(k)];
  }

  bool& held(key k) {
    return key_held[static_cast<std::size_t>(k)];
  }
};

class GameStack {
public:
  GameStack(io::Filesystem& fs, io::IoLayer& io_layer, Mixer& mixer);

  io::IoLayer& io_layer() {
    return io_layer_;
  }

  const io::IoLayer& io_layer() const {
    return io_layer_;
  }

  Mixer& mixer() {
    return mixer_;
  }

  data::config& config() {
    return config_;
  }

  data::savegame& savegame() {
    return save_;
  }

  const data::config& config() const {
    return config_;
  }

  const data::savegame& savegame() const {
    return save_;
  }

  const input_frame& input() const {
    return input_;
  }

  void compute_input_frame(bool controller_change);
  void write_config();
  void write_savegame();
  void write_replay(const data::ReplayWriter& writer, const std::string& name, std::uint64_t score);

  void rumble(std::uint32_t time);
  void set_volume(float volume);
  void play_sound(sound s);
  void play_sound(sound s, float volume, float pan, float pitch);

private:
  io::Filesystem& fs_;
  io::IoLayer& io_layer_;
  Mixer& mixer_;

  data::config config_;
  data::savegame save_;
  input_frame input_;
};

}  // namespace ii::ui

#endif