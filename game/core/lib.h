#ifndef IISPACE_GAME_CORE_LIB_H
#define IISPACE_GAME_CORE_LIB_H
#include "game/common/z.h"
#include "game/data/replay.h"
#include "game/logic/sim/sim_io.h"
#include "game/mixer/sound.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace ii {
class SimState;
}  // namespace ii
namespace ii::io {
class Filesystem;
class IoLayer;
}  // namespace ii::io
namespace ii::render {
class GlRenderer;
}  // namespace ii::render

class Lib {
public:
  Lib(ii::io::Filesystem& fs, ii::io::IoLayer& io_layer, ii::render::GlRenderer& renderer);
  ~Lib();

  ii::io::Filesystem& filesystem() {
    return fs_;
  }

  // Constants
  //------------------------------
  enum class key {
    kFire,
    kBomb,
    kAccept,
    kCancel,
    kMenu,
    kUp,
    kDown,
    kLeft,
    kRight,
    kMax,
  };

  enum pad_type {
    kPadNone,
    kPadKeyboardMouse,
    kPadGamepad,
  };

  static constexpr std::int32_t kWidth = 640;
  static constexpr std::int32_t kHeight = 480;
  static constexpr std::int32_t kTextWidth = 16;
  static constexpr std::int32_t kTextHeight = 16;

  // General
  //------------------------------
  void set_player_count(std::int32_t players) {
    players_ = players;
  }
  std::int32_t player_count() const {
    return players_;
  }

  bool begin_frame();
  void end_frame();
  void capture_mouse(bool enabled);
  void new_game();
  void post_update(ii::SimState& sim);

  // Input
  //------------------------------
  // TODO: refactor with proper controller join UI and abstract input forward layer for sim state.
  pad_type get_pad_type(std::int32_t player) const;

  bool is_key_pressed(std::int32_t player, key k) const;
  bool is_key_held(std::int32_t player, key k) const;

  bool is_key_pressed(key k) const;
  bool is_key_held(key k) const;

  vec2 get_move_velocity(std::int32_t player) const;
  std::pair<bool, vec2> get_fire_target(std::int32_t player) const;

  // Output
  //------------------------------
  void clear_screen() const;
  void render_line(const fvec2& a, const fvec2& b, colour_t c) const;
  void render_text(const fvec2& v, const std::string& text, colour_t c) const;
  void
  render_rect(const fvec2& low, const fvec2& hi, colour_t c, std::int32_t line_width = 0) const;
  void render() const;

  void rumble(std::int32_t player, std::int32_t time);
  void play_sound(ii::sound);
  void set_volume(std::int32_t volume);

  // Wacky colours
  //------------------------------
  void set_colour_cycle(std::int32_t cycle);
  std::int32_t get_colour_cycle() const;

private:
  ii::io::Filesystem& fs_;
  ii::io::IoLayer& io_layer_;
  ii::render::GlRenderer& renderer_;

  std::int32_t colour_cycle_ = 0;
  std::int32_t players_ = 1;

  struct Internals;
  std::unique_ptr<Internals> internals_;
};

class LibInputAdapter : public ii::InputAdapter {
public:
  LibInputAdapter(Lib& lib, ii::ReplayWriter& writer);
  std::vector<ii::input_frame> get() override;

private:
  Lib& lib_;
  ii::ReplayWriter& writer_;
};

#endif
