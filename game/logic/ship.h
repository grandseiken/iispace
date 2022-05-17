#ifndef IISPACE_GAME_LOGIC_SHIP_H
#define IISPACE_GAME_LOGIC_SHIP_H
#include "game/core/lib.h"
#include "game/logic/shape.h"
#include "game/logic/sim_state.h"

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

  Ship(const vec2& position, ship_category type);
  virtual ~Ship();

  void set_game(SimState& game) {
    _game = &game;
  }

  Lib& lib() const {
    return _game->lib();
  }

  SimState& game() const {
    return *_game;
  }

  void destroy();
  bool is_destroyed() const {
    return _destroy;
  }

  ship_category type() const {
    return _type;
  }

  const CompoundShape& shape() const {
    return _shape;
  }

  CompoundShape& shape() {
    return _shape;
  }

  void move(const vec2& move_amount) {
    _shape.centre += move_amount;
  }

  fixed bounding_width() const {
    return _bounding_width;
  }

  // Operations
  //------------------------------
  bool check_point(const vec2& v, std::int32_t category = 0) const;
  void spawn(Ship* ship) const;
  void spawn(const Particle& particle) const;

  // Helpful functions
  //------------------------------
  void explosion(colour_t c = 0, std::int32_t time = 8, bool towards = false,
                 const fvec2& v = fvec2()) const;
  void render_with_colour(colour_t colour) const;

  bool is_on_screen() const {
    return _shape.centre >= vec2() && _shape.centre <= vec2(Lib::kWidth, Lib::kHeight);
  }

  static vec2 get_screen_centre() {
    return vec2(Lib::kWidth / 2, Lib::kHeight / 2);
  }

  Player* nearest_player() const {
    return _game->nearest_player(_shape.centre);
  }

  bool play_sound(Lib::sound sound) {
    return lib().play_sound(sound, 1.f, 2.f * _shape.centre.x.to_float() / Lib::kWidth - 1.f);
  }

  bool play_sound_random(Lib::sound sound, float pitch = 0.f, float volume = 1.f) {
    return lib().play_sound(sound, volume * (.5f * z::rand_fixed().to_float() + .5f),
                            2.f * _shape.centre.x.to_float() / Lib::kWidth - 1.f, pitch);
  }

  std::int32_t enemy_value() const {
    return _enemy_value;
  }

  void set_enemy_value(std::int32_t value) {
    _enemy_value = value;
  }

  // Generic behaviour
  //------------------------------
  virtual void update() = 0;
  virtual void render() const;
  // Player can be null
  virtual void damage(std::int32_t damage, bool magic, Player* source) {}

protected:
  const CompoundShape::shape_list& shapes() const;
  void add_shape(Shape* shape);
  void destroy_shape(std::size_t index);
  void clear_shapes();

  void set_bounding_width(fixed width) {
    _bounding_width = width;
  }

private:
  SimState* _game;
  ship_category _type;
  bool _destroy;
  CompoundShape _shape;
  fixed _bounding_width;
  std::int32_t _enemy_value;
};

#endif
