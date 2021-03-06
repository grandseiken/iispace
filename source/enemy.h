#ifndef ENEMY_H
#define ENEMY_H

#include "ship.h"

// Basic enemy
//------------------------------
class Enemy : public Ship {
public:

    Enemy( const Vec2& position, int hp, bool explodeOnDestroy = true );
    virtual ~Enemy() { }

    virtual bool IsEnemy() const
    {
        return true;
    }

    long GetScore() const
    {
        return _score;
    }
    void SetScore( long score )
    {
        _score = score;
    }
    int GetHP() const
    {
        return _hp;
    }
    void SetDestroySound( Lib::Sound sound )
    { 
        _destroySound = sound;
    }

    // Generic behaviour
    //------------------------------
    virtual void Damage( int damage, bool magic, Player* source );
    virtual void Render() const;
    virtual void OnDestroy( bool bomb ) { }

private:

    int _hp;
    long _score;
    mutable int _damaged;
    Lib::Sound _destroySound;
    bool _explodeOnDestroy;

};

// Follower enemy
//------------------------------
class Follow : public Enemy {
public:

    static const int TIME;
    static const fixed SPEED;

    Follow( const Vec2& position, fixed radius = 10, int hp = 1 );
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

    static const int TIME;
    static const fixed SPEED;

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

    static const fixed SPEED;

    Square( const Vec2& position, fixed rotation = M_PI / M_TWO );
    virtual ~Square() {}

    virtual void Update();
    virtual void Render() const;
    virtual bool IsWall() const
    {
        return true;
    }

private:

    Vec2 _dir;
    int _timer;

};

// Wall enemy
//------------------------------
class Wall : public Enemy {
public:

    static const int TIMER;
    static const fixed SPEED;

    Wall( const Vec2& position, bool rdir );
    virtual ~Wall() {}

    virtual void Update();
    virtual void OnDestroy( bool bomb );
    virtual bool IsWall() const
    {
        return true;
    }

private:

    Vec2 _dir;
    int  _timer;
    bool _rotate;
    bool _rdir;

};

// Follower-spawning enemy
//------------------------------
class FollowHub : public Enemy {
public:

    static const int TIMER;
    static const fixed SPEED;

    FollowHub( const Vec2& position, bool powerA = false, bool powerB = false );
    virtual ~FollowHub() { }

    virtual void Update();
    virtual void OnDestroy( bool bomb );

private:

    int _timer;
    Vec2 _dir;
    int _count;
    bool _powerA;
    bool _powerB;

};

// Shielder enemy
//------------------------------
class Shielder : public Enemy
{
public:

    static const fixed SPEED;
    static const int TIMER;

    Shielder( const Vec2& position, bool power = false );
    virtual ~Shielder() { }

    virtual void Update();

private:

    Vec2 _dir;
    int _timer;
    bool _rotate;
    bool _rDir;
    bool _power;

};

// Tractor beam enemy
//------------------------------
class Tractor : public Enemy
{
public:

    static const int TIMER;
    static const fixed SPEED;
    static const fixed TRACTOR_SPEED;

    Tractor( const Vec2& position, bool power = false );
    virtual ~Tractor() { }

    virtual void Update();
    virtual void Render() const;

private:

    int  _timer;
    Vec2 _dir;
    bool _power;

    bool _ready;
    bool _spinning;
    z0Game::ShipList _players;

};

#endif
