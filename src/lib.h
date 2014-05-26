#ifndef IISPACE_SRC_LIB_H
#define IISPACE_SRC_LIB_H

#include "z.h"
#include <map>
#include <memory>
struct score_finished {};
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

  struct Settings {
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

  void Init();
  void BeginFrame();
  void EndFrame();
  void CaptureMouse(bool enabled);
  void NewGame();
  void SetWorkingDirectory(bool original);

  void Exit(bool exit);
  bool Exit() const;

  Settings LoadSettings() const;
  void SetVolume(int volume);

  SaveData LoadSaveData();
  void SaveSaveData(const SaveData& version2);
  Settings LoadSaveSettings() const;
  void SaveSaveSettings(const Settings& settings);
  static std::string Crypt(const std::string& text, const std::string& key);

  // Input
  //------------------------------
  PadType IsPadConnected(int32_t player) const;

  bool IsKeyPressed(int32_t player, Key k) const;
  bool IsKeyReleased(int32_t player, Key k) const;
  bool IsKeyHeld(int32_t player, Key k) const;

  bool IsKeyPressed(Key k) const;
  bool IsKeyReleased(Key k) const;
  bool IsKeyHeld(Key k) const;

  Vec2 GetMoveVelocity(int32_t player) const;
  Vec2 GetFireTarget(int32_t player, const Vec2& position) const;

  // Output
  //------------------------------
  void ClearScreen() const;
  void RenderLine(const Vec2f& a, const Vec2f& b, colour c) const;
  void RenderText(const Vec2f& v, const std::string& text, colour c) const;
  void RenderRect(const Vec2f& low, const Vec2f& hi,
                  colour c, int lineWidth = 0) const;
  void Render() const;

  void Rumble(int player, int time);
  void StopRumble();
  bool PlaySound(
      Sound sound, float volume = 1.f, float pan = 0.f, float repitch = 0.f);

  void TakeScreenShot();

  // Recording
  //------------------------------
  void StartRecording(int32_t players, bool canFaceSecretBoss, bool isBossMode,
                      bool isHardMode, bool isFastMode, bool isWhatMode);
  void Record(Vec2 velocity, Vec2 target, int keys);
  void EndRecording(const std::string& name, uint64_t score, int32_t players,
                    bool bossMode, bool hardMode, bool fastMode, bool whatMode);

  void OnScore(long seed, int32_t players, bool bossMode, uint64_t score,
               bool hardMode, bool fastMode, bool whatMode);

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
  colour Cycle(colour c) const;

private:

  bool _recording;
  std::size_t _frameCount;
  int _cycle;
  Recording _replay;
  long _recordSeed;
  std::size_t _players;
  std::stringstream _record;

  // Internal
  //------------------------------
  bool _exit;
  char* _cwd;
  std::vector<char> _exe;

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
  Settings _settings;
  std::unique_ptr<Internals> _internals;
  std::size_t _scoreFrame;
  friend class Handler;

};

#endif
