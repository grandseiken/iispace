#ifndef IISPACE_SRC_SHIP_H
#define IISPACE_SRC_SHIP_H

#include "shape.h"
#include "z0_game.h"

class Particle;

class Ship {
public:

  enum shape_category {
    VULNERABLE = 1,
    DANGEROUS = 2,
    SHIELD = 4,
    VULNSHIELD = 8
  };

  enum ship_category {
    SHIP_NONE = 0,
    SHIP_PLAYER = 1,
    SHIP_WALL = 2,
    SHIP_ENEMY = 4,
    SHIP_BOSS = 8,
    SHIP_POWERUP = 16,
  };

  Ship(const vec2& position, ship_category type);
  virtual ~Ship();

  void SetGame(z0Game& game)
  {
    _z0 = &game;
  }
  Lib& GetLib() const;

  void destroy();
  bool is_destroyed() const
  {
    return _destroy;
  }

  // Type-checking
  //------------------------------
  bool is_player() const
  {
    return (_type & SHIP_PLAYER) != 0;
  }

  bool is_wall() const
  {
    return (_type & SHIP_WALL) != 0;
  }

  bool is_enemy() const
  {
    return (_type & SHIP_ENEMY) != 0;
  }

  bool is_boss() const
  {
    return (_type & SHIP_BOSS) != 0;
  }

  bool is_powerup() const
  {
    return (_type & SHIP_POWERUP) != 0;
  }

  // Position and rotation
  //------------------------------
  const vec2& GetPosition() const
  {
    return _position;
  }

  void SetPosition(const vec2& position)
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
  bool CheckPoint(const vec2& v, int category = 0) const;
  void Spawn(Ship* ship) const;
  void Spawn(Particle* particle) const;

  void RenderHPBar(float fill) const
  {
    _z0->RenderHPBar(fill);
  }

  // Helpful functions
  //------------------------------
  void Move(const vec2& v)
  {
    SetPosition(GetPosition() + v);
  }

  void Explosion(colour c = 0, int time = 8,
                 bool towards = false, const flvec2& v = flvec2()) const;
  void RenderWithColour(colour colour) const;

  bool IsOnScreen() const
  {
    return
        GetPosition().x >= 0 && GetPosition().x <= Lib::WIDTH &&
        GetPosition().y >= 0 && GetPosition().y <= Lib::HEIGHT;
  }

  static vec2 GetScreenCentre()
  {
    return vec2(Lib::WIDTH / 2, Lib::HEIGHT / 2);
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

  z0Game::ShipList GetCollisionList(const vec2& point, int category) const
  {
    return _z0->GetCollisionList(point, category);
  }

  z0Game::ShipList GetShipsInRadius(const vec2& point, fixed radius) const
  {
    return _z0->GetShipsInRadius(point, radius);
  }

  z0Game::ShipList GetShips() const
  {
    return _z0->GetShips();
  }

  bool AnyCollisionList(const vec2& point, int category) const
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
        sound, 1.f, 2.f * GetPosition().x.to_float() / Lib::WIDTH - 1.f);
  }

  bool PlaySoundRandom(Lib::Sound sound, float pitch = 0.f, float volume = 1.f)
  {
    return GetLib().PlaySound(
        sound, volume * (.5f * z::rand_fixed().to_float() + .5f),
        2.f * GetPosition().x.to_float() / Lib::WIDTH - 1.f, pitch);
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

  z0Game* _z0;

  ship_category _type;
  bool _destroy;

  vec2 _position;
  fixed _rotation;
  fixed _boundingWidth;
  int _enemyValue;

  typedef std::vector< Shape* > ShapeList;
  ShapeList _shapeList;

};

// Particle effects
//------------------------------
class Particle {
public:

  Particle(const flvec2& position, colour colour,
           const flvec2& velocity, int time)
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
    flvec2 a = _position + flvec2(1, 1);
    flvec2 b = _position - flvec2(1, 1);
    _z0->GetLib().RenderRect(a, b, _colour);
  }

private:

  bool _destroy;
  flvec2 _position;
  flvec2 _velocity;
  int _timer;
  colour _colour;
  z0Game* _z0;

};

#endif
