#ifndef BOSSENEMY_H
#define BOSSENEMY_H

#include "Enemy.h"

// Big follow
//------------------------------
class BigFollow : public Follow
{
public:

    BigFollow( const Vec2& position );
    virtual void OnDestroy( bool bomb );

};

// Generic boss shot
//------------------------------
class SBBossShot : public Enemy
{
public:

    SBBossShot( const Vec2& position, const Vec2& velocity, Colour c = 0x999999ff );
    virtual ~SBBossShot() { }

    virtual void Update();

private:

    Vec2 _dir;

};

// Tractor beam shot
//------------------------------
class TBossShot : public Enemy
{
public:

    TBossShot( const Vec2& position, float angle );
    virtual ~TBossShot() { }

    virtual void Update();

private:

    Vec2 _dir;

};

// Ghost boss wall
//------------------------------
class GhostWall : public Enemy
{
public:

    static const float SPEED = 3.0f;

    GhostWall( bool swap, bool noGap, bool ignored );
    GhostWall( bool swap, bool swapGap );
    virtual ~GhostWall() { }

    virtual void Update();

private:

    Vec2 _dir;

};

// Ghost boss mine
//------------------------------
class GhostMine : public Enemy
{
public:

    GhostMine( const Vec2& position, Ship* ghost );
    virtual ~GhostMine() { }

    virtual void Update();
    virtual void Render();

private:

    int _timer;
    Ship* _ghost;

};

// Death ray
//------------------------------
class DeathRay : public Enemy
{
public:

    static const float SPEED = 10.0f;

    DeathRay( const Vec2& position );
    virtual ~DeathRay() { }

    virtual void Update();

};

// Death ray boss arm
//------------------------------
class DeathArm : public Enemy
{
public:

    DeathArm( DeathRayBoss* boss, bool top, int hp );
    virtual ~DeathArm() { }

    virtual void Update();
    virtual void OnDestroy( bool bomb );

private:

    DeathRayBoss* _boss;
    bool          _top;
    int           _timer;
    bool          _attacking;
    Vec2          _dir;
    int           _start;

};

#endif
