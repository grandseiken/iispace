#include "game/logic/ship/ship.h"
#include "game/logic/ship/ecs_index.h"

namespace ii {

void Health::damage(SimInterface& sim, ecs::handle h, std::uint32_t damage, damage_type type,
                    std::optional<ecs::entity_id> source) {
  if (damage_transform) {
    damage = damage_transform(sim, h, type, damage);
    if (!damage) {
      return;
    }
  }
  if (on_hit) {
    on_hit(type);
  }

  hp = hp < damage ? 0 : hp - damage;
  vec2 position = {kSimDimensions.x / 2, kSimDimensions.y / 2};
  if (auto c = h.get<Position>(); c) {
    position = c->centre;
  } else if (auto c = h.get<LegacyShip>(); c) {
    position = c->ship->shape().centre;
  }

  if (hit_sound0 && damage) {
    sim.play_sound(*hit_sound0, position, /* random */ true);
  }
  if (h.has<Destroy>()) {
    return;
  }

  if (!hp) {
    if (destroy_sound) {
      sim.play_sound(*destroy_sound, position, /* random */ true);
    }
    if (on_destroy) {
      on_destroy(type);
    }
    h.add(Destroy{.source = source});
  } else {
    if (hit_sound1 && damage) {
      sim.play_sound(*hit_sound1, position, /* random */ true);
    }
    hit_timer = std::max<std::uint32_t>(hit_timer, type == damage_type::kBomb ? 25 : 1);
  }
}

Ship::Ship(SimInterface& sim, const vec2& position, ship_flag type)
: IShip{sim}, type_{type}, shape_{position, 0} {}

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

void Ship::spawn(const particle& particle) const {
  sim().add_particle(particle);
}

void Ship::explosion(const std::optional<glm::vec4>& c, std::uint32_t time, bool towards,
                     const glm::vec2& v) const {
  for (const auto& shape : shape_.shapes()) {
    auto pos = shape->convert_fl_point(to_float(shape_.centre), shape_.rotation().to_float(),
                                       glm::vec2{0.f});
    sim().explosion(pos, c ? *c : shape->colour, time,
                    towards ? std::make_optional(v) : std::nullopt);
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