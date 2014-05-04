#ifndef SHIP_H
#define SHIP_H

#include "shape.h"
#include "z0game.h"

class Ship {
public:

    // Collision masks
    //------------------------------
    enum Category {
        VULNERABLE = 1,
        ENEMY      = 2,
        SHIELD     = 4
    };

    // Creation and destruction
    //------------------------------
    Ship( const Vec2& position );
    virtual ~Ship();

    void SetGame( z0Game& game )
    { _z0 = &game; }
    Lib& GetLib() const;

    void Destroy();
    bool IsDestroyed() const
    { return _destroy; }

    // Type-checking overrides
    //------------------------------
    virtual bool IsPlayer() const
    { return false; }
    virtual bool IsEnemy() const
    { return false; }
    virtual bool IsPowerup() const
    { return false; }

    // Position and rotation
    //------------------------------
    Vec2 GetPosition() const
    { return _position; }
    void SetPosition( const Vec2& position )
    { _position = position; }

    float GetRotation() const
    { return _rotation; }
    void SetRotation( float rotation )
    { _rotation = ( ( rotation > 2.0f * M_PI ) ? ( rotation - 2.0f * M_PI ) : ( rotation < 0 ? rotation + 2.0f * M_PI : rotation ) ); }
    void Rotate( float rotation )
    { SetRotation( GetRotation() + rotation ); }

    float GetBoundingWidth() const { return _boundingWidth; }

    // Operations
    //------------------------------
    bool CheckPoint( const Vec2& v, int category = 0 ) const;
    void Spawn( Ship* ship ) const;
    void RenderHPBar( float fill )
    { _z0->RenderHPBar( fill ); }

    // Helpful functions
    //------------------------------
    void Move( const Vec2& v )
    { SetPosition( GetPosition() + v ); }
    void Explosion( Colour c = 0, int time = 8 ) const;
    void RenderWithColour( Colour colour );
    bool IsOnScreen() const
    {
        return GetPosition()._x >= 0 && GetPosition()._x <= Lib::WIDTH &&
               GetPosition()._y >= 0 && GetPosition()._y <= Lib::HEIGHT;
    }
    static Vec2 GetScreenCentre()
    { return Vec2( Lib::WIDTH / 2.0f, Lib::HEIGHT / 2.0f ); }

    void AddLife()
    { _z0->AddLife(); }
    void SubLife()
    { _z0->SubLife(); }
    int GetLives() const
    { return _z0->GetLives(); }

    z0Game::ShipList GetCollisionList( const Vec2& point, int category ) const
    { return _z0->GetCollisionList( point, category ); }
    z0Game::ShipList GetShipsInRadius( const Vec2& point, float radius ) const
    { return _z0->GetShipsInRadius( point, radius ); }
    bool             AnyCollisionList( const Vec2& point, int category ) const
    { return _z0->AnyCollisionList( point, category ); }

    int CountPlayers() const
    { return _z0->CountPlayers(); }
    Player* GetNearestPlayer() const
    { return _z0->GetNearestPlayer( GetPosition() ); }
    z0Game::ShipList GetPlayers() const
    { return _z0->GetPlayers(); }
    bool IsBossMode() const
    { return _z0->IsBossMode(); }
    void SetBossKilled( z0Game::BossList boss )
    { _z0->SetBossKilled( boss ); }

    bool PlaySound( Lib::Sound sound )
    { return GetLib().PlaySound( sound, 1.0f, 2.0f * GetPosition()._x / Lib::WIDTH - 1.0f ); }
    bool PlaySoundRandom( Lib::Sound sound, float pitch = 0.0f )
    { return GetLib().PlaySound( sound, 0.5f * GetLib().RandFloat() + 0.5f, 2.0f * GetPosition()._x / Lib::WIDTH - 1.0f, pitch ); }

    // Generic behaviour
    //------------------------------
    virtual void Update() = 0;
    virtual void Render();
    // Player can be 0
    virtual void Damage( int damage, Player* source ) { }

protected:

    // Shape control
    //------------------------------
    void AddShape( Shape* shape );
    void DestroyShape( int i );
    Shape& GetShape( int i ) const;
    unsigned int CountShapes() const;
    void SetBoundingWidth( float width )
    { _boundingWidth = width; }

private:

    // Internals
    //------------------------------
    z0Game* _z0;

    bool  _destroy;
    Vec2  _position;
    float _rotation;
    float _boundingWidth;

    typedef std::vector< Shape* > ShapeList;
    ShapeList _shapeList;

};

// Particle effects
//------------------------------
class Particle : public Ship {
public:

    Particle( const Vec2& position, Colour colour, const Vec2& velocity, int time )
    : Ship( position ), _velocity( velocity ), _timer( time ) {
        AddShape( new Fill( Vec2(), 1.f, 1.f, colour ) );
    }
    virtual ~Particle() { }

    virtual void Update()
    {
        Move( _velocity );
        _timer--;
        Vec2 p = GetPosition();
        if ( !IsOnScreen() || !_timer ) {
            Destroy();
            return;
        }
    }

private:

    Vec2 _velocity;
    int _timer;

};

#endif
