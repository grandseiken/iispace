#ifndef IISPACE_SRC_PLAYER_H
#define IISPACE_SRC_PLAYER_H

#include "ship.h"

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
  Player(const Vec2& position, int playerNumber);
  virtual ~Player();

  int GetPlayerNumber() const
  {
    return _playerNumber;
  }

  // Player behaviour
  //------------------------------
  virtual void Update();
  virtual void Render() const;
  virtual void Damage();

  void ActivateMagicShots();
  void ActivateMagicShield();
  void ActivateBomb();

  static void UpdateFireTimer()
  {
    _fireTimer = (_fireTimer + 1) % SHOT_TIMER;
  }

  static void SetReplay(Lib::Recording replay)
  {
    _replay = replay;
    _replayFrame = 0;
  }

  static void DisableReplay()
  {
    _replay._okay = false;
  }

  static bool IsReplaying()
  {
    return _replay._okay;
  }

  static std::size_t ReplayFrame()
  {
    return _replayFrame;
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
  Vec2 _tempTarget;
  int _deathCounter;

  static int _fireTimer;
  static z0Game::ShipList _killQueue;
  static z0Game::ShipList _shotSoundQueue;
  static Lib::Recording _replay;
  static std::size_t _replayFrame;

};

// Player projectile
//------------------------------
class Shot : public Ship {
public:

  Shot(const Vec2& position, Player* player,
       const Vec2& direction, bool magic = false);
  virtual ~Shot() {}

  virtual void Update();
  virtual void Render() const;

private:

  Player* _player;
  Vec2 _velocity;
  bool _magic;
  bool _flash;

};

#endif
