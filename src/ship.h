#ifndef IISPACE_SRC_SHIP_H
#define IISPACE_SRC_SHIP_H

#include "shape.h"
#include "z0_game.h"

struct Particle {

  Particle(const flvec2& position, colour_t colour,
           const flvec2& velocity, int32_t time)
    : destroy(false)
    , position(position)
    , velocity(velocity)
    , timer(time)
    , colour(colour) {}

  bool destroy;
  flvec2 position;
  flvec2 velocity;
  int32_t timer;
  colour_t colour;

};

class Ship {
public:

  enum shape_category {
    VULNERABLE = 1,
    DANGEROUS = 2,
    SHIELD = 4,
    VULNSHIELD = 8
  };

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

  void set_game(z0Game& game)
  {
    _z0 = &game;
  }

  Lib& lib() const
  {
    return _z0->lib();
  }

  z0Game& z0() const
  {
    return *_z0;
  }

  void destroy();
  bool is_destroyed() const
  {
    return _destroy;
  }

  ship_category type() const
  {
    return _type;
  }

  // Position and rotation
  //------------------------------
  const vec2& position() const
  {
    return _position;
  }

  void set_position(const vec2& position)
  {
    _position = position;
  }

  void move(const vec2& v)
  {
    set_position(position() + v);
  }

  fixed rotation() const
  {
    return _rotation;
  }

  void set_rotation(fixed rotation)
  {
    _rotation =
        rotation > 2 * fixed::pi ? rotation - 2 * fixed::pi :
        rotation < 0 ? rotation + 2 * fixed::pi : rotation;
  }

  void rotate(fixed rotation_amount)
  {
    set_rotation(rotation() + rotation_amount);
  }

  fixed bounding_width() const
  {
    return _bounding_width;
  }

  // Operations
  //------------------------------
  bool check_point(const vec2& v, int category = 0) const;
  void spawn(Ship* ship) const;
  void spawn(const Particle& particle) const;

  void render_hp_bar(float fill) const
  {
    _z0->RenderHPBar(fill);
  }

  // Helpful functions
  //------------------------------
  void explosion(colour_t c = 0, int time = 8,
                 bool towards = false, const flvec2& v = flvec2()) const;
  void render_with_colour(colour_t colour) const;

  bool is_on_screen() const
  {
    return
        position().x >= 0 && position().x <= Lib::WIDTH &&
        position().y >= 0 && position().y <= Lib::HEIGHT;
  }

  static vec2 get_screen_centre()
  {
    return vec2(Lib::WIDTH / 2, Lib::HEIGHT / 2);
  }

  Player* nearest_player() const
  {
    return _z0->nearest_player(position());
  }

  bool play_sound(Lib::Sound sound)
  {
    return lib().PlaySound(
        sound, 1.f, 2.f * position().x.to_float() / Lib::WIDTH - 1.f);
  }

  bool play_sound_random(Lib::Sound sound, float pitch = 0.f, float volume = 1.f)
  {
    return lib().PlaySound(
        sound, volume * (.5f * z::rand_fixed().to_float() + .5f),
        2.f * position().x.to_float() / Lib::WIDTH - 1.f, pitch);
  }

  int32_t enemy_value() const
  {
    return _enemy_value;
  }

  void set_enemy_value(int32_t value)
  {
    _enemy_value = value;
  }

  // Generic behaviour
  //------------------------------
  virtual void update() = 0;
  virtual void render() const;
  // Player can be null
  virtual void damage(int32_t damage, bool magic, Player* source) {}

protected:

  // Shape control
  //------------------------------
  void add_shape(Shape* shape);
  void destroy_shape(std::size_t i);
  Shape& get_shape(std::size_t i) const;
  std::size_t count_shapes() const;

  void set_bounding_width(fixed width)
  {
    _bounding_width = width;
  }

private:

  z0Game* _z0;

  ship_category _type;
  bool _destroy;

  vec2 _position;
  fixed _rotation;
  fixed _bounding_width;
  int32_t _enemy_value;

  std::vector<std::unique_ptr<Shape>> _shapes;

};

#endif
