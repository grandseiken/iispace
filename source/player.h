#ifndef PLAYER_H
#define PLAYER_H

#include "ship.h"

class Player : public Ship {
public:

    // Constants
    //------------------------------
    static const float SPEED           = 5.0f;
    static const float SHOT_SPEED      = 10.0f;
    static const int   SHOT_TIMER      = 4;
    static const float BOMB_RADIUS     = 180.0f;
    static const int   BOMB_DAMAGE     = 50;
    static const int   REVIVE_TIME     = 100;
    static const int   SHIELD_TIME     = 50;
    static const int   MAGICSHOT_COUNT = 120;

    // Player ship
    //------------------------------
    Player( const Vec2& position, int playerNumber );
    virtual ~Player();

    virtual bool IsPlayer() const
    { return true; }
    int GetPlayerNumber() const
    { return _playerNumber; }

    // Player behaviour
    //------------------------------
    virtual void Update();
    virtual void Render();
    virtual void Damage();

    void ActivateMagicShots();
    void ActivateMagicShield();
    void ActivateBomb();

    static void UpdateFireTimer()
    { _fireTimer = ( _fireTimer + 1 ) % SHOT_TIMER; }

    // Scoring
    //------------------------------
    long GetScore() const
    { return _score; }
    void AddScore( long score );

    // Colour
    //------------------------------
    static Colour GetPlayerColour( int playerNumber );
    Colour GetPlayerColour() const
    { return GetPlayerColour( GetPlayerNumber() ); }

    // Temporary death
    //------------------------------
    static int CountKilledPlayers()
    { return _killQueue.size(); }
    bool IsKilled()
    { return _killTimer; }

private:

    // Internals
    //------------------------------
    int  _playerNumber;
    long _score;
    int  _multiplier;
    int  _mulCount;
    int  _killTimer;
    int  _reviveTimer;
    int  _magicShotTimer;
    bool _shield;
    bool _bomb;

    static int              _fireTimer;
    static z0Game::ShipList _killQueue;
    static z0Game::ShipList _shotSoundQueue;

};

// Player projectile
//------------------------------
class Shot : public Ship {
public:

    Shot( const Vec2& position, Player* player, const Vec2& direction, bool magic = false );
    virtual ~Shot() { }

    virtual void Update();
    virtual void Render();

private:

    Player* _player;
    Vec2 _velocity;
    bool _magic;
    bool _flash;

};

#endif
