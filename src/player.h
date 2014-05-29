#ifndef IISPACE_SRC_PLAYER_H
#define IISPACE_SRC_PLAYER_H

#include "ship.h"
#include "replay.h"

class Player : public Ship {
public:

  // Constants
  //------------------------------
  static const fixed SPEED;
  static const fixed SHOT_SPEED;
  static const int SHOT_TIMER;
  static const fixed BOMB_RADIUS;
  static const fixed BOMB_BOSSRADIUS;
  static const int BOMB_DAMAGE;
  static const int REVIVE_TIME;
  static const int SHIELD_TIME;
  static const int MAGICSHOT_COUNT;

  // Player ship
  //------------------------------
  Player(const vec2& position, int playerNumber);
  ~Player() override;

  int GetPlayerNumber() const
  {
    return _playerNumber;
  }

  static Replay replay;
  static std::size_t replay_frame;

  // Player behaviour
  //------------------------------
  void update() override;
  void render() const override;
  void damage();

  void ActivateMagicShots();
  void ActivateMagicShield();
  void ActivateBomb();

  static void UpdateFireTimer()
  {
    _fireTimer = (_fireTimer + 1) % SHOT_TIMER;
  }

  // Scoring
  //------------------------------
  long GetScore() const
  {
    return _score;
  }

  int GetDeaths() const
  {
    return _deathCounter;
  }

  void AddScore(long score);

  // Colour
  //------------------------------
  static colour GetPlayerColour(std::size_t playerNumber);
  colour GetPlayerColour() const
  {
    return GetPlayerColour(GetPlayerNumber());
  }

  // Temporary death
  //------------------------------
  static std::size_t CountKilledPlayers()
  {
    return _killQueue.size();
  }

  bool IsKilled()
  {
    return _killTimer != 0;
  }

private:

  // Internals
  //------------------------------
  int _playerNumber;
  long _score;
  int _multiplier;
  int _mulCount;
  int _killTimer;
  int _reviveTimer;
  int _magicShotTimer;
  bool _shield;
  bool _bomb;
  vec2 _tempTarget;
  int _deathCounter;

  static int _fireTimer;
  static z0Game::ShipList _killQueue;
  static z0Game::ShipList _shotSoundQueue;

};

// Player projectile
//------------------------------
class Shot : public Ship {
public:

  Shot(const vec2& position, Player* player,
       const vec2& direction, bool magic = false);
  ~Shot() override {}

  void update() override;
  void render() const override;

private:

  Player* _player;
  vec2 _velocity;
  bool _magic;
  bool _flash;

};

#endif
