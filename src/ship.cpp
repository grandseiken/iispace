#include "ship.h"
#include "z0_game.h"

Ship::Ship(const Vec2& position, bool particle, bool player, bool boss)
: _z0(0)
, _destroy(false)
, _position(position)
, _rotation(0)
, _boundingWidth(0)
, _player(player)
, _boss(boss)
, _enemyValue(1)
{
}

Ship::~Ship()
{
  for (unsigned int i = 0; i < CountShapes(); i++) {
    delete _shapeList[i];
  }
}

void Ship::AddShape(Shape* shape)
{
  _shapeList.push_back(shape);
}

void Ship::DestroyShape(std::size_t i)
{
  if (i >= CountShapes()) {
    return;
  }

  delete _shapeList[i];
  _shapeList.erase(_shapeList.begin() + i);
}

bool Ship::CheckPoint(const Vec2& v, int category) const
{
  bool aa = false;
  Vec2 a;
  for (unsigned int i = 0; i < CountShapes(); i++) {
    if (GetShape(i).GetCategory() &&
        (!category || (GetShape(i).GetCategory() & category) == category)) {
      if (!aa) {
        a = v - GetPosition();
        a.Rotate(-GetRotation());
        aa = true;
      }

      if (GetShape(i).CheckPoint(a)) {
        return true;
      }
    }
  }
  return false;
}

void Ship::Render() const
{
  for (std::size_t i = 0; i < CountShapes(); i++) {
    GetShape(i).Render(GetLib(), Vec2f(GetPosition()), z_float(GetRotation()));
  }
}

void Ship::RenderWithColour(Colour colour) const
{
  for (unsigned int i = 0; i < CountShapes(); i++) {
    GetShape(i).Render(
        GetLib(), Vec2f(GetPosition()),
        z_float(GetRotation()),
        colour & (0xffffff00 | (GetShape(i).GetColour() & 0x000000ff)));
  }
}

Shape& Ship::GetShape(std::size_t i) const
{
  return *_shapeList[i];
}

std::size_t Ship::CountShapes() const
{
  return _shapeList.size();
}

Lib& Ship::GetLib() const
{
  return _z0->GetLib();
}

void Ship::Destroy()
{
  if (IsDestroyed()) {
    return;
  }

  _destroy = true;
  for (unsigned int i = 0; i < CountShapes(); i++) {
    GetShape(i).SetCategory(0);
  }
}

void Ship::Spawn(Ship* ship) const
{
  _z0->AddShip(ship);
}

void Ship::Spawn(Particle* particle) const
{
  _z0->AddParticle(particle);
}

void Ship::Explosion(Colour c, int time, bool towards, const Vec2f& v) const
{
  for (unsigned int i = 0; i < CountShapes(); i++) {
    int n = towards ? GetLib().RandInt(2) + 1 : GetLib().RandInt(8) + 8;
    for (int j = 0; j < n; j++) {
      Vec2f pos = GetShape(i).ConvertPointf(
          Vec2f(GetPosition()), z_float(GetRotation()), Vec2f());

      Vec2f dir;
      dir.SetPolar(z_float(GetLib().RandFloat()) * 2 * M_PIf, 6.f);

      if (towards && v - pos != Vec2f()) {
        dir = v - pos;
        dir.Normalise();
        float angle =
            dir.Angle() + (z_float(GetLib().RandFloat()) - 0.5f) * M_PIf / 4;
        dir.SetPolar(angle, 6.f);
      }

      Spawn(new Particle(
          pos, c ? c : GetShape(i).GetColour(),
          dir, time + GetLib().RandInt(8)));
    }
  }
}
