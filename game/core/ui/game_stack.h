#ifndef II_GAME_CORE_UI_GAME_STACK_H
#define II_GAME_CORE_UI_GAME_STACK_H
#include "game/common/enum.h"
#include "game/core/ui/element.h"
#include "game/core/ui/input.h"
#include "game/data/config.h"
#include "game/data/replay.h"
#include "game/data/save.h"
#include "game/mixer/sound.h"
#include <cstdint>
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
  kCaptureInput = 0b0001,
  kCaptureUpdate = 0b0010,
  kCaptureRender = 0b0100,
};
}  // namespace ii::ui

namespace ii {
template <>
struct bitmask_enum<ui::layer_flag> : std::true_type {};
}  // namespace ii

namespace ii::ui {

class GameStack;
class GameLayer : public Element {
public:
  ~GameLayer() override = default;
  GameLayer(GameStack& stack, layer_flag flags = layer_flag::kNone)
  : stack_{stack}, flags_{flags} {}

  const GameStack& stack() const {
    return stack_;
  }

  GameStack& stack() {
    return stack_;
  }

  layer_flag flags() const {
    return flags_;
  }

protected:
  void update_content(const input_frame&) override;

private:
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