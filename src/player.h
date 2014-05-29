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
  static const int32_t SHOT_TIMER;
  static const fixed BOMB_RADIUS;
  static const fixed BOMB_BOSSRADIUS;
  static const int32_t BOMB_DAMAGE;
  static const int32_t REVIVE_TIME;
  static const int32_t SHIELD_TIME;
  static const int32_t MAGICSHOT_COUNT;

  // Player ship
  //------------------------------
  Player(const vec2& position, int32_t playerNumber);
  ~Player() override;

  int32_t GetPlayerNumber() const
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
  int64_t GetScore() const
  {
    return _score;
  }

  int32_t GetDeaths() const
  {
    return _deathCounter;
  }

  void AddScore(int64_t score);

  // Colour
  //------------------------------
  static colour_t GetPlayerColour(std::size_t playerNumber);
  colour_t GetPlayerColour() const
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
  int32_t _playerNumber;
  int64_t _score;
  int32_t _multiplier;
  int32_t _mulCount;
  int32_t _killTimer;
  int32_t _reviveTimer;
  int32_t _magicShotTimer;
  bool _shield;
  bool _bomb;
  vec2 _tempTarget;
  int32_t _deathCounter;

  static int32_t _fireTimer;
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
