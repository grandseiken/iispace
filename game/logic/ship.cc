#include "game/logic/ship.h"
#include "game/core/z0_game.h"

Ship::Ship(const vec2& position, ship_category type)
: game_(nullptr)
, type_(type)
, destroy_(false)
, shape_(position, 0)
, bounding_width_(0)
, enemy_value_(1) {}

Ship::~Ship() {}

bool Ship::check_point(const vec2& v, std::int32_t category) const {
  bool aa = false;
  vec2 a;
  for (const auto& shape : shape_.shapes()) {
    if (shape->category && (!category || (shape->category & category) == category)) {
      if (!aa) {
        a = (v - shape_.centre).rotated(-shape_.rotation());
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
  for (const auto& shape : shape_.shapes()) {
    shape->render(lib(), to_float(shape_.centre), shape_.rotation().to_float());
  }
}

void Ship::render_with_colour(colour_t colour) const {
  for (const auto& shape : shape_.shapes()) {
    shape->render(lib(), to_float(shape_.centre), shape_.rotation().to_float(),
                  colour & (0xffffff00 | (shape->colour & 0x000000ff)));
  }
}

void Ship::destroy() {
  if (is_destroyed()) {
    return;
  }

  destroy_ = true;
  for (const auto& shape : shape_.shapes()) {
    shape->category = 0;
  }
}

void Ship::spawn(Ship* ship) const {
  game_->add_ship(ship);
}

void Ship::spawn(const Particle& particle) const {
  game_->add_particle(particle);
}

void Ship::explosion(colour_t c, std::int32_t time, bool towards, const fvec2& v) const {
  for (const auto& shape : shape_.shapes()) {
    std::int32_t n = towards ? z::rand_int(2) + 1 : z::rand_int(8) + 8;
    for (std::int32_t j = 0; j < n; j++) {
      fvec2 pos =
          shape->convert_fl_point(to_float(shape_.centre), shape_.rotation().to_float(), fvec2());

      fvec2 dir = fvec2::from_polar(z::rand_fixed().to_float() * 2 * M_PIf, 6.f);

      if (towards && v - pos != fvec2()) {
        dir = (v - pos).normalised();
        float angle = dir.angle() + (z::rand_fixed().to_float() - 0.5f) * M_PIf / 4;
        dir = fvec2::from_polar(angle, 6.f);
      }

      spawn(Particle(pos, c ? c : shape->colour, dir, time + z::rand_int(8)));
    }
  }
}

const CompoundShape::shape_list& Ship::shapes() const {
  return shape_.shapes();
}

void Ship::add_shape(Shape* shape) {
  shape_.add_shape(shape);
}

void Ship::destroy_shape(std::size_t index) {
  shape_.destroy_shape(index);
}

void Ship::clear_shapes() {
  shape_.clear_shapes();
}