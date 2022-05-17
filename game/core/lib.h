#ifndef IISPACE_GAME_CORE_LIB_H
#define IISPACE_GAME_CORE_LIB_H
#include "game/common/z.h"
#include <map>
#include <memory>
#include <vector>
struct Internals;

class Lib {
public:
  Lib();
  ~Lib();

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

  enum class sound {
    kPlayerDestroy,
    kPlayerRespawn,
    kPlayerFire,
    kPlayerShield,
    kExplosion,
    kEnemyHit,
    kEnemyDestroy,
    kEnemyShatter,
    kEnemySpawn,
    kBossAttack,
    kBossFire,
    kPowerupLife,
    kPowerupOther,
    kMenuClick,
    kMenuAccept,
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
  static constexpr std::int32_t kSoundTimer = 4;

  static const std::string kEncryptionKey;
  static const std::string kSuperEncryptionKey;

  // General
  //------------------------------
  void set_player_count(std::int32_t players) {
    players_ = players;
  }

  bool begin_frame();
  void end_frame();
  void capture_mouse(bool enabled);
  void new_game();
  static void set_working_directory(bool original);

  // Input
  //------------------------------
  pad_type get_pad_type(std::int32_t player) const;

  bool is_key_pressed(std::int32_t player, key k) const;
  bool is_key_released(std::int32_t player, key k) const;
  bool is_key_held(std::int32_t player, key k) const;

  bool is_key_pressed(key k) const;
  bool is_key_released(key k) const;
  bool is_key_held(key k) const;

  vec2 get_move_velocity(std::int32_t player) const;
  vec2 get_fire_target(std::int32_t player, const vec2& position) const;

  // Output
  //------------------------------
  void clear_screen() const;
  void render_line(const fvec2& a, const fvec2& b, colour_t c) const;
  void render_text(const fvec2& v, const std::string& text, colour_t c) const;
  void
  render_rect(const fvec2& low, const fvec2& hi, colour_t c, std::int32_t line_width = 0) const;
  void render() const;

  void rumble(std::int32_t player, std::int32_t time);
  void stop_rumble();
  bool play_sound(sound, float volume = 1.f, float pan = 0.f, float repitch = 0.f);
  void set_volume(std::int32_t volume);

  void take_screenshot();

  // Wacky colours
  //------------------------------
  void set_colour_cycle(std::int32_t cycle);
  std::int32_t get_colour_cycle() const;

private:
  std::int32_t cycle_;
  std::int32_t players_;

  // Internal
  //------------------------------
  std::vector<std::vector<bool>> keys_pressed_;
  std::vector<std::vector<bool>> keys_held_;
  std::vector<std::vector<bool>> keys_released_;

  bool capture_mouse_;
  ivec2 mouse_;
  ivec2 extra_;
  mutable bool mouse_moving_;

  void load_sounds();

  // Data
  //------------------------------
  std::unique_ptr<Internals> internals_;
  std::size_t score_frame_;
  friend class Handler;
};

#endif
