#ifndef II_GAME_CORE_GAME_STACK_H
#define II_GAME_CORE_GAME_STACK_H
#include "game/common/enum.h"
#include "game/data/config.h"
#include "game/data/replay.h"
#include "game/data/save.h"
#include "game/mixer/sound.h"
#include <array>
#include <deque>
#include <memory>

namespace ii::io {
class Filesystem;
class IoLayer;
}  // namespace ii::io
namespace ii::render {
class GlRenderer;
}  // namespace ii::render
namespace ii {
class Mixer;
}  // namespace ii

namespace ii::ui {
enum class layer_flag : std::uint32_t {
  kNone = 0b0000,
  kCaptureUpdate = 0b0001,
  kCaptureRender = 0b0010,
};
}  // namespace ii::ui

namespace ii {
template <>
struct bitmask_enum<ui::layer_flag> : std::true_type {};
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

class GameStack;
class GameLayer {
public:
  GameLayer(GameStack& stack, layer_flag flags = layer_flag::kNone)
  : stack_{stack}, flags_{flags} {}

  virtual ~GameLayer() = default;
  virtual void update(const input_frame&) = 0;
  virtual void render(render::GlRenderer&) const = 0;

  const GameStack& stack() const {
    return stack_;
  }

  GameStack& stack() {
    return stack_;
  }

  void close() {
    close_ = true;
  }

  bool is_closed() const {
    return close_;
  }

  layer_flag flags() const {
    return flags_;
  }

private:
  bool close_ = false;
  layer_flag flags_ = layer_flag::kNone;
  GameStack& stack_;
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

  std::uint32_t fps() const {
    return fps_;
  }

  void set_fps(std::uint32_t fps) {
    fps_ = fps;
  }

  template <typename T, typename... Args>
  void add(Args&&... args) {
    layers_.emplace_back(std::make_unique<T>(*this, std::forward<Args>(args)...));
  }

  bool empty() const {
    return layers_.empty();
  }

  GameLayer* top() {
    return layers_.empty() ? nullptr : layers_.back().get();
  }

  const GameLayer* top() const {
    return layers_.empty() ? nullptr : layers_.back().get();
  }

  void update(bool controller_change);
  void render(render::GlRenderer& renderer) const;

  void write_config();
  void write_savegame();
  void write_replay(const data::ReplayWriter& writer, const std::string& name, std::uint64_t score);

  void set_volume(float volume);
  void play_sound(sound s);
  void play_sound(sound s, float volume, float pan, float pitch);

private:
  io::Filesystem& fs_;
  io::IoLayer& io_layer_;
  Mixer& mixer_;

  data::config config_;
  data::savegame save_;

  std::uint32_t fps_ = 60;
  std::deque<std::unique_ptr<GameLayer>> layers_;
};

}  // namespace ii::ui

#endif