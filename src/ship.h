#ifndef IISPACE_SRC_SHIP_H
#define IISPACE_SRC_SHIP_H

#include "shape.h"
#include "z0_game.h"

class Particle;

class Ship {
public:

  // Collision masks
  //------------------------------
  enum Category {
    VULNERABLE = 1,
    ENEMY = 2,
    SHIELD = 4,
    VULNSHIELD = 8
  };

  // Creation and destruction
  //------------------------------
  Ship(const Vec2& position, bool particle = false,
       bool player = false, bool boss = false);
  virtual ~Ship();

  void SetGame(z0Game& game)
  {
    _z0 = &game;
  }
  Lib& GetLib() const;

  void Destroy();
  bool IsDestroyed() const
  {
    return _destroy;
  }

  // Type-checking overrides
  //------------------------------
  bool IsPlayer() const
  {
    return _player;
  }

  virtual bool IsWall() const
  {
    return false;
  }

  virtual bool IsEnemy() const
  {
    return false;
  }

  bool IsBoss() const
  {
    return _boss;
  }

  virtual bool IsPowerup() const
  {
    return false;
  }

  // Position and rotation
  //------------------------------
  const Vec2& GetPosition() const
  {
    return _position;
  }

  void SetPosition(const Vec2& position)
  {
    _position = position;
  }

  fixed GetRotation() const
  {
    return _rotation;
  }

  void SetRotation(fixed rotation)
  {
    _rotation =
        rotation > 2 * fixed::pi ? rotation - 2 * fixed::pi :
        rotation < 0 ? rotation + 2 * fixed::pi : rotation;
  }

  void Rotate(fixed rotation)
  {
    SetRotation(GetRotation() + rotation);
  }

  fixed GetBoundingWidth() const
  {
    return _boundingWidth;
  }

  // Operations
  //------------------------------
  bool CheckPoint(const Vec2& v, int category = 0) const;
  void Spawn(Ship* ship) const;
  void Spawn(Particle* particle) const;

  void RenderHPBar(float fill) const
  {
    _z0->RenderHPBar(fill);
  }

  // Helpful functions
  //------------------------------
  void Move(const Vec2& v)
  {
    SetPosition(GetPosition() + v);
  }

  void Explosion(Colour c = 0, int time = 8,
                 bool towards = false, const Vec2f& v = Vec2f()) const;
  void RenderWithColour(Colour colour) const;

  bool IsOnScreen() const
  {
    return
        GetPosition()._x >= 0 && GetPosition()._x <= Lib::WIDTH &&
        GetPosition()._y >= 0 && GetPosition()._y <= Lib::HEIGHT;
  }

  static Vec2 GetScreenCentre()
  {
    return Vec2(Lib::WIDTH / 2, Lib::HEIGHT / 2);
  }

  void AddLife()
  {
    _z0->AddLife();
  }

  void SubLife()
  {
    _z0->SubLife();
  }

  int GetLives() const
  {
    return _z0->GetLives();
  }

  int GetNonWallCount() const
  {
    return _z0->GetNonWallCount();
  }

  z0Game::ShipList GetCollisionList(const Vec2& point, int category) const
  {
    return _z0->GetCollisionList(point, category);
  }

  z0Game::ShipList GetShipsInRadius(const Vec2& point, fixed radius) const
  {
    return _z0->GetShipsInRadius(point, radius);
  }

  z0Game::ShipList GetShips() const
  {
    return _z0->GetShips();
  }

  bool AnyCollisionList(const Vec2& point, int category) const
  {
    return _z0->AnyCollisionList(point, category);
  }

  int CountPlayers() const
  {
    return _z0->CountPlayers();
  }

  Player* GetNearestPlayer() const
  {
    return _z0->GetNearestPlayer(GetPosition());
  }

  z0Game::ShipList GetPlayers() const
  {
    return _z0->GetPlayers();
  }

  bool IsBossMode() const
  {
    return _z0->IsBossMode();
  }

  bool IsHardMode() const
  {
    return _z0->IsHardMode();
  }

  bool IsFastMode() const
  {
    return _z0->IsFastMode();
  }

  bool IsWhatMode() const
  {
    return _z0->IsWhatMode();
  }

  void SetBossKilled(z0Game::BossList boss)
  {
    _z0->SetBossKilled(boss);
  }

  bool PlaySound(Lib::Sound sound)
  {
    return GetLib().PlaySound(
        sound, 1.f, 2.f * GetPosition()._x.to_float() / Lib::WIDTH - 1.f);
  }

  bool PlaySoundRandom(Lib::Sound sound, float pitch = 0.f, float volume = 1.f)
  {
    return GetLib().PlaySound(
        sound, volume * (.5f * GetLib().RandFloat().to_float() + .5f),
        2.f * GetPosition()._x.to_float() / Lib::WIDTH - 1.f, pitch);
  }

  int GetEnemyValue() const
  {
    return _enemyValue;
  }

  void SetEnemyValue(int enemyValue)
  {
    _enemyValue = enemyValue;
  }

  // Generic behaviour
  //------------------------------
  virtual void Update() = 0;
  virtual void Render() const;
  // Player can be 0
  virtual void Damage(int damage, bool magic, Player* source) {}

protected:

  // Shape control
  //------------------------------
  void AddShape(Shape* shape);
  void DestroyShape(std::size_t i);
  Shape& GetShape(std::size_t i) const;
  std::size_t CountShapes() const;
  void SetBoundingWidth(fixed width)
  {
    _boundingWidth = width;
  }

private:

  // Internals
  //------------------------------
  z0Game* _z0;

  bool _destroy;
  Vec2 _position;
  fixed _rotation;
  fixed _boundingWidth;
  bool _player;
  bool _boss;
  int _enemyValue;

  typedef std::vector< Shape* > ShapeList;
  ShapeList _shapeList;

};

// Particle effects
//------------------------------
class Particle {
public:

  Particle(const Vec2f& position, Colour colour,
           const Vec2f& velocity, int time)
    : _destroy(false)
    , _position(position)
    , _velocity(velocity)
    , _timer(time)
    , _colour(colour)
    , _z0(0)
  {
  }

  void SetGame(z0Game& game)
  {
    _z0 = &game;
  }

  bool IsDestroyed() const
  {
    return _destroy;
  }

  void Update()
  {
    _position += _velocity;
    --_timer;
    if (_timer <= 0) {
      _destroy = true;
      return;
    }
  }

  void Render() const
  {
    Vec2f a = _position + Vec2f(1, 1);
    Vec2f b = _position - Vec2f(1, 1);
    _z0->GetLib().RenderRect(a, b, _colour);
  }

private:

  bool _destroy;
  Vec2f _position;
  Vec2f _velocity;
  int _timer;
  Colour _colour;
  z0Game* _z0;

};

#endif
