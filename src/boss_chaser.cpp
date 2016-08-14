#include "boss_chaser.h"
#include <algorithm>
#include "player.h"

static const int32_t CB_BASE_HP = 60;
static const int32_t CB_MAX_SPLIT = 7;
static const fixed CB_SPEED = 4;

int32_t ChaserBoss::_count;
bool ChaserBoss::_has_counted;
int32_t ChaserBoss::_shared_hp;

// power(HP_REDUCE_POWER = 1.7, split).
static const fixed HP_REDUCE_POWER_lookup[8] = {fixed(1),
                                                1 + 7 * (fixed(1) / 10),
                                                2 + 9 * (fixed(1) / 10),
                                                4 + 9 * (fixed(1) / 10),
                                                8 + 4 * (fixed(1) / 10),
                                                14 + 2 * (fixed(1) / 10),
                                                24 + 2 * (fixed(1) / 10),
                                                41};

// power(1.5, split).
static const fixed ONE_AND_HALF_lookup[8] = {fixed(1),
                                             1 + (fixed(1) / 2),
                                             2 + (fixed(1) / 4),
                                             3 + 4 * (fixed(1) / 10),
                                             5 + (fixed(1) / 10),
                                             7 + 6 * (fixed(1) / 10),
                                             11 + 4 * (fixed(1) / 10),
                                             17 + (fixed(1) / 10)};

// power(1.1, split).
static const fixed ONE_PT_ONE_lookup[8] = {fixed(1),
                                           1 + (fixed(1) / 10),
                                           1 + 2 * (fixed(1) / 10),
                                           1 + 3 * (fixed(1) / 10),
                                           1 + 5 * (fixed(1) / 10),
                                           1 + 6 * (fixed(1) / 10),
                                           1 + 8 * (fixed(1) / 10),
                                           1 + 9 * (fixed(1) / 10)};

// power(1.15, split).
static const fixed ONE_PT_ONE_FIVE_lookup[8] = {fixed(1),
                                                1 + 15 * (fixed(1) / 100),
                                                1 + 3 * (fixed(1) / 10),
                                                1 + 5 * (fixed(1) / 10),
                                                1 + 7 * (fixed(1) / 10),
                                                2,
                                                2 + 3 * (fixed(1) / 10),
                                                2 + 7 * (fixed(1) / 10)};

// power(1.2, split).
static const fixed ONE_PT_TWO_lookup[8] = {fixed(1),
                                           1 + 2 * (fixed(1) / 10),
                                           1 + 4 * (fixed(1) / 10),
                                           1 + 7 * (fixed(1) / 10),
                                           2 + (fixed(1) / 10),
                                           2 + (fixed(1) / 2),
                                           3,
                                           3 + 6 * (fixed(1) / 10)};

// power(1.15, remaining).
static const int32_t ONE_PT_ONE_FIVE_intLookup[128] = {
    1,        1,        1,        2,        2,        2,        2,        3,        3,
    4,        4,        5,        5,        6,        7,        8,        9,        11,
    12,       14,       16,       19,       22,       25,       29,       33,       38,
    44,       50,       58,       66,       76,       88,       101,      116,      133,
    153,      176,      203,      233,      268,      308,      354,      407,      468,
    539,      620,      713,      819,      942,      1084,     1246,     1433,     1648,
    1895,     2180,     2507,     2883,     3315,     3812,     4384,     5042,     5798,
    6668,     7668,     8818,     10140,    11662,    13411,    15422,    17736,    20396,
    23455,    26974,    31020,    35673,    41024,    47177,    54254,    62392,    71751,
    82513,    94890,    109124,   125493,   144316,   165964,   190858,   219487,   252410,
    290272,   333813,   383884,   441467,   507687,   583840,   671416,   772129,   887948,
    1021140,  1174311,  1350458,  1553026,  1785980,  2053877,  2361959,  2716252,  3123690,
    3592244,  4131080,  4750742,  5463353,  6282856,  7225284,  8309077,  9555438,  10988754,
    12637066, 14532626, 16712520, 19219397, 22102306, 25417652, 29230299, 33614843, 38657069,
    44455628, 51123971};

ChaserBoss::ChaserBoss(int32_t players, int32_t cycle, int32_t split, const vec2& position,
                       int32_t time, int32_t stagger)
: Boss(!split ? vec2(Lib::WIDTH / 2, -Lib::HEIGHT / 2) : position, GameModal::BOSS_1C,
       1 + CB_BASE_HP / (fixed_c::half + HP_REDUCE_POWER_lookup[split]).to_int(), players, cycle,
       split <= 1)
, _on_screen(split != 0)
, _move(false)
, _timer(time)
, _dir()
, _last_dir()
, _players(players)
, _cycle(cycle)
, _split(split)
, _stagger(stagger) {
  add_shape(new Polygon(vec2(), 10 * ONE_AND_HALF_lookup[CB_MAX_SPLIT - _split], 5, 0x3399ffff, 0,
                        0, Polygon::T::POLYGRAM));
  add_shape(new Polygon(vec2(), 9 * ONE_AND_HALF_lookup[CB_MAX_SPLIT - _split], 5, 0x3399ffff, 0, 0,
                        Polygon::T::POLYGRAM));
  add_shape(new Polygon(vec2(), 8 * ONE_AND_HALF_lookup[CB_MAX_SPLIT - _split], 5, 0, 0,
                        DANGEROUS | VULNERABLE, Polygon::T::POLYGRAM));
  add_shape(new Polygon(vec2(), 7 * ONE_AND_HALF_lookup[CB_MAX_SPLIT - _split], 5, 0, 0, SHIELD,
                        Polygon::T::POLYGRAM));

  set_ignore_damage_colour_index(2);
  set_bounding_width(10 * ONE_AND_HALF_lookup[CB_MAX_SPLIT - _split]);
  if (!_split) {
    _shared_hp = 0;
    _count = 0;
  }
}

void ChaserBoss::update() {
  const auto& remaining = game().all_ships();
  _last_dir = _dir.normalised();
  if (is_on_screen()) {
    _on_screen = true;
  }

  if (_timer) {
    _timer--;
  }
  if (_timer <= 0) {
    _timer = TIMER * (_move + 1);
    int32_t count = remaining.size() - game().players().size();
    if (_split != 0 &&
        (_move || z::rand_int(8 + _split) == 0 || count <= 4 ||
         !z::rand_int(ONE_PT_ONE_FIVE_intLookup[std::max(0, std::min(127, count))]))) {
      _move = !_move;
    }
    if (_move) {
      _dir = CB_SPEED * ONE_PT_ONE_lookup[_split] *
          (nearest_player()->shape().centre - shape().centre).normalised();
    }
  }
  if (_move) {
    move(_dir);
  } else {
    const auto& nearby = game().all_ships(SHIP_PLAYER | SHIP_BOSS);
    const fixed attract = 256 * ONE_PT_ONE_lookup[CB_MAX_SPLIT - _split];
    const fixed align = 128 * ONE_PT_ONE_FIVE_lookup[CB_MAX_SPLIT - _split];
    const fixed repulse = 64 * ONE_PT_TWO_lookup[CB_MAX_SPLIT - _split];
    static const fixed c_attract = -(1 + 2 * fixed_c::tenth);
    static const fixed c_dir0 = CB_SPEED / 14;
    static const fixed c_dir1 = 8 * CB_SPEED / (14 * 9);
    static const fixed c_dir2 = 8 * CB_SPEED / (14 * 11);
    static const fixed c_dir3 = 8 * CB_SPEED / (14 * 2);
    static const fixed c_dir4 = 8 * CB_SPEED / (14 * 3);
    static const fixed c_rotate = fixed_c::hundredth * 5 / fixed(CB_MAX_SPLIT);

    _dir = _last_dir;
    if (_stagger ==
        _count % (_split == 0 ? 1 : _split == 1 ? 2 : _split == 2 ? 4 : _split == 3 ? 8 : 16)) {
      _dir.x = _dir.y = 0;
      for (const auto& ship : nearby) {
        if (ship == this) {
          continue;
        }

        vec2 v = shape().centre - ship->shape().centre;
        fixed len = v.length();
        if (len > 0) {
          v /= len;
        }
        vec2 p;
        if (ship->type() & SHIP_BOSS) {
          ChaserBoss* c = (ChaserBoss*) ship;
          fixed pow = ONE_PT_ONE_FIVE_lookup[CB_MAX_SPLIT - c->_split];
          v *= pow;
          p = c->_last_dir * pow;
        } else {
          p = vec2::from_polar(ship->shape().rotation(), 1);
        }

        if (len > attract) {
          continue;
        }
        // Attract.
        _dir += v * c_attract;
        if (len > align) {
          continue;
        }
        // Align.
        _dir += p;
        if (len > repulse) {
          continue;
        }
        // Repulse.
        _dir += v * 3;
      }
    }
    if (int32_t(remaining.size()) - game().players().size() < 4 && _split >= CB_MAX_SPLIT - 1) {
      if ((shape().centre.x < 32 && _dir.x < 0) ||
          (shape().centre.x >= Lib::WIDTH - 32 && _dir.x > 0)) {
        _dir.x = -_dir.x;
      }
      if ((shape().centre.y < 32 && _dir.y < 0) ||
          (shape().centre.y >= Lib::HEIGHT - 32 && _dir.y > 0)) {
        _dir.y = -_dir.y;
      }
    } else if (int32_t(remaining.size()) - game().players().size() < 8 &&
               _split >= CB_MAX_SPLIT - 1) {
      if ((shape().centre.x < 0 && _dir.x < 0) || (shape().centre.x >= Lib::WIDTH && _dir.x > 0)) {
        _dir.x = -_dir.x;
      }
      if ((shape().centre.y < 0 && _dir.y < 0) || (shape().centre.y >= Lib::HEIGHT && _dir.y > 0)) {
        _dir.y = -_dir.y;
      }
    } else {
      if ((shape().centre.x < -32 && _dir.x < 0) ||
          (shape().centre.x >= Lib::WIDTH + 32 && _dir.x > 0)) {
        _dir.x = -_dir.x;
      }
      if ((shape().centre.y < -32 && _dir.y < 0) ||
          (shape().centre.y >= Lib::HEIGHT + 32 && _dir.y > 0)) {
        _dir.y = -_dir.y;
      }
    }

    if (shape().centre.x < -256) {
      _dir = vec2(1, 0);
    } else if (shape().centre.x >= Lib::WIDTH + 256) {
      _dir = vec2(-1, 0);
    } else if (shape().centre.y < -256) {
      _dir = vec2(0, 1);
    } else if (shape().centre.y >= Lib::HEIGHT + 256) {
      _dir = vec2(0, -1);
    } else {
      _dir = _dir.normalised();
    }

    _dir = _split == 0 ? _dir + _last_dir * 7 : _split == 1 ? _dir * 2 + _last_dir * 7 : _split == 2
                ? _dir * 4 + _last_dir * 7
                : _split == 3 ? _dir + _last_dir : _dir * 2 + _last_dir;

    _dir *= ONE_PT_ONE_lookup[_split] * (_split == 0 ? c_dir0 : _split == 1 ? c_dir1 : _split == 2
                                                     ? c_dir2
                                                     : _split == 3 ? c_dir3 : c_dir4);
    move(_dir);
    shape().rotate(fixed_c::hundredth * 2 + fixed(_split) * c_rotate);
  }
  _shared_hp = 0;
  if (!_has_counted) {
    _has_counted = true;
    ++_count;
  }
}

void ChaserBoss::render() const {
  Boss::render();
  static std::vector<int32_t> hp_lookup;
  if (hp_lookup.empty()) {
    int32_t hp = 0;
    for (int32_t i = 0; i < 8; i++) {
      hp = 2 * hp +
          CalculateHP(1 + CB_BASE_HP / (fixed_c::half + HP_REDUCE_POWER_lookup[7 - i]).to_int(),
                      _players, _cycle);
      hp_lookup.push_back(hp);
    }
  }
  _shared_hp += (_split == 7 ? 0 : 2 * hp_lookup[6 - _split]) + get_remaining_hp() * 30;
  if (_on_screen) {
    game().render_hp_bar(float(_shared_hp) / float(hp_lookup[CB_MAX_SPLIT]));
  }
}

int32_t ChaserBoss::get_damage(int32_t damage, bool magic) {
  return damage;
}

void ChaserBoss::on_destroy() {
  bool last = false;
  if (_split < CB_MAX_SPLIT) {
    for (int32_t i = 0; i < 2; i++) {
      vec2 d = vec2::from_polar(i * fixed_c::pi + shape().rotation(),
                                8 * ONE_PT_TWO_lookup[CB_MAX_SPLIT - 1 - _split]);
      Ship* s = new ChaserBoss(
          _players, _cycle, _split + 1, shape().centre + d, (i + 1) * TIMER / 2,
          z::rand_int(_split + 1 == 1 ? 2 : _split + 1 == 2 ? 4 : _split + 1 == 3 ? 8 : 16));
      spawn(s);
      s->shape().set_rotation(shape().rotation());
    }
  } else {
    last = true;
    for (const auto& ship : game().all_ships(SHIP_ENEMY)) {
      if (!ship->is_destroyed() && ship != this) {
        last = false;
        break;
      }
    }

    if (last) {
      set_killed();
      for (int32_t i = 0; i < PLAYERS; i++) {
        lib().rumble(i, 25);
      }
      for (const auto& ship : game().players()) {
        Player* p = (Player*) ship;
        if (!p->is_killed()) {
          p->add_score(get_score() / game().alive_players());
        }
      }
      int32_t n = 1;
      for (int32_t i = 0; i < 16; ++i) {
        vec2 v = vec2::from_polar(z::rand_fixed() * (2 * fixed_c::pi),
                                  8 + z::rand_int(64) + z::rand_int(64));
        _fireworks.push_back(
            std::make_pair(n, std::make_pair(shape().centre + v, shapes()[0]->colour)));
        n += i;
      }
    }
  }

  for (int32_t i = 0; i < PLAYERS; i++) {
    lib().rumble(i, _split < 3 ? 10 : 3);
  }

  explosion();
  explosion(0xffffffff, 12);
  if (_split < 3 || last) {
    explosion(shapes()[0]->colour, 24);
  }
  if (_split < 2 || last) {
    explosion(0xffffffff, 36);
  }
  if (_split < 1 || last) {
    explosion(shapes()[0]->colour, 48);
  }

  if (_split < 3 || last) {
    play_sound(Lib::SOUND_EXPLOSION);
  } else {
    play_sound_random(Lib::SOUND_EXPLOSION);
  }
}
