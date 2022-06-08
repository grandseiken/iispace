#ifndef II_GAME_LOGIC_SHIP_SHIP_H
#define II_GAME_LOGIC_SHIP_SHIP_H
#include "game/logic/ship/ecs_index.h"
#include "game/logic/ship/shape.h"
#include "game/logic/sim/sim_interface.h"

namespace ii {

struct Destroy : ecs::component {};
struct Collision : ecs::component {
  Collision(fixed bounding_width) : bounding_width{bounding_width} {}
  fixed bounding_width = 0;
};

class Ship {
public:
  enum shape_category : std::uint32_t {
    kVulnerable = 1,
    kDangerous = 2,
    kShield = 4,
    kVulnShield = 8
  };

  enum ship_category : std::uint32_t {
    kShipNone = 0,
    kShipPlayer = 1,
    kShipWall = 2,
    kShipEnemy = 4,
    kShipBoss = 8,
    kShipPowerup = 16,
  };

  Ship(SimInterface& sim, const vec2& position, ship_category type);
  virtual ~Ship();

  ecs::handle handle() const {
    return *handle_;
  }

  void set_handle(ecs::handle handle) {
    handle_ = handle;
  }

  SimInterface& sim() const {
    return *sim_;
  }

  void destroy();
  bool is_destroyed() const;

  ship_category type() const {
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
  bool check_point(const vec2& v, std::uint32_t category = 0) const;
  void spawn(const particle& particle) const;

  // Helpful functions
  //------------------------------
  void explosion(const std::optional<glm::vec4>& c = std::nullopt, std::uint32_t time = 8,
                 bool towards = false, const glm::vec2& v = glm::vec2{0.f}) const;
  void render_with_colour(const glm::vec4& colour) const;

  bool is_on_screen() const {
    return all(greaterThanEqual(shape_.centre, vec2{0})) &&
        all(lessThanEqual(shape_.centre, vec2{kSimDimensions.x, kSimDimensions.y}));
  }

  static vec2 get_screen_centre() {
    return {kSimDimensions.x / 2, kSimDimensions.y / 2};
  }

  Player* nearest_player() const {
    return sim_->nearest_player(shape_.centre);
  }

  void play_sound(sound sound) {
    sim().play_sound(sound, 1.f, 2.f * shape_.centre.x.to_float() / kSimDimensions.x - 1.f);
  }

  void play_sound_random(sound sound, float pitch = 0.f, float volume = 1.f) {
    sim().play_sound(sound, volume * (.5f * sim().random_fixed().to_float() + .5f),
                     2.f * shape_.centre.x.to_float() / kSimDimensions.x - 1.f, pitch);
  }

  std::uint32_t enemy_value() const {
    return enemy_value_;
  }

  void set_enemy_value(std::uint32_t value) {
    enemy_value_ = value;
  }

  // Generic behaviour
  //------------------------------
  virtual void update() = 0;
  virtual void render() const;
  // Player can be null
  virtual void damage(std::uint32_t damage, bool magic, Player* source) {}

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
  SimInterface* sim_ = nullptr;
  std::optional<ecs::handle> handle_;
  ship_category type_ = static_cast<ship_category>(0);
  CompoundShape shape_;
  std::uint32_t enemy_value_ = 1;
};

}  // namespace ii

#endif
