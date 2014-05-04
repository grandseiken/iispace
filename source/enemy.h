#ifndef ENEMY_H
#define ENEMY_H

#include "ship.h"

// Basic enemy
//------------------------------
class Enemy : public Ship {
public:

    Enemy( const Vec2& position, int hp );
    virtual ~Enemy() { }

    virtual bool IsEnemy() const
    { return true; }

    long GetScore() const
    { return _score; }
    void SetScore( long score )
    { _score = score; }
    int GetHP() const
    { return _hp; }
    void SetDestroySound( Lib::Sound sound )
    { _destroySound = sound; }

    // Generic behaviour
    //------------------------------
    virtual void Damage( int damage, Player* source );
    virtual void Render();
    virtual void OnDestroy( bool bomb ) { }

private:

    int _hp;
    long _score;
    bool _damaged;
    Lib::Sound _destroySound;

};

// Follower enemy
//------------------------------
class Follow : public Enemy {
public:

    static const int TIME = 90;
    static const int SPEED = 2.0f;

    Follow( const Vec2& position, float radius = 10, int hp = 1 );
    virtual ~Follow() { }

    virtual void Update();

private:

    int   _timer;
    Ship* _target;
        
};

// Chaser enemy
//------------------------------
class Chaser : public Enemy {
public:

    static const int TIME = 60;
    static const float SPEED = 4.0f;

    Chaser( const Vec2& position );
    virtual ~Chaser() { }

    virtual void Update();

private:

    bool _move;
    int _timer;
    Vec2  _dir;

};

// Square enemy
//------------------------------
class Square : public Enemy {
public:

    static const float SPEED = 2.25f;

    Square( const Vec2& position, float rotation = M_PI / 2.0f );
    virtual ~Square() { }

    virtual void Update();

private:

    Vec2 _dir;

};

// Wall enemy
//------------------------------
class Wall : public Enemy {
public:

    static const float SPEED = 1.25f;
    static const int TIMER   = 80;

    Wall( const Vec2& position );
    virtual ~Wall() { }

    virtual void Update();
    virtual void OnDestroy( bool bomb );

private:

    Vec2 _dir;
    int  _timer;
    bool _rotate;

};

// Follower-spawning enemy
//------------------------------
class FollowHub : public Enemy {
public:

    static const int TIMER = 170;
    static const float SPEED = 1.0f;

    FollowHub( const Vec2& position );
    virtual ~FollowHub() { }

    virtual void Update();
    virtual void OnDestroy( bool bomb );

private:

    int _timer;
    Vec2 _dir;
    int _count;

};

// Shielder enemy
//------------------------------
class Shielder : public Enemy
{
public:

    static const float SPEED = 2.0f;
    static const int TIMER = 80;

    Shielder( const Vec2& position );
    virtual ~Shielder() { }

    virtual void Update();

private:

    Vec2 _dir;
    int _timer;
    bool _rotate;
    bool _rDir;

};

// Tractor beam enemy
//------------------------------
class Tractor : public Enemy
{
public:

    static const int TIMER = 50;
    static const float SPEED = 0.6f;
    static const float TRACTOR_SPEED = 2.5f;

    Tractor( const Vec2& position );
    virtual ~Tractor() { }

    virtual void Update();
    virtual void Render();

private:

    int  _timer;
    Vec2 _dir;

    bool _ready;
    bool _spinning;
    z0Game::ShipList _players;

};

#endif
