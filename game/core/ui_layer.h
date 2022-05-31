#ifndef IISPACE_GAME_CORE_UI_LAYER_H
#define IISPACE_GAME_CORE_UI_LAYER_H
#include "game/data/replay.h"
#include "game/data/save.h"
#include <array>

namespace ii::io {
class Filesystem;
class IoLayer;
}  // namespace ii::io

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

class UiLayer {
public:
  UiLayer(io::Filesystem& fs, io::IoLayer& io_layer);

  io::IoLayer& io_layer() {
    return io_layer_;
  }
  const io::IoLayer& io_layer() const {
    return io_layer_;
  }

  void compute_input_frame();
  const input_frame& input() const {
    return input_;
  }

  Config& config() {
    return config_;
  }
  SaveGame& save_game() {
    return save_;
  }
  const Config& config() const {
    return config_;
  }
  const SaveGame& save_game() const {
    return save_;
  }

  void write_config();
  void write_save_game();
  void write_replay(const ii::ReplayWriter& writer, const std::string& name, std::int64_t score);

private:
  io::Filesystem& fs_;
  io::IoLayer& io_layer_;

  Config config_;
  SaveGame save_;
  input_frame input_;
};

}  // namespace ii::ui

#endif