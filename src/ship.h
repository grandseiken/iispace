#ifndef IISPACE_SRC_SHIP_H
#define IISPACE_SRC_SHIP_H

#include "shape.h"
#include "z0_game.h"

struct Particle {
  Particle(const fvec2& position, colour_t colour, const fvec2& velocity, int32_t time)
  : destroy(false), position(position), velocity(velocity), timer(time), colour(colour) {}

  bool destroy;
  fvec2 position;
  fvec2 velocity;
  int32_t timer;
  colour_t colour;
};

class Ship {
public:
  enum shape_category { VULNERABLE = 1, DANGEROUS = 2, SHIELD = 4, VULNSHIELD = 8 };

  enum ship_category {
    SHIP_NONE = 0,
    SHIP_PLAYER = 1,
    SHIP_WALL = 2,
    SHIP_ENEMY = 4,
    SHIP_BOSS = 8,
    SHIP_POWERUP = 16,
  };

  Ship(const vec2& position, ship_category type);
  virtual ~Ship();

  void set_game(GameModal& game) {
    _game = &game;
  }

  Lib& lib() const {
    return _game->lib();
  }

  GameModal& game() const {
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
  bool check_point(const vec2& v, int32_t category = 0) const;
  void spawn(Ship* ship) const;
  void spawn(const Particle& particle) const;

  // Helpful functions
  //------------------------------
  void explosion(colour_t c = 0, int32_t time = 8, bool towards = false,
                 const fvec2& v = fvec2()) const;
  void render_with_colour(colour_t colour) const;

  bool is_on_screen() const {
    return _shape.centre >= vec2() && _shape.centre <= vec2(Lib::WIDTH, Lib::HEIGHT);
  }

  static vec2 get_screen_centre() {
    return vec2(Lib::WIDTH / 2, Lib::HEIGHT / 2);
  }

  Player* nearest_player() const {
    return _game->nearest_player(_shape.centre);
  }

  bool play_sound(Lib::Sound sound) {
    return lib().play_sound(sound, 1.f, 2.f * _shape.centre.x.to_float() / Lib::WIDTH - 1.f);
  }

  bool play_sound_random(Lib::Sound sound, float pitch = 0.f, float volume = 1.f) {
    return lib().play_sound(sound, volume * (.5f * z::rand_fixed().to_float() + .5f),
                            2.f * _shape.centre.x.to_float() / Lib::WIDTH - 1.f, pitch);
  }

  int32_t enemy_value() const {
    return _enemy_value;
  }

  void set_enemy_value(int32_t value) {
    _enemy_value = value;
  }

  // Generic behaviour
  //------------------------------
  virtual void update() = 0;
  virtual void render() const;
  // Player can be null
  virtual void damage(int32_t damage, bool magic, Player* source) {}

protected:
  const CompoundShape::shape_list& shapes() const;
  void add_shape(Shape* shape);
  void destroy_shape(std::size_t index);
  void clear_shapes();

  void set_bounding_width(fixed width) {
    _bounding_width = width;
  }

private:
  GameModal* _game;
  ship_category _type;
  bool _destroy;
  CompoundShape _shape;
  fixed _bounding_width;
  int32_t _enemy_value;
};

#endif
