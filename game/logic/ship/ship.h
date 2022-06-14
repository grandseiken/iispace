#ifndef II_GAME_LOGIC_SHIP_SHIP_H
#define II_GAME_LOGIC_SHIP_SHIP_H
#include "game/common/enum.h"
#include "game/logic/ship/ecs_index.h"
#include "game/logic/ship/shape.h"
#include "game/logic/sim/sim_interface.h"
#include <functional>

namespace ii {

enum class ship_flag : std::uint32_t {
  kNone = 0,
  kPlayer = 1,
  kWall = 2,
  kEnemy = 4,
  kBoss = 8,
  kPowerup = 16,
};

template <>
struct bitmask_enum<ship_flag> : std::true_type {};

class IShip;
struct LegacyShip : ecs::component {
  std::unique_ptr<IShip> ship;
};

struct Destroy : ecs::component {
  std::optional<ecs::entity_id> source;
};

struct ShipFlag : ecs::component {
  ship_flag flags = ship_flag::kNone;
};

struct Transform : ecs::component {
  vec2 centre = {0, 0};
  fixed rotation = 0;

  void move(const vec2& v) {
    centre += v;
  }
  void rotate(fixed amount) {
    set_rotation(rotation + amount);
  }
  void set_rotation(fixed r) {
    rotation = normalise_angle(r);
  }
};

struct Collision : ecs::component {
  fixed bounding_width = 0;
  std::function<bool(ecs::const_handle, const vec2&, shape_flag)> check;

  vec2 centre(ecs::const_handle h) const;
};

struct Update : ecs::component {
  std::function<void(SimInterface&, ecs::handle)> update;
};

struct Render : ecs::component {
  std::optional<glm::vec4> colour_override;
  std::function<void(const SimInterface&, ecs::const_handle)> render;
};

enum class damage_type {
  kNone,
  kMagic,
  kBomb,
};

struct Health : ecs::component {
  std::uint32_t hp = 0;
  std::uint32_t max_hp = hp;

  std::uint32_t hit_timer = 0;
  std::optional<std::uint32_t> hit_flash_ignore_index;

  std::optional<ii::sound> hit_sound0 = ii::sound::kEnemyHit;
  std::optional<ii::sound> hit_sound1 = ii::sound::kEnemyHit;
  std::optional<ii::sound> destroy_sound = ii::sound::kEnemyDestroy;

  std::function<std::uint32_t(SimInterface&, ecs::handle, damage_type, std::uint32_t)>
      damage_transform;
  std::function<void(SimInterface&, ecs::handle, damage_type)> on_hit;
  std::function<void(SimInterface&, ecs::handle, damage_type)> on_destroy;

  bool is_hp_low() const {
    return 3 * hp <= max_hp + max_hp / 5;
  }

  void damage(SimInterface&, ecs::handle h, std::uint32_t damage, damage_type type,
              std::optional<ecs::entity_id> source);
};

struct Enemy : ecs::component {
  std::uint32_t threat_value = 1;
  std::uint32_t score_reward = 0;
  std::uint32_t boss_score_reward = 0;
};

struct Boss : ecs::component {
  boss_flag boss = boss_flag{0};
  bool show_hp_bar = false;
};

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
    return h.get<ShipFlag>()->flags;
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
    c->damage(sim(), handle(), damage,
              magic             ? damage_type::kMagic
                  : damage > 10 ? damage_type::kBomb
                                : damage_type::kNone,
              source_id);
  }
}

inline vec2 Collision::centre(ecs::const_handle h) const {
  if (auto c = h.get<Transform>()) {
    return c->centre;
  }
  if (auto c = h.get<LegacyShip>()) {
    return c->ship->position();
  }
  return vec2{0};
}

}  // namespace ii

#endif
