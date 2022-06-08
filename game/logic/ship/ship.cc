#include "game/logic/ship/ship.h"
#include "game/logic/ship/ecs_index.h"
#include <glm/gtc/constants.hpp>

namespace ii {

Ship::Ship(SimInterface& sim, const vec2& position, ship_flag type)
: sim_{&sim}, type_{type}, shape_{position, 0} {}

Ship::~Ship() {}

bool Ship::check_point(const vec2& v, shape_flag category) const {
  bool aa = false;
  vec2 a{0};
  for (const auto& shape : shape_.shapes()) {
    if (+shape->category && (!category || (shape->category & category) == category)) {
      if (!aa) {
        a = rotate(v - shape_.centre, -shape_.rotation());
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
    shape->render(sim(), to_float(shape_.centre), shape_.rotation().to_float());
  }
}

void Ship::render_with_colour(const glm::vec4& colour) const {
  for (const auto& shape : shape_.shapes()) {
    shape->render(sim(), to_float(shape_.centre), shape_.rotation().to_float(),
                  glm::vec4{colour.r, colour.g, colour.b, shape->colour.a});
  }
}

void Ship::destroy() {
  handle().emplace<Destroy>();
}

bool Ship::is_destroyed() const {
  return handle().has<Destroy>();
}

void Ship::spawn(const particle& particle) const {
  sim_->add_particle(particle);
}

void Ship::explosion(const std::optional<glm::vec4>& c, std::uint32_t time, bool towards,
                     const glm::vec2& v) const {
  for (const auto& shape : shape_.shapes()) {
    auto n = towards ? sim().random(2) + 1 : sim().random(8) + 8;
    for (std::uint32_t j = 0; j < n; ++j) {
      auto pos = shape->convert_fl_point(to_float(shape_.centre), shape_.rotation().to_float(),
                                         glm::vec2{0.f});

      auto dir = from_polar(sim().random_fixed().to_float() * 2 * glm::pi<float>(), 6.f);

      if (towards && v - pos != glm::vec2{0.f}) {
        dir = glm::normalize(v - pos);
        float angle = std::atan2(dir.y, dir.x) +
            (sim().random_fixed().to_float() - 0.5f) * glm::pi<float>() / 4;
        dir = from_polar(angle, 6.f);
      }

      spawn(particle{pos, c ? *c : shape->colour, dir, time + sim().random(8)});
    }
  }
}

const CompoundShape::shape_list& Ship::shapes() const {
  return shape_.shapes();
}

Shape* Ship::add_shape(std::unique_ptr<Shape> shape) {
  return shape_.add_shape(std::move(shape));
}

void Ship::destroy_shape(std::size_t index) {
  shape_.destroy_shape(index);
}

void Ship::clear_shapes() {
  shape_.clear_shapes();
}

}  // namespace ii