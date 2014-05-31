#ifndef IISPACE_SRC_Z0_GAME_H
#define IISPACE_SRC_Z0_GAME_H

#include <memory>

#include "lib.h"
#include "replay.h"
#include "save.h"
#include "z.h"
class Overmind;
struct Particle;
class Player;
class Ship;

#define ALLOWED_CHARS "ABCDEFGHiJKLMNOPQRSTUVWXYZ 1234567890! "

struct score_finished {};
class z0Game {
public:

  // Constants
  //------------------------------
  enum game_state {
    STATE_MENU,
    STATE_PAUSE,
    STATE_GAME,
    STATE_HIGHSCORE,
  };

  enum game_mode {
    NORMAL_MODE,
    BOSS_MODE,
    HARD_MODE,
    FAST_MODE,
    WHAT_MODE,
  };

  enum boss_list {
    BOSS_1A = 1,
    BOSS_1B = 2,
    BOSS_1C = 4,
    BOSS_2A = 8,
    BOSS_2B = 16,
    BOSS_2C = 32,
    BOSS_3A = 64,
  };

  static const colour_t PANEL_TEXT = 0xeeeeeeff;
  static const colour_t PANEL_TRAN = 0xeeeeee99;
  static const colour_t PANEL_BACK = 0x000000ff;
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

  game_mode mode() const
  {
    return _mode;
  }

  // Ships
  //------------------------------
  void AddShip(Ship* ship);
  void AddParticle(const Particle& particle);
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

  void set_boss_killed(boss_list boss);

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
    return (_save.bosses_killed & 63) == 63;
  }

  bool IsHardModeUnlocked() const
  {
    return IsBossModeUnlocked() &&
        (_save.high_scores[Lib::PLAYERS][0].score > 0 ||
         _save.high_scores[Lib::PLAYERS][1].score > 0 ||
         _save.high_scores[Lib::PLAYERS][2].score > 0 ||
         _save.high_scores[Lib::PLAYERS][3].score > 0);
  }

  bool IsFastModeUnlocked() const
  {
    return IsHardModeUnlocked() && ((_save.hard_mode_bosses_killed & 63) == 63);
  }

  bool IsWhatModeUnlocked() const
  {
    return IsHardModeUnlocked() && ((_save.hard_mode_bosses_killed & 64) == 64);
  }

  std::string ConvertToTime(int64_t score) const;

  // Scores
  //------------------------------
  int64_t GetPlayerScore(int32_t playerNumber) const;
  int64_t GetPlayerDeaths(int32_t playerNumber) const;
  int64_t GetTotalScore() const;
  bool IsHighScore() const;

  void NewGame(bool canFaceSecretBoss, bool replay, game_mode mode);
  void EndGame();

  Lib& _lib;
  game_state _state;
  int32_t _players;
  int32_t _lives;
  game_mode _mode;
  int32_t _frame_count;

  mutable bool _showHPBar;
  mutable float _fillHPBar;

  int32_t _selection;
  int32_t _specialSelection;
  int32_t _killTimer;
  int32_t _exitTimer;

  std::string _enterName;
  int32_t _enterChar;
  int32_t _enterR;
  int32_t _enterTime;
  int32_t _compliment;
  int32_t _scoreScreenTimer;

  std::vector<Particle> _particles;
  std::vector<std::unique_ptr<Ship>> _ships;
  ShipList _playerList;
  ShipList _collisions;

  int32_t _controllersConnected;
  bool _controllersDialog;
  bool _firstControllersDialog;

  std::unique_ptr<Overmind> _overmind;
  std::vector<std::string> _compliments;
  SaveData _save;
  Settings _settings;

};

#endif
