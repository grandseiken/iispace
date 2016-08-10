#include "overmind.h"
#include "enemy.h"
#include "boss_chaser.h"
#include "boss_deathray.h"
#include "boss_ghost.h"
#include "boss_shield.h"
#include "boss_square.h"
#include "boss_super.h"
#include "boss_tractor.h"
#include "player.h"
#include "stars.h"
#include <algorithm>

class formation_base {
public:

  static std::vector<Overmind::entry> static_formations;

  virtual void operator()() = 0;

  void operator()(
      GameModal* game, int32_t row, int32_t power, int32_t* hard_already)
  {
    _game = game;
    _row = row;
    _power = power;
    _hard_already = hard_already;
    operator()();
  }

  void spawn(Ship* ship)
  {
    _game->add_ship(ship);
  }

  vec2 spawn_point(bool top, int32_t num, int32_t div)
  {
    div = std::max(2, div);
    num = std::max(0, std::min(div - 1, num));

    fixed x = fixed(top ? num : div - 1 - num) * Lib::WIDTH / fixed(div - 1);
    fixed y = top ? -(_row + 1) * (fixed::hundredth * 16) * Lib::HEIGHT :
        Lib::HEIGHT * (1 + (_row + 1) * (fixed::hundredth * 16));
    return vec2(x, y);
  }

  void spawn_follow(int32_t num, int32_t div, int32_t side)
  {
    do_sides(side, [&](bool b) {
      spawn(new Follow(spawn_point(b, num, div)));
    });
  }

  void spawn_chaser(int32_t num, int32_t div, int32_t side)
  {
    do_sides(side, [&](bool b) {
      spawn(new Chaser(spawn_point(b, num, div)));
    });
  }

  void spawn_square(int32_t num, int32_t div, int32_t side)
  {
    do_sides(side, [&](bool b) {
      spawn(new Square(spawn_point(b, num, div)));
    });
  }

  void spawn_wall(int32_t num, int32_t div, int32_t side, bool dir)
  {
    do_sides(side, [&](bool b) {
      spawn(new Wall(spawn_point(b, num, div), dir));
    });
  }

  void spawn_follow_hub(int32_t num, int32_t div, int32_t side)
  {
    do_sides(side, [&](bool b) {
      bool p1 = z::rand_int(64) < std::min(32, _power - 32) - *_hard_already;
      if (p1) {
        *_hard_already += 2;
      }
      bool p2 = z::rand_int(64) < std::min(32, _power - 40) - *_hard_already;
      if (p2) {
        *_hard_already += 2;
      }
      spawn(new FollowHub(spawn_point(b, num, div), p1, p2));
    });
  }

  void spawn_shielder(int32_t num, int32_t div, int32_t side)
  {
    do_sides(side, [&](bool b) {
      bool p = z::rand_int(64) < std::min(32, _power - 36) - *_hard_already;
      if (p) {
        *_hard_already += 3;
      }
      spawn(new Shielder(spawn_point(b, num, div), p));
    });
  }

  void spawn_tractor(int32_t num, int32_t div, int32_t side)
  {
    do_sides(side, [&](bool b) {
      bool p = z::rand_int(64) < std::min(32, _power - 46) - *_hard_already;
      if (p) {
        *_hard_already += 4;
      }
      spawn(new Tractor(spawn_point(b, num, div), p));
    });
  }

private:

  template<typename F>
  void do_sides(int32_t side, const F& f)
  {
    if (side == 0 || side == 1) {
      f(false);
    }
    if (side == 0 || side == 2) {
      f(true);
    }
  }

  GameModal* _game;
  int32_t _row;
  int32_t _power;
  int32_t* _hard_already;

};

std::vector<Overmind::entry> formation_base::static_formations;

Overmind::Overmind(GameModal& game, bool can_face_secret_boss)
  : _game(game)
  , _power(0)
  , _timer(0)
  , _count(0)
  , _non_wall_count(0)
  , _levels_mod(0)
  , _groups_mod(0)
  , _boss_mod_bosses(0)
  , _boss_mod_fights(0)
  , _boss_mod_secret(0)
  , _can_face_secret_boss(can_face_secret_boss)
  , _powerup_mod(0)
  , _lives_target(0)
  , _is_boss_next(false)
  , _is_boss_level(false)
  , _elapsed_time(0)
  , _waves_total(0)
  , _hard_already(0)
  , _bosses_to_go(0)
{
  add_formations();

  auto queue = [] {
    int32_t a = z::rand_int(3);
    int32_t b = z::rand_int(2);
    if (a == 0 || (a == 1 && b == 1)) {
      ++b;
    }
    int32_t c = (a + b == 3) ? 0 : (a + b == 2) ? 1 : 2;
    return std::vector<int32_t>{a, b, c};
  };
  _boss1_queue = queue();
  _boss2_queue = queue();

  if (_game.mode() == Mode::BOSS) {
    return;
  }
  _power = INITIAL_POWER + 2 - _game.players().size() * 2;
  if (_game.mode() == Mode::HARD) {
    _power += 20;
    _waves_total = 15;
  }
  _timer = POWERUP_TIME;
  _boss_rest_timer = BOSS_REST_TIME / 8;
  Stars::clear();
}

Overmind::~Overmind()
{
}

void Overmind::update()
{
  ++_elapsed_time;
  Stars::update();

  if (_game.mode() == Mode::BOSS) {
    if (_count <= 0) {
      Stars::change();
      if (_boss_mod_bosses < 6) {
        if (_boss_mod_bosses)
        for (std::size_t i = 0; i < _game.players().size(); ++i) {
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
      2 * (_game.mode() == Mode::HARD);
  int32_t trigger_val = INITIAL_TRIGGERVAL;
  for (int32_t i = 0; i < trigger_stage; ++i) {
    trigger_val += i < 2 ? 4 :
                   i < 7 - int32_t(_game.players().size()) ? 3 : 2;
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
        _power += 2 - 2 * _game.players().size();
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
    if (_groups_mod >= BASE_GROUPS_PER_BOSS + int32_t(_game.players().size())) {
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

void Overmind::spawn(Ship* ship)
{
  _game.add_ship(ship);
}

void Overmind::spawn_powerup()
{
  if (_game.all_ships(Ship::SHIP_POWERUP).size() >= 4) {
    return;
  }

  _powerup_mod = (1 + _powerup_mod) % 4;
  if (_powerup_mod == 0) {
    ++_lives_target;
  }

  int32_t r = z::rand_int(4);
  vec2 v = r == 0 ? vec2(-Lib::WIDTH, Lib::HEIGHT / 2) :
           r == 1 ? vec2(Lib::WIDTH * 2, Lib::HEIGHT / 2) :
           r == 2 ? vec2(Lib::WIDTH / 2, -Lib::HEIGHT) :
                    vec2(Lib::WIDTH / 2, Lib::HEIGHT * 2);

  int32_t m = 4;
  for (int32_t i = 1; i <= PLAYERS; i++) {
    if (_game.get_lives() <= int32_t(_game.players().size()) - i) {
      m++;
    }
  }
  if (!_game.get_lives()) {
    m += _game.killed_players();
  }
  if (_lives_target > 0) {
    m += _lives_target;
  }
  if (_game.killed_players() == 0 && _lives_target < -1) {
    m = 3;
  }

  r = z::rand_int(m);
  spawn(new Powerup(v, r == 0 ? Powerup::BOMB :
                       r == 1 ? Powerup::MAGIC_SHOTS :
                       r == 2 ? Powerup::SHIELD :
                       (--_lives_target, Powerup::EXTRA_LIFE)));
}

void Overmind::spawn_boss_reward()
{
  int32_t r = z::rand_int(4);
  vec2 v = r == 0 ? vec2(-Lib::WIDTH / 4, Lib::HEIGHT / 2) :
           r == 1 ? vec2(Lib::WIDTH + Lib::WIDTH / 4, Lib::HEIGHT / 2) :
           r == 2 ? vec2(Lib::WIDTH / 2, -Lib::HEIGHT / 4) :
                    vec2(Lib::WIDTH / 2, Lib::HEIGHT + Lib::HEIGHT / 4);

  spawn(new Powerup(v, Powerup::EXTRA_LIFE));
  if (_game.mode() != Mode::BOSS) {
    spawn_powerup();
  }
}

void Overmind::wave()
{
  if (_game.mode() == Mode::FAST) {
    for (int32_t i = 0; i < z::rand_int(7); ++i) {
      z::rand_int(1);
    }
  }
  if (_game.mode() == Mode::WHAT) {
    for (int32_t i = 0; i < z::rand_int(11); ++i) {
      z::rand_int(1);
    }
  }

  int32_t resources = _power;
  std::vector<const entry*> valid;
  for (const auto& f : _formations) {
    if (resources >= f.min_resource) {
      valid.emplace_back(&f);
    }
  }

  std::vector<const entry*> chosen;
  while (resources > 0) {
    std::size_t max = 0;
    while (max < valid.size() && valid[max]->cost <= resources) {
      ++max;
    }
    if (max == 0) {
      break;
    }
    int32_t n = max == 1 ? 0 : z::rand_int(max);

    chosen.insert(chosen.begin() + z::rand_int(chosen.size() + 1), valid[n]);
    resources -= valid[n]->cost;
  }

  // This permutation is bugged and does nothing but backwards compatibility.
  std::vector<std::size_t> perm;
  for (std::size_t i = 0; i < chosen.size(); ++i) {
    perm.push_back(i);
  }
  for (std::size_t i = 0; i < chosen.size() - 1; ++i) {
    std::swap(perm[i], perm[i + z::rand_int(chosen.size() - i)]);
  }
  _hard_already = 0;
  for (std::size_t row = 0; row < chosen.size(); ++row) {
    chosen[perm[row]]->function->operator()(
        &_game, perm[row], _power, &_hard_already);
  }
}

void Overmind::boss()
{
  int32_t count = _game.players().size();
  int32_t cycle = (_game.mode() == Mode::HARD) + _boss_mod_bosses / 2;
  bool secret_chance =
      (_game.mode() != Mode::NORMAL &&
       _game.mode() != Mode::BOSS) ?
      (_boss_mod_fights > 1 ? z::rand_int(4) == 0 :
       _boss_mod_fights > 0 ? z::rand_int(8) == 0 : false) :
      (_boss_mod_fights > 2 ? z::rand_int(4) == 0 :
       _boss_mod_fights > 1 ? z::rand_int(6) == 0 : false);

  if (_can_face_secret_boss && _bosses_to_go == 0 &&
      _boss_mod_secret == 0 && secret_chance) {
    int32_t secret_cycle = std::max(
        0, (_boss_mod_bosses + (_game.mode() == Mode::HARD) - 2) / 2);
    spawn(new SuperBoss(count, secret_cycle));
    _boss_mod_secret = 2;
  }

  else if (_boss_mod_bosses % 2 == 0) {
    spawn(_boss1_queue[0] == 0 ? (Boss*)new BigSquareBoss(count, cycle) :
          _boss1_queue[0] == 1 ? (Boss*)new ShieldBombBoss(count, cycle) :
                                 (Boss*)new ChaserBoss(count, cycle));

    _boss1_queue.push_back(_boss1_queue.front());
    _boss1_queue.erase(_boss1_queue.begin());
  }
  else {
    if (_boss_mod_secret > 0) {
      --_boss_mod_secret;
    }
    spawn(_boss2_queue[0] == 0 ? (Boss*)new TractorBoss(count, cycle) :
          _boss2_queue[0] == 1 ? (Boss*)new GhostBoss(count, cycle) :
                                 (Boss*)new DeathRayBoss(count, cycle));

    _boss2_queue.push_back(_boss2_queue.front());
    _boss2_queue.erase(_boss2_queue.begin());
  }
}

void Overmind::boss_mode_boss()
{
  int32_t boss = _boss_mod_bosses;
  int32_t count = _game.players().size();
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
}

// Formations
//------------------------------
template<int32_t I, typename F, int32_t C, int32_t R = 0>
struct formation : formation_base {
  static std::unique_ptr<F> function;
  struct init_t {
    init_t()
    {
      function.reset(new F());
      static_formations.emplace_back(Overmind::entry{I, C, R, function.get()});
    }
    // Ensure static initialisation.
    void operator()() {}
  };

  static init_t init_v;
  static void init()
  {
    init_v();
  }
};
template<int32_t I, typename F, int32_t C, int32_t R>
std::unique_ptr<F> formation<I, F, C, R>::function;
template<int32_t I, typename F, int32_t C, int32_t R>
typename formation<I, F, C, R>::init_t formation<I, F, C, R>::init_v;

struct square1 : formation<0, square1, 4> {
  void operator()() override
  {
    for (int32_t i = 1; i < 5; i++) {
      spawn_square(i, 6, 0);
    }
  }
};
struct square2 : formation<1, square2, 11> {
  void operator()() override
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
  }
};
struct square3 : formation<2, square3, 20, 24> {
  void operator()() override
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
  }
};
struct square1side : formation<3, square1side, 2> {
  void operator()() override
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
  }
};
struct square2side : formation<4, square2side, 5> {
  void operator()() override
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
  }
};
struct square3side : formation<5, square3side, 10, 12> {
  void operator()() override
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
  }
};
struct wall1 : formation<6, wall1, 5> {
  void operator()() override
  {
    bool dir = z::rand_int(2) != 0;
    for (int32_t i = 1; i < 3; ++i) {
      spawn_wall(i, 4, 0, dir);
    }
  }
};
struct wall2 : formation<7, wall2, 12> {
  void operator()() override
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
  }
};
struct wall3 : formation<8, wall3, 22, 26> {
  void operator()() override
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
  }
};
struct wall1side : formation<9, wall1side, 3> {
  void operator()() override
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
  }
};
struct wall2side : formation<10, wall2side, 6> {
  void operator()() override
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
  }
};
struct wall3side : formation<11, wall3side, 11, 13> {
  void operator()() override
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
  }
};
struct follow1 : formation<12, follow1, 3> {
  void operator()() override
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
  }
};
struct follow2 : formation<13, follow2, 7> {
  void operator()() override
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
  }
};
struct follow3 : formation<14, follow3, 14> {
  void operator()() override
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
  }
};
struct follow1side : formation<15, follow1side, 2> {
  void operator()() override
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
  }
};
struct follow2side : formation<16, follow2side, 3> {
  void operator()() override
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
  }
};
struct follow3side : formation<17, follow3side, 7> {
  void operator()() override
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
  }
};
struct chaser1 : formation<18, chaser1, 4> {
  void operator()() override
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
  }
};
struct chaser2 : formation<19, chaser2, 8> {
  void operator()() override
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
  }
};
struct chaser3 : formation<20, chaser3, 16> {
  void operator()() override
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
  }
};
struct chaser4 : formation<21, chaser4, 20> {
  void operator()() override
  {
    for (int32_t i = 0; i < 22; i++) {
      spawn_chaser(i, 22, 0);
    }
  }
};
struct chaser1side : formation<22, chaser1side, 2> {
  void operator()() override
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
  }
};
struct chaser2side : formation<23, chaser2side, 4> {
  void operator()() override
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
  }
};
struct chaser3side : formation<24, chaser3side, 8> {
  void operator()() override
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
  }
};
struct chaser4side : formation<25, chaser4side, 10> {
  void operator()() override
  {
    int32_t r = 1 + z::rand_int(2);
    for (int32_t i = 0; i < 22; i++) {
      spawn_chaser(i, 22, r);
    }
  }
};
struct hub1 : formation<26, hub1, 6> {
  void operator()() override
  {
    spawn_follow_hub(1 + z::rand_int(3), 5, 0);
  }
};
struct hub2 : formation<27, hub2, 12> {
  void operator()() override
  {
    int32_t p = z::rand_int(3);
    spawn_follow_hub(p == 1 ? 2 : 1, 5, 0);
    spawn_follow_hub(p == 2 ? 2 : 3, 5, 0);
  }
};
struct hub1side : formation<28, hub1side, 3> {
  void operator()() override
  {
    spawn_follow_hub(1 + z::rand_int(3), 5, 1 + z::rand_int(2));
  }
};
struct hub2side : formation<29, hub2side, 6> {
  void operator()() override
  {
    int32_t r = 1 + z::rand_int(2);
    int32_t p = z::rand_int(3);
    spawn_follow_hub(p == 1 ? 2 : 1, 5, r);
    spawn_follow_hub(p == 2 ? 2 : 3, 5, r);
  }
};
struct mixed1 : formation<30, mixed1, 6> {
  void operator()() override
  {
    int32_t p = z::rand_int(2);
    spawn_follow(p == 0 ? 0 : 2, 4, 0);
    spawn_follow(p == 0 ? 1 : 3, 4, 0);
    spawn_chaser(p == 0 ? 2 : 0, 4, 0);
    spawn_chaser(p == 0 ? 3 : 1, 4, 0);
  }
};
struct mixed2 : formation<31, mixed2, 12> {
  void operator()() override
  {
    for (int32_t i = 0; i < 13; i++) {
      if (i % 2) {
        spawn_follow(i, 13, 0);
      }
      else {
        spawn_chaser(i, 13, 0);
      }
    }
  }
};
struct mixed3 : formation<32, mixed3, 16> {
  void operator()() override
  {
    bool dir = z::rand_int(2) != 0;
    spawn_wall(3, 7, 0, dir);
    spawn_follow_hub(1, 7, 0);
    spawn_follow_hub(5, 7, 0);
    spawn_chaser(2, 7, 0);
    spawn_chaser(4, 7, 0);
  }
};
struct mixed4 : formation<33, mixed4, 18> {
  void operator()() override
  {
    spawn_square(1, 7, 0);
    spawn_square(5, 7, 0);
    spawn_follow_hub(3, 7, 0);
    spawn_chaser(2, 9, 0);
    spawn_chaser(3, 9, 0);
    spawn_chaser(5, 9, 0);
    spawn_chaser(6, 9, 0);
  }
};
struct mixed5 : formation<34, mixed5, 22, 38> {
  void operator()() override
  {
    spawn_follow_hub(1, 7, 0);
    spawn_tractor(3, 7, 1 + z::rand_int(2));
  }
};
struct mixed6 : formation<35, mixed6, 16, 30> {
  void operator()() override
  {
    spawn_follow_hub(1, 5, 0);
    spawn_shielder(3, 5, 0);
  }
};
struct mixed7 : formation<36, mixed7, 18, 16> {
  void operator()() override
  {
    bool dir = z::rand_int(2) != 0;
    spawn_shielder(2, 5, 0);
    spawn_wall(1, 10, 0, dir);
    spawn_wall(8, 10, 0, dir);
    spawn_chaser(2, 10, 0);
    spawn_chaser(3, 10, 0);
    spawn_chaser(4, 10, 0);
    spawn_chaser(5, 10, 0);
  }
};
struct mixed1side : formation<37, mixed1side, 3> {
  void operator()() override
  {
    int32_t r = 1 + z::rand_int(2);
    int32_t p = z::rand_int(2);
    spawn_follow(p == 0 ? 0 : 2, 4, r);
    spawn_follow(p == 0 ? 1 : 3, 4, r);
    spawn_chaser(p == 0 ? 2 : 0, 4, r);
    spawn_chaser(p == 0 ? 3 : 1, 4, r);
  }
};
struct mixed2side : formation<38, mixed2side, 6> {
  void operator()() override
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
  }
};
struct mixed3side : formation<39, mixed3side, 8> {
  void operator()() override
  {
    bool dir = z::rand_int(2) != 0;
    int32_t r = 1 + z::rand_int(2);
    spawn_wall(3, 7, r, dir);
    spawn_follow_hub(1, 7, r);
    spawn_follow_hub(5, 7, r);
    spawn_chaser(2, 7, r);
    spawn_chaser(4, 7, r);
  }
};
struct mixed4side : formation<40, mixed4side, 9> {
  void operator()() override
  {
    int32_t r = 1 + z::rand_int(2);
    spawn_square(1, 7, r);
    spawn_square(5, 7, r);
    spawn_follow_hub(3, 7, r);
    spawn_chaser(2, 9, r);
    spawn_chaser(3, 9, r);
    spawn_chaser(5, 9, r);
    spawn_chaser(6, 9, r);
  }
};
struct mixed5side : formation<41, mixed5side, 19, 36> {
  void operator()() override
  {
    int32_t r = 1 + z::rand_int(2);
    spawn_follow_hub(1, 7, r);
    spawn_tractor(3, 7, r);
  }
};
struct mixed6side : formation<42, mixed6side, 8, 20> {
  void operator()() override
  {
    int32_t r = 1 + z::rand_int(2);
    spawn_follow_hub(1, 5, r);
    spawn_shielder(3, 5, r);
  }
};
struct mixed7side : formation<43, mixed7side, 9, 16> {
  void operator()() override
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
  }
};
struct tractor1 : formation<44, tractor1, 16, 30> {
  void operator()() override
  {
    spawn_tractor(z::rand_int(3) + 1, 5, 1 + z::rand_int(2));
  }
};
struct tractor2 : formation<45, tractor2, 28, 46> {
  void operator()() override
  {
    spawn_tractor(z::rand_int(3) + 1, 5, 2);
    spawn_tractor(z::rand_int(3) + 1, 5, 1);
  }
};
struct tractor1side : formation<46, tractor1side, 16, 36> {
  void operator()() override
  {
    spawn_tractor(z::rand_int(7) + 1, 9, 1 + z::rand_int(2));
  }
};
struct tractor2side : formation<47, tractor2side, 14, 32> {
  void operator()() override
  {
    spawn_tractor(z::rand_int(5) + 1, 7, 1 + z::rand_int(2));
  }
};
struct shielder1 : formation<48, shielder1, 10, 28> {
  void operator()() override
  {
    spawn_shielder(z::rand_int(3) + 1, 5, 0);
  }
};
struct shielder1side : formation<49, shielder1side, 5, 22> {
  void operator()() override
  {
    spawn_shielder(z::rand_int(3) + 1, 5, 1 + z::rand_int(2));
  }
};

template<typename F>
void Overmind::add_formation()
{
  _formations.push_back(entry{F::cost, F::min_resource, new F()});
}

void Overmind::add_formations()
{
  square1::init();
  square2::init();
  square3::init();
  square1side::init();
  square2side::init();
  square3side::init();
  wall1::init();
  wall2::init();
  wall3::init();
  wall1side::init();
  wall2side::init();
  wall3side::init();
  follow1::init();
  follow2::init();
  follow3::init();
  follow1side::init();
  follow2side::init();
  follow3side::init();
  chaser1::init();
  chaser2::init();
  chaser3::init();
  chaser4::init();
  chaser1side::init();
  chaser2side::init();
  chaser3side::init();
  chaser4side::init();
  hub1::init();
  hub2::init();
  hub1side::init();
  hub2side::init();
  mixed1::init();
  mixed2::init();
  mixed3::init();
  mixed4::init();
  mixed5::init();
  mixed6::init();
  mixed7::init();
  mixed1side::init();
  mixed2side::init();
  mixed3side::init();
  mixed4side::init();
  mixed5side::init();
  mixed6side::init();
  mixed7side::init();
  tractor1::init();
  tractor2::init();
  tractor1side::init();
  tractor2side::init();
  shielder1::init();
  shielder1side::init();

  for (const auto& f : formation_base::static_formations) {
    _formations.emplace_back(f);
  }
  auto id_sort = [](const entry& a, const entry& b) {
    return a.id < b.id;
  };
  auto cost_sort = [](const entry& a, const entry& b) {
    return a.cost < b.cost;
  };
  // Sort by ID first for backwards compatibility.
  std::sort(_formations.begin(), _formations.end(), id_sort);
  std::stable_sort(_formations.begin(), _formations.end(), cost_sort);
}
