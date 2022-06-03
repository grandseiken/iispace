#ifndef II_GAME_LOGIC_SHIP_H
#define II_GAME_LOGIC_SHIP_H
#include "game/logic/shape.h"
#include "game/logic/sim/sim_interface.h"

class Ship {
public:
  enum shape_category { kVulnerable = 1, kDangerous = 2, kShield = 4, kVulnShield = 8 };

  enum ship_category {
    kShipNone = 0,
    kShipPlayer = 1,
    kShipWall = 2,
    kShipEnemy = 4,
    kShipBoss = 8,
    kShipPowerup = 16,
  };

  Ship(ii::SimInterface& sim, const vec2& position, ship_category type);
  virtual ~Ship();

  ii::SimInterface& sim() const {
    return *sim_;
  }

  void destroy();
  bool is_destroyed() const {
    return destroy_;
  }

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

  fixed bounding_width() const {
    return bounding_width_;
  }

  // Operations
  //------------------------------
  bool check_point(const vec2& v, std::int32_t category = 0) const;
  Ship* spawn(std::unique_ptr<Ship> ship) const;
  void spawn(const ii::particle& particle) const;

  template <typename T, typename... Args>
  T* spawn_new(Args&&... args) {
    return static_cast<T*>(spawn(std::make_unique<T>(sim(), std::forward<Args>(args)...)));
  }

  // Helpful functions
  //------------------------------
  void explosion(colour_t c = 0, std::int32_t time = 8, bool towards = false,
                 const glm::vec2& v = glm::vec2{0.f}) const;
  void render_with_colour(colour_t colour) const;

  bool is_on_screen() const {
    return shape_.centre >= vec2{} && shape_.centre <= vec2{ii::kSimWidth, ii::kSimHeight};
  }

  static vec2 get_screen_centre() {
    return {ii::kSimWidth / 2, ii::kSimHeight / 2};
  }

  Player* nearest_player() const {
    return sim_->nearest_player(shape_.centre);
  }

  void play_sound(ii::sound sound) {
    sim().play_sound(sound, 1.f, 2.f * shape_.centre.x.to_float() / ii::kSimWidth - 1.f);
  }

  void play_sound_random(ii::sound sound, float pitch = 0.f, float volume = 1.f) {
    sim().play_sound(sound, volume * (.5f * sim().random_fixed().to_float() + .5f),
                     2.f * shape_.centre.x.to_float() / ii::kSimWidth - 1.f, pitch);
  }

  std::int32_t enemy_value() const {
    return enemy_value_;
  }

  void set_enemy_value(std::int32_t value) {
    enemy_value_ = value;
  }

  // Generic behaviour
  //------------------------------
  virtual void update() = 0;
  virtual void render() const;
  // Player can be null
  virtual void damage(std::int32_t damage, bool magic, Player* source) {}

protected:
  const CompoundShape::shape_list& shapes() const;
  template <typename T, typename... Args>
  T* add_new_shape(Args&&... args) {
    return static_cast<T*>(add_shape(std::make_unique<T>(std::forward<Args>(args)...)));
  }

  Shape* add_shape(std::unique_ptr<Shape> shape);
  void destroy_shape(std::size_t index);
  void clear_shapes();

  void set_bounding_width(fixed width) {
    bounding_width_ = width;
  }

private:
  ii::SimInterface* sim_ = nullptr;
  ship_category type_ = static_cast<ship_category>(0);
  bool destroy_ = false;
  CompoundShape shape_;
  fixed bounding_width_ = 0;
  std::int32_t enemy_value_ = 1;
};

#endif
