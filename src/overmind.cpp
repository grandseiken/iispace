#include "overmind.h"
#include "enemy.h"
#include "boss.h"
#include "player.h"
#include "stars.h"
#include <algorithm>

int32_t Overmind::_trow;
z0Game* Overmind::_tz0;
int32_t Overmind::_power = 0;
int32_t Overmind::_hard_already = 0;

Overmind::Overmind(z0Game& z0)
  : _z0(z0),
  _timer(POWERUP_TIME),
  _count(0),
  _non_wall_count(0),
  _levels_mod(0),
  _groups_mod(0),
  _boss_mod_bosses(0),
  _boss_mod_fights(0),
  _boss_mod_secret(0),
  _can_face_secret_boss(false),
  _is_boss_next(false),
  _is_boss_level(false),
  _bosses_to_go(0)
{
  _tz0 = 0;
  _trow = 0;
  add_formations();
}

void Overmind::reset(bool can_face_secret_boss)
{
  int32_t a, b, c;
  _boss1_queue.clear();
  _boss2_queue.clear();

  a = z::rand_int(3);
  b = z::rand_int(2);
  if (a == 0 || (a == 1 && b == 1)) {
    b++;
  }
  c = (a + b == 3) ? 0 : (a + b == 2) ? 1 : 2;

  _boss1_queue.push_back(a);
  _boss1_queue.push_back(b);
  _boss1_queue.push_back(c);

  a = z::rand_int(3);
  b = z::rand_int(2);
  if (a == 0 || (a == 1 && b == 1)) {
    b++;
  }
  c = (a + b == 3) ? 0 : (a + b == 2) ? 1 : 2;

  _boss2_queue.push_back(a);
  _boss2_queue.push_back(b);
  _boss2_queue.push_back(c);

  _is_boss_next = false;
  _is_boss_level = false;
  _levels_mod = 0;
  _groups_mod = 0;
  _boss_mod_bosses = 0;
  _boss_mod_fights = 0;
  _boss_mod_secret = 0;
  _can_face_secret_boss = can_face_secret_boss;
  _powerup_mod = 0;
  _lives_target = 0;
  _elapsed_time = 0;
  _bosses_to_go = 0;
  _waves_total = 0;

  if (_z0.mode() == z0Game::BOSS_MODE) {
    _power = 0;
    _timer = 0;
    return;
  }
  _power = INITIAL_POWER + 2 - _z0.count_players() * 2;
  if (_z0.mode() == z0Game::HARD_MODE) {
    _power += 20;
    _waves_total = 15;
  }
  _timer = POWERUP_TIME;
  _boss_rest_timer = BOSS_REST_TIME / 8;

  Stars::clear();
}

void Overmind::add_formation(spawn_formation_function function)
{
  _formations.push_back(formation(function(true, 0), function));
  std::stable_sort(_formations.begin(), _formations.end(), &sort_formations);
}

void Overmind::update()
{
  ++_elapsed_time;
  Stars::update();

  if (_z0.mode() == z0Game::BOSS_MODE) {
    if (_count <= 0) {
      Stars::change();
      if (_boss_mod_bosses < 6) {
        if (_boss_mod_bosses)
        for (int32_t i = 0; i < _z0.count_players(); ++i) {
          spawn_boss_reward();
        }
        boss_mode_boss();
      }
      if (_boss_mod_bosses < 7) {
        _boss_mod_bosses++;
      }
    }
    return;
  }

  if (_boss_rest_timer > 0) {
    --_boss_rest_timer;
    return;
  }

  _timer++;
  if (_timer == POWERUP_TIME && !_is_boss_level) {
    spawn_powerup();
  }

  int32_t boss_cycles = _boss_mod_fights;
  int32_t trigger_stage = _groups_mod + boss_cycles +
      2 * (_z0.mode() == z0Game::HARD_MODE);
  int32_t trigger_val = INITIAL_TRIGGERVAL;
  for (int32_t i = 0; i < trigger_stage; ++i) {
    trigger_val += i < 2 ? 4 :
                   i < 7 - _z0.count_players() ? 3 : 2;
  }
  if (trigger_val < 0 || _is_boss_level || _is_boss_next) {
    trigger_val = 0;
  }

  if ((_timer > TIMER && !_is_boss_level && !_is_boss_next) ||
      _count <= trigger_val) {
    if (_timer < POWERUP_TIME && !_is_boss_level) {
      spawn_powerup();
    }
    _timer = 0;
    Stars::change();

    if (_is_boss_level) {
      ++_boss_mod_bosses;
      _is_boss_level = false;
      if (_bosses_to_go <= 0) {
        spawn_boss_reward();
        ++_boss_mod_fights;
        _power += 2 - 2 * _z0.count_players();
        _boss_rest_timer = BOSS_REST_TIME;
        _bosses_to_go = 0;
      }
      else {
        _boss_rest_timer = BOSS_REST_TIME / 4;
      }
    }
    else {
      if (_is_boss_next) {
        --_bosses_to_go;
        _is_boss_next = _bosses_to_go > 0;
        _is_boss_level = true;
        boss();
      }
      else {
        wave();
        if (_waves_total < 5) {
          _power += 2;
        }
        else {
          ++_power;
        }
        ++_waves_total;
        ++_levels_mod;
      }
    }

    if (_levels_mod >= LEVELS_PER_GROUP) {
      _levels_mod = 0;
      _groups_mod++;
    }
    if (_groups_mod >= BASE_GROUPS_PER_BOSS + _z0.count_players()) {
      _groups_mod = 0;
      _is_boss_next = true;
      _bosses_to_go = _boss_mod_fights >= 4 ? 3 : _boss_mod_fights >= 2 ? 2 : 1;
    }
  }
}

void Overmind::on_enemy_destroy(const Ship& ship)
{
  _count -= ship.enemy_value();
  if (!(ship.type() & Ship::SHIP_WALL)) {
    _non_wall_count--;
  }
}

void Overmind::on_enemy_create(const Ship& ship)
{
  _count += ship.enemy_value();
  if (!(ship.type() & Ship::SHIP_WALL)) {
    _non_wall_count++;
  }
}

// Individual spawns
//------------------------------
void Overmind::spawn_powerup()
{
  if (_z0.all_ships(Ship::SHIP_POWERUP).size() >= 4) {
    return;
  }

  ++_powerup_mod;
  if (_powerup_mod == 4) {
    _powerup_mod = 0;
    ++_lives_target;
  }

  int32_t r = z::rand_int(4);
  vec2 v = r == 0 ? vec2(-Lib::WIDTH, Lib::HEIGHT / 2) :
           r == 1 ? vec2(Lib::WIDTH * 2, Lib::HEIGHT / 2) :
           r == 2 ? vec2(Lib::WIDTH / 2, -Lib::HEIGHT) :
                    vec2(Lib::WIDTH / 2, Lib::HEIGHT * 2);

  int32_t m = 4;
  for (int32_t i = 1; i <= Lib::PLAYERS; i++) {
    if (_z0.get_lives() <= _z0.count_players() - i) {
      m++;
    }
  }
  if (!_z0.get_lives()) {
    m += _z0.killed_players();
  }
  if (_lives_target > 0) {
    m += _lives_target;
  }
  if (_z0.killed_players() == 0 && _lives_target < -1) {
    m = 3;
  }
  r = z::rand_int(m);
  _tz0 = &_z0;

  spawn(new Powerup(v, r == 0 ? Powerup::BOMB :
                       r == 1 ? Powerup::MAGIC_SHOTS :
                       r == 2 ? Powerup::SHIELD :
                       (--_lives_target, Powerup::EXTRA_LIFE)));
  _tz0 = 0;
}

void Overmind::spawn_boss_reward()
{
  _tz0 = &_z0;
  int32_t r = z::rand_int(4);
  vec2 v = r == 0 ? vec2(-Lib::WIDTH / 4, Lib::HEIGHT / 2) :
           r == 1 ? vec2(Lib::WIDTH + Lib::WIDTH / 4, Lib::HEIGHT / 2) :
           r == 2 ? vec2(Lib::WIDTH / 2, -Lib::HEIGHT / 4) :
                    vec2(Lib::WIDTH / 2, Lib::HEIGHT + Lib::HEIGHT / 4);

  spawn(new Powerup(v, Powerup::EXTRA_LIFE));
  _tz0 = 0;
  if (_z0.mode() != z0Game::BOSS_MODE) {
    spawn_powerup();
  }
}

void Overmind::spawn_follow(int32_t num, int32_t div, int32_t side)
{
  if (side == 0 || side == 1) {
    spawn(new Follow(spawn_point(false, _trow, num, div)));
  }
  if (side == 0 || side == 2) {
    spawn(new Follow(spawn_point(true, _trow, num, div)));
  }
}

void Overmind::spawn_chaser(int32_t num, int32_t div, int32_t side)
{
  if (side == 0 || side == 1) {
    spawn(new Chaser(spawn_point(false, _trow, num, div)));
  }
  if (side == 0 || side == 2) {
    spawn(new Chaser(spawn_point(true, _trow, num, div)));
  }
}

void Overmind::spawn_square(int32_t num, int32_t div, int32_t side)
{
  if (side == 0 || side == 1) {
    spawn(new Square(spawn_point(false, _trow, num, div)));
  }
  if (side == 0 || side == 2) {
    spawn(new Square(spawn_point(true, _trow, num, div)));
  }
}

void Overmind::spawn_wall(int32_t num, int32_t div, int32_t side, bool dir)
{
  if (side == 0 || side == 1) {
    spawn(new Wall(spawn_point(false, _trow, num, div), dir));
  }
  if (side == 0 || side == 2) {
    spawn(new Wall(spawn_point(true, _trow, num, div), dir));
  }
}

void Overmind::spawn_follow_hub(int32_t num, int32_t div, int32_t side)
{
  if (side == 0 || side == 1) {
    bool p1 = z::rand_int(64) < std::min(32, _power - 32) - _hard_already;
    if (p1) {
      _hard_already += 2;
    }
    bool p2 = z::rand_int(64) < std::min(32, _power - 40) - _hard_already;
    if (p2) {
      _hard_already += 2;
    }
    spawn(new FollowHub(spawn_point(false, _trow, num, div), p1, p2));
  }
  if (side == 0 || side == 2) {
    bool p1 = z::rand_int(64) < std::min(32, _power - 32) - _hard_already;
    if (p1) {
      _hard_already += 2;
    }
    bool p2 = z::rand_int(64) < std::min(32, _power - 40) - _hard_already;
    if (p2) {
      _hard_already += 2;
    }
    spawn(new FollowHub(spawn_point(true, _trow, num, div), p1, p2));
  }
}

void Overmind::spawn_shielder(int32_t num, int32_t div, int32_t side)
{
  if (side == 0 || side == 1) {
    bool p = z::rand_int(64) < std::min(32, _power - 36) - _hard_already;
    if (p) {
      _hard_already += 3;
    }
    spawn(new Shielder(spawn_point(false, _trow, num, div), p));
  }
  if (side == 0 || side == 2) {
    bool p = z::rand_int(64) < std::min(32, _power - 36) - _hard_already;
    if (p) {
      _hard_already += 3;
    }
    spawn(new Shielder(spawn_point(true, _trow, num, div), p));
  }
}

void Overmind::spawn_tractor(int32_t num, int32_t div, int32_t side)
{
  if (side == 0 || side == 1) {
    bool p = z::rand_int(64) < std::min(32, _power - 46) - _hard_already;
    if (p) {
      _hard_already += 4;
    }
    spawn(new Tractor(spawn_point(false, _trow, num, div), p));
  }
  if (side == 0 || side == 2) {
    bool p = z::rand_int(64) < std::min(32, _power - 46) - _hard_already;
    if (p) {
      _hard_already += 4;
    }
    spawn(new Tractor(spawn_point(true, _trow, num, div), p));
  }
}

// spawn formation coordinates
//------------------------------
void Overmind::spawn(Ship* ship)
{
  if (_tz0) {
    _tz0->add_ship(ship);
  }
}

vec2 Overmind::spawn_point(bool top, int32_t row, int32_t num, int32_t div)
{
  div = std::max(2, div);
  num = std::max(0, std::min(div - 1, num));

  fixed x = fixed(top ? num : div - 1 - num) * Lib::WIDTH / fixed(div - 1);
  fixed y = top ? -(row + 1) * (fixed::hundredth * 16) * Lib::HEIGHT :
      Lib::HEIGHT * (1 + (row + 1) * (fixed::hundredth * 16));
  return vec2(x, y);
}

// Spawn logic
//------------------------------
void Overmind::wave()
{
  if (_z0.mode() == z0Game::FAST_MODE) {
    for (int32_t i = 0; i < z::rand_int(7); ++i) {
      z::rand_int(1);
    }
  }
  if (_z0.mode() == z0Game::WHAT_MODE) {
    for (int32_t i = 0; i < z::rand_int(11); ++i) {
      z::rand_int(1);
    }
  }

  _tz0 = &_z0;
  int32_t resources = _power;

  std::vector<std::pair<int32_t, spawn_formation_function>> validFormations;
  for (std::size_t i = 0; i < _formations.size(); i++) {
    if (resources >= _formations[i].first.second) {
      validFormations.push_back(std::pair<int32_t, spawn_formation_function>(
          _formations[i].first.first, _formations[i].second));
    }
  }

  std::vector<std::pair<int32_t, spawn_formation_function>> chosenFormations;
  while (resources > 0) {
    std::size_t max = 0;
    while (max < validFormations.size() &&
           validFormations[max].first <= resources) {
      max++;
    }

    int32_t n;
    if (max == 0) {
      break;
    }
    else if (max == 1) {
      n = 0;
    }
    else {
      n = z::rand_int(max);
    }

    chosenFormations.insert(
        chosenFormations.begin() + z::rand_int(chosenFormations.size() + 1),
        validFormations[n]);
    resources -= validFormations[n].first;
  }

  std::vector<std::size_t> perm;
  for (std::size_t i = 0; i < chosenFormations.size(); ++i) {
    perm.push_back(i);
  }
  for (std::size_t i = 0; i < chosenFormations.size() - 1; ++i) {
    std::swap(perm[i], perm[i + z::rand_int(chosenFormations.size() - i)]);
  }
  _hard_already = 0;
  for (std::size_t row = 0; row < chosenFormations.size(); ++row) {
    chosenFormations[perm[row]].second(false, int32_t(perm[row]));
  }
  _tz0 = 0;
}

void Overmind::boss()
{
  _tz0 = &_z0;
  _trow = 0;
  int32_t count = _z0.count_players();
  int32_t cycle = (_z0.mode() == z0Game::HARD_MODE) + _boss_mod_bosses / 2;
  bool secret_chance =
      (_z0.mode() != z0Game::NORMAL_MODE && _z0.mode() != z0Game::BOSS_MODE) ?
      (_boss_mod_fights > 1 ? z::rand_int(4) == 0 :
       _boss_mod_fights > 0 ? z::rand_int(8) == 0 : false) :
      (_boss_mod_fights > 2 ? z::rand_int(4) == 0 :
       _boss_mod_fights > 1 ? z::rand_int(6) == 0 : false);

  if (_can_face_secret_boss && _bosses_to_go == 0 &&
      _boss_mod_secret == 0 && secret_chance) {
    int32_t secret_cycle = std::max(
        0, (_boss_mod_bosses + (_z0.mode() == z0Game::HARD_MODE) - 2) / 2);
    spawn(new SuperBoss(count, secret_cycle));
    _boss_mod_secret = 2;
  }

  else if (_boss_mod_bosses % 2 == 0) {
    spawn(_boss1_queue[0] == 0 ? (Boss*)new BigSquareBoss(count, cycle) :
          _boss1_queue[0] == 1 ? (Boss*)new ShieldBombBoss(count, cycle) :
                                 (Boss*)new ChaserBoss(count, cycle));

    _boss1_queue.push_back(_boss1_queue[0]);
    _boss1_queue.erase(_boss1_queue.begin());
  }
  else {
    if (_boss_mod_secret > 0) {
      --_boss_mod_secret;
    }
    spawn(_boss2_queue[0] == 0 ? (Boss*)new TractorBoss(count, cycle) :
          _boss2_queue[0] == 1 ? (Boss*)new GhostBoss(count, cycle) :
                                 (Boss*)new DeathRayBoss(count, cycle));

    _boss2_queue.push_back(_boss2_queue[0]);
    _boss2_queue.erase(_boss2_queue.begin());
  }
  _tz0 = 0;
}

void Overmind::boss_mode_boss()
{
  _tz0 = &_z0;
  _trow = 0;
  int32_t boss = _boss_mod_bosses;
  int32_t count = _z0.count_players();
  if (_boss_mod_bosses < 3) {
    spawn(_boss1_queue[boss] == 0 ? (Boss*)new BigSquareBoss(count, 0) :
          _boss1_queue[boss] == 1 ? (Boss*)new ShieldBombBoss(count, 0) :
                                    (Boss*)new ChaserBoss(count, 0));
  }
  else {
    spawn(_boss2_queue[boss] == 0 ? (Boss*)new TractorBoss(count, 0) :
          _boss2_queue[boss] == 1 ? (Boss*)new GhostBoss(count, 0) :
                                    (Boss*)new DeathRayBoss(count, 0));
  }
  _tz0 = 0;
}

// Formations
//------------------------------
void Overmind::add_formations()
{
  FORM_USE(Square1);
  FORM_USE(Square2);
  FORM_USE(Square3);
  FORM_USE(Square1Side);
  FORM_USE(Square2Side);
  FORM_USE(Square3Side);
  FORM_USE(Wall1);
  FORM_USE(Wall2);
  FORM_USE(Wall3);
  FORM_USE(Wall1Side);
  FORM_USE(Wall2Side);
  FORM_USE(Wall3Side);
  FORM_USE(Follow1);
  FORM_USE(Follow2);
  FORM_USE(Follow3);
  FORM_USE(Follow1Side);
  FORM_USE(Follow2Side);
  FORM_USE(Follow3Side);
  FORM_USE(Chaser1);
  FORM_USE(Chaser2);
  FORM_USE(Chaser3);
  FORM_USE(Chaser4);
  FORM_USE(Chaser1Side);
  FORM_USE(Chaser2Side);
  FORM_USE(Chaser3Side);
  FORM_USE(Chaser4Side);
  FORM_USE(Hub1);
  FORM_USE(Hub2);
  FORM_USE(Hub1Side);
  FORM_USE(Hub2Side);
  FORM_USE(Mixed1);
  FORM_USE(Mixed2);
  FORM_USE(Mixed3);
  FORM_USE(Mixed4);
  FORM_USE(Mixed5);
  FORM_USE(Mixed6);
  FORM_USE(Mixed7);
  FORM_USE(Mixed1Side);
  FORM_USE(Mixed2Side);
  FORM_USE(Mixed3Side);
  FORM_USE(Mixed4Side);
  FORM_USE(Mixed5Side);
  FORM_USE(Mixed6Side);
  FORM_USE(Mixed7Side);
  FORM_USE(Tractor1);
  FORM_USE(Tractor2);
  FORM_USE(Tractor1Side);
  FORM_USE(Tractor2Side);
  FORM_USE(Shielder1);
  FORM_USE(Shielder1Side);
}

FORM_DEF(Square1, 4, 0)
{
  for (int32_t i = 1; i < 5; i++) {
    spawn_square(i, 6, 0);
  }
} FORM_END;

FORM_DEF(Square2, 11, 0)
{
  int32_t r = z::rand_int(4);
  int32_t p1 = 2 + z::rand_int(8);
  int32_t p2 = 2 + z::rand_int(8);
  for (int32_t i = 1; i < 11; ++i) {
    if (r < 2 || i != p1) {
      spawn_square(i, 12, 1);
    }
    if (r < 2 || (r == 2 && i != 11 - p1) || (r == 3 && i != p2)) {
      spawn_square(i, 12, 2);
    }
  }
} FORM_END;

FORM_DEF(Square3, 20, 24)
{
  int32_t r1 = z::rand_int(4);
  int32_t r2 = z::rand_int(4);
  int32_t p11 = 2 + z::rand_int(14);
  int32_t p12 = 2 + z::rand_int(14);
  int32_t p21 = 2 + z::rand_int(14);
  int32_t p22 = 2 + z::rand_int(14);

  for (int32_t i = 0; i < 18; i++) {
    if (r1 < 2 || i != p11) {
      if (r2 < 2 || i != p21) {
        spawn_square(i, 18, 1);
      }
    }
    if (r1 < 2 || (r1 == 2 && i != 17 - p11) || (r1 == 3 && i != p12)) {
      if (r2 < 2 || (r2 == 2 && i != 17 - p21) || (r2 == 3 && i != p22)) {
        spawn_square(i, 18, 2);
      }
    }
  }
} FORM_END;

FORM_DEF(Square1Side, 2, 0)
{
  int32_t r = z::rand_int(2);
  int32_t p = z::rand_int(4);

  if (p < 2) {
    for (int32_t i = 1; i < 5; ++i) {
      spawn_square(i, 6, 1 + r);
    }
  }
  else if (p == 2) {
    for (int32_t i = 1; i < 5; ++i) {
      spawn_square((i + r) % 2 == 0 ? i : 5 - i, 6, 1 + ((i + r) % 2));
    }
  }
  else for (int32_t i = 1; i < 3; ++i) {
    spawn_square(r == 0 ? i : 5 - i, 6, 0);
  }
} FORM_END;

FORM_DEF(Square2Side, 5, 0)
{
  int32_t r = z::rand_int(2);
  int32_t p = z::rand_int(4);

  if (p < 2) {
    for (int32_t i = 1; i < 11; ++i) {
      spawn_square(i, 12, 1 + r);
    }
  }
  else if (p == 2) {
    for (int32_t i = 1; i < 11; ++i) {
      spawn_square((i + r) % 2 == 0 ? i : 11 - i, 12, 1 + ((i + r) % 2));
    }
  }
  else for (int32_t i = 1; i < 6; ++i) {
    spawn_square(r == 0 ? i : 11 - i, 12, 0);
  }
} FORM_END;

FORM_DEF(Square3Side, 10, 12)
{
  int32_t r = z::rand_int(2);
  int32_t p = z::rand_int(4);
  int32_t r2 = z::rand_int(2);
  int32_t p2 = 1 + z::rand_int(16);

  if (p < 2) {
    for (int32_t i = 0; i < 18; ++i) {
      if (r2 == 0 || i != p2) {
        spawn_square(i, 18, 1 + r);
      }
    }
  }
  else if (p == 2) {
    for (int32_t i = 0; i < 18; ++i) {
      spawn_square((i + r) % 2 == 0 ? i : 17 - i, 18, 1 + ((i + r) % 2));
    }
  }
  else for (int32_t i = 0; i < 9; ++i) {
    spawn_square(r == 0 ? i : 17 - i, 18, 0);
  }
} FORM_END;

FORM_DEF(Wall1, 5, 0)
{
  bool dir = z::rand_int(2) != 0;
  for (int32_t i = 1; i < 3; ++i) {
    spawn_wall(i, 4, 0, dir);
  }
} FORM_END;

FORM_DEF(Wall2, 12, 0)
{
  bool dir = z::rand_int(2) != 0;
  int32_t r = z::rand_int(4);
  int32_t p1 = 2 + z::rand_int(5);
  int32_t p2 = 2 + z::rand_int(5);
  for (int32_t i = 1; i < 8; ++i) {
    if (r < 2 || i != p1) {
      spawn_wall(i, 9, 1, dir);
    }
    if (r < 2 || (r == 2 && i != 8 - p1) || (r == 3 && i != p2)) {
      spawn_wall(i, 9, 2, dir);
    }
  }
} FORM_END;

FORM_DEF(Wall3, 22, 26)
{
  bool dir = z::rand_int(2) != 0;
  int32_t r1 = z::rand_int(4);
  int32_t r2 = z::rand_int(4);
  int32_t p11 = 1 + z::rand_int(10);
  int32_t p12 = 1 + z::rand_int(10);
  int32_t p21 = 1 + z::rand_int(10);
  int32_t p22 = 1 + z::rand_int(10);

  for (int32_t i = 0; i < 12; ++i) {
    if (r1 < 2 || i != p11) {
      if (r2 < 2 || i != p21) {
        spawn_wall(i, 12, 1, dir);
      }
    }
    if (r1 < 2 || (r1 == 2 && i != 11 - p11) || (r1 == 3 && i != p12)) {
      if (r2 < 2 || (r2 == 2 && i != 11 - p21) || (r2 == 3 && i != p22)) {
        spawn_wall(i, 12, 2, dir);
      }
    }
  }
} FORM_END;

FORM_DEF(Wall1Side, 3, 0)
{
  bool dir = z::rand_int(2) != 0;
  int32_t r = z::rand_int(2);
  int32_t p = z::rand_int(4);

  if (p < 2) {
    for (int32_t i = 1; i < 3; ++i) {
      spawn_wall(i, 4, 1 + r, dir);
    }
  }
  else if (p == 2) {
    for (int32_t i = 1; i < 3; ++i) {
      spawn_wall((i + r) % 2 == 0 ? i : 3 - i, 4, 1 + ((i + r) % 2), dir);
    }
  }
  else {
    spawn_wall(1 + r, 4, 0, dir);
  }
} FORM_END;

FORM_DEF(Wall2Side, 6, 0)
{
  bool dir = z::rand_int(2) != 0;
  int32_t r = z::rand_int(2);
  int32_t p = z::rand_int(4);

  if (p < 2) {
    for (int32_t i = 1; i < 8; ++i) {
      spawn_wall(i, 9, 1 + r, dir);
    }
  }
  else if (p == 2) {
    for (int32_t i = 1; i < 8; ++i) {
      spawn_wall(i, 9, 1 + ((i + r) % 2), dir);
    }
  }
  else for (int32_t i = 0; i < 4; ++i) {
    spawn_wall(r == 0 ? i : 8 - i, 9, 0, dir);
  }
} FORM_END;

FORM_DEF(Wall3Side, 11, 13)
{
  bool dir = z::rand_int(2) != 0;
  int32_t r = z::rand_int(2);
  int32_t p = z::rand_int(4);
  int32_t r2 = z::rand_int(2);
  int32_t p2 = 1 + z::rand_int(10);

  if (p < 2) {
    for (int32_t i = 0; i < 12; ++i) {
      if (r2 == 0 || i != p2) {
        spawn_wall(i, 12, 1 + r, dir);
      }
    }
  }
  else if (p == 2) {
    for (int32_t i = 0; i < 12; ++i) {
      spawn_wall((i + r) % 2 == 0 ? i : 11 - i, 12, 1 + ((i + r) % 2), dir);
    }
  }
  else for (int32_t i = 0; i < 6; ++i) {
    spawn_wall(r == 0 ? i : 11 - i, 12, 0, dir);
  }
} FORM_END;

FORM_DEF(Follow1, 3, 0)
{
  int32_t p = z::rand_int(3);
  if (p == 0) {
    spawn_follow(0, 3, 0);
    spawn_follow(2, 3, 0);
  }
  else if (p == 1) {
    spawn_follow(1, 4, 0);
    spawn_follow(2, 4, 0);
  }
  else {
    spawn_follow(0, 5, 0);
    spawn_follow(4, 5, 0);
  }
} FORM_END;

FORM_DEF(Follow2, 7, 0)
{
  int32_t p = z::rand_int(2);
  if (p == 0) {
    for (int32_t i = 0; i < 8; i++) {
      spawn_follow(i, 8, 0);
    }
  }
  else for (int32_t i = 0; i < 8; i++) {
    spawn_follow(4 + i, 16, 0);
  }
} FORM_END;

FORM_DEF(Follow3, 14, 0)
{
  int32_t p = z::rand_int(2);
  if (p == 0) {
    for (int32_t i = 0; i < 16; ++i) {
      spawn_follow(i, 16, 0);
    }
  }
  else for (int32_t i = 0; i < 8; ++i) {
    spawn_follow(i, 28, 0);
    spawn_follow(27 - i, 28, 0);
  }
} FORM_END;

FORM_DEF(Follow1Side, 2, 0)
{
  int32_t r = 1 + z::rand_int(2);
  int32_t p = z::rand_int(3);
  if (p == 0) {
    spawn_follow(0, 3, r);
    spawn_follow(2, 3, r);
  }
  else if (p == 1) {
    spawn_follow(1, 4, r);
    spawn_follow(2, 4, r);
  }
  else {
    spawn_follow(0, 5, r);
    spawn_follow(4, 5, r);
  }
} FORM_END;

FORM_DEF(Follow2Side, 3, 0)
{
  int32_t r = 1 + z::rand_int(2);
  int32_t p = z::rand_int(2);
  if (p == 0) {
    for (int32_t i = 0; i < 8; i++) {
      spawn_follow(i, 8, r);
    }
  }
  else for (int32_t i = 0; i < 8; i++) {
    spawn_follow(4 + i, 16, r);
  }
} FORM_END;

FORM_DEF(Follow3Side, 7, 0)
{
  int32_t r = 1 + z::rand_int(2);
  int32_t p = z::rand_int(2);
  if (p == 0) {
    for (int32_t i = 0; i < 16; ++i) {
      spawn_follow(i, 16, r);
    }
  }
  else for (int32_t i = 0; i < 8; ++i) {
    spawn_follow(i, 28, r);
    spawn_follow(27 - i, 28, r);
  }
} FORM_END;

FORM_DEF(Chaser1, 4, 0)
{
  int32_t p = z::rand_int(3);
  if (p == 0) {
    spawn_chaser(0, 3, 0);
    spawn_chaser(2, 3, 0);
  }
  else if (p == 1) {
    spawn_chaser(1, 4, 0);
    spawn_chaser(2, 4, 0);
  }
  else {
    spawn_chaser(0, 5, 0);
    spawn_chaser(4, 5, 0);
  }
} FORM_END;

FORM_DEF(Chaser2, 8, 0)
{
  int32_t p = z::rand_int(2);
  if (p == 0) {
    for (int32_t i = 0; i < 8; i++) {
      spawn_chaser(i, 8, 0);
    }
  }
  else for (int32_t i = 0; i < 8; i++) {
    spawn_chaser(4 + i, 16, 0);
  }
} FORM_END;

FORM_DEF(Chaser3, 16, 0)
{
  int32_t p = z::rand_int(2);
  if (p == 0) {
    for (int32_t i = 0; i < 16; ++i) {
      spawn_chaser(i, 16, 0);
    }
  }
  else for (int32_t i = 0; i < 8; ++i) {
    spawn_chaser(i, 28, 0);
    spawn_chaser(27 - i, 28, 0);
  }
} FORM_END;

FORM_DEF(Chaser4, 20, 0)
{
  for (int32_t i = 0; i < 22; i++) {
    spawn_chaser(i, 22, 0);
  }
} FORM_END;

FORM_DEF(Chaser1Side, 2, 0)
{
  int32_t r = 1 + z::rand_int(2);
  int32_t p = z::rand_int(3);
  if (p == 0) {
    spawn_chaser(0, 3, r);
    spawn_chaser(2, 3, r);
  }
  else if (p == 1) {
    spawn_chaser(1, 4, r);
    spawn_chaser(2, 4, r);
  }
  else {
    spawn_chaser(0, 5, r);
    spawn_chaser(4, 5, r);
  }
} FORM_END;

FORM_DEF(Chaser2Side, 4, 0)
{
  int32_t r = 1 + z::rand_int(2);
  int32_t p = z::rand_int(2);
  if (p == 0) {
    for (int32_t i = 0; i < 8; i++) {
      spawn_chaser(i, 8, r);
    }
  }
  else for (int32_t i = 0; i < 8; i++) {
    spawn_chaser(4 + i, 16, r);
  }
} FORM_END;

FORM_DEF(Chaser3Side, 8, 0)
{
  int32_t r = 1 + z::rand_int(2);
  int32_t p = z::rand_int(2);
  if (p == 0) {
    for (int32_t i = 0; i < 16; ++i) {
      spawn_chaser(i, 16, r);
    }
  }
  else for (int32_t i = 0; i < 8; ++i) {
    spawn_chaser(i, 28, r);
    spawn_chaser(27 - i, 28, r);
  }
} FORM_END;

FORM_DEF(Chaser4Side, 10, 0)
{
  int32_t r = 1 + z::rand_int(2);
  for (int32_t i = 0; i < 22; i++) {
    spawn_chaser(i, 22, r);
  }
} FORM_END;

FORM_DEF(Hub1, 6, 0)
{
  spawn_follow_hub(1 + z::rand_int(3), 5, 0);
} FORM_END;

FORM_DEF(Hub2, 12, 0)
{
  int32_t p = z::rand_int(3);
  spawn_follow_hub(p == 1 ? 2 : 1, 5, 0);
  spawn_follow_hub(p == 2 ? 2 : 3, 5, 0);
} FORM_END;

FORM_DEF(Hub1Side, 3, 0)
{
  spawn_follow_hub(1 + z::rand_int(3), 5, 1 + z::rand_int(2));
} FORM_END;

FORM_DEF(Hub2Side, 6, 0)
{
  int32_t r = 1 + z::rand_int(2);
  int32_t p = z::rand_int(3);
  spawn_follow_hub(p == 1 ? 2 : 1, 5, r);
  spawn_follow_hub(p == 2 ? 2 : 3, 5, r);
} FORM_END;

FORM_DEF(Mixed1, 6, 0)
{
  int32_t p = z::rand_int(2);
  spawn_follow(p == 0 ? 0 : 2, 4, 0);
  spawn_follow(p == 0 ? 1 : 3, 4, 0);
  spawn_chaser(p == 0 ? 2 : 0, 4, 0);
  spawn_chaser(p == 0 ? 3 : 1, 4, 0);
} FORM_END;

FORM_DEF(Mixed2, 12, 0)
{
  for (int32_t i = 0; i < 13; i++) {
    if (i % 2) {
      spawn_follow(i, 13, 0);
    }
    else {
      spawn_chaser(i, 13, 0);
    }
  }
} FORM_END;

FORM_DEF(Mixed3, 16, 0)
{
  bool dir = z::rand_int(2) != 0;
  spawn_wall(3, 7, 0, dir);
  spawn_follow_hub(1, 7, 0);
  spawn_follow_hub(5, 7, 0);
  spawn_chaser(2, 7, 0);
  spawn_chaser(4, 7, 0);
} FORM_END;

FORM_DEF(Mixed4, 18, 0)
{
  spawn_square(1, 7, 0);
  spawn_square(5, 7, 0);
  spawn_follow_hub(3, 7, 0);
  spawn_chaser(2, 9, 0);
  spawn_chaser(3, 9, 0);
  spawn_chaser(5, 9, 0);
  spawn_chaser(6, 9, 0);
} FORM_END;

FORM_DEF(Mixed5, 22, 38)
{
  spawn_follow_hub(1, 7, 0);
  spawn_tractor(3, 7, 1 + z::rand_int(2));
} FORM_END;

FORM_DEF(Mixed6, 16, 30)
{
  spawn_follow_hub(1, 5, 0);
  spawn_shielder(3, 5, 0);
} FORM_END;

FORM_DEF(Mixed7, 18, 16)
{
  bool dir = z::rand_int(2) != 0;
  spawn_shielder(2, 5, 0);
  spawn_wall(1, 10, 0, dir);
  spawn_wall(8, 10, 0, dir);
  spawn_chaser(2, 10, 0);
  spawn_chaser(3, 10, 0);
  spawn_chaser(4, 10, 0);
  spawn_chaser(5, 10, 0);
} FORM_END;

FORM_DEF(Mixed1Side, 3, 0)
{
  int32_t r = 1 + z::rand_int(2);
  int32_t p = z::rand_int(2);
  spawn_follow(p == 0 ? 0 : 2, 4, r);
  spawn_follow(p == 0 ? 1 : 3, 4, r);
  spawn_chaser(p == 0 ? 2 : 0, 4, r);
  spawn_chaser(p == 0 ? 3 : 1, 4, r);
} FORM_END;

FORM_DEF(Mixed2Side, 6, 0)
{
  int32_t r = z::rand_int(2);
  int32_t p = z::rand_int(2);
  for (int32_t i = 0; i < 13; i++) {
    if (i % 2) {
      spawn_follow(i, 13, 1 + r);
    }
    else {
      spawn_chaser(i, 13, p == 0 ? 1 + r : 2 - r);
    }
  }
} FORM_END;

FORM_DEF(Mixed3Side, 8, 0)
{
  bool dir = z::rand_int(2) != 0;
  int32_t r = 1 + z::rand_int(2);
  spawn_wall(3, 7, r, dir);
  spawn_follow_hub(1, 7, r);
  spawn_follow_hub(5, 7, r);
  spawn_chaser(2, 7, r);
  spawn_chaser(4, 7, r);
} FORM_END;

FORM_DEF(Mixed4Side, 9, 0)
{
  int32_t r = 1 + z::rand_int(2);
  spawn_square(1, 7, r);
  spawn_square(5, 7, r);
  spawn_follow_hub(3, 7, r);
  spawn_chaser(2, 9, r);
  spawn_chaser(3, 9, r);
  spawn_chaser(5, 9, r);
  spawn_chaser(6, 9, r);
} FORM_END;

FORM_DEF(Mixed5Side, 19, 36)
{
  int32_t r = 1 + z::rand_int(2);
  spawn_follow_hub(1, 7, r);
  spawn_tractor(3, 7, r);
} FORM_END;

FORM_DEF(Mixed6Side, 8, 20)
{
  int32_t r = 1 + z::rand_int(2);
  spawn_follow_hub(1, 5, r);
  spawn_shielder(3, 5, r);
} FORM_END;

FORM_DEF(Mixed7Side, 9, 16)
{
  bool dir = z::rand_int(2) != 0;
  int32_t r = 1 + z::rand_int(2);
  spawn_shielder(2, 5, r);
  spawn_wall(1, 10, r, dir);
  spawn_wall(8, 10, r, dir);
  spawn_chaser(2, 10, r);
  spawn_chaser(3, 10, r);
  spawn_chaser(4, 10, r);
  spawn_chaser(5, 10, r);
} FORM_END;

FORM_DEF(Tractor1, 16, 30)
{
  spawn_tractor(z::rand_int(3) + 1, 5, 1 + z::rand_int(2));
} FORM_END;

FORM_DEF(Tractor2, 28, 46)
{
  spawn_tractor(z::rand_int(3) + 1, 5, 2);
  spawn_tractor(z::rand_int(3) + 1, 5, 1);
} FORM_END;

FORM_DEF(Tractor1Side, 16, 36)
{
  spawn_tractor(z::rand_int(7) + 1, 9, 1 + z::rand_int(2));
} FORM_END;

FORM_DEF(Tractor2Side, 14, 32)
{
  spawn_tractor(z::rand_int(5) + 1, 7, 1 + z::rand_int(2));
} FORM_END;

FORM_DEF(Shielder1, 10, 28)
{
  spawn_shielder(z::rand_int(3) + 1, 5, 0);
} FORM_END;

FORM_DEF(Shielder1Side, 5, 22)
{
  spawn_shielder(z::rand_int(3) + 1, 5, 1 + z::rand_int(2));
} FORM_END;
