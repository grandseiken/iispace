#include "powerup.h"
#include "player.h"

Powerup::Powerup(const Vec2& position, type_t type)
  : Ship(position)
  , _type(type)
  , _frame(0)
  , _dir(0, 1)
  , _rotate(false)
  , _first_frame(true)
{
  AddShape(new Polygon(Vec2(), 13, 5, 0, fixed::pi / 2, 0));
  AddShape(new Polygon(Vec2(), 9, 5, 0, fixed::pi / 2, 0));

  switch (type) {
  case EXTRA_LIFE:
    AddShape(new Polygon(Vec2(), 8, 3, 0xffffffff, fixed::pi / 2));
    break;

  case MAGIC_SHOTS:
    AddShape(new Fill(Vec2(), 3, 3, 0xffffffff));
    break;

  case SHIELD:
    AddShape(new Polygon(Vec2(), 11, 5, 0xffffffff, fixed::pi / 2));
    break;

  case BOMB:
    AddShape(new Polystar(Vec2(), 11, 10, 0xffffffff, fixed::pi / 2));
    break;
  }
}

void Powerup::Update()
{
  GetShape(0).SetColour(Player::GetPlayerColour(_frame / 2));
  _frame = (_frame + 1) % (Lib::PLAYERS * 2);
  GetShape(1).SetColour(Player::GetPlayerColour(_frame / 2));

  static const int32_t rotate_time = 100;
  if (!IsOnScreen()) {
    _dir = GetScreenCentre() - GetPosition();
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

  Player* p = GetNearestPlayer();
  bool alive = !p->IsKilled();
  Vec2 pv = p->GetPosition() - GetPosition();
  if (pv.length() <= 40 && alive) {
    _dir = pv;
  }
  _dir.normalise();

  static const fixed speed = 1;
  Move(_dir * speed * ((pv.length() <= 40) ? 3 : 1));
  if (pv.length() <= 10 && alive) {
    Damage(1, false, (Player*) p);
  }
}

void Powerup::Damage(int damage, bool magic, Player* source)
{
  if (source) {
    switch (_type) {
    case EXTRA_LIFE:
      AddLife();
      PlaySound(Lib::SOUND_POWERUP_LIFE);
      break;

    case MAGIC_SHOTS:
      source->ActivateMagicShots();
      PlaySound(Lib::SOUND_POWERUP_OTHER);
      break;

    case SHIELD:
      source->ActivateMagicShield();
      PlaySound(Lib::SOUND_POWERUP_OTHER);
      break;

    case BOMB:
      source->ActivateBomb();
      PlaySound(Lib::SOUND_POWERUP_OTHER);
      break;
    }
    GetLib().Rumble(source->GetPlayerNumber(), 6);
  }

  int r = 5 + z::rand_int(5);
  for (int i = 0; i < r; i++) {
    Vec2 dir;
    dir.set_polar(z::rand_fixed() * 2 * fixed::pi, 6);
    Spawn(new Particle(to_float(GetPosition()), 0xffffffff,
                       to_float(dir), 4 + z::rand_int(8)));
  }
  Destroy();
}