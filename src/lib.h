#ifndef IISPACE_SRC_LIB_H
#define IISPACE_SRC_LIB_H

#include <map>
#include <memory>
#include <vector>
#include "z.h"
struct Internals;

class Lib {
public:
  Lib();
  ~Lib();

  // Constants
  //------------------------------
  enum Key {
    KEY_FIRE,
    KEY_BOMB,
    KEY_ACCEPT,
    KEY_CANCEL,
    KEY_MENU,
    KEY_UP,
    KEY_DOWN,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_MAX,
  };

  enum Sound {
    SOUND_PLAYER_DESTROY,
    SOUND_PLAYER_RESPAWN,
    SOUND_PLAYER_FIRE,
    SOUND_PLAYER_SHIELD,
    SOUND_EXPLOSION,
    SOUND_ENEMY_HIT,
    SOUND_ENEMY_DESTROY,
    SOUND_ENEMY_SHATTER,
    SOUND_ENEMY_SPAWN,
    SOUND_BOSS_ATTACK,
    SOUND_BOSS_FIRE,
    SOUND_POWERUP_LIFE,
    SOUND_POWERUP_OTHER,
    SOUND_MENU_CLICK,
    SOUND_MENU_ACCEPT,
    SOUND_MAX,
  };

  enum PadType {
    PAD_NONE,
    PAD_KEYMOUSE,
    PAD_GAMEPAD,
  };

  static const int32_t WIDTH = 640;
  static const int32_t HEIGHT = 480;
  static const int32_t TEXT_WIDTH = 16;
  static const int32_t TEXT_HEIGHT = 16;
  static const int32_t SOUND_TIMER = 4;

  static const std::string ENCRYPTION_KEY;
  static const std::string SUPER_ENCRYPTION_KEY;

  // General
  //------------------------------
  void set_player_count(int32_t players) {
    _players = players;
  }

  bool begin_frame();
  void end_frame();
  void capture_mouse(bool enabled);
  void new_game();
  static void set_working_directory(bool original);

  // Input
  //------------------------------
  PadType get_pad_type(int32_t player) const;

  bool is_key_pressed(int32_t player, Key k) const;
  bool is_key_released(int32_t player, Key k) const;
  bool is_key_held(int32_t player, Key k) const;

  bool is_key_pressed(Key k) const;
  bool is_key_released(Key k) const;
  bool is_key_held(Key k) const;

  vec2 get_move_velocity(int32_t player) const;
  vec2 get_fire_target(int32_t player, const vec2& position) const;

  // Output
  //------------------------------
  void clear_screen() const;
  void render_line(const flvec2& a, const flvec2& b, colour_t c) const;
  void render_text(const flvec2& v, const std::string& text, colour_t c) const;
  void render_rect(const flvec2& low, const flvec2& hi, colour_t c, int32_t line_width = 0) const;
  void render() const;

  void rumble(int32_t player, int32_t time);
  void stop_rumble();
  bool play_sound(Sound sound, float volume = 1.f, float pan = 0.f, float repitch = 0.f);
  void set_volume(int32_t volume);

  void take_screenshot();

  // Wacky colours
  //------------------------------
  void set_colour_cycle(int32_t cycle);
  int32_t get_colour_cycle() const;

private:
  int32_t _cycle;
  int32_t _players;

  // Internal
  //------------------------------
  std::vector<std::vector<bool>> _keys_pressed;
  std::vector<std::vector<bool>> _keys_held;
  std::vector<std::vector<bool>> _keys_released;

  bool _capture_mouse;
  ivec2 _mouse;
  ivec2 _extra;
  mutable bool _mouse_moving;

  void load_sounds();

  // Data
  //------------------------------
  std::unique_ptr<Internals> _internals;
  std::size_t _score_frame;
  friend class Handler;
};

#endif
