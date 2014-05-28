#ifndef IISPACE_SRC_Z0_GAME_H
#define IISPACE_SRC_Z0_GAME_H

#include <memory>

#include "lib.h"
#include "z.h"
class Overmind;
class Particle;
class Player;
class Ship;

#define ALLOWED_CHARS "ABCDEFGHiJKLMNOPQRSTUVWXYZ 1234567890! "

class z0Game {
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

  static const colour PANEL_TEXT = 0xeeeeeeff;
  static const colour PANEL_TRAN = 0xeeeeee99;
  static const colour PANEL_BACK = 0x000000ff;
  static const int STARTING_LIVES;
  static const int BOSSMODE_LIVES;

  typedef std::vector<Ship*> ShipList;

  z0Game(Lib& lib, std::vector<std::string> args);
  ~z0Game();

  // Main functions
  //------------------------------
  void Run();
  void Update();
  void Render() const;

  Lib& lib() const
  {
    return _lib;
  }

  // Ships
  //------------------------------
  void AddShip(Ship* ship);
  void AddParticle(Particle* particle);
  int32_t get_non_wall_count() const;

  ShipList all_ships(int32_t ship_mask = 0) const;
  ShipList ships_in_radius(const vec2& point, fixed radius,
                           int32_t ship_mask = 0) const;
  ShipList collision_list(const vec2& point, int32_t category) const;
  bool any_collision(const vec2& point, int32_t category) const;

  // Players
  //------------------------------
  int32_t count_players() const
  {
    return _players;
  }

  int32_t alive_players() const;
  int32_t killed_players() const;

  Player* nearest_player(const vec2& point) const;

  ShipList get_players() const
  {
    return _playerList;
  }

  bool is_boss_mode() const
  {
    return _bossMode;
  }

  bool is_hard_mode() const
  {
    return _hardMode;
  }

  bool is_fast_mode() const
  {
    return _fastMode;
  }

  bool is_what_mode() const
  {
    return _whatMode;
  }

  void set_boss_killed(BossList boss);

  void RenderHPBar(float fill)
  {
    _showHPBar = true; _fillHPBar = fill;
  }

  // Lives
  //------------------------------
  void add_life()
  {
    _lives++;
  }

  void sub_life()
  {
    if (_lives) {
      _lives--;
    }
  }

  int32_t get_lives() const
  {
    return _lives;
  }

private:

  // Internals
  //------------------------------
  void RenderPanel(const flvec2& low, const flvec2& hi) const;
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

  std::string ConvertToTime(uint64_t score) const;

  // Scores
  //------------------------------
  uint64_t GetPlayerScore(int32_t playerNumber) const;
  uint64_t GetPlayerDeaths(int32_t playerNumber) const;
  uint64_t GetTotalScore() const;
  bool IsHighScore() const;

  void NewGame(bool canFaceSecretBoss, bool bossMode = false,
               bool replay = false, bool hardMode = false,
               bool fastMode = false, bool whatMode = false);
  void EndGame();

  Lib& _lib;
  GameState _state;
  int32_t _players;
  int32_t _lives;
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

  std::vector<std::unique_ptr<Particle>> _particles;
  std::vector<std::unique_ptr<Ship>> _ships;
  ShipList _playerList;
  ShipList _collisions;

  int _controllersConnected;
  bool _controllersDialog;
  bool _firstControllersDialog;

  std::unique_ptr<Overmind> _overmind;
  int _bossesKilled;
  int _hardModeBossesKilled;
  Lib::HighScoreTable _highScores;
  std::vector<std::string> _compliments;
  Lib::Recording _replay;

};

#endif
