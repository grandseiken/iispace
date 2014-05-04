#ifndef POWERUP_H
#define POWERUP_H

#include "ship.h"

// Basic powerup
//------------------------------
class Powerup : public Ship 
{
public:
    
    static const float SPEED = 1.0f;
    static const int   TIME  = 100;

    Powerup( const Vec2& position );
    virtual ~Powerup() { }

    virtual void Update();
    virtual void Damage( int damage, Player* source );
    virtual bool IsPowerup() const
    { return true; }

private:

    virtual void OnGet( Player* player ) = 0;

    int  _frame;
    Vec2 _dir;
    bool _rotate;
    bool _firstFrame;

};

// Extra life
//------------------------------
class ExtraLife : public Powerup
{
public:

    ExtraLife( const Vec2& position );
    virtual ~ExtraLife() { }

private:

    virtual void OnGet( Player* player );

};

// Magic shots
//------------------------------
class MagicShots : public Powerup
{
public:

    MagicShots( const Vec2& position );
    virtual ~MagicShots() { }

private:

    virtual void OnGet( Player* player );

};

// Magic shield
//------------------------------
class MagicShield : public Powerup
{
public:

    MagicShield( const Vec2& position );
    virtual ~MagicShield() { }

private:

    virtual void OnGet( Player* player );

};

// Bomb
//------------------------------
class Bomb : public Powerup
{
public:

    Bomb( const Vec2& position );
    virtual ~Bomb() { }

private:

    virtual void OnGet( Player* player );

};

#endif
