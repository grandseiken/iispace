#ifndef II_GAME_LOGIC_SHIP_SHIP_H
#define II_GAME_LOGIC_SHIP_SHIP_H
#include "game/logic/ecs/index.h"
#include "game/logic/ship/components.h"
#include "game/logic/ship/shape.h"
#include "game/logic/sim/sim_interface.h"
#include <optional>

namespace ii {

class IShip {
public:
  IShip(SimInterface& sim) : sim_{sim} {}
  virtual ~IShip() {}

  SimInterface& sim() const {
    return sim_;
  }

  void destroy(std::optional<ecs::entity_id> source = std::nullopt) {
    handle().add(Destroy{.source = source});
  }

  bool is_destroyed() const {
    return handle().has<Destroy>();
  }

  virtual ship_flag type() const = 0;
  virtual ecs::handle handle() const = 0;
  virtual vec2& position() = 0;
  virtual vec2 position() const = 0;
  virtual fixed rotation() const = 0;
  virtual void damage(std::uint32_t damage, bool magic, IShip* source);

private:
  SimInterface& sim_;
};

struct ShipForwarder : IShip {
  ecs::handle h;
  ShipForwarder(SimInterface& sim, ecs::handle h) : IShip{sim}, h{h} {}
  ship_flag type() const override {
    return h.get<ShipFlags>()->flags;
  }
  ecs::handle handle() const override {
    return h;
  }
  vec2& position() override {
    return h.get<Transform>()->centre;
  }
  vec2 position() const override {
    return h.get<Transform>()->centre;
  }
  fixed rotation() const override {
    return h.get<Transform>()->rotation;
  }
};

class Ship : public IShip {
public:
  Ship(SimInterface& sim, const vec2& position, ship_flag type);

  ecs::handle handle() const override {
    return *handle_;
  }

  void set_handle(ecs::handle handle) {
    handle_ = handle;
  }

  ship_flag type() const override {
    return type_;
  }

  const CompoundShape& shape() const {
    return shape_;
  }

  CompoundShape& shape() {
    return shape_;
  }

  void move(const vec2& move_amount) {
    shape_.centre += move_amount;
  }

  // Operations
  //------------------------------
  bool check_point(const vec2& v, shape_flag category = shape_flag{0}) const;
  void spawn(const particle& particle) const;

  // Helpful functions
  //------------------------------
  void explosion(const std::optional<glm::vec4>& c = std::nullopt, std::uint32_t time = 8,
                 bool towards = false, const glm::vec2& v = glm::vec2{0.f}) const;
  void render_with_colour(const glm::vec4& colour) const;

  bool is_on_screen() const {
    return sim().is_on_screen(shape_.centre);
  }

  void play_sound(sound sound) {
    sim().play_sound(sound, shape_.centre);
  }

  void play_sound_random(sound sound, float pitch = 0.f, float volume = 1.f) {
    sim().play_sound(sound, shape_.centre, true, volume);
  }

  vec2& position() override {
    return shape().centre;
  }
  vec2 position() const override {
    return shape().centre;
  }
  fixed rotation() const override {
    return shape().rotation();
  }
  virtual void update() = 0;
  virtual void render() const;

protected:
  const CompoundShape::shape_list& shapes() const;
  template <typename T, typename... Args>
  T* add_new_shape(Args&&... args) {
    return static_cast<T*>(add_shape(std::make_unique<T>(std::forward<Args>(args)...)));
  }

  Shape* add_shape(std::unique_ptr<Shape> shape);
  void destroy_shape(std::size_t index);
  void clear_shapes();

private:
  std::optional<ecs::handle> handle_;
  ship_flag type_ = ship_flag{0};
  CompoundShape shape_;
};

inline Collision legacy_collision(fixed w) {
  Collision c;
  c.bounding_width = w;
  c.flags = shape_flag::kEverything;
  c.check = [](ecs::const_handle h, const vec2& v, shape_flag f) {
    return static_cast<ii::Ship*>(h.get<LegacyShip>()->ship.get())->check_point(v, f);
  };
  return c;
}

inline void IShip::damage(std::uint32_t damage, bool magic, IShip* source) {
  if (auto c = handle().get<Health>(); c) {
    std::optional<ecs::entity_id> source_id;
    if (source) {
      source_id = source->handle().id();
    }
    // TODO: damage > 10: kBombDamage
    c->damage(handle(), sim(), damage,
              magic             ? damage_type::kMagic
                  : damage > 10 ? damage_type::kBomb
                                : damage_type::kNone,
              source_id);
  }
}

}  // namespace ii

#endif
