#include "ship.h"
#include "z0_game.h"

Ship::Ship(const vec2& position, ship_category type)
  : _z0(nullptr)
  , _type(type)
  , _destroy(false)
  , _position(position)
  , _rotation(0)
  , _boundingWidth(0)
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

bool Ship::CheckPoint(const vec2& v, int category) const
{
  bool aa = false;
  vec2 a;
  for (unsigned int i = 0; i < CountShapes(); i++) {
    if (GetShape(i).GetCategory() &&
        (!category || (GetShape(i).GetCategory() & category) == category)) {
      if (!aa) {
        a = v - GetPosition();
        a.rotate(-GetRotation());
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
    GetShape(i).Render(
        GetLib(), to_float(GetPosition()), GetRotation().to_float());
  }
}

void Ship::RenderWithColour(colour colour) const
{
  for (unsigned int i = 0; i < CountShapes(); i++) {
    GetShape(i).Render(
        GetLib(), to_float(GetPosition()),
        GetRotation().to_float(),
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

void Ship::destroy()
{
  if (is_destroyed()) {
    return;
  }

  _destroy = true;
  for (std::size_t i = 0; i < CountShapes(); i++) {
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

void Ship::Explosion(colour c, int time, bool towards, const flvec2& v) const
{
  for (unsigned int i = 0; i < CountShapes(); i++) {
    int n = towards ? z::rand_int(2) + 1 : z::rand_int(8) + 8;
    for (int j = 0; j < n; j++) {
      flvec2 pos = GetShape(i).ConvertPointf(
          to_float(GetPosition()), GetRotation().to_float(), flvec2());

      flvec2 dir;
      dir.set_polar(z::rand_fixed().to_float() * 2 * M_PIf, 6.f);

      if (towards && v - pos != flvec2()) {
        dir = v - pos;
        dir.normalise();
        float angle =
            dir.angle() + (z::rand_fixed().to_float() - 0.5f) * M_PIf / 4;
        dir.set_polar(angle, 6.f);
      }

      Spawn(new Particle(
          pos, c ? c : GetShape(i).GetColour(),
          dir, time + z::rand_int(8)));
    }
  }
}
