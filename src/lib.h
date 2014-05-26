#ifndef IISPACE_SRC_LIB_H
#define IISPACE_SRC_LIB_H

#include "z0.h"
#include <map>
struct score_finished {};

class Lib {
public:

  Lib();
  virtual ~Lib() {}

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
    PAD_NONE = 0,
    PAD_KEYMOUSE = 1,
    PAD_GAMEPAD = 2,
  };

  struct Settings {
    bool _disableBackground;
    int _hudCorrection;
    bool _windowed;
    fixed _volume;
  };

  typedef std::pair<std::string, uint64_t> HighScore;
  typedef std::vector<HighScore> HighScoreList;
  typedef std::vector<HighScoreList> HighScoreTable;
  struct SaveData {
    int _bossesKilled;
    int _hardModeBossesKilled;
    HighScoreTable _highScores;
  };
  static const std::size_t MAX_NAME_LENGTH = 17;
  static const std::size_t MAX_SCORE_LENGTH = 10;
  static const std::size_t NUM_HIGH_SCORES = 8;

  static const int32_t PLAYERS = 4;
  static const int32_t WIDTH = 640;
  static const int32_t HEIGHT = 480;
  static const int32_t TEXT_WIDTH = 16;
  static const int32_t TEXT_HEIGHT = 16;
  static const int32_t SOUND_TIMER = 4;

  static const std::string ENCRYPTION_KEY;
  static const std::string SUPER_ENCRYPTION_KEY;

  static bool ScoreSort(const HighScore& a, const HighScore& b)
  {
    return a.second > b.second;
  }

  // General
  //------------------------------
  std::size_t GetFrameCount() const
  {
    return _frameCount;
  }

  void SetFrameCount(std::size_t fc)
  {
    _frameCount = fc;
  }

  int32_t GetPlayerCount() const
  {
    return _players;
  }

  void SetPlayerCount(int32_t players)
  {
    _players = players;
  }

  virtual void Init() = 0;
  virtual void BeginFrame() = 0;
  virtual void EndFrame() = 0;
  virtual void CaptureMouse(bool enabled) {}
  virtual void NewGame() {}
  virtual void SetWorkingDirectory(bool original) = 0;

  virtual void Exit(bool exit) = 0;
  virtual bool Exit() const = 0;

  virtual int RandInt(int lessThan) = 0;
  virtual fixed RandFloat() = 0;

  virtual Settings LoadSettings() const = 0;
  virtual void SetVolume(int volume) = 0;

  virtual std::string SavePath() const = 0;
  virtual std::string SettingsPath() const = 0;

  SaveData LoadSaveData();
  void SaveSaveData(const SaveData& version2);
  Settings LoadSaveSettings() const;
  void SaveSaveSettings(const Settings& settings);
  static std::string Crypt(const std::string& text, const std::string& key);

  // Input
  //------------------------------
  virtual PadType IsPadConnected(int32_t player) const = 0;

  virtual bool IsKeyPressed(int32_t player, Key k) const = 0;
  virtual bool IsKeyReleased(int32_t player, Key k) const = 0;
  virtual bool IsKeyHeld(int32_t player, Key k) const = 0;

  virtual bool IsKeyPressed(Key k) const;
  virtual bool IsKeyReleased(Key k) const;
  virtual bool IsKeyHeld(Key k) const;

  virtual Vec2 GetMoveVelocity(int32_t player) const = 0;
  virtual Vec2 GetFireTarget(int32_t player, const Vec2& position) const = 0;

  // Output
  //------------------------------
  virtual void ClearScreen() const = 0;
  virtual void RenderLine(
      const Vec2f& a, const Vec2f& b, Colour c) const = 0;
  virtual void RenderText(
      const Vec2f& v, const std::string& text, Colour c) const = 0;
  virtual void RenderRect(
      const Vec2f& low, const Vec2f& hi, Colour c, int lineWidth = 0) const = 0;
  virtual void Render() const = 0;

  virtual void Rumble(int player, int time) = 0;
  virtual void StopRumble() = 0;
  virtual bool PlaySound(
      Sound sound,
      float volume = 1.f, float pan = 0.f, float repitch = 0.f) = 0;

  virtual void TakeScreenShot() = 0;

  // Recording
  //------------------------------
  void StartRecording(int32_t players, bool canFaceSecretBoss, bool isBossMode,
                      bool isHardMode, bool isFastMode, bool isWhatMode);
  void Record(Vec2 velocity, Vec2 target, int keys);
  void EndRecording(const std::string& name, uint64_t score, int32_t players,
                    bool bossMode, bool hardMode, bool fastMode, bool whatMode);

  virtual void OnScore(long seed, int32_t players, bool bossMode, uint64_t score,
                       bool hardMode, bool fastMode, bool whatMode) {}

  struct PlayerFrame {
    Vec2 _velocity;
    Vec2 _target;
    int _keys;
  };
  struct Recording {
    bool _okay;
    std::string _error;
    long _seed;
    int _players;
    bool _canFaceSecretBoss;
    bool _isBossMode;
    bool _isHardMode;
    bool _isFastMode;
    bool _isWhatMode;
    std::vector<PlayerFrame> _playerFrames;
  };
  const Recording& PlayRecording(const std::string& path);

  // Wacky colours
  //------------------------------
  void SetColourCycle(int cycle);
  int GetColourCycle() const;
  Colour Cycle(Colour c, int cycle) const;

protected:

  Colour RgbToHsl(Colour rgb) const;
  Colour HslToRgb(Colour hsl) const;
  Colour Cycle(Colour rgb) const;

private:

  bool _recording;
  std::size_t _frameCount;
  int _cycle;
  Recording _replay;
  long _recordSeed;
  std::size_t _players;
  std::stringstream _record;
  mutable std::map<std::pair<Colour, int>, Colour> _cycleMap;

};

#endif
