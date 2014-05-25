#ifndef z0GAME_H
#define z0GAME_H

#include "game.h"
#include "lib.h"
class Particle;

class z0Game : public Game {
public:

  // Constants
  //------------------------------
  enum GameState {
    STATE_MENU,
    STATE_PAUSE,
    STATE_GAME,
    STATE_HIGHSCORE,
  };
  enum BossList {
    BOSS_1A = 1,
    BOSS_1B = 2,
    BOSS_1C = 4,
    BOSS_2A = 8,
    BOSS_2B = 16,
    BOSS_2C = 32,
    BOSS_3A = 64
  };

  static const Colour PANEL_TEXT = 0xeeeeeeff;
  static const Colour PANEL_TRAN = 0xeeeeee99;
  static const Colour PANEL_BACK = 0x000000ff;
  static const int STARTING_LIVES;
  static const int BOSSMODE_LIVES;

#define ALLOWED_CHARS "ABCDEFGHiJKLMNOPQRSTUVWXYZ 1234567890! "

  typedef std::vector<Ship*> ShipList;

  z0Game(Lib& lib, std::vector<std::string> args);
  virtual ~z0Game();

  // Main functions
  //------------------------------
  virtual void Update();
  virtual void Render() const;

  GameState GetState() const
  {
    return _state;
  }

  // Ships
  //------------------------------
  void AddShip(Ship* ship);
  void AddParticle(Particle* particle);
  int GetNonWallCount() const;
  ShipList GetCollisionList(const Vec2& point, int category) const;
  ShipList GetShipsInRadius(const Vec2& point, fixed radius) const;
  bool AnyCollisionList(const Vec2& point, int category) const;
  const ShipList& GetShips() const;

  // Players
  //------------------------------
  int CountPlayers() const
  {
    return _players;
  }

  Player* GetNearestPlayer(const Vec2& point) const;

  ShipList GetPlayers() const
  {
    return _playerList;
  }

  bool IsBossMode() const
  {
    return _bossMode;
  }

  bool IsHardMode() const
  {
    return _hardMode;
  }

  bool IsFastMode() const
  {
    return _fastMode;
  }

  bool IsWhatMode() const
  {
    return _whatMode;
  }

  void SetBossKilled(BossList boss);

  void RenderHPBar(float fill)
  {
    _showHPBar = true; _fillHPBar = fill;
  }

  // Lives
  //------------------------------
  void AddLife()
  {
    _lives++;
  }

  void SubLife()
  {
    if (_lives) {
      _lives--;
    }
  }

  int GetLives() const
  {
    return _lives;
  }

private:

  // Internals
  //------------------------------
  void RenderPanel(const Vec2f& low, const Vec2f& hi) const;
  static bool SortShips(Ship* const& a, Ship* const& b);

  bool IsBossModeUnlocked() const
  {
    return (_bossesKilled & 63) == 63;
  }

  bool IsHardModeUnlocked() const
  {
    return IsBossModeUnlocked() &&
        (_highScores[Lib::PLAYERS][0].second > 0 ||
         _highScores[Lib::PLAYERS][1].second > 0 ||
         _highScores[Lib::PLAYERS][2].second > 0 ||
         _highScores[Lib::PLAYERS][3].second > 0);
  }

  bool IsFastModeUnlocked() const
  {
    return IsHardModeUnlocked() && ((_hardModeBossesKilled & 63) == 63);
  }

  bool IsWhatModeUnlocked() const
  {
    return IsFastModeUnlocked() && ((_hardModeBossesKilled & 64) == 64);
  }

  std::string ConvertToTime(long score) const;

  // Scores
  //------------------------------
  long GetPlayerScore(int playerNumber) const;
  int  GetPlayerDeaths(int playerNumber) const;
  long GetTotalScore() const;
  bool IsHighScore() const;

  void NewGame(bool canFaceSecretBoss, bool bossMode = false,
               bool replay = false, bool hardMode = false,
               bool fastMode = false, bool whatMode = false);
  void EndGame();

  GameState _state;
  int _players;
  int _lives;
  bool _bossMode;
  bool _hardMode;
  bool _fastMode;
  bool _whatMode;

  mutable bool _showHPBar;
  mutable float _fillHPBar;

  int _selection;
  int _specialSelection;
  int _killTimer;
  int _exitTimer;

  std::string _enterName;
  int _enterChar;
  int _enterR;
  int _enterTime;
  int _compliment;
  int _scoreScreenTimer;

  std::vector<Particle*> _particleList;
  ShipList _shipList;
  ShipList _playerList;
  ShipList _collisionList;

  int _controllersConnected;
  bool _controllersDialog;
  bool _firstControllersDialog;

  Overmind* _overmind;
  int _bossesKilled;
  int _hardModeBossesKilled;
  Lib::HighScoreTable _highScores;
  std::vector<std::string> _compliments;
  Lib::Recording _replay;

};

#endif
