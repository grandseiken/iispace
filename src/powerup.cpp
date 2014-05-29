#include "powerup.h"
#include "player.h"

Powerup::Powerup(const vec2& position, type_t type)
  : Ship(position, SHIP_POWERUP)
  , _type(type)
  , _frame(0)
  , _dir(0, 1)
  , _rotate(false)
  , _first_frame(true)
{
  add_shape(new Polygon(vec2(), 13, 5, 0, fixed::pi / 2, 0));
  add_shape(new Polygon(vec2(), 9, 5, 0, fixed::pi / 2, 0));

  switch (type) {
  case EXTRA_LIFE:
    add_shape(new Polygon(vec2(), 8, 3, 0xffffffff, fixed::pi / 2));
    break;

  case MAGIC_SHOTS:
    add_shape(new Fill(vec2(), 3, 3, 0xffffffff));
    break;

  case SHIELD:
    add_shape(new Polygon(vec2(), 11, 5, 0xffffffff, fixed::pi / 2));
    break;

  case BOMB:
    add_shape(new Polystar(vec2(), 11, 10, 0xffffffff, fixed::pi / 2));
    break;
  }
}

void Powerup::update()
{
  get_shape(0).SetColour(Player::GetPlayerColour(_frame / 2));
  _frame = (_frame + 1) % (Lib::PLAYERS * 2);
  get_shape(1).SetColour(Player::GetPlayerColour(_frame / 2));

  static const int32_t rotate_time = 100;
  if (!is_on_screen()) {
    _dir = get_screen_centre() - position();
  }
  else {
    if (_first_frame) {
      _dir.set_polar(z::rand_fixed() * 2 * fixed::pi, 1);
    }

    _dir.rotate(2 * fixed::hundredth * (_rotate ? 1 : -1));
    if (!z::rand_int(rotate_time)) {
      _rotate = !_rotate;
    }
  }
  _first_frame = false;

  Player* p = nearest_player();
  bool alive = !p->IsKilled();
  vec2 pv = p->position() - position();
  if (pv.length() <= 40 && alive) {
    _dir = pv;
  }
  _dir.normalise();

  static const fixed speed = 1;
  move(_dir * speed * ((pv.length() <= 40) ? 3 : 1));
  if (pv.length() <= 10 && alive) {
    damage(1, false, (Player*) p);
  }
}

void Powerup::damage(int damage, bool magic, Player* source)
{
  if (source) {
    switch (_type) {
    case EXTRA_LIFE:
      z0().add_life();
      play_sound(Lib::SOUND_POWERUP_LIFE);
      break;

    case MAGIC_SHOTS:
      source->ActivateMagicShots();
      play_sound(Lib::SOUND_POWERUP_OTHER);
      break;

    case SHIELD:
      source->ActivateMagicShield();
      play_sound(Lib::SOUND_POWERUP_OTHER);
      break;

    case BOMB:
      source->ActivateBomb();
      play_sound(Lib::SOUND_POWERUP_OTHER);
      break;
    }
    lib().Rumble(source->GetPlayerNumber(), 6);
  }

  int r = 5 + z::rand_int(5);
  for (int i = 0; i < r; i++) {
    vec2 dir;
    dir.set_polar(z::rand_fixed() * 2 * fixed::pi, 6);
    spawn(new Particle(to_float(position()), 0xffffffff,
                       to_float(dir), 4 + z::rand_int(8)));
  }
  destroy();
}