#include "ship.h"
#include "z0_game.h"

Ship::Ship(const vec2& position, ship_category type)
: _game(nullptr)
, _type(type)
, _destroy(false)
, _shape(position, 0)
, _bounding_width(0)
, _enemy_value(1) {}

Ship::~Ship() {}

bool Ship::check_point(const vec2& v, int32_t category) const {
  bool aa = false;
  vec2 a;
  for (const auto& shape : _shape.shapes()) {
    if (shape->category && (!category || (shape->category & category) == category)) {
      if (!aa) {
        a = (v - _shape.centre).rotated(-_shape.rotation());
        aa = true;
      }

      if (shape->check_point(a)) {
        return true;
      }
    }
  }
  return false;
}

void Ship::render() const {
  for (const auto& shape : _shape.shapes()) {
    shape->render(lib(), to_float(_shape.centre), _shape.rotation().to_float());
  }
}

void Ship::render_with_colour(colour_t colour) const {
  for (const auto& shape : _shape.shapes()) {
    shape->render(lib(), to_float(_shape.centre), _shape.rotation().to_float(),
                  colour & (0xffffff00 | (shape->colour & 0x000000ff)));
  }
}

void Ship::destroy() {
  if (is_destroyed()) {
    return;
  }

  _destroy = true;
  for (const auto& shape : _shape.shapes()) {
    shape->category = 0;
  }
}

void Ship::spawn(Ship* ship) const {
  _game->add_ship(ship);
}

void Ship::spawn(const Particle& particle) const {
  _game->add_particle(particle);
}

void Ship::explosion(colour_t c, int32_t time, bool towards, const flvec2& v) const {
  for (const auto& shape : _shape.shapes()) {
    int32_t n = towards ? z::rand_int(2) + 1 : z::rand_int(8) + 8;
    for (int32_t j = 0; j < n; j++) {
      flvec2 pos =
          shape->convert_fl_point(to_float(_shape.centre), _shape.rotation().to_float(), flvec2());

      flvec2 dir = flvec2::from_polar(z::rand_fixed().to_float() * 2 * M_PIf, 6.f);

      if (towards && v - pos != flvec2()) {
        dir = (v - pos).normalised();
        float angle = dir.angle() + (z::rand_fixed().to_float() - 0.5f) * M_PIf / 4;
        dir = flvec2::from_polar(angle, 6.f);
      }

      spawn(Particle(pos, c ? c : shape->colour, dir, time + z::rand_int(8)));
    }
  }
}

const CompoundShape::shape_list& Ship::shapes() const {
  return _shape.shapes();
}

void Ship::add_shape(Shape* shape) {
  _shape.add_shape(shape);
}

void Ship::destroy_shape(std::size_t index) {
  _shape.destroy_shape(index);
}

void Ship::clear_shapes() {
  _shape.clear_shapes();
}