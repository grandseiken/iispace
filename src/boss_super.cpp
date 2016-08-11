#include "boss_super.h"
#include "player.h"

static const int32_t SB_BASE_HP = 520;
static const int32_t SB_ARC_HP = 75;

SuperBossArc::SuperBossArc(const vec2& position, int32_t players, int32_t cycle, int32_t i,
                           Ship* boss, int32_t timer)
: Boss(position, GameModal::boss_list(0), SB_ARC_HP, players, cycle)
, _boss(boss)
, _i(i)
, _timer(timer)
, _stimer(0) {
  add_shape(new PolyArc(vec2(), 140, 32, 2, 0, i * 2 * fixed::pi / 16, 0));
  add_shape(new PolyArc(vec2(), 135, 32, 2, 0, i * 2 * fixed::pi / 16, 0));
  add_shape(new PolyArc(vec2(), 130, 32, 2, 0, i * 2 * fixed::pi / 16, 0));
  add_shape(new PolyArc(vec2(), 125, 32, 2, 0, i * 2 * fixed::pi / 16, SHIELD));
  add_shape(new PolyArc(vec2(), 120, 32, 2, 0, i * 2 * fixed::pi / 16, 0));
  add_shape(new PolyArc(vec2(), 115, 32, 2, 0, i * 2 * fixed::pi / 16, 0));
  add_shape(new PolyArc(vec2(), 110, 32, 2, 0, i * 2 * fixed::pi / 16, 0));
  add_shape(new PolyArc(vec2(), 105, 32, 2, 0, i * 2 * fixed::pi / 16, SHIELD));
}

void SuperBossArc::update() {
  shape().rotate(fixed(6) / 1000);
  for (int32_t i = 0; i < 8; ++i) {
    shapes()[7 - i]->colour =
        z::colour_cycle(is_hp_low() ? 0xff000099 : 0xff0000ff, (i * 32 + 2 * _timer) % 256);
  }
  ++_timer;
  ++_stimer;
  if (_stimer == 64) {
    shapes()[0]->category = DANGEROUS | VULNERABLE;
  }
}

void SuperBossArc::render() const {
  if (_stimer >= 64 || _stimer % 4 < 2) {
    Boss::render(false);
  }
}

int32_t SuperBossArc::get_damage(int32_t damage, bool magic) {
  if (damage >= Player::BOMB_DAMAGE) {
    return damage / 2;
  }
  return damage;
}

void SuperBossArc::on_destroy() {
  vec2 d = vec2::from_polar(_i * 2 * fixed::pi / 16 + shape().rotation(), 120);
  move(d);
  explosion();
  explosion(0xffffffff, 12);
  explosion(shapes()[0]->colour, 24);
  explosion(0xffffffff, 36);
  explosion(shapes()[0]->colour, 48);
  play_sound_random(Lib::SOUND_EXPLOSION);
  ((SuperBoss*) _boss)->_destroyed[_i] = true;
}

SuperBoss::SuperBoss(int32_t players, int32_t cycle)
: Boss(vec2(Lib::WIDTH / 2, -Lib::HEIGHT / (2 + fixed::half)), GameModal::BOSS_3A, SB_BASE_HP,
       players, cycle)
, _players(players)
, _cycle(cycle)
, _ctimer(0)
, _timer(0)
, _state(STATE_ARRIVE)
, _snakes(0) {
  add_shape(new Polygon(vec2(), 40, 32, 0, 0, DANGEROUS | VULNERABLE));
  add_shape(new Polygon(vec2(), 35, 32, 0, 0, 0));
  add_shape(new Polygon(vec2(), 30, 32, 0, 0, SHIELD));
  add_shape(new Polygon(vec2(), 25, 32, 0, 0, 0));
  add_shape(new Polygon(vec2(), 20, 32, 0, 0, 0));
  add_shape(new Polygon(vec2(), 15, 32, 0, 0, 0));
  add_shape(new Polygon(vec2(), 10, 32, 0, 0, 0));
  add_shape(new Polygon(vec2(), 5, 32, 0, 0, 0));
  for (int32_t i = 0; i < 16; ++i) {
    _destroyed.push_back(false);
  }
  set_ignore_damage_colour_index(8);
}

void SuperBoss::update() {
  if (_arcs.empty()) {
    for (int32_t i = 0; i < 16; ++i) {
      SuperBossArc* s = new SuperBossArc(shape().centre, _players, _cycle, i, this);
      spawn(s);
      _arcs.push_back(s);
    }
  } else {
    for (int32_t i = 0; i < 16; ++i) {
      if (_destroyed[i]) {
        continue;
      }
      _arcs[i]->shape().centre = shape().centre;
    }
  }
  vec2 move_vec;
  shape().rotate(fixed(6) / 1000);
  colour_t c = z::colour_cycle(0xff0000ff, 2 * _ctimer);
  for (int32_t i = 0; i < 8; ++i) {
    shapes()[7 - i]->colour = z::colour_cycle(0xff0000ff, (i * 32 + 2 * _ctimer) % 256);
  }
  ++_ctimer;
  if (shape().centre.y < Lib::HEIGHT / 2) {
    move_vec = vec2(0, 1);
  } else if (_state == STATE_ARRIVE) {
    _state = STATE_IDLE;
  }

  ++_timer;
  if (_state == STATE_ATTACK && _timer == 192) {
    _timer = 100;
    _state = STATE_IDLE;
  }

  static const fixed d5d1000 = fixed(5) / 1000;
  static const fixed pi2d64 = 2 * fixed::pi / 64;
  static const fixed pi2d32 = 2 * fixed::pi / 32;

  if (_state == STATE_IDLE && is_on_screen() && _timer != 0 && _timer % 300 == 0) {
    int32_t r = z::rand_int(6);
    if (r == 0 || r == 1) {
      _snakes = 16;
    } else if (r == 2 || r == 3) {
      _state = STATE_ATTACK;
      _timer = 0;
      fixed f = z::rand_fixed() * (2 * fixed::pi);
      fixed rf = d5d1000 * (1 + z::rand_int(2));
      for (int32_t i = 0; i < 32; ++i) {
        vec2 d = vec2::from_polar(f + i * pi2d32, 1);
        if (r == 2) {
          rf = d5d1000 * (1 + z::rand_int(4));
        }
        spawn(new Snake(shape().centre + d * 16, c, d, rf));
        play_sound_random(Lib::SOUND_BOSS_ATTACK);
      }
    } else if (r == 5) {
      _state = STATE_ATTACK;
      _timer = 0;
      fixed f = z::rand_fixed() * (2 * fixed::pi);
      for (int32_t i = 0; i < 64; ++i) {
        vec2 d = vec2::from_polar(f + i * pi2d64, 1);
        spawn(new Snake(shape().centre + d * 16, c, d));
        play_sound_random(Lib::SOUND_BOSS_ATTACK);
      }
    } else {
      _state = STATE_ATTACK;
      _timer = 0;
      _snakes = 32;
    }
  }

  if (_state == STATE_IDLE && is_on_screen() && _timer % 300 == 200 && !is_destroyed()) {
    std::vector<int32_t> wide3;
    int32_t timer = 0;
    for (int32_t i = 0; i < 16; ++i) {
      if (_destroyed[(i + 15) % 16] && _destroyed[i] && _destroyed[(i + 1) % 16]) {
        wide3.push_back(i);
      }
      if (!_destroyed[i]) {
        timer = _arcs[i]->GetTimer();
      }
    }
    if (!wide3.empty()) {
      int32_t r = z::rand_int(int32_t(wide3.size()));
      SuperBossArc* s = new SuperBossArc(shape().centre, _players, _cycle, wide3[r], this, timer);
      s->shape().set_rotation(shape().rotation() - (fixed(6) / 1000));
      spawn(s);
      _arcs[wide3[r]] = s;
      _destroyed[wide3[r]] = false;
      play_sound(Lib::SOUND_ENEMY_SPAWN);
    }
  }
  static const fixed pi2d128 = 2 * fixed::pi / 128;
  if (_state == STATE_IDLE && _timer % 72 == 0) {
    for (int32_t i = 0; i < 128; ++i) {
      vec2 d = vec2::from_polar(i * pi2d128, 1);
      spawn(new RainbowShot(shape().centre + d * 42, move_vec + d * 3, this));
      play_sound_random(Lib::SOUND_BOSS_FIRE);
    }
  }

  if (_snakes) {
    --_snakes;
    vec2 d = vec2::from_polar(z::rand_fixed() * (2 * fixed::pi), z::rand_int(32) + z::rand_int(16));
    spawn(new Snake(d + shape().centre, c));
    play_sound_random(Lib::SOUND_ENEMY_SPAWN);
  }
  move(move_vec);
}

int32_t SuperBoss::get_damage(int32_t damage, bool magic) {
  return damage;
}

void SuperBoss::on_destroy() {
  set_killed();
  for (const auto& ship : game().all_ships(SHIP_ENEMY)) {
    if (ship != this) {
      ship->damage(Player::BOMB_DAMAGE * 100, false, 0);
    }
  }
  explosion();
  explosion(0xffffffff, 12);
  explosion(shapes()[0]->colour, 24);
  explosion(0xffffffff, 36);
  explosion(shapes()[0]->colour, 48);

  int32_t n = 1;
  for (int32_t i = 0; i < 16; ++i) {
    vec2 v =
        vec2::from_polar(z::rand_fixed() * (2 * fixed::pi), 8 + z::rand_int(64) + z::rand_int(64));
    colour_t c = z::colour_cycle(shapes()[0]->colour, n * 2);
    _fireworks.push_back(std::make_pair(n, std::make_pair(shape().centre + v, c)));
    n += i;
  }
  for (int32_t i = 0; i < PLAYERS; i++) {
    lib().rumble(i, 25);
  }
  play_sound(Lib::SOUND_EXPLOSION);

  for (const auto& ship : game().players()) {
    Player* p = (Player*) ship;
    if (!p->is_killed()) {
      p->add_score(get_score() / game().alive_players());
    }
  }

  for (int32_t i = 0; i < 8; ++i) {
    spawn(new Powerup(shape().centre, Powerup::BOMB));
  }
}

SnakeTail::SnakeTail(const vec2& position, colour_t colour)
: Enemy(position, SHIP_NONE, 1), _tail(0), _head(0), _timer(150), _dtimer(0) {
  add_shape(new Polygon(vec2(), 10, 4, colour, 0, DANGEROUS | SHIELD | VULNSHIELD));
  set_bounding_width(22);
  set_score(0);
}

void SnakeTail::update() {
  static const fixed z15 = fixed::hundredth * 15;
  shape().rotate(z15);
  --_timer;
  if (!_timer) {
    on_destroy(false);
    destroy();
    explosion();
  }
  if (_dtimer) {
    --_dtimer;
    if (!_dtimer) {
      if (_tail) {
        _tail->_dtimer = 4;
      }
      on_destroy(false);
      destroy();
      explosion();
      play_sound_random(Lib::SOUND_ENEMY_DESTROY);
    }
  }
}

void SnakeTail::on_destroy(bool bomb) {
  if (_head) {
    _head->_tail = 0;
  }
  if (_tail) {
    _tail->_head = 0;
  }
  _head = 0;
  _tail = 0;
}

Snake::Snake(const vec2& position, colour_t colour, const vec2& dir, fixed rot)
: Enemy(position, SHIP_NONE, 5)
, _tail(0)
, _timer(0)
, _dir(0, 0)
, _count(0)
, _colour(colour)
, _shot_snake(false)
, _shot_rot(rot) {
  add_shape(new Polygon(vec2(), 14, 3, colour, 0, VULNERABLE));
  add_shape(new Polygon(vec2(), 10, 3, 0, 0, DANGEROUS));
  set_score(0);
  set_bounding_width(32);
  set_enemy_value(5);
  set_destroy_sound(Lib::SOUND_PLAYER_DESTROY);
  if (dir == vec2()) {
    int32_t r = z::rand_int(4);
    _dir = r == 0 ? vec2(1, 0) : r == 1 ? vec2(-1, 0) : r == 2 ? vec2(0, 1) : vec2(0, -1);
  } else {
    _dir = dir.normalised();
    _shot_snake = true;
  }
  shape().set_rotation(_dir.angle());
}

void Snake::update() {
  if (!(shape().centre.x >= -8 && shape().centre.x <= Lib::WIDTH + 8 && shape().centre.y >= -8 &&
        shape().centre.y <= Lib::HEIGHT + 8)) {
    _tail = 0;
    destroy();
    return;
  }

  colour_t c = z::colour_cycle(_colour, _timer % 256);
  shapes()[0]->colour = c;
  _timer++;
  if (_timer % (_shot_snake ? 4 : 8) == 0) {
    SnakeTail* t = new SnakeTail(shape().centre, (c & 0xffffff00) | 0x00000099);
    if (_tail != 0) {
      _tail->_head = t;
      t->_tail = _tail;
    }
    play_sound_random(Lib::SOUND_BOSS_FIRE, 0, .5f);
    _tail = t;
    spawn(t);
  }
  if (!_shot_snake && _timer % 48 == 0 && !z::rand_int(3)) {
    _dir = _dir.rotated((z::rand_int(2) ? 1 : -1) * fixed::pi / 2);
    shape().set_rotation(_dir.angle());
  }
  move(_dir * (_shot_snake ? 4 : 2));
  if (_timer % 8 == 0) {
    _dir = _dir.rotated(8 * _shot_rot);
    shape().set_rotation(_dir.angle());
  }
}

void Snake::on_destroy(bool bomb) {
  if (_tail) {
    _tail->_dtimer = 4;
  }
}

RainbowShot::RainbowShot(const vec2& position, const vec2& velocity, Ship* boss)
: BossShot(position, velocity), _boss(boss), _timer(0) {}

void RainbowShot::update() {
  BossShot::update();
  static const vec2 center = vec2(Lib::WIDTH / 2, Lib::HEIGHT / 2);

  if ((shape().centre - center).length() > 100 && _timer % 2 == 0) {
    const auto& list = game().collision_list(shape().centre, SHIELD);
    SuperBoss* s = (SuperBoss*) _boss;
    for (std::size_t i = 0; i < list.size(); ++i) {
      bool boss = false;
      for (std::size_t j = 0; j < s->_arcs.size(); ++j) {
        if (list[i] == s->_arcs[j]) {
          boss = true;
          break;
        }
      }
      if (!boss) {
        continue;
      }

      explosion(0, 4, true, to_float(shape().centre - _dir));
      destroy();
      return;
    }
  }

  ++_timer;
  static const fixed r = 6 * (8 * fixed::hundredth / 10);
  if (_timer % 8 == 0) {
    _dir = _dir.rotated(r);
  }
}
