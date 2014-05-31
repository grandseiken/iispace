#include "player.h"
#include "lib.h"
#include "enemy.h"

#include <algorithm>

const fixed Player::SPEED = 5;
const fixed Player::SHOT_SPEED = 10;
const int Player::SHOT_TIMER = 4;
const fixed Player::BOMB_RADIUS = 180;
const fixed Player::BOMB_BOSSRADIUS = 280;
const int Player::BOMB_DAMAGE = 50;
const int Player::REVIVE_TIME = 100;
const int Player::SHIELD_TIME = 50;
const int Player::MAGICSHOT_COUNT = 120;

z0Game::ShipList Player::_kill_queue;
z0Game::ShipList Player::_shot_sound_queue;
int Player::_fire_timer;
Replay Player::replay(0, 0, false);
std::size_t Player::replay_frame;

Player::Player(const vec2& position, int player_number)
  : Ship(position, SHIP_PLAYER)
  , _player_number(player_number)
  , _score(0)
  , _multiplier(1)
  , _multiplier_count(0)
  , _kill_timer(0)
  , _revive_timer(REVIVE_TIME)
  , _magic_shot_timer(0)
  , _shield(false)
  , _bomb(false)
  , _death_count(0)
{
  add_shape(new Polygon(vec2(), 16, 3, colour()));
  add_shape(new Fill(vec2(8, 0), 2, 2, colour()));
  add_shape(new Fill(vec2(8, 0), 1, 1, colour() & 0xffffff33));
  add_shape(new Fill(vec2(8, 0), 3, 3, colour() & 0xffffff33));
  add_shape(new Polygon(vec2(), 8, 3, colour(), fixed::pi));
  _kill_queue.clear();
  _shot_sound_queue.clear();
  _fire_timer = 0;
}

Player::~Player()
{
}

void Player::update()
{
  vec2 velocity = lib().get_move_velocity(_player_number);
  vec2 fireTarget = lib().get_fire_target(_player_number, shape().centre);
  int keys =
      int(lib().is_key_held(_player_number, Lib::KEY_FIRE)) |
      (lib().is_key_pressed(_player_number, Lib::KEY_BOMB) << 1);

  if (replay.recording) {
    replay.record(velocity, fireTarget, keys);
  }
  else {
    if (replay_frame < replay.player_frames.size()) {
      const PlayerFrame& pf = replay.player_frames[replay_frame];
      velocity = pf.velocity;
      fireTarget = pf.target;
      keys = pf.keys;
      ++replay_frame;
    }
    else {
      velocity = vec2();
      fireTarget = _temp_target;
      keys = 0;
    }
  }
  _temp_target = fireTarget;

  if (!(keys & 1) || _kill_timer) {
    for (unsigned int i = 0; i < _shot_sound_queue.size(); i++) {
      if (_shot_sound_queue[i] == this) {
        _shot_sound_queue.erase(_shot_sound_queue.begin() + i);
        break;
      }
    }
  }

  // Temporary death
  if (_kill_timer) {
    _kill_timer--;
    if (!_kill_timer && !_kill_queue.empty()) {
      if (z0().get_lives() && _kill_queue[0] == this) {
        _kill_queue.erase(_kill_queue.begin());
        _revive_timer = REVIVE_TIME;
        shape().centre = vec2(
            (1 + _player_number) * Lib::WIDTH / (1 + z0().count_players()),
            Lib::HEIGHT / 2);
        z0().sub_life();
        lib().rumble(_player_number, 10);
        play_sound(Lib::SOUND_PLAYER_RESPAWN);
      }
      else {
        _kill_timer++;
      }
    }
    return;
  }
  if (_revive_timer) {
    _revive_timer--;
  }

  // Movement
  vec2 move = velocity;
  if (move.length() > fixed::hundredth) {
    move = (move.length() > 1 ? move.normalised() : move) * SPEED;
    vec2 pos = move + shape().centre;

    pos.x = std::max(fixed(0), std::min(fixed(Lib::WIDTH), pos.x));
    pos.y = std::max(fixed(0), std::min(fixed(Lib::HEIGHT), pos.y));

    shape().centre = pos;
    shape().set_rotation(move.angle());
  }

  // Bombs
  if (_bomb && keys & 2) {
    _bomb = false;
    destroy_shape(5);

    explosion(0xffffffff, 16);
    explosion(colour(), 32);
    explosion(0xffffffff, 48);

    vec2 t = shape().centre;
    flvec2 tf = to_float(t);
    for (int i = 0; i < 64; ++i) {
      shape().centre =
          t + vec2::from_polar(2 * i * fixed::pi / 64, BOMB_RADIUS);
      explosion((i % 2) ? colour() : 0xffffffff,
                8 + z::rand_int(8) + z::rand_int(8), true, tf);
    }
    shape().centre = t;

    lib().rumble(_player_number, 10);
    play_sound(Lib::SOUND_EXPLOSION);

    z0Game::ShipList list =
        z0().ships_in_radius(shape().centre, BOMB_BOSSRADIUS, SHIP_ENEMY);
    for (unsigned int i = 0; i < list.size(); i++) {
      if ((list[i]->shape().centre - shape().centre).length() <= BOMB_RADIUS ||
          (list[i]->type() & SHIP_BOSS)) {
        list[i]->damage(BOMB_DAMAGE, false, 0);
      }
      if (!(list[i]->type() & SHIP_BOSS) &&
          ((Enemy*) list[i])->GetScore() > 0) {
        add_score(0);
      }
    }
  }

  // Shots
  if (!_fire_timer && keys & 1) {
    vec2 v = fireTarget - shape().centre;
    if (v.length() > 0) {
      spawn(new Shot(shape().centre, this, v, _magic_shot_timer != 0));
      if (_magic_shot_timer) {
        _magic_shot_timer--;
      }

      bool couldPlay = false;
      // Avoid randomness errors due to sound timings
      float volume = .5f * z::rand_fixed().to_float() + .5f;
      float pitch = (z::rand_fixed().to_float() - 1.f) / 12.f;
      if (_shot_sound_queue.empty() || _shot_sound_queue[0] == this) {
        couldPlay = lib().play_sound(
            Lib::SOUND_PLAYER_FIRE, volume,
            2.f * shape().centre.x.to_float() / Lib::WIDTH - 1.f, pitch);
      }
      if (couldPlay && !_shot_sound_queue.empty()) {
        _shot_sound_queue.erase(_shot_sound_queue.begin());
      }
      if (!couldPlay) {
        bool inQueue = false;
        for (unsigned int i = 0; i < _shot_sound_queue.size(); i++) {
          if (_shot_sound_queue[i] == this) {
            inQueue = true;
          }
        }
        if (!inQueue) {
          _shot_sound_queue.push_back(this);
        }
      }
    }
  }

  // Damage
  if (z0().any_collision(shape().centre, DANGEROUS)) {
    damage();
  }
}

void Player::render() const
{
  int n = _player_number;
  if (!_kill_timer && (z0().mode() != z0Game::WHAT_MODE || _revive_timer > 0)) {
    flvec2 t = to_float(_temp_target);
    if (t.x >= 0 && t.x <= Lib::WIDTH && t.y >= 0 && t.y <= Lib::HEIGHT) {
      lib().render_line(t + flvec2(0, 9), t - flvec2(0, 8), colour());
      lib().render_line(t + flvec2(9, 1), t - flvec2(8, -1), colour());
    }
    if (_revive_timer % 2) {
      render_with_colour(0xffffffff);
    }
    else {
      Ship::render();
    }
  }

  if (z0().mode() == z0Game::BOSS_MODE) {
    return;
  }

  std::stringstream ss;
  ss << _multiplier << "X";
  std::string s = ss.str();

  flvec2 v =
      n == 1 ? flvec2(Lib::WIDTH / Lib::TEXT_WIDTH - 1.f - s.length(), 1.f) :
      n == 2 ? flvec2(1.f, Lib::HEIGHT / Lib::TEXT_HEIGHT - 2.f) :
      n == 3 ? flvec2(Lib::WIDTH / Lib::TEXT_WIDTH - 1.f - s.length(),
                      Lib::HEIGHT / Lib::TEXT_HEIGHT - 2.f) : flvec2(1.f, 1.f);
  lib().render_text(v, s, z0Game::PANEL_TEXT);

  std::stringstream sss;
  if (n % 2 == 1) {
    sss << _score << "   ";
  }
  else {
    sss << "   " << _score;
  }
  s = sss.str();
  lib().render_text(v, s, colour());

  if (_magic_shot_timer != 0) {
    if (n == 0 || n == 2) {
      v.x += int(s.length());
    }
    else {
      v.x -= 1;
    }
    v *= 16;
    lib().render_rect(
        v + flvec2(5.f, 11.f - (10 * _magic_shot_timer) / MAGICSHOT_COUNT),
        v + flvec2(9.f, 13.f), 0xffffffff, 2);
  }
}

void Player::damage()
{
  if (_kill_timer || _revive_timer) {
    return;
  }
  for (unsigned int i = 0; i < _kill_queue.size(); i++) {
    if (_kill_queue[i] == this) {
      return;
    }
  }

  if (_shield) {
    lib().rumble(_player_number, 10);
    play_sound(Lib::SOUND_PLAYER_SHIELD);
    destroy_shape(5);
    _shield = false;

    _revive_timer = SHIELD_TIME;
    return;
  }

  explosion();
  explosion(0xffffffff, 14);
  explosion(0, 20);

  _magic_shot_timer = 0;
  _multiplier = 1;
  _multiplier_count = 0;
  _kill_timer = REVIVE_TIME;
  ++_death_count;
  if (_shield || _bomb) {
    destroy_shape(5);
    _shield = false;
    _bomb = false;
  }
  _kill_queue.push_back(this);
  lib().rumble(_player_number, 25);
  play_sound(Lib::SOUND_PLAYER_DESTROY);
}

static const int MULTIPLIER_lookup[24] = {
  1, 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024, 2048, 4096, 8192, 16384, 32768,
  65536, 131072, 262144, 524288, 1048576, 2097152, 4194304, 8388608
};

void Player::add_score(int64_t score)
{
  lib().rumble(_player_number, 3);
  _score += score * _multiplier;
  _multiplier_count++;
  if (MULTIPLIER_lookup[std::min(_multiplier + 3, 23)] <= _multiplier_count) {
    _multiplier_count = 0;
    _multiplier++;
  }
}

colour_t Player::player_colour(std::size_t player_number)
{
  return
      player_number == 0 ? 0xff0000ff :
      player_number == 1 ? 0xff5500ff :
      player_number == 2 ? 0xffaa00ff :
      player_number == 3 ? 0xffff00ff : 0x00ff00ff;
}

void Player::activate_magic_shots()
{
  _magic_shot_timer = MAGICSHOT_COUNT;
}

void Player::activate_magic_shield()
{
  if (_shield) {
    return;
  }

  if (_bomb) {
    destroy_shape(5);
    _bomb = false;
  }
  _shield = true;
  add_shape(new Polygon(vec2(), 16, 10, 0xffffffff));
}

void Player::activate_bomb()
{
  if (_bomb) {
    return;
  }

  if (_shield) {
    destroy_shape(5);
    _shield = false;
  }
  _bomb = true;
  add_shape(new Polygon(vec2(-8, 0), 6, 5, 0xffffffff,
                        fixed::pi, 0, Polygon::T::POLYSTAR));
}

Shot::Shot(const vec2& position, Player* player,
           const vec2& direction, bool magic)
  : Ship(position, SHIP_NONE)
  , _player(player)
  , _velocity(direction)
  , _magic(magic)
  , _flash(false)
{
  _velocity = _velocity.normalised() * Player::SHOT_SPEED;
  add_shape(new Fill(vec2(), 2, 2, _player->colour()));
  add_shape(new Fill(vec2(), 1, 1, _player->colour() & 0xffffff33));
  add_shape(new Fill(vec2(), 3, 3, _player->colour() & 0xffffff33));
}

void Shot::render() const
{
  if (z0().mode() == z0Game::WHAT_MODE) {
    return;
  }
  if (_flash) {
    render_with_colour(0xffffffff);
  }
  else {
    Ship::render();
  }
}

void Shot::update()
{
  if (_magic) {
    _flash = z::rand_int(2) != 0;
  }

  move(_velocity);
  bool onScreen =
      shape().centre.x >= -4 && shape().centre.x < 4 + Lib::WIDTH &&
      shape().centre.y >= -4 && shape().centre.y < 4 + Lib::HEIGHT;
  if (!onScreen) {
    destroy();
    return;
  }

  z0Game::ShipList kill = z0().collision_list(shape().centre, VULNERABLE);
  for (const auto& ship : kill) {
    ship->damage(1, _magic, _player);
    if (!_magic) {
      destroy();
    }
  }

  if (z0().any_collision(shape().centre, SHIELD) ||
      (!_magic && z0().any_collision(shape().centre, VULNSHIELD))) {
    destroy();
  }
}

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
    add_shape(new Polygon(vec2(), 11, 10, 0xffffffff,
                          fixed::pi / 2, 0, Polygon::T::POLYSTAR));
    break;
  }
}

void Powerup::update()
{
  shapes()[0]->colour = Player::player_colour(_frame / 2);
  _frame = (_frame + 1) % (Lib::PLAYERS * 2);
  shapes()[1]->colour = Player::player_colour(_frame / 2);

  static const int32_t rotate_time = 100;
  if (!is_on_screen()) {
    _dir = get_screen_centre() - shape().centre;
  }
  else {
    if (_first_frame) {
      _dir = vec2::from_polar(z::rand_fixed() * 2 * fixed::pi, 1);
    }

    _dir = _dir.rotated(2 * fixed::hundredth * (_rotate ? 1 : -1));
    if (!z::rand_int(rotate_time)) {
      _rotate = !_rotate;
    }
  }
  _first_frame = false;

  Player* p = nearest_player();
  bool alive = !p->is_killed();
  vec2 pv = p->shape().centre - shape().centre;
  if (pv.length() <= 40 && alive) {
    _dir = pv;
  }
  _dir = _dir.normalised();

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
      source->activate_magic_shots();
      play_sound(Lib::SOUND_POWERUP_OTHER);
      break;

    case SHIELD:
      source->activate_magic_shield();
      play_sound(Lib::SOUND_POWERUP_OTHER);
      break;

    case BOMB:
      source->activate_bomb();
      play_sound(Lib::SOUND_POWERUP_OTHER);
      break;
    }
    lib().rumble(source->player_number(), 6);
  }

  int r = 5 + z::rand_int(5);
  for (int i = 0; i < r; i++) {
    vec2 dir = vec2::from_polar(z::rand_fixed() * 2 * fixed::pi, 6);
    spawn(Particle(to_float(shape().centre), 0xffffffff,
                   to_float(dir), 4 + z::rand_int(8)));
  }
  destroy();
}