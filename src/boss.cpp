#include "boss.h"
#include "boss_enemy.h"
#include "player.h"

#include <algorithm>

std::vector<std::pair<int32_t, std::pair<vec2, colour_t>>> Boss::_fireworks;
std::vector<vec2> Boss::_warnings;
const fixed Boss::HP_PER_EXTRA_PLAYER = fixed(1) / 10;
const fixed Boss::HP_PER_EXTRA_CYCLE = 3 * fixed(1) / 10;

const int32_t BigSquareBoss::BASE_HP = 400;
const fixed BigSquareBoss::SPEED = 2 + fixed(1) / 2;
const int32_t BigSquareBoss::TIMER = 100;
const int32_t BigSquareBoss::STIMER = 80;
const fixed BigSquareBoss::ATTACK_RADIUS = 120;
const int32_t BigSquareBoss::ATTACK_TIME = 90;

const int32_t ShieldBombBoss::BASE_HP = 320;
const int32_t ShieldBombBoss::TIMER = 100;
const int32_t ShieldBombBoss::UNSHIELD_TIME = 300;
const int32_t ShieldBombBoss::ATTACK_TIME = 80;
const fixed ShieldBombBoss::SPEED = 1;

const int32_t ChaserBoss::BASE_HP = 60;
const fixed ChaserBoss::SPEED = 4;
const int32_t ChaserBoss::TIMER = 60;
const int32_t ChaserBoss::MAX_SPLIT = 7;

const int32_t TractorBoss::BASE_HP = 900;
const fixed TractorBoss::SPEED = 2;
const int32_t TractorBoss::TIMER = 100;

const int32_t GhostBoss::BASE_HP = 700;
const int32_t GhostBoss::TIMER = 250;
const int32_t GhostBoss::ATTACK_TIME = 100;

const int32_t DeathRayBoss::BASE_HP = 600;
const int32_t DeathRayBoss::ARM_HP = 100;
const int32_t DeathRayBoss::ARM_ATIMER = 300;
const int32_t DeathRayBoss::ARM_RTIMER = 400;
const fixed DeathRayBoss::ARM_SPEED = 4;
const int32_t DeathRayBoss::RAY_TIMER = 100;
const int32_t DeathRayBoss::TIMER = 100;
const fixed DeathRayBoss::SPEED = 5;

const int32_t SuperBoss::BASE_HP = 520;
const int32_t SuperBoss::ARC_HP = 75;

// Generic boss
//------------------------------
Boss::Boss(const vec2& position, z0Game::boss_list boss, int32_t hp,
           int32_t players, int32_t cycle, bool explode_on_damage)
  : Ship(position, Ship::ship_category(SHIP_BOSS | SHIP_ENEMY))
  , _hp(0)
  , _max_hp(0)
  , _flag(boss)
  , _score(0)
  , _ignore_damage_colour(256)
  , _damaged(0)
  , _show_hp(false)
  , _explode_on_damage(explode_on_damage)
{
  set_bounding_width(640);
  set_ignore_damage_colour_index(100);
  long s = 5000 * (cycle + 1) +
           2500 * (boss > z0Game::BOSS_1C) +
           2500 * (boss > z0Game::BOSS_2C);

  _score += s;
  for (int32_t i = 0; i < players - 1; ++i) {
    _score += (int32_t(s) * HP_PER_EXTRA_PLAYER).to_int();
  }
  _hp = CalculateHP(hp, players, cycle);
  _max_hp = _hp;
}

int32_t Boss::CalculateHP(int32_t base, int32_t players, int32_t cycle)
{
  int32_t hp = base * 30;
  int32_t r = hp;
  for (int32_t i = 0; i < cycle; ++i) {
    r += (hp * HP_PER_EXTRA_CYCLE).to_int();
  }
  for (int32_t i = 0; i < players - 1; ++i) {
    r += (hp * HP_PER_EXTRA_PLAYER).to_int();
  }
  for (int32_t i = 0; i < cycle * (players - 1); ++i) {
    r += (hp * HP_PER_EXTRA_CYCLE + HP_PER_EXTRA_PLAYER).to_int();
  }
  return r;
}

void Boss::damage(int32_t damage, bool magic, Player* source)
{
  int32_t actualDamage = get_damage(damage, magic);
  if (actualDamage <= 0) {
    return;
  }

  if (damage >= Player::bomb_damage) {
    if (_explode_on_damage) {
      explosion();
      explosion(0xffffffff, 16);
      explosion(0, 24);
    }

    _damaged = 25;
  }
  else if (_damaged < 1) {
    _damaged = 1;
  }

  actualDamage *= 60 /
      (1 + (z0().get_lives() ? z0().count_players() : z0().alive_players()));
  _hp -= actualDamage;

  if (_hp <= 0 && !is_destroyed()) {
    destroy();
    on_destroy();
  }
  else if (!is_destroyed()) {
    play_sound_random(Lib::SOUND_ENEMY_SHATTER);
  }
}

void Boss::render() const
{
  render(true);
}

void Boss::render(bool hpBar) const
{
  if (hpBar) {
    render_hp_bar();
  }

  if (!_damaged) {
    Ship::render();
    return;
  }
  for (std::size_t i = 0; i < shapes().size(); i++) {
    shapes()[i]->render(
        lib(), to_float(shape().centre), shape().rotation().to_float(),
        int32_t(i) < _ignore_damage_colour ? 0xffffffff : 0);
  }
  _damaged--;
}

void Boss::render_hp_bar() const
{
  if (!_show_hp && is_on_screen()) {
    _show_hp = true;
  }

  if (_show_hp) {
    Ship::render_hp_bar(float(_hp) / float(_max_hp));
  }
}

void Boss::on_destroy()
{
  set_killed();
  for (const auto& ship : z0().all_ships(SHIP_ENEMY)) {
    if (ship != this) {
      ship->damage(Player::bomb_damage, false, 0);
    }
  }
  explosion();
  explosion(0xffffffff, 12);
  explosion(shapes()[0]->colour, 24);
  explosion(0xffffffff, 36);
  explosion(shapes()[0]->colour, 48);
  int32_t n = 1;
  for (int32_t i = 0; i < 16; ++i) {
    vec2 v = vec2::from_polar(z::rand_fixed() * (2 * fixed::pi),
                              8 + z::rand_int(64) + z::rand_int(64));
    _fireworks.push_back(
        std::make_pair(n, std::make_pair(shape().centre + v, shapes()[0]->colour)));
    n += i;
  }
  for (int32_t i = 0; i < Lib::PLAYERS; i++) {
    lib().rumble(i, 25);
  }
  play_sound(Lib::SOUND_EXPLOSION);

  for (const auto& player : z0().get_players()) {
    Player* p = (Player*) player;
    if (!p->is_killed() && get_score() > 0) {
      p->add_score(get_score() / z0().alive_players());
    }
  }
}


// Big square boss
//------------------------------
BigSquareBoss::BigSquareBoss(int32_t players, int32_t cycle)
  : Boss(vec2(Lib::WIDTH * fixed::hundredth * 75, Lib::HEIGHT * 2),
         z0Game::BOSS_1A, BASE_HP, players, cycle)
  , _dir(0, -1)
  , _reverse(false)
  , _timer(TIMER * 6)
  , _spawn_timer(0)
  , _special_timer(0)
  , _special_attack(false)
  , _special_attack_rotate(false)
  , _attack_player(0)
{
  add_shape(new Polygon(vec2(), 160, 4, 0x9933ffff, 0, 0));
  add_shape(new Polygon(vec2(), 140, 4, 0x9933ffff, 0, DANGEROUS));
  add_shape(new Polygon(vec2(), 120, 4, 0x9933ffff, 0, DANGEROUS));
  add_shape(new Polygon(vec2(), 100, 4, 0x9933ffff, 0, 0));
  add_shape(new Polygon(vec2(), 80, 4, 0x9933ffff, 0, 0));
  add_shape(new Polygon(vec2(), 60, 4, 0x9933ffff, 0, VULNERABLE));

  add_shape(new Polygon(vec2(), 155, 4, 0x9933ffff, 0, 0));
  add_shape(new Polygon(vec2(), 135, 4, 0x9933ffff, 0, 0));
  add_shape(new Polygon(vec2(), 115, 4, 0x9933ffff, 0, 0));
  add_shape(new Polygon(vec2(), 95, 4, 0x6600ccff, 0, 0));
  add_shape(new Polygon(vec2(), 75, 4, 0x6600ccff, 0, 0));
  add_shape(new Polygon(vec2(), 55, 4, 0x330099ff, 0, SHIELD));
}

void BigSquareBoss::update()
{
  const vec2& pos = shape().centre;
  if (pos.y < Lib::HEIGHT * fixed::hundredth * 25 && _dir.y == -1) {
    _dir = vec2(_reverse ? 1 : -1, 0);
  }
  if (pos.x < Lib::WIDTH * fixed::hundredth * 25 && _dir.x == -1) {
    _dir = vec2(0, _reverse ? -1 : 1);
  }
  if (pos.y > Lib::HEIGHT * fixed::hundredth * 75 && _dir.y == 1) {
    _dir = vec2(_reverse ? -1 : 1, 0);
  }
  if (pos.x > Lib::WIDTH * fixed::hundredth * 75 && _dir.x == 1) {
    _dir = vec2(0, _reverse ? 1 : -1);
  }

  if (_special_attack) {
    _special_timer--;
    if (_attack_player->is_killed()) {
      _special_timer = 0;
      _attack_player = 0;
    }
    else if (!_special_timer) {
      vec2 d(ATTACK_RADIUS, 0);
      if (_special_attack_rotate) {
        d = d.rotated(fixed::pi / 2);
      }
      for (int32_t i = 0; i < 6; i++) {
        Enemy* s = new Follow(_attack_player->shape().centre + d);
        s->shape().set_rotation(fixed::pi / 4);
        s->set_score(0);
        spawn(s);
        d = d.rotated(2 * fixed::pi / 6);
      }
      _attack_player = 0;
      play_sound(Lib::SOUND_ENEMY_SPAWN);
    }
    if (!_attack_player) {
      _special_attack = false;
    }
  }
  else if (is_on_screen()) {
    _timer--;
    if (_timer <= 0) {
      _timer = (z::rand_int(6) + 1) * TIMER;
      _dir = vec2() - _dir;
      _reverse = !_reverse;
    }
    _spawn_timer++;
    if (_spawn_timer >=
        int32_t(STIMER - z0().alive_players() * 10) / (is_hp_low() ? 2 : 1)) {
      _spawn_timer = 0;
      _special_timer++;
      spawn(new BigFollow(shape().centre, false));
      play_sound_random(Lib::SOUND_BOSS_FIRE);
    }
    if (_special_timer >= 8 && z::rand_int(4)) {
      _special_timer = ATTACK_TIME;
      _special_attack = true;
      _special_attack_rotate = z::rand_int(2) != 0;
      _attack_player = nearest_player();
      play_sound(Lib::SOUND_BOSS_ATTACK);
    }
  }

  if (_special_attack) {
    return;
  }

  move(_dir * SPEED);
  for (std::size_t i = 0; i < 6; ++i) {
    shapes()[i]->rotate((i % 2 ? -1 : 1) * fixed::hundredth * ((1 + i) * 5));
  }
  for (std::size_t i = 0; i < 6; ++i) {
    shapes()[i + 6]->set_rotation(shapes()[i]->rotation());
  }
}

void BigSquareBoss::render() const
{
  Boss::render();
  if ((_special_timer / 4) % 2 && _attack_player) {
    flvec2 d(ATTACK_RADIUS.to_float(), 0);
    if (_special_attack_rotate) {
      d = d.rotated(M_PIf / 2);
    }
    for (int32_t i = 0; i < 6; i++) {
      flvec2 p = to_float(_attack_player->shape().centre) + d;
      Polygon s(vec2(), 10, 4, 0x9933ffff, fixed::pi / 4, 0);
      s.render(lib(), p, 0);
      d = d.rotated(2 * M_PIf / 6);
    }
  }
}

int32_t BigSquareBoss::get_damage(int32_t damage, bool magic)
{
  return damage;
}

// Shield bomb boss
//------------------------------
ShieldBombBoss::ShieldBombBoss(int32_t players, int32_t cycle)
  : Boss(vec2(-Lib::WIDTH / 2, Lib::HEIGHT / 2),
         z0Game::BOSS_1B, BASE_HP, players, cycle)
  , _timer(0)
  , _count(0)
  , _unshielded(0)
  , _attack(0)
  , _side(false)
  , _shot_alternate(false)
{
  add_shape(new Polygon(vec2(), 48, 8, 0x339966ff, 0,
                        DANGEROUS | VULNERABLE, Polygon::T::POLYGRAM));

  for (int32_t i = 0; i < 16; i++) {
    vec2 a = vec2(120, 0).rotated(i * fixed::pi / 8);
    vec2 b = vec2(80, 0).rotated(i * fixed::pi / 8);

    add_shape(new Line(vec2(), a, b, 0x999999ff, 0));
  }

  add_shape(new Polygon(vec2(), 130, 16, 0xccccccff, 0, VULNSHIELD | DANGEROUS));
  add_shape(new Polygon(vec2(), 125, 16, 0xccccccff, 0, 0));
  add_shape(new Polygon(vec2(), 120, 16, 0xccccccff, 0, 0));

  add_shape(new Polygon(vec2(), 42, 16, 0, 0, SHIELD));

  set_ignore_damage_colour_index(1);
}

void ShieldBombBoss::update()
{
  if (!_side && shape().centre.x < Lib::WIDTH * fixed::tenth * 6) {
    move(vec2(1, 0) * SPEED);
  }
  else if (_side && shape().centre.x > Lib::WIDTH * fixed::tenth * 4) {
    move(vec2(-1, 0) * SPEED);
  }

  shape().rotate(-fixed::hundredth * 2);
  shapes()[0]->rotate(-fixed::hundredth * 4);
  shapes()[20]->set_rotation(shapes()[0]->rotation());

  if (!is_on_screen()) {
    return;
  }

  if (is_hp_low()) {
    _timer++;
  }

  if (_unshielded) {
    _timer++;

    _unshielded--;
    for (std::size_t i = 0; i < 3; i++) {
      shapes()[i + 17]->colour =
          (_unshielded / 2) % 4 ? 0x00000000 : 0x666666ff;
    }
    for (std::size_t i = 0; i < 16; i++) {
      shapes()[i + 1]->colour = 
          (_unshielded / 2) % 4 ? 0x00000000 : 0x333333ff;
    }

    if (!_unshielded) {
      shapes()[0]->category = DANGEROUS | VULNERABLE;
      shapes()[17]->category = DANGEROUS | VULNSHIELD;

      for (std::size_t i = 0; i < 3; i++) {
        shapes()[i + 17]->colour = 0xccccccff;
      }
      for (std::size_t i = 0; i < 16; i++) {
        shapes()[i + 1]->colour = 0x999999ff;
      }
    }
  }

  if (_attack) {
    vec2 d = _attack_dir.rotated(
        (ATTACK_TIME - _attack) * fixed::half * fixed::pi / ATTACK_TIME);
    spawn(new SBBossShot(shape().centre, d));
    _attack--;
    play_sound_random(Lib::SOUND_BOSS_FIRE);
  }

  _timer++;
  if (_timer > TIMER) {
    _count++;
    _timer = 0;

    if (_count >= 4 && (!z::rand_int(4) || _count >= 8)) {
      _count = 0;
      if (!_unshielded) {
        if (z0().all_ships(SHIP_POWERUP).size() < 5) {
          spawn(new Powerup(shape().centre, Powerup::BOMB));
        }
      }
    }

    if (!z::rand_int(3)) {
      _side = !_side;
    }

    if (z::rand_int(2)) {
      vec2 d = vec2(5, 0).rotated(shape().rotation());
      for (int32_t i = 0; i < 12; i++) {
        spawn(new SBBossShot(shape().centre, d));
        d = d.rotated(2 * fixed::pi / 12);
      }
      play_sound(Lib::SOUND_BOSS_ATTACK);
    }
    else {
      _attack = ATTACK_TIME;
      _attack_dir = vec2::from_polar(z::rand_fixed() * (2 * fixed::pi), 5);
    }
  }
}

int32_t ShieldBombBoss::get_damage(int32_t damage, bool magic)
{
  if (_unshielded) {
    return damage;
  }
  if (damage >= Player::bomb_damage && !_unshielded) {
    _unshielded = UNSHIELD_TIME;
    shapes()[0]->category = VULNERABLE | DANGEROUS;
    shapes()[17]->category = 0;
  }
  if (!magic) {
    return 0;
  }
  _shot_alternate = !_shot_alternate;
  if (_shot_alternate) {
    restore_hp(60 /
        (1 + (z0().get_lives() ? z0().count_players() : z0().alive_players())));
  }
  return damage;
}

// Chaser boss
//------------------------------
int32_t  ChaserBoss::_count;
bool ChaserBoss::_hasCounted;
int32_t  ChaserBoss::_shared_hp;

// power(HP_REDUCE_POWER = 1.7, split)
static const fixed HP_REDUCE_POWER_lookup[8] = {
  fixed(1),
  1 + 7 * (fixed(1) / 10),
  2 + 9 * (fixed(1) / 10),
  4 + 9 * (fixed(1) / 10),
  8 + 4 * (fixed(1) / 10),
  14 + 2 * (fixed(1) / 10),
  24 + 2 * (fixed(1) / 10),
  41
};

// power(1.5, split)
static const fixed ONE_AND_HALF_lookup[8] = {
  fixed(1),
  1 + (fixed(1) / 2),
  2 + (fixed(1) / 4),
  3 + 4 * (fixed(1) / 10),
  5 + (fixed(1) / 10),
  7 + 6 * (fixed(1) / 10),
  11 + 4 * (fixed(1) / 10),
  17 + (fixed(1) / 10)
};

// power(1.1, split)
static const fixed ONE_PT_ONE_lookup[8] = {
  fixed(1),
  1 + (fixed(1) / 10),
  1 + 2 * (fixed(1) / 10),
  1 + 3 * (fixed(1) / 10),
  1 + 5 * (fixed(1) / 10),
  1 + 6 * (fixed(1) / 10),
  1 + 8 * (fixed(1) / 10),
  1 + 9 * (fixed(1) / 10)
};

// power(1.15, split)
static const fixed ONE_PT_ONE_FIVE_lookup[8] = {
  fixed(1),
  1 + 15 * (fixed(1) / 100),
  1 + 3 * (fixed(1) / 10),
  1 + 5 * (fixed(1) / 10),
  1 + 7 * (fixed(1) / 10),
  2,
  2 + 3 * (fixed(1) / 10),
  2 + 7 * (fixed(1) / 10)
};

// power(1.2, split)
static const fixed ONE_PT_TWO_lookup[8] = {
  fixed(1),
  1 + 2 * (fixed(1) / 10),
  1 + 4 * (fixed(1) / 10),
  1 + 7 * (fixed(1) / 10),
  2 + (fixed(1) / 10),
  2 + (fixed(1) / 2),
  3,
  3 + 6 * (fixed(1) / 10)
};

// power(1.15, remaining)
static const int32_t ONE_PT_ONE_FIVE_intLookup[128] = {
  1, 1, 1, 2, 2, 2, 2, 3, 3, 4, 4, 5, 5, 6, 7, 8, 9, 11, 12, 14, 16, 19, 22,
  25, 29, 33, 38, 44, 50, 58, 66, 76, 88, 101, 116, 133, 153, 176, 203, 233,
  268, 308, 354, 407, 468, 539, 620, 713, 819, 942, 1084, 1246, 1433, 1648,
  1895, 2180, 2507, 2883, 3315, 3812, 4384, 5042, 5798, 6668, 7668, 8818, 10140,
  11662, 13411, 15422, 17736, 20396, 23455, 26974, 31020, 35673, 41024, 47177,
  54254, 62392, 71751, 82513, 94890, 109124, 125493, 144316, 165964, 190858,
  219487, 252410, 290272, 333813, 383884, 441467, 507687, 583840, 671416,
  772129, 887948, 1021140, 1174311, 1350458, 1553026, 1785980, 2053877, 2361959,
  2716252, 3123690, 3592244, 4131080, 4750742, 5463353, 6282856, 7225284,
  8309077, 9555438, 10988754, 12637066, 14532626, 16712520, 19219397, 22102306,
  25417652, 29230299, 33614843, 38657069, 44455628, 51123971
};

ChaserBoss::ChaserBoss(int32_t players, int32_t cycle, int32_t split,
                       const vec2& position, int32_t time, int32_t stagger)
  : Boss(!split ? vec2(Lib::WIDTH / 2, -Lib::HEIGHT / 2) : position,
         z0Game::BOSS_1C,
         1 + BASE_HP / (fixed::half + HP_REDUCE_POWER_lookup[split]).to_int(),
         players, cycle, split <= 1)
  , _on_screen(split != 0)
  , _move(false)
  , _timer(time)
  , _dir()
  , _last_dir()
  , _players(players)
  , _cycle(cycle)
  , _split(split)
  , _stagger(stagger)
{
  add_shape(new Polygon(vec2(), 10 * ONE_AND_HALF_lookup[MAX_SPLIT - _split],
                        5, 0x3399ffff, 0, 0, Polygon::T::POLYGRAM));
  add_shape(new Polygon(vec2(), 9 * ONE_AND_HALF_lookup[MAX_SPLIT - _split],
                        5, 0x3399ffff, 0, 0, Polygon::T::POLYGRAM));
  add_shape(new Polygon(vec2(), 8 * ONE_AND_HALF_lookup[MAX_SPLIT - _split],
                        5, 0, 0, DANGEROUS | VULNERABLE, Polygon::T::POLYGRAM));
  add_shape(new Polygon(vec2(), 7 * ONE_AND_HALF_lookup[MAX_SPLIT - _split],
                        5, 0, 0, SHIELD, Polygon::T::POLYGRAM));

  set_ignore_damage_colour_index(2);
  set_bounding_width(10 * ONE_AND_HALF_lookup[MAX_SPLIT - _split]);
  if (!_split) {
    _shared_hp = 0;
    _count = 0;
  }
}

void ChaserBoss::update()
{
  z0Game::ShipList remaining = z0().all_ships();
  _last_dir = _dir.normalised();
  if (is_on_screen()) {
    _on_screen = true;
  }

  if (_timer) {
    _timer--;
  }
  if (_timer <= 0) {
    _timer = TIMER * (_move + 1);
    if (_split != 0 &&
        (_move || z::rand_int(8 + _split) == 0 ||
         int32_t(remaining.size()) - z0().count_players() <= 4 ||
         !z::rand_int(ONE_PT_ONE_FIVE_intLookup[
             std::max(0, std::min(127, int32_t(remaining.size()) -
                                       z0().count_players()))]))) {
      _move = !_move;
    }
    if (_move) {
      _dir = SPEED * ONE_PT_ONE_lookup[_split] *
          (nearest_player()->shape().centre - shape().centre).normalised();
    }
  }
  if (_move) {
    move(_dir);
  }
  else {
    const z0Game::ShipList& nearby = z0().all_ships(SHIP_PLAYER | SHIP_BOSS);
    const fixed attract = 256 * ONE_PT_ONE_lookup[MAX_SPLIT - _split];
    const fixed align = 128 * ONE_PT_ONE_FIVE_lookup[MAX_SPLIT - _split];
    const fixed repulse = 64 * ONE_PT_TWO_lookup[MAX_SPLIT - _split];
    static const fixed c_attract = -(1 + 2 * fixed::tenth);
    static const fixed c_dir0 = SPEED / 14;
    static const fixed c_dir1 = 8 * SPEED / (14 * 9);
    static const fixed c_dir2 = 8 * SPEED / (14 * 11);
    static const fixed c_dir3 = 8 * SPEED / (14 * 2);
    static const fixed c_dir4 = 8 * SPEED / (14 * 3);
    static const fixed c_rotate = fixed::hundredth * 5 / fixed(MAX_SPLIT);

    _dir = _last_dir;
    if (_stagger == _count % (_split == 0 ? 1 :
                              _split == 1 ? 2 :
                              _split == 2 ? 4 :
                              _split == 3 ? 8 : 16)) {
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
          fixed pow = ONE_PT_ONE_FIVE_lookup[MAX_SPLIT - c->_split];
          v *= pow;
          p = c->_last_dir * pow;
        }
        else {
          p = vec2::from_polar(ship->shape().rotation(), 1);
        }

        if (len > attract) {
          continue;
        }
        // Attract
        _dir += v * c_attract;
        if (len > align) {
          continue;
        }
        // Align
        _dir += p;
        if (len > repulse) {
          continue;
        }
        // Repulse
        _dir += v * 3;
      }
    }
    if (int32_t(remaining.size()) - z0().count_players() < 4 &&
        _split >= MAX_SPLIT - 1) {
      if ((shape().centre.x < 32 && _dir.x < 0) ||
          (shape().centre.x >= Lib::WIDTH - 32 && _dir.x > 0)) {
        _dir.x = -_dir.x;
      }
      if ((shape().centre.y < 32 && _dir.y < 0) ||
          (shape().centre.y >= Lib::HEIGHT - 32 && _dir.y > 0)) {
        _dir.y = -_dir.y;
      }
    }
    else if (int32_t(remaining.size()) - z0().count_players() < 8 &&
             _split >= MAX_SPLIT - 1) {
      if ((shape().centre.x < 0 && _dir.x < 0) ||
          (shape().centre.x >= Lib::WIDTH && _dir.x > 0)) {
        _dir.x = -_dir.x;
      }
      if ((shape().centre.y < 0 && _dir.y < 0) ||
          (shape().centre.y >= Lib::HEIGHT && _dir.y > 0)) {
        _dir.y = -_dir.y;
      }
    }
    else {
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
    }
    else if (shape().centre.x >= Lib::WIDTH + 256) {
      _dir = vec2(-1, 0);
    }
    else if (shape().centre.y < -256) {
      _dir = vec2(0, 1);
    }
    else if (shape().centre.y >= Lib::HEIGHT + 256) {
      _dir = vec2(0, -1);
    }
    else {
      _dir = _dir.normalised();
    }

    _dir =
        _split == 0 ? _dir + _last_dir * 7 :
        _split == 1 ? _dir * 2 + _last_dir * 7 :
        _split == 2 ? _dir * 4 + _last_dir * 7 :
        _split == 3 ? _dir + _last_dir : _dir * 2 + _last_dir;

    _dir *= ONE_PT_ONE_lookup[_split] *
        (_split == 0 ? c_dir0 :
         _split == 1 ? c_dir1 :
         _split == 2 ? c_dir2 :
         _split == 3 ? c_dir3 : c_dir4);
    move(_dir);
    shape().rotate(fixed::hundredth * 2 + fixed(_split) * c_rotate);
  }
  _shared_hp = 0;
  if (!_hasCounted) {
    _hasCounted = true;
    ++_count;
  }
}

void ChaserBoss::render() const
{
  Boss::render();
  static std::vector<int32_t> hpLookup;
  if (hpLookup.empty()) {
    int32_t hp = 0;
    for (int32_t i = 0; i < 8; i++) {
      hp = 2 * hp + CalculateHP(
          1 + BASE_HP / (fixed::half + HP_REDUCE_POWER_lookup[7 - i]).to_int(),
          _players, _cycle);
      hpLookup.push_back(hp);
    }
  }
  _shared_hp += (_split == 7 ? 0 : 2 * hpLookup[6 - _split]) +
      get_remaining_hp() * 30;
  if (_on_screen) {
    Ship::render_hp_bar(float(_shared_hp) / float(hpLookup[MAX_SPLIT]));
  }
}

int32_t ChaserBoss::get_damage(int32_t damage, bool magic)
{
  return damage;
}

void ChaserBoss::on_destroy()
{
  bool last = false;
  if (_split < MAX_SPLIT) {
    for (int32_t i = 0; i < 2; i++) {
      vec2 d = vec2::from_polar(i * fixed::pi + shape().rotation(),
                                8 * ONE_PT_TWO_lookup[MAX_SPLIT - 1 - _split]);
      Ship* s = new ChaserBoss(
          _players, _cycle, _split + 1, shape().centre + d,
          (i + 1) * TIMER / 2,
          z::rand_int(_split + 1 == 1 ? 2 :
                      _split + 1 == 2 ? 4 :
                      _split + 1 == 3 ? 8 : 16));
      spawn(s);
      s->shape().set_rotation(shape().rotation());
    }
  }
  else {
    last = true;
    for (const auto& ship : z0().all_ships(SHIP_ENEMY)) {
      if (!ship->is_destroyed() && ship != this) {
        last = false;
        break;
      }
    }

    if (last) {
      set_killed();
      for (int32_t i = 0; i < Lib::PLAYERS; i++) {
        lib().rumble(i, 25);
      }
      z0Game::ShipList players = z0().get_players();
      int32_t n = 0;
      for (std::size_t i = 0; i < players.size(); i++) {
        Player* p = (Player*) players[i];
        if (!p->is_killed()) {
          n++;
        }
      }
      for (std::size_t i = 0; i < players.size(); i++) {
        Player* p = (Player*) players[i];
        if (!p->is_killed()) {
          p->add_score(get_score() / n);
        }
      }
      n = 1;
      for (int32_t i = 0; i < 16; ++i) {
        vec2 v = vec2::from_polar(z::rand_fixed() * (2 * fixed::pi),
                                  8 + z::rand_int(64) + z::rand_int(64));
        _fireworks.push_back(std::make_pair(
            n, std::make_pair(shape().centre + v, shapes()[0]->colour)));
        n += i;
      }
    }
  }

  for (int32_t i = 0; i < Lib::PLAYERS; i++) {
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
  }
  else {
    play_sound_random(Lib::SOUND_EXPLOSION);
  }
}

// Tractor beam boss
//------------------------------
TractorBoss::TractorBoss(int32_t players, int32_t cycle)
  : Boss(vec2(Lib::WIDTH * (1 + fixed::half), Lib::HEIGHT / 2),
         z0Game::BOSS_2A, BASE_HP, players, cycle)
  , _will_attack(false)
  , _stopped(false)
  , _generating(false)
  , _attacking(false)
  , _continue(false)
  , _gen_dir(0)
  , _shoot_type(z::rand_int(2))
  , _sound(true)
  , _timer(0)
  , _attack_size(0)
{
  _s1 = new CompoundShape(vec2(0, -96), 0, DANGEROUS | VULNERABLE);

  _s1->add_shape(
      new Polygon(vec2(), 12, 6, 0x882288ff, 0, 0, Polygon::T::POLYGRAM));
  _s1->add_shape(new Polygon(vec2(), 12, 12, 0x882288ff, 0, 0));
  _s1->add_shape(new Polygon(vec2(), 2, 6, 0x882288ff, 0, 0));
  _s1->add_shape(new Polygon(vec2(), 36, 12, 0xcc33ccff, 0, 0));
  _s1->add_shape(new Polygon(vec2(), 34, 12, 0xcc33ccff, 0, 0));
  _s1->add_shape(new Polygon(vec2(), 32, 12, 0xcc33ccff, 0, 0));
  for (int32_t i = 0; i < 8; i++) {
    vec2 d = vec2(24, 0).rotated(i * fixed::pi / 4);
    _s1->add_shape(
        new Polygon(d, 12, 6, 0xcc33ccff, 0, 0, Polygon::T::POLYGRAM));
  }

  _s2 = new CompoundShape(vec2(0, 96), 0, DANGEROUS | VULNERABLE);

  _s2->add_shape(
      new Polygon(vec2(), 12, 6, 0x882288ff, 0, 0, Polygon::T::POLYGRAM));
  _s2->add_shape(new Polygon(vec2(), 12, 12, 0x882288ff, 0, 0));
  _s2->add_shape(new Polygon(vec2(), 2, 6, 0x882288ff, 0, 0));

  _s2->add_shape(new Polygon(vec2(), 36, 12, 0xcc33ccff, 0, 0));
  _s2->add_shape(new Polygon(vec2(), 34, 12, 0xcc33ccff, 0, 0));
  _s2->add_shape(new Polygon(vec2(), 32, 12, 0xcc33ccff, 0, 0));
  for (int32_t i = 0; i < 8; i++) {
    vec2 d = vec2(24, 0).rotated(i * fixed::pi / 4);
    _s2->add_shape(
        new Polygon(d, 12, 6, 0xcc33ccff, 0, 0, Polygon::T::POLYGRAM));
  }

  add_shape(_s1);
  add_shape(_s2);

  _sattack = new Polygon(vec2(), 0, 16, 0x993399ff);
  add_shape(_sattack);

  add_shape(new Line(vec2(), vec2(-2, -96), vec2(-2, 96), 0xcc33ccff, 0));
  add_shape(new Line(vec2(), vec2(0, -96), vec2(0, 96), 0x882288ff, 0));
  add_shape(new Line(vec2(), vec2(2, -96), vec2(2, 96), 0xcc33ccff, 0));

  add_shape(new Polygon(vec2(0, 96), 30, 12, 0, 0, SHIELD));
  add_shape(new Polygon(vec2(0, -96), 30, 12, 0, 0, SHIELD));

  _attack_shapes = shapes().size();
}

void TractorBoss::update()
{
  if (shape().centre.x <= Lib::WIDTH / 2 &&
      _will_attack && !_stopped && !_continue) {
    _stopped = true;
    _generating = true;
    _gen_dir = z::rand_int(2) == 0;
    _timer = 0;
  }

  if (shape().centre.x < -150) {
    shape().centre.x = Lib::WIDTH + 150;
    _will_attack = !_will_attack;
    _shoot_type = z::rand_int(2);
    if (_will_attack) {
      _shoot_type = 0;
    }
    _continue = false;
    _sound = !_will_attack;
    shape().rotate(z::rand_fixed() * 2 * fixed::pi);
  }

  _timer++;
  if (!_stopped) {
    move(SPEED * vec2(-1, 0));
    if (!_will_attack && is_on_screen() &&
        _timer % (16 - z0().alive_players() * 2) == 0) {
      if (_shoot_type == 0 || (is_hp_low() && _shoot_type == 1)) {
        Player* p = nearest_player();

        vec2 v = _s1->convert_point(shape().centre, shape().rotation(), vec2());
        vec2 d = (p->shape().centre - v).normalised();
        spawn(new SBBossShot(v, d * 5, 0xcc33ccff));
        spawn(new SBBossShot(v, d * -5, 0xcc33ccff));

        v = _s2->convert_point(shape().centre, shape().rotation(), vec2());
        d = (p->shape().centre - v).normalised();
        spawn(new SBBossShot(v, d * 5, 0xcc33ccff));
        spawn(new SBBossShot(v, d * -5, 0xcc33ccff));

        play_sound_random(Lib::SOUND_BOSS_FIRE);
      }
      if (_shoot_type == 1 || is_hp_low()) {
        Player* p = nearest_player();
        vec2 v = shape().centre;

        vec2 d = (p->shape().centre - v).normalised();
        spawn(new SBBossShot(v, d * 5, 0xcc33ccff));
        spawn(new SBBossShot(v, d * -5, 0xcc33ccff));
        play_sound_random(Lib::SOUND_BOSS_FIRE);
      }
    }
    if ((!_will_attack || _continue) && is_on_screen()) {
      if (_sound) {
        play_sound(Lib::SOUND_BOSS_ATTACK);
        _sound = false;
      }
      _targets.clear();
      for (const auto& ship : z0().all_ships(SHIP_PLAYER)) {
        if (((Player*) ship)->is_killed()) {
          continue;
        }
        vec2 pos = ship->shape().centre;
        _targets.push_back(pos);
        vec2 d = (shape().centre - pos).normalised();
        ship->move(d * Tractor::TRACTOR_SPEED);
      }
    }
  }
  else {
    if (_generating) {
      if (_timer >= TIMER * 5) {
        _timer = 0;
        _generating = false;
        _attacking = false;
        _attack_size = 0;
        play_sound(Lib::SOUND_BOSS_ATTACK);
      }

      if (_timer % (10 - 2 * z0().alive_players()) == 0 && _timer < TIMER * 4) {
        Ship* s = new TBossShot(
            _s1->convert_point(shape().centre, shape().rotation(), vec2()),
            _gen_dir ? shape().rotation() + fixed::pi : shape().rotation());
        spawn(s);

        s = new TBossShot(
            _s2->convert_point(shape().centre, shape().rotation(), vec2()),
            shape().rotation() + (_gen_dir ? 0 : fixed::pi));
        spawn(s);
        play_sound_random(Lib::SOUND_ENEMY_SPAWN);
      }

      if (is_hp_low() && _timer % (20 - z0().alive_players() * 2) == 0) {
        Player* p = nearest_player();
        vec2 v = shape().centre;

        vec2 d = (p->shape().centre - v).normalised();
        spawn(new SBBossShot(v, d * 5, 0xcc33ccff));
        spawn(new SBBossShot(v, d * -5, 0xcc33ccff));
        play_sound_random(Lib::SOUND_BOSS_FIRE);
      }
    }
    else {
      if (!_attacking) {
        if (_timer >= TIMER * 4) {
          _timer = 0;
          _attacking = true;
        }
        if (_timer % (TIMER / (1 + fixed::half)).to_int() == TIMER / 8) {
          vec2 v = _s1->convert_point(
              shape().centre, shape().rotation(), vec2());
          vec2 d = vec2::from_polar(z::rand_fixed() * (2 * fixed::pi), 5);
          spawn(new SBBossShot(v, d, 0xcc33ccff));
          d = d.rotated(fixed::pi / 2);
          spawn(new SBBossShot(v, d, 0xcc33ccff));
          d = d.rotated(fixed::pi / 2);
          spawn(new SBBossShot(v, d, 0xcc33ccff));
          d = d.rotated(fixed::pi / 2);
          spawn(new SBBossShot(v, d, 0xcc33ccff));

          v = _s2->convert_point(shape().centre, shape().rotation(), vec2());
          d = vec2::from_polar(z::rand_fixed() * (2 * fixed::pi), 5);
          spawn(new SBBossShot(v, d, 0xcc33ccff));
          d = d.rotated(fixed::pi / 2);
          spawn(new SBBossShot(v, d, 0xcc33ccff));
          d = d.rotated(fixed::pi / 2);
          spawn(new SBBossShot(v, d, 0xcc33ccff));
          d = d.rotated(fixed::pi / 2);
          spawn(new SBBossShot(v, d, 0xcc33ccff));
          play_sound_random(Lib::SOUND_BOSS_FIRE);
        }
        _targets.clear();
        for (const auto& ship : z0().all_ships(SHIP_PLAYER | SHIP_ENEMY)) {
          if (ship == this || ((ship->type() & SHIP_PLAYER) &&
                               ((Player*) ship)->is_killed())) {
            continue;
          }

          if (ship->type() & SHIP_ENEMY) {
            play_sound_random(Lib::SOUND_BOSS_ATTACK, 0, 0.3f);
          }
          vec2 pos = ship->shape().centre;
          _targets.push_back(pos);
          fixed speed = 0;
          if (ship->type() & SHIP_PLAYER) {
            speed = Tractor::TRACTOR_SPEED;
          }
          if (ship->type() & SHIP_ENEMY) {
            speed = 4 + fixed::half;
          }
          vec2 d = (shape().centre - pos).normalised();
          ship->move(d * speed);

          if ((ship->type() & SHIP_ENEMY) &&
              !(ship->type() & SHIP_WALL) &&
              (ship->shape().centre - shape().centre).length() <= 40) {
            ship->destroy();
            _attack_size++;
            _sattack->radius = _attack_size / (1 + fixed::half);
            add_shape(new Polygon(vec2(), 8, 6, 0xcc33ccff, 0, 0));
          }
        }
      }
      else {
        _timer = 0;
        _stopped = false;
        _continue = true;
        for (int32_t i = 0; i < _attack_size; i++) {
          vec2 d = vec2::from_polar(i * (2 * fixed::pi) / _attack_size, 5);
          spawn(new SBBossShot(shape().centre, d, 0xcc33ccff));
        }
        play_sound(Lib::SOUND_BOSS_FIRE);
        play_sound_random(Lib::SOUND_EXPLOSION);
        _attack_size = 0;
        _sattack->radius = 0;
        while (_attack_shapes < shapes().size()) {
          destroy_shape(_attack_shapes);
        }
      }
    }
  }

  for (std::size_t i = _attack_shapes; i < shapes().size(); ++i) {
    vec2 v = vec2::from_polar(z::rand_fixed() * (2 * fixed::pi),
                              2 * (z::rand_fixed() - fixed::half) *
                                  fixed(_attack_size) / (1 + fixed::half));
    shapes()[i]->centre = v;
  }

  fixed r = 0;
  if (!_will_attack || (_stopped && !_generating && !_attacking)) {
    r = fixed::hundredth;
  }
  else if (_stopped && _attacking && !_generating) {
    r = 0;
  }
  else {
    r = fixed::hundredth / 2;
  }
  shape().rotate(r);

  _s1->rotate(fixed::tenth / 2);
  _s2->rotate(-fixed::tenth / 2);
  for (std::size_t i = 0; i < 8; i++) {
    _s1->shapes()[4 + i]->rotate(-fixed::tenth);
    _s2->shapes()[4 + i]->rotate(fixed::tenth);
  }
}

void TractorBoss::render() const
{
  Boss::render();
  if ((_stopped && !_generating && !_attacking) ||
      (!_stopped && (_continue || !_will_attack) && is_on_screen())) {
    for (std::size_t i = 0; i < _targets.size(); ++i) {
      if (((_timer + i * 4) / 4) % 2) {
        lib().render_line(to_float(shape().centre),
                          to_float(_targets[i]), 0xcc33ccff);
      }
    }
  }
}

int32_t TractorBoss::get_damage(int32_t damage, bool magic)
{
  return damage;
}

// Ghost boss
//------------------------------
GhostBoss::GhostBoss(int32_t players, int32_t cycle)
  : Boss(vec2(Lib::WIDTH / 2, Lib::HEIGHT / 2),
         z0Game::BOSS_2B, BASE_HP, players, cycle)
  , _visible(false)
  , _vtime(0)
  , _timer(0)
  , _attack_time(ATTACK_TIME)
  , _attack(0)
  , _rDir(false)
  , _start_time(120)
  , _danger_circle(0)
  , _danger_offset1(0)
  , _danger_offset2(0)
  , _danger_offset3(0)
  , _danger_offset4(0)
  , _shot_type(false)
{
  add_shape(new Polygon(vec2(), 32, 8, 0x9933ccff, 0, 0));
  add_shape(new Polygon(vec2(), 48, 8, 0xcc66ffff, 0, 0));

  for (int32_t i = 0; i < 8; i++) {
    vec2 c = vec2::from_polar(i * fixed::pi / 4, 48);

    CompoundShape* s = new CompoundShape(c, 0, 0);
    for (int32_t j = 0; j < 8; j++) {
      vec2 d = vec2::from_polar(j * fixed::pi / 4, 1);
      s->add_shape(new Line(vec2(), d * 10, d * 10 * 2, 0x9933ccff, 0));
    }
    add_shape(s);
  }

  add_shape(new Polygon(vec2(), 24, 8, 0xcc66ffff, 0, 0, Polygon::T::POLYGRAM));

  for (int32_t n = 0; n < 5; n++) {
    CompoundShape* s = new CompoundShape(vec2(), 0, 0);
    for (int32_t i = 0; i < 16 + n * 6; i++) {
      vec2 d = vec2::from_polar(i * 2 * fixed::pi / (16 + n * 6), 100 + n * 60);
      s->add_shape(new Polygon(d, 16, 8, n ? 0x330066ff : 0x9933ccff,
                               0, 0, Polygon::T::POLYSTAR));
      s->add_shape(new Polygon(d, 12, 8, n ? 0x330066ff : 0x9933ccff));
    }
    add_shape(s);
  }

  for (int32_t n = 0; n < 5; n++) {
    CompoundShape* s = new CompoundShape(vec2(), 0, DANGEROUS);
    add_shape(s);
  }

  set_ignore_damage_colour_index(12);
}

void GhostBoss::update()
{
  for (int32_t n = 1; n < 5; ++n) {
    CompoundShape* s = (CompoundShape*) shapes()[11 + n].get();
    CompoundShape* c = (CompoundShape*) shapes()[16 + n].get();
    c->clear_shapes();

    for (int32_t i = 0; i < 16 + n * 6; ++i) {
      Shape& s1 = *s->shapes()[2 * i];
      Shape& s2 = *s->shapes()[1 + 2 * i];
      int32_t t =
          n == 1 ? (i + _danger_offset1) % 11 :
          n == 2 ? (i + _danger_offset2) % 7 :
          n == 3 ? (i + _danger_offset3) % 17 : (i + _danger_offset4) % 10;

      bool b =
          n == 1 ? (t % 11 == 0 || t % 11 == 5) :
          n == 2 ? (t % 7 == 0) :
          n == 3 ? (t % 17 == 0 || t % 17 == 8) : (t % 10 == 0);

      if (((n == 4 && _attack != 2) ||
           (n + (n == 3)) & _danger_circle) &&
          _visible && b) {
        s1.colour = 0xcc66ffff;
        s2.colour = 0xcc66ffff;
        if (_vtime == 0) {
          vec2 d = vec2::from_polar(
              i * 2 * fixed::pi / (16 + n * 6), 100 + n * 60);
          c->add_shape(new Polygon(d, 9, 8, 0));
          _warnings.push_back(
              c->convert_point(shape().centre, shape().rotation(), d));
        }
      }
      else {
        s1.colour = 0x330066ff;
        s2.colour = 0x330066ff;
      }
    }
  }
  if (!(_attack == 2 && _attack_time < ATTACK_TIME * 4 && _attack_time)) {
    shape().rotate(-fixed::hundredth * 4);
  }

  for (std::size_t i = 0; i < 8; i++) {
    shapes()[i + 2]->rotate(fixed::hundredth * 4);
  }
  shapes()[10]->rotate(fixed::hundredth * 6);
  for (int32_t n = 0; n < 5; n++) {
    CompoundShape* s = (CompoundShape*) shapes()[11 + n].get();
    if (n % 2) {
      s->rotate(fixed::hundredth * 3 + (fixed(3) / 2000) * n);
    }
    else {
      s->rotate(fixed::hundredth * 5 - (fixed(3) / 2000) * n);
    }
    for (const auto& t : s->shapes()) {
      t->rotate(-fixed::tenth);
    }

    s = (CompoundShape*) shapes()[16 + n].get();
    if (n % 2) {
      s->rotate(fixed::hundredth * 3 + (fixed(3) / 2000) * n);
    }
    else {
      s->rotate(fixed::hundredth * 4 - (fixed(3) / 2000) * n);
    }
    for (const auto& t : s->shapes()) {
      t->rotate(-fixed::tenth);
    }
  }

  if (_start_time) {
    _start_time--;
    return;
  }

  if (!_attack_time) {
    _timer++;
    if (_timer == 9 * TIMER / 10 ||
        (_timer >= TIMER / 10 && _timer < 9 * TIMER / 10 - 16 &&
         ((_timer % 16 == 0 && _attack == 2) ||
          (_timer % 32 == 0 && _shot_type)))) {
      for (int32_t i = 0; i < 8; i++) {
        vec2 d = vec2::from_polar(i * fixed::pi / 4 + shape().rotation(), 5);
        spawn(new SBBossShot(shape().centre, d, 0xcc66ffff));
      }
      play_sound_random(Lib::SOUND_BOSS_FIRE);
    }
    if (_timer != 9 * TIMER / 10 && _timer >= TIMER / 10 &&
        _timer < 9 * TIMER / 10 - 16 && _timer % 16 == 0 &&
        (!_shot_type || _attack == 2)) {
      Player* p = nearest_player();
      vec2 d = (p->shape().centre - shape().centre).normalised();

      if (d.length() > fixed::half) {
        spawn(new SBBossShot(shape().centre, d * 5, 0xcc66ffff));
        spawn(new SBBossShot(shape().centre, d * -5, 0xcc66ffff));
        play_sound_random(Lib::SOUND_BOSS_FIRE);
      }
    }
  }
  else {
    _attack_time--;

    if (_attack == 2) {
      if (_attack_time >= ATTACK_TIME * 4 && _attack_time % 8 == 0) {
        vec2 pos(z::rand_int(Lib::WIDTH + 1), z::rand_int(Lib::HEIGHT + 1));
        spawn(new GhostMine(pos, this));
        play_sound_random(Lib::SOUND_ENEMY_SPAWN);
        _danger_circle = 0;
      }
      else if (_attack_time == ATTACK_TIME * 4 - 1) {
        _visible = true;
        _vtime = 60;
        add_shape(new Box(vec2(Lib::WIDTH / 2 + 32, 0),
                               Lib::WIDTH / 2, 10, 0xcc66ffff, 0, 0));
        add_shape(new Box(vec2(Lib::WIDTH / 2 + 32, 0),
                               Lib::WIDTH / 2, 7, 0xcc66ffff, 0, 0));
        add_shape(new Box(vec2(Lib::WIDTH / 2 + 32, 0),
                               Lib::WIDTH / 2, 4, 0xcc66ffff, 0, 0));

        add_shape(new Box(vec2(-Lib::WIDTH / 2 - 32, 0),
                               Lib::WIDTH / 2, 10, 0xcc66ffff, 0, 0));
        add_shape(new Box(vec2(-Lib::WIDTH / 2 - 32, 0),
                               Lib::WIDTH / 2, 7, 0xcc66ffff, 0, 0));
        add_shape(new Box(vec2(-Lib::WIDTH / 2 - 32, 0),
                               Lib::WIDTH / 2, 4, 0xcc66ffff, 0, 0));
        play_sound(Lib::SOUND_BOSS_FIRE);
      }
      else if (_attack_time < ATTACK_TIME * 3) {
        if (_attack_time == ATTACK_TIME * 3 - 1) {
          play_sound(Lib::SOUND_BOSS_ATTACK);
        }
        shape().rotate((_rDir ? 1 : -1) * 2 * fixed::pi / (ATTACK_TIME * 6));
        if (!_attack_time) {
          for (int32_t i = 0; i < 6; i++) {
            destroy_shape(21);
          }
        }
        _danger_offset1 = z::rand_int(11);
        _danger_offset2 = z::rand_int(7);
        _danger_offset3 = z::rand_int(17);
        _danger_offset3 = z::rand_int(10);
      }
    }

    if (_attack == 0 && _attack_time == ATTACK_TIME * 2) {
      bool r = z::rand_int(2) != 0;
      spawn(new GhostWall(r, true));
      spawn(new GhostWall(!r, false));
    }
    if (_attack == 1 && _attack_time == ATTACK_TIME * 2) {
      _rDir = z::rand_int(2) != 0;
      spawn(new GhostWall(!_rDir, false, 0));
    }
    if (_attack == 1 && _attack_time == ATTACK_TIME * 1) {
      spawn(new GhostWall(_rDir, true, 0));
    }

    if ((_attack == 0 || _attack == 1) && is_hp_low() &&
        _attack_time == ATTACK_TIME * 1) {
      int32_t r = z::rand_int(4);
      vec2 v = r == 0 ? vec2(-Lib::WIDTH / 4, Lib::HEIGHT / 2) :
               r == 1 ? vec2(Lib::WIDTH + Lib::WIDTH / 4, Lib::HEIGHT / 2) :
               r == 2 ? vec2(Lib::WIDTH / 2, -Lib::HEIGHT / 4) :
                        vec2(Lib::WIDTH / 2, Lib::HEIGHT + Lib::HEIGHT / 4);
      spawn(new BigFollow(v, false));
    }
    if (!_attack_time && !_visible) {
      _visible = true;
      _vtime = 60;
    }
    if (_attack_time == 0 && _attack != 2) {
      int32_t r = z::rand_int(3);
      _danger_circle |= 1 + (r == 2 ? 3 : r);
      if (z::rand_int(2) || is_hp_low()) {
        r = z::rand_int(3);
        _danger_circle |= 1 + (r == 2 ? 3 : r);
      }
      if (z::rand_int(2) || is_hp_low()) {
        r = z::rand_int(3);
        _danger_circle |= 1 + (r == 2 ? 3 : r);
      }
    }
    else {
      _danger_circle = 0;
    }
  }

  if (_timer >= TIMER) {
    _timer = 0;
    _visible = false;
    _vtime = 60;
    _attack = z::rand_int(3);
    _shot_type = z::rand_int(2) == 0;
    shapes()[0]->category = 0;
    shapes()[1]->category = 0;
    shapes()[11]->category = 0;
    if (shapes().size() >= 22) {
      shapes()[21]->category = 0;
      shapes()[24]->category = 0;
    }
    play_sound(Lib::SOUND_BOSS_ATTACK);

    if (_attack == 0) {
      _attack_time = ATTACK_TIME * 2 + 50;
    }
    if (_attack == 1) {
      _attack_time = ATTACK_TIME * 3;
      for (int32_t i = 0; i < z0().count_players(); i++) {
        spawn(new Powerup(shape().centre, Powerup::SHIELD));
      }
    }
    if (_attack == 2) {
      _attack_time = ATTACK_TIME * 6;
      _rDir = z::rand_int(2) != 0;
    }
  }

  if (_vtime) {
    _vtime--;
    if (!_vtime && _visible) {
      shapes()[0]->category = SHIELD;
      shapes()[1]->category = DANGEROUS | VULNERABLE;
      shapes()[11]->category = DANGEROUS;
      if (shapes().size() >= 22) {
        shapes()[21]->category = DANGEROUS;
        shapes()[24]->category = DANGEROUS;
      }
    }
  }
}

void GhostBoss::render() const
{
  if ((_start_time / 4) % 2 == 1) {
    render_with_colour(1);
    return;
  }
  if ((_visible && ((_vtime / 4) % 2 == 0)) ||
      (!_visible && ((_vtime / 4) % 2 == 1))) {
    Boss::render();
    return;
  }

  render_with_colour(0x330066ff);
  if (!_start_time) {
    render_hp_bar();
  }
}

int32_t GhostBoss::get_damage(int32_t damage, bool magic)
{
  return damage;
}

// Death ray boss
//------------------------------
DeathRayBoss::DeathRayBoss(int32_t players, int32_t cycle)
  : Boss(vec2(Lib::WIDTH * (fixed(3) / 20), -Lib::HEIGHT),
         z0Game::BOSS_2C, BASE_HP, players, cycle)
  , _timer(TIMER * 2)
  , _laser(false)
  , _dir(true)
  , _pos(1)
  , _arm_timer(0)
  , _shot_timer(0)
  , _ray_attack_timer(0)
  , _ray_src1()
  , _ray_src2()
{
  add_shape(new Polygon(
      vec2(), 110, 12, 0x228855ff, fixed::pi / 12, 0, Polygon::T::POLYSTAR));
  add_shape(new Polygon(
      vec2(), 70, 12, 0x33ff99ff, fixed::pi / 12, 0, Polygon::T::POLYGRAM));
  add_shape(new Polygon(
      vec2(), 120, 12, 0x33ff99ff, fixed::pi / 12, DANGEROUS | VULNERABLE));
  add_shape(new Polygon(vec2(), 115, 12, 0x33ff99ff, fixed::pi / 12, 0));
  add_shape(new Polygon(
      vec2(), 110, 12, 0x33ff99ff, fixed::pi / 12, SHIELD));

  CompoundShape* s1 = new CompoundShape(vec2(), 0, DANGEROUS);
  for (int32_t i = 1; i < 12; i++) {
    CompoundShape* s2 = new CompoundShape(vec2(), i * fixed::pi / 6, 0);
    s2->add_shape(new Box(vec2(130, 0), 10, 24, 0x33ff99ff, 0, 0));
    s2->add_shape(new Box(vec2(130, 0), 8, 22, 0x228855ff, 0, 0));
    s1->add_shape(s2);
  }
  add_shape(s1);

  set_ignore_damage_colour_index(5);
  shape().rotate(2 * fixed::pi * fixed(z::rand_int()) / z::rand_max);
}

void DeathRayBoss::update()
{
  bool positioned = true;
  fixed d =
      _pos == 0 ? 1 * Lib::HEIGHT / 4 - shape().centre.y :
      _pos == 1 ? 2 * Lib::HEIGHT / 4 - shape().centre.y :
      _pos == 2 ? 3 * Lib::HEIGHT / 4 - shape().centre.y :
      _pos == 3 ? 1 * Lib::HEIGHT / 8 - shape().centre.y :
      _pos == 4 ? 3 * Lib::HEIGHT / 8 - shape().centre.y :
      _pos == 5 ? 5 * Lib::HEIGHT / 8 - shape().centre.y :
      7 * Lib::HEIGHT / 8 - shape().centre.y;

  if (d.abs() > 3) {
    move(vec2(0, d / d.abs()) * SPEED);
    positioned = false;
  }

  if (_ray_attack_timer) {
    _ray_attack_timer--;
    if (_ray_attack_timer == 40) {
      _ray_dest = nearest_player()->shape().centre;
    }
    if (_ray_attack_timer < 40) {
      vec2 d = (_ray_dest - shape().centre).normalised();
      spawn(new SBBossShot(shape().centre, d * 10, 0xccccccff));
      play_sound_random(Lib::SOUND_BOSS_ATTACK);
      explosion();
    }
  }

  bool goingFast = false;
  if (_laser) {
    if (_timer) {
      if (positioned) {
        _timer--;
      }

      if (_timer < TIMER * 2 && !(_timer % 3)) {
        spawn(new DeathRay(shape().centre + vec2(100, 0)));
        play_sound_random(Lib::SOUND_BOSS_FIRE);
      }
      if (!_timer) {
        _laser = false;
        _timer = TIMER * (2 + z::rand_int(3));
      }
    }
    else {
      fixed r = shape().rotation();
      if (r == 0) {
        _timer = TIMER * 2;
      }

      if (r < fixed::tenth || r > 2 * fixed::pi - fixed::tenth) {
        shape().set_rotation(0);
      }
      else {
        goingFast = true;
        shape().rotate(_dir ? fixed::tenth : -fixed::tenth);
      }
    }
  }
  else {
    shape().rotate(_dir ? fixed::hundredth * 2 : -fixed::hundredth * 2);
    if (is_on_screen()) {
      _timer--;
      if (_timer % TIMER == 0 && _timer != 0 && !z::rand_int(4)) {
        _dir = z::rand_int(2) != 0;
        _pos = z::rand_int(7);
      }
      if (_timer == TIMER * 2 + 50 && _arms.size() == 2) {
        _ray_attack_timer = RAY_TIMER;
        _ray_src1 = _arms[0]->shape().centre;
        _ray_src2 = _arms[1]->shape().centre;
        play_sound(Lib::SOUND_ENEMY_SPAWN);
      }
    }
    if (_timer <= 0) {
      _laser = true;
      _timer = 0;
      _pos = z::rand_int(7);
    }
  }

  ++_shot_timer;
  if (_arms.empty() && !is_hp_low()) {
    if (_shot_timer % 8 == 0) {
      _shot_queue.push_back(std::pair<int32_t, int32_t>((_shot_timer / 8) % 12, 16));
      play_sound_random(Lib::SOUND_BOSS_FIRE);
    }
  }
  if (_arms.empty() && is_hp_low()) {
    if (_shot_timer % 48 == 0) {
      for (int32_t i = 1; i < 16; i += 3) {
        _shot_queue.push_back(std::pair<int32_t, int32_t>(i, 16));
      }
      play_sound_random(Lib::SOUND_BOSS_FIRE);
    }
    if (_shot_timer % 48 == 16) {
      for (int32_t i = 2; i < 12; i += 3) {
        _shot_queue.push_back(std::pair<int32_t, int32_t>(i, 16));
      }
      play_sound_random(Lib::SOUND_BOSS_FIRE);
    }
    if (_shot_timer % 48 == 32) {
      for (int32_t i = 3; i < 12; i += 3) {
        _shot_queue.push_back(std::pair<int32_t, int32_t>(i, 16));
      }
      play_sound_random(Lib::SOUND_BOSS_FIRE);
    }
    if (_shot_timer % 128 == 0) {
      _ray_attack_timer = RAY_TIMER;
      vec2 d1 = vec2::from_polar(z::rand_fixed() * 2 * fixed::pi, 110);
      vec2 d2 = vec2::from_polar(z::rand_fixed() * 2 * fixed::pi, 110);
      _ray_src1 = shape().centre + d1;
      _ray_src2 = shape().centre + d2;
      play_sound(Lib::SOUND_ENEMY_SPAWN);
    }
  }
  if (_arms.empty()) {
    _arm_timer++;
    if (_arm_timer >= ARM_RTIMER) {
      _arm_timer = 0;
      if (!is_hp_low()) {
        int32_t players = z0().get_lives() ?
            z0().count_players() : z0().alive_players();
        int32_t hp =
            (ARM_HP * (7 * fixed::tenth + 3 * fixed::tenth * players)).to_int();
        _arms.push_back(new DeathArm(this, true, hp));
        _arms.push_back(new DeathArm(this, false, hp));
        play_sound(Lib::SOUND_ENEMY_SPAWN);
        spawn(_arms[0]);
        spawn(_arms[1]);
      }
    }
  }

  for (std::size_t i = 0; i < _shot_queue.size(); ++i) {
    if (!goingFast || _shot_timer % 2) {
      int32_t n = _shot_queue[i].first;
      vec2 d = vec2(1, 0).rotated(shape().rotation() + n * fixed::pi / 6);
      Ship* s = new SBBossShot(shape().centre + d * 120, d * 5, 0x33ff99ff);
      spawn(s);
    }
    _shot_queue[i].second--;
    if (!_shot_queue[i].second) {
      _shot_queue.erase(_shot_queue.begin() + i);
      --i;
    }
  }
}

void DeathRayBoss::render() const
{
  Boss::render();
  for (int32_t i = _ray_attack_timer - 8; i <= _ray_attack_timer + 8; i++) {
    if (i < 40 || i > RAY_TIMER) {
      continue;
    }

    flvec2 pos = to_float(shape().centre);
    flvec2 d = to_float(_ray_src1) - pos;
    d *= float(i - 40) / float(RAY_TIMER - 40);
    Polygon s(vec2(), 10, 6, 0x999999ff, 0, 0, Polygon::T::POLYSTAR);
    s.render(lib(), d + pos, 0);

    d = to_float(_ray_src2) - pos;
    d *= float(i - 40) / float(RAY_TIMER - 40);
    Polygon s2(vec2(), 10, 6, 0x999999ff, 0, 0, Polygon::T::POLYSTAR);
    s2.render(lib(), d + pos, 0);
  }
}

int32_t DeathRayBoss::get_damage(int32_t damage, bool magic)
{
  return _arms.size() ? 0 : damage;
}

void DeathRayBoss::on_arm_death(Ship* arm)
{
  for (auto it = _arms.begin(); it != _arms.end(); ++it) {
    if (*it == arm) {
      _arms.erase(it);
      break;
    }
  }
}

// Super boss
//------------------------------
SuperBossArc::SuperBossArc(
    const vec2& position,
    int32_t players, int32_t cycle, int32_t i, Ship* boss, int32_t timer)
  : Boss(position, z0Game::boss_list(0), SuperBoss::ARC_HP, players, cycle)
  , _boss(boss)
  , _i(i)
  , _timer(timer)
  , _stimer(0)
{
  add_shape(new PolyArc(vec2(), 140, 32, 2, 0, i * 2 * fixed::pi / 16, 0));
  add_shape(new PolyArc(vec2(), 135, 32, 2, 0, i * 2 * fixed::pi / 16, 0));
  add_shape(new PolyArc(vec2(), 130, 32, 2, 0, i * 2 * fixed::pi / 16, 0));
  add_shape(new PolyArc(vec2(), 125, 32, 2, 0, i * 2 * fixed::pi / 16, SHIELD));
  add_shape(new PolyArc(vec2(), 120, 32, 2, 0, i * 2 * fixed::pi / 16, 0));
  add_shape(new PolyArc(vec2(), 115, 32, 2, 0, i * 2 * fixed::pi / 16, 0));
  add_shape(new PolyArc(vec2(), 110, 32, 2, 0, i * 2 * fixed::pi / 16, 0));
  add_shape(new PolyArc(vec2(), 105, 32, 2, 0, i * 2 * fixed::pi / 16, SHIELD));
}

void SuperBossArc::update()
{
  shape().rotate(fixed(6) / 1000);
  for (int32_t i = 0; i < 8; ++i) {
    shapes()[7 - i]->colour = z::colour_cycle(
        is_hp_low() ? 0xff000099 : 0xff0000ff, (i * 32 + 2 * _timer) % 256);
  }
  ++_timer;
  ++_stimer;
  if (_stimer == 64) {
    shapes()[0]->category = DANGEROUS | VULNERABLE;
  }
}

void SuperBossArc::render() const
{
  if (_stimer >= 64 || _stimer % 4 < 2) {
    Boss::render(false);
  }
}

int32_t SuperBossArc::get_damage(int32_t damage, bool magic)
{
  if (damage >= Player::bomb_damage) {
    return damage / 2;
  }
  return damage;
}

void SuperBossArc::on_destroy()
{
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
  : Boss(vec2(Lib::WIDTH / 2, -Lib::HEIGHT / (2 + fixed::half)),
         z0Game::BOSS_3A, BASE_HP, players, cycle)
  , _players(players)
  , _cycle(cycle)
  , _ctimer(0)
  , _timer(0)
  , _state(STATE_ARRIVE)
  , _snakes(0)
{
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

void SuperBoss::update()
{
  if (_arcs.empty()) {
    for (int32_t i = 0; i < 16; ++i) {
      SuperBossArc* s =
          new SuperBossArc(shape().centre, _players, _cycle, i, this);
      spawn(s);
      _arcs.push_back(s);
    }
  }
  else {
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
    shapes()[7 - i]->colour = 
        z::colour_cycle(0xff0000ff, (i * 32 + 2 * _ctimer) % 256);
  }
  ++_ctimer;
  if (shape().centre.y < Lib::HEIGHT / 2) {
    move_vec = vec2(0, 1);
  }
  else if (_state == STATE_ARRIVE) {
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

  if (_state == STATE_IDLE && is_on_screen() &&
      _timer != 0 && _timer % 300 == 0) {
    int32_t r = z::rand_int(6);
    if (r == 0 || r == 1) {
      _snakes = 16;
    }
    else if (r == 2 || r == 3) {
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
    }
    else if (r == 5) {
      _state = STATE_ATTACK;
      _timer = 0;
      fixed f = z::rand_fixed() * (2 * fixed::pi);
      for (int32_t i = 0; i < 64; ++i) {
        vec2 d = vec2::from_polar(f + i * pi2d64, 1);
        spawn(new Snake(shape().centre + d * 16, c, d));
        play_sound_random(Lib::SOUND_BOSS_ATTACK);
      }
    }
    else {
      _state = STATE_ATTACK;
      _timer = 0;
      _snakes = 32;
    }
  }

  if (_state == STATE_IDLE && is_on_screen() &&
      _timer % 300 == 200 && !is_destroyed()) {
    std::vector<int32_t> wide3;
    int32_t timer = 0;
    for (int32_t i = 0; i < 16; ++i) {
      if (_destroyed[(i + 15) % 16] &&
          _destroyed[i] && _destroyed[(i + 1) % 16]) {
        wide3.push_back(i);
      }
      if (!_destroyed[i]) {
        timer = _arcs[i]->GetTimer();
      }
    }
    if (!wide3.empty()) {
      int32_t r = z::rand_int(int32_t(wide3.size()));
      SuperBossArc* s = new SuperBossArc(
          shape().centre, _players, _cycle, wide3[r], this, timer);
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
    vec2 d = vec2::from_polar(z::rand_fixed() * (2 * fixed::pi),
                              z::rand_int(32) + z::rand_int(16));
    spawn(new Snake(d + shape().centre, c));
    play_sound_random(Lib::SOUND_ENEMY_SPAWN);
  }
  move(move_vec);
}

int32_t SuperBoss::get_damage(int32_t damage, bool magic)
{
  return damage;
}

void SuperBoss::on_destroy()
{
  set_killed();
  for (const auto& ship : z0().all_ships(SHIP_ENEMY)) {
    if (ship != this) {
      ship->damage(Player::bomb_damage * 100, false, 0);
    }
  }
  explosion();
  explosion(0xffffffff, 12);
  explosion(shapes()[0]->colour, 24);
  explosion(0xffffffff, 36);
  explosion(shapes()[0]->colour, 48);

  int32_t n = 1;
  for (int32_t i = 0; i < 16; ++i) {
    vec2 v = vec2::from_polar(z::rand_fixed() * (2 * fixed::pi),
                              8 + z::rand_int(64) + z::rand_int(64));
    colour_t c = z::colour_cycle(shapes()[0]->colour, n * 2);
    _fireworks.push_back(
        std::make_pair(n, std::make_pair(shape().centre + v, c)));
    n += i;
  }
  for (int32_t i = 0; i < Lib::PLAYERS; i++) {
    lib().rumble(i, 25);
  }
  play_sound(Lib::SOUND_EXPLOSION);

  z0Game::ShipList players = z0().get_players();
  n = 0;
  for (std::size_t i = 0; i < players.size(); i++) {
    Player* p = (Player*) players[i];
    if (!p->is_killed()) {
      n++;
    }
  }

  for (std::size_t i = 0; i < players.size(); i++) {
    Player* p = (Player*) players[i];
    if (!p->is_killed()) {
      p->add_score(get_score() / n);
    }
  }

  for (int32_t i = 0; i < 8; ++i) {
    spawn(new Powerup(shape().centre, Powerup::BOMB));
  }
}
