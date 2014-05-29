#include "ship.h"
#include "z0_game.h"

Ship::Ship(const vec2& position, ship_category type)
  : _z0(nullptr)
  , _type(type)
  , _destroy(false)
  , _position(position)
  , _rotation(0)
  , _bounding_width(0)
  , _enemy_value(1)
{
}

Ship::~Ship()
{
}

void Ship::add_shape(Shape* shape)
{
  _shapes.emplace_back(shape);
}

void Ship::destroy_shape(std::size_t i)
{
  if (i >= count_shapes()) {
    return;
  }
  _shapes.erase(_shapes.begin() + i);
}

bool Ship::check_point(const vec2& v, int category) const
{
  bool aa = false;
  vec2 a;
  for (const auto& shape : _shapes) {
    if (shape->category &&
        (!category || (shape->category & category) == category)) {
      if (!aa) {
        a = v - position();
        a.rotate(-rotation());
        aa = true;
      }

      if (shape->check_point(a)) {
        return true;
      }
    }
  }
  return false;
}

void Ship::render() const
{
  for (const auto& shape : _shapes) {
    shape->render(lib(), to_float(position()), rotation().to_float());
  }
}

void Ship::render_with_colour(colour_t colour) const
{
  for (const auto& shape : _shapes) {
    shape->render(
        lib(), to_float(position()), rotation().to_float(),
        colour & (0xffffff00 | (shape->colour & 0x000000ff)));
  }
}

Shape& Ship::get_shape(std::size_t i) const
{
  return *_shapes[i];
}

std::size_t Ship::count_shapes() const
{
  return _shapes.size();
}

void Ship::destroy()
{
  if (is_destroyed()) {
    return;
  }

  _destroy = true;
  for (const auto& shape : _shapes) {
    shape->category = 0;
  }
}

void Ship::spawn(Ship* ship) const
{
  _z0->AddShip(ship);
}

void Ship::spawn(const Particle& particle) const
{
  _z0->AddParticle(particle);
}

void Ship::explosion(colour_t c, int time, bool towards, const flvec2& v) const
{
  for (const auto& shape : _shapes) {
    int n = towards ? z::rand_int(2) + 1 : z::rand_int(8) + 8;
    for (int j = 0; j < n; j++) {
      flvec2 pos = shape->convert_fl_point(
          to_float(position()), rotation().to_float(), flvec2());

      flvec2 dir;
      dir.set_polar(z::rand_fixed().to_float() * 2 * M_PIf, 6.f);

      if (towards && v - pos != flvec2()) {
        dir = v - pos;
        dir.normalise();
        float angle =
            dir.angle() + (z::rand_fixed().to_float() - 0.5f) * M_PIf / 4;
        dir.set_polar(angle, 6.f);
      }

      spawn(Particle(pos, c ? c : shape->colour, dir, time + z::rand_int(8)));
    }
  }
}
