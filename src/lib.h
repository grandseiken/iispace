#ifndef IISPACE_SRC_LIB_H
#define IISPACE_SRC_LIB_H

#include "z.h"
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
  };

  enum PadType {
    PAD_NONE,
    PAD_KEYMOUSE,
    PAD_GAMEPAD,
  };

  static const int32_t PLAYERS = 4;
  static const int32_t WIDTH = 640;
  static const int32_t HEIGHT = 480;
  static const int32_t TEXT_WIDTH = 16;
  static const int32_t TEXT_HEIGHT = 16;
  static const int32_t SOUND_TIMER = 4;

  static const std::string ENCRYPTION_KEY;
  static const std::string SUPER_ENCRYPTION_KEY;

  // General
  //------------------------------
  std::size_t get_frame_count() const
  {
    return _frame_count;
  }

  void set_frame_count(std::size_t fc)
  {
    _frame_count = fc;
  }

  void set_player_count(int32_t players)
  {
    _players = players;
  }

  void Init();
  void BeginFrame();
  void EndFrame();
  void CaptureMouse(bool enabled);
  void NewGame();
  static void SetWorkingDirectory(bool original);

  void Exit(bool exit);
  bool Exit() const;

  // Input
  //------------------------------
  PadType IsPadConnected(int32_t player) const;

  bool IsKeyPressed(int32_t player, Key k) const;
  bool IsKeyReleased(int32_t player, Key k) const;
  bool IsKeyHeld(int32_t player, Key k) const;

  bool IsKeyPressed(Key k) const;
  bool IsKeyReleased(Key k) const;
  bool IsKeyHeld(Key k) const;

  vec2 GetMoveVelocity(int32_t player) const;
  vec2 GetFireTarget(int32_t player, const vec2& position) const;

  // Output
  //------------------------------
  void ClearScreen() const;
  void RenderLine(const flvec2& a, const flvec2& b, colour_t c) const;
  void RenderText(const flvec2& v, const std::string& text, colour_t c) const;
  void RenderRect(const flvec2& low, const flvec2& hi,
                  colour_t c, int lineWidth = 0) const;
  void Render() const;

  void Rumble(int player, int time);
  void StopRumble();
  bool PlaySound(
      Sound sound, float volume = 1.f, float pan = 0.f, float repitch = 0.f);
  void SetVolume(int volume);

  void TakeScreenShot();

  // Wacky colours
  //------------------------------
  void SetColourCycle(int cycle);
  int GetColourCycle() const;
  colour_t Cycle(colour_t c) const;

private:

  int32_t _frame_count;
  int32_t _cycle;
  int32_t _players;

  // Internal
  //------------------------------
  bool _exit;
  std::vector<std::vector<bool>> _keysPressed;
  std::vector<std::vector<bool>> _keysHeld;
  std::vector<std::vector<bool>> _keysReleased;

  bool _captureMouse;
  int _mousePosX;
  int _mousePosY;
  int _extraX;
  int _extraY;
  mutable bool _mouseMoving;

  void LoadSounds();

  // Data
  //------------------------------
  std::unique_ptr<Internals> _internals;
  std::size_t _score_frame;
  friend class Handler;

};

#endif
