#include "boss.h"
#include "powerup.h"
#include "boss_enemy.h"
#include "player.h"

#include <algorithm>

std::vector<std::pair<int, std::pair<vec2, colour>>> Boss::_fireworks;
std::vector<vec2> Boss::_warnings;
const fixed Boss::HP_PER_EXTRA_PLAYER = fixed(1) / 10;
const fixed Boss::HP_PER_EXTRA_CYCLE = 3 * fixed(1) / 10;

const int BigSquareBoss::BASE_HP = 400;
const fixed BigSquareBoss::SPEED = 2 + fixed(1) / 2;
const int BigSquareBoss::TIMER = 100;
const int BigSquareBoss::STIMER = 80;
const fixed BigSquareBoss::ATTACK_RADIUS = 120;
const int BigSquareBoss::ATTACK_TIME = 90;

const int ShieldBombBoss::BASE_HP = 320;
const int ShieldBombBoss::TIMER = 100;
const int ShieldBombBoss::UNSHIELD_TIME = 300;
const int ShieldBombBoss::ATTACK_TIME = 80;
const fixed ShieldBombBoss::SPEED = 1;

const int ChaserBoss::BASE_HP = 60;
const fixed ChaserBoss::SPEED = 4;
const int ChaserBoss::TIMER = 60;
const int ChaserBoss::MAX_SPLIT = 7;

const int TractorBoss::BASE_HP = 900;
const fixed TractorBoss::SPEED = 2;
const int TractorBoss::TIMER = 100;

const int GhostBoss::BASE_HP = 700;
const int GhostBoss::TIMER = 250;
const int GhostBoss::ATTACK_TIME = 100;

const int DeathRayBoss::BASE_HP = 600;
const int DeathRayBoss::ARM_HP = 100;
const int DeathRayBoss::ARM_ATIMER = 300;
const int DeathRayBoss::ARM_RTIMER = 400;
const fixed DeathRayBoss::ARM_SPEED = 4;
const int DeathRayBoss::RAY_TIMER = 100;
const int DeathRayBoss::TIMER = 100;
const fixed DeathRayBoss::SPEED = 5;

const int SuperBoss::BASE_HP = 520;
const int SuperBoss::ARC_HP = 75;

// Generic boss
//------------------------------
Boss::Boss(const vec2& position, z0Game::BossList boss, int hp,
           int players, int cycle, bool explodeOnDamage)
  : Ship(position, Ship::ship_category(SHIP_BOSS | SHIP_ENEMY))
  , _hp(0)
  , _maxHp(0)
  , _flag(boss)
  , _score(0)
  , _ignoreDamageColour(256)
  , _damaged(0)
  , _showHp(false)
  , _explodeOnDamage(explodeOnDamage)
{
  set_bounding_width(640);
  SetIgnoreDamageColourIndex(100);
  long s =
      5000 * (cycle + 1) +
      2500 * (boss > z0Game::BOSS_1C) +
      2500 * (boss > z0Game::BOSS_2C);

  _score += s;
  for (int i = 0; i < players - 1; ++i) {
    _score += (int(s) * HP_PER_EXTRA_PLAYER).to_int();
  }
  _hp = CalculateHP(hp, players, cycle);
  _maxHp = _hp;
}

int Boss::CalculateHP(int base, int players, int cycle)
{
  int hp = base * 30;
  int r = hp;
  for (int i = 0; i < cycle; ++i) {
    r += (hp * HP_PER_EXTRA_CYCLE).to_int();
  }
  for (int i = 0; i < players - 1; ++i) {
    r += (hp * HP_PER_EXTRA_PLAYER).to_int();
  }
  for (int i = 0; i < cycle * (players - 1); ++i) {
    r += (hp * HP_PER_EXTRA_CYCLE + HP_PER_EXTRA_PLAYER).to_int();
  }
  return r;
}

void Boss::Damage(int damage, bool magic, Player* source)
{
  int actualDamage = GetDamage(damage, magic);
  if (actualDamage <= 0) {
    return;
  }

  if (damage >= Player::BOMB_DAMAGE) {
    if (_explodeOnDamage) {
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
    OnDestroy();
  }
  else if (!is_destroyed()) {
    play_sound_random(Lib::SOUND_ENEMY_SHATTER);
  }
}

void Boss::Render() const
{
  Render(true);
}

void Boss::Render(bool hpBar) const
{
  if (hpBar) {
    RenderHPBar();
  }

  if (!_damaged) {
    Ship::Render();
    return;
  }
  for (unsigned int i = 0; i < count_shapes(); i++) {
    get_shape(i).Render(
        lib(), to_float(position()), rotation().to_float(),
        int(i) < _ignoreDamageColour ? 0xffffffff : 0);
  }
  _damaged--;
}

void Boss::RenderHPBar() const
{
  if (!_showHp && is_on_screen()) {
    _showHp = true;
  }

  if (_showHp) {
    Ship::render_hp_bar(float(_hp) / float(_maxHp));
  }
}

void Boss::OnDestroy()
{
  SetKilled();
  for (const auto& ship : z0().all_ships(SHIP_ENEMY)) {
    if (ship != this) {
      ship->Damage(Player::BOMB_DAMAGE, false, 0);
    }
  }
  explosion();
  explosion(0xffffffff, 12);
  explosion(get_shape(0).GetColour(), 24);
  explosion(0xffffffff, 36);
  explosion(get_shape(0).GetColour(), 48);
  int n = 1;
  for (int i = 0; i < 16; ++i) {
    vec2 v;
    v.set_polar(z::rand_fixed() * (2 * fixed::pi),
                8 + z::rand_int(64) + z::rand_int(64));
    _fireworks.push_back(
        std::make_pair(n, std::make_pair(position() + v,
                                         get_shape(0).GetColour())));
    n += i;
  }
  for (int i = 0; i < Lib::PLAYERS; i++) {
    lib().Rumble(i, 25);
  }
  play_sound(Lib::SOUND_EXPLOSION);

  z0Game::ShipList players = z0().get_players();

  n = 0;
  for (unsigned int i = 0; i < players.size(); i++) {
    Player* p = (Player*) players[i];
    if (!p->IsKilled()) {
      n++;
    }
  }

  for (unsigned int i = 0; i < players.size(); i++) {
    Player* p = (Player*) players[i];
    if (!p->IsKilled()) {
      if (GetScore() > 0) {
        p->AddScore(GetScore() / n);
      }
    }
  }
}


// Big square boss
//------------------------------
BigSquareBoss::BigSquareBoss(int players, int cycle)
  : Boss(vec2(Lib::WIDTH * fixed::hundredth * 75, Lib::HEIGHT * 2),
         z0Game::BOSS_1A, BASE_HP, players, cycle)
  , _dir(0, -1)
  , _reverse(false)
  , _timer(TIMER * 6)
  , _spawnTimer(0)
  , _specialTimer(0)
  , _specialAttack(false)
  , _specialAttackRotate(false)
  , _attackPlayer(0)
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

void BigSquareBoss::Update()
{
  vec2 pos = position();
  if (pos.y < Lib::HEIGHT * fixed::hundredth * 25 && _dir.y == -1) {
    _dir.set(_reverse ? 1 : -1, 0);
  }
  if (pos.x < Lib::WIDTH * fixed::hundredth * 25 && _dir.x == -1) {
    _dir.set(0, _reverse ? -1 : 1);
  }
  if (pos.y > Lib::HEIGHT * fixed::hundredth * 75 && _dir.y == 1) {
    _dir.set(_reverse ? -1 : 1, 0);
  }
  if (pos.x > Lib::WIDTH * fixed::hundredth * 75 && _dir.x == 1) {
    _dir.set(0, _reverse ? 1 : -1);
  }

  if (_specialAttack) {
    _specialTimer--;
    if (_attackPlayer->IsKilled()) {
      _specialTimer = 0;
      _attackPlayer = 0;
    }
    else if (!_specialTimer) {
      vec2 d(ATTACK_RADIUS, 0);
      if (_specialAttackRotate) {
        d.rotate(fixed::pi / 2);
      }
      for (int i = 0; i < 6; i++) {
        Enemy* s = new Follow(_attackPlayer->position() + d);
        s->set_rotation(fixed::pi / 4);
        s->SetScore(0);
        spawn(s);
        d.rotate(2 * fixed::pi / 6);
      }
      _attackPlayer = 0;
      play_sound(Lib::SOUND_ENEMY_SPAWN);
    }
    if (!_attackPlayer) {
      _specialAttack = false;
    }
  }
  else if (is_on_screen()) {
    _timer--;
    if (_timer <= 0) {
      _timer = (z::rand_int(6) + 1) * TIMER;
      _dir = vec2() - _dir;
      _reverse = !_reverse;
    }
    _spawnTimer++;
    if (_spawnTimer >=
        int(STIMER - z0().alive_players() * 10) / (IsHPLow() ? 2 : 1)) {
      _spawnTimer = 0;
      _specialTimer++;
      spawn(new BigFollow(position(), false));
      play_sound_random(Lib::SOUND_BOSS_FIRE);
    }
    if (_specialTimer >= 8 && z::rand_int(4)) {
      int r = z::rand_int(2);
      _specialTimer = ATTACK_TIME;
      _specialAttack = true;
      _specialAttackRotate = r != 0;
      _attackPlayer = nearest_player();
      play_sound(Lib::SOUND_BOSS_ATTACK);
    }
  }

  if (_specialAttack) {
    return;
  }

  move(_dir * SPEED);
  for (std::size_t i = 0; i < 6; ++i) {
    get_shape(i).rotate((i % 2 ? -1 : 1) * fixed::hundredth * ((1 + i) * 5));
  }
  for (std::size_t i = 0; i < 6; ++i) {
    get_shape(i + 6).set_rotation(get_shape(i).rotation());
  }
}

void BigSquareBoss::Render() const
{
  Boss::Render();
  if ((_specialTimer / 4) % 2 && _attackPlayer) {
    flvec2 d(ATTACK_RADIUS.to_float(), 0);
    if (_specialAttackRotate) {
      d.rotate(M_PIf / 2);
    }
    for (int i = 0; i < 6; i++) {
      flvec2 p = to_float(_attackPlayer->position()) + d;
      Polygon s(vec2(), 10, 4, 0x9933ffff, fixed::pi / 4, 0);
      s.Render(lib(), p, 0);
      d.rotate(2 * M_PIf / 6);
    }
  }
}

int BigSquareBoss::GetDamage(int damage, bool magic)
{
  return damage;
}

// Shield bomb boss
//------------------------------
ShieldBombBoss::ShieldBombBoss(int players, int cycle)
  : Boss(vec2(-Lib::WIDTH / 2, Lib::HEIGHT / 2),
         z0Game::BOSS_1B, BASE_HP, players, cycle)
  , _timer(0)
  , _count(0)
  , _unshielded(0)
  , _attack(0)
  , _side(false)
  , _shotAlternate(false)
{
  add_shape(new Polygram(vec2(), 48, 8, 0x339966ff, 0, DANGEROUS | VULNERABLE));

  for (int i = 0; i < 16; i++) {
    vec2 a(120, 0);
    vec2 b(80, 0);

    a.rotate(i * fixed::pi / 8);
    b.rotate(i * fixed::pi / 8);

    add_shape(new Line(vec2(), a, b, 0x999999ff, 0));
  }

  add_shape(new Polygon(vec2(), 130, 16, 0xccccccff, 0, VULNSHIELD | DANGEROUS));
  add_shape(new Polygon(vec2(), 125, 16, 0xccccccff, 0, 0));
  add_shape(new Polygon(vec2(), 120, 16, 0xccccccff, 0, 0));

  add_shape(new Polygon(vec2(), 42, 16, 0, 0, SHIELD));

  SetIgnoreDamageColourIndex(1);
}

void ShieldBombBoss::Update()
{
  if (!_side && position().x < Lib::WIDTH * fixed::tenth * 6) {
    move(vec2(1, 0) * SPEED);
  }
  else if (_side && position().x > Lib::WIDTH * fixed::tenth * 4) {
    move(vec2(-1, 0) * SPEED);
  }

  rotate(-fixed::hundredth * 2);
  get_shape(0).rotate(-fixed::hundredth * 4);
  get_shape(20).set_rotation(get_shape(0).rotation());

  if (!is_on_screen()) {
    return;
  }

  if (IsHPLow()) {
    _timer++;
  }

  if (_unshielded) {
    _timer++;

    _unshielded--;
    for (int i = 0; i < 3; i++) {
      get_shape(i + 17).SetColour(
          (_unshielded / 2) % 4 ? 0x00000000 : 0x666666ff);
    }
    for (int i = 0; i < 16; i++) {
      get_shape(i + 1).SetColour(
          (_unshielded / 2) % 4 ? 0x00000000 : 0x333333ff);
    }

    if (!_unshielded) {
      get_shape(0).SetCategory(DANGEROUS | VULNERABLE);
      get_shape(17).SetCategory(VULNSHIELD | DANGEROUS);

      for (int i = 0; i < 3; i++) {
        get_shape(i + 17).SetColour(0xccccccff);
      }
      for (int i = 0; i < 16; i++) {
        get_shape(i + 1).SetColour(0x999999ff);
      }
    }
  }

  if (_attack) {
    vec2 d(_attackDir);
    d.rotate((ATTACK_TIME - _attack) * fixed::half * fixed::pi / ATTACK_TIME);
    spawn(new SBBossShot(position(), d));
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
          spawn(new Powerup(position(), Powerup::BOMB));
        }
      }
    }

    if (!z::rand_int(3)) {
      _side = !_side;
    }

    if (z::rand_int(2)) {
      vec2 d(5, 0);
      d.rotate(rotation());
      for (int i = 0; i < 12; i++) {
        spawn(new SBBossShot(position(), d));
        d.rotate(2 * fixed::pi / 12);
      }
      play_sound(Lib::SOUND_BOSS_ATTACK);
    }
    else {
      _attack = ATTACK_TIME;
      _attackDir.set_polar(z::rand_fixed() * (2 * fixed::pi), 5);
    }
  }
}

int ShieldBombBoss::GetDamage(int damage, bool magic)
{
  if (_unshielded) {
    return damage;
  }
  if (damage >= Player::BOMB_DAMAGE && !_unshielded) {
    _unshielded = UNSHIELD_TIME;
    get_shape(0).SetCategory(VULNERABLE | DANGEROUS);
    get_shape(17).SetCategory(0);
  }
  if (!magic) {
    return 0;
  }
  _shotAlternate = !_shotAlternate;
  if (_shotAlternate) {
    RestoreHP(60 /
        (1 + (z0().get_lives() ? z0().count_players() : z0().alive_players())));
  }
  return damage;
}

// Chaser boss
//------------------------------
int  ChaserBoss::_count;
bool ChaserBoss::_hasCounted;
int  ChaserBoss::_sharedHp;

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
static const int ONE_PT_ONE_FIVE_intLookup[128] = {
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

ChaserBoss::ChaserBoss(int players, int cycle, int split,
                       const vec2& position, int time, int stagger)
  : Boss(!split ? vec2(Lib::WIDTH / 2, -Lib::HEIGHT / 2) : position,
         z0Game::BOSS_1C,
         1 + BASE_HP / (fixed::half + HP_REDUCE_POWER_lookup[split]).to_int(),
         players, cycle, split <= 1)
  , _onScreen(split != 0)
  , _move(false)
  , _timer(time)
  , _dir()
  , _lastDir()
  , _players(players)
  , _cycle(cycle)
  , _split(split)
  , _stagger(stagger)
{
  add_shape(new Polygram(vec2(), 10 * ONE_AND_HALF_lookup[MAX_SPLIT - _split],
                         5, 0x3399ffff, 0, 0));
  add_shape(new Polygram(vec2(), 9 * ONE_AND_HALF_lookup[MAX_SPLIT - _split],
                         5, 0x3399ffff, 0, 0));
  add_shape(new Polygram(vec2(), 8 * ONE_AND_HALF_lookup[MAX_SPLIT - _split],
                         5, 0, 0, DANGEROUS | VULNERABLE));
  add_shape(new Polygram(vec2(), 7 * ONE_AND_HALF_lookup[MAX_SPLIT - _split],
                         5, 0, 0, SHIELD));

  SetIgnoreDamageColourIndex(2);
  set_bounding_width(10 * ONE_AND_HALF_lookup[MAX_SPLIT - _split]);
  if (!_split) {
    _sharedHp = 0;
    _count = 0;
  }
}

void ChaserBoss::Update()
{
  z0Game::ShipList remaining = z0().all_ships();
  _lastDir = _dir;
  _lastDir.normalise();
  if (is_on_screen()) {
    _onScreen = true;
  }

  if (_timer) {
    _timer--;
  }
  if (_timer <= 0) {
    _timer = TIMER * (_move + 1);
    if (_split != 0 &&
        (_move || z::rand_int(8 + _split) == 0 ||
         int(remaining.size()) - z0().count_players() <= 4 ||
         !z::rand_int(ONE_PT_ONE_FIVE_intLookup[
             std::max(0, std::min(127, int32_t(remaining.size()) -
                                       z0().count_players()))]))) {
      _move = !_move;
    }
    if (_move) {
      Ship* p = nearest_player();
      _dir = p->position() - position();
      _dir.normalise();
      _dir *= SPEED * ONE_PT_ONE_lookup[_split];
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

    _dir = _lastDir;
    if (_stagger == _count % (_split == 0 ? 1 :
                              _split == 1 ? 2 :
                              _split == 2 ? 4 :
                              _split == 3 ? 8 : 16)) {
      _dir.x = _dir.y = 0;
      for (const auto& ship : nearby) {
        if (ship == this) {
          continue;
        }

        vec2 v = position() - ship->position();
        fixed len = v.length();
        if (len > 0) {
          v /= len;
        }
        vec2 p;
        if (ship->type() & SHIP_BOSS) {
          ChaserBoss* c = (ChaserBoss*) ship;
          fixed pow = ONE_PT_ONE_FIVE_lookup[MAX_SPLIT - c->_split];
          v *= pow;
          p = c->_lastDir * pow;
        }
        else {
          p.set_polar(ship->rotation(), 1);
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
    if (int(remaining.size()) - z0().count_players() < 4 && _split >= MAX_SPLIT - 1) {
      if ((position().x < 32 && _dir.x < 0) ||
          (position().x >= Lib::WIDTH - 32 && _dir.x > 0)) {
        _dir.x = -_dir.x;
      }
      if ((position().y < 32 && _dir.y < 0) ||
          (position().y >= Lib::HEIGHT - 32 && _dir.y > 0)) {
        _dir.y = -_dir.y;
      }
    }
    else if (int(remaining.size()) - z0().count_players() < 8 &&
             _split >= MAX_SPLIT - 1) {
      if ((position().x < 0 && _dir.x < 0) ||
          (position().x >= Lib::WIDTH && _dir.x > 0)) {
        _dir.x = -_dir.x;
      }
      if ((position().y < 0 && _dir.y < 0) ||
          (position().y >= Lib::HEIGHT && _dir.y > 0)) {
        _dir.y = -_dir.y;
      }
    }
    else {
      if ((position().x < -32 && _dir.x < 0) ||
          (position().x >= Lib::WIDTH + 32 && _dir.x > 0)) {
        _dir.x = -_dir.x;
      }
      if ((position().y < -32 && _dir.y < 0) ||
          (position().y >= Lib::HEIGHT + 32 && _dir.y > 0)) {
        _dir.y = -_dir.y;
      }
    }

    if (position().x < -256) {
      _dir = vec2(1, 0);
    }
    else if (position().x >= Lib::WIDTH + 256) {
      _dir = vec2(-1, 0);
    }
    else if (position().y < -256) {
      _dir = vec2(0, 1);
    }
    else if (position().y >= Lib::HEIGHT + 256) {
      _dir = vec2(0, -1);
    }
    else {
      _dir.normalise();
    }

    _dir =
        _split == 0 ? _dir + _lastDir * 7 :
        _split == 1 ? _dir * 2 + _lastDir * 7 :
        _split == 2 ? _dir * 4 + _lastDir * 7 :
        _split == 3 ? _dir + _lastDir : _dir * 2 + _lastDir;

    _dir *= ONE_PT_ONE_lookup[_split] *
        (_split == 0 ? c_dir0 :
         _split == 1 ? c_dir1 :
         _split == 2 ? c_dir2 :
         _split == 3 ? c_dir3 : c_dir4);
    move(_dir);
    rotate(fixed::hundredth * 2 + fixed(_split) * c_rotate);
  }
  _sharedHp = 0;
  if (!_hasCounted) {
    _hasCounted = true;
    ++_count;
  }
}

void ChaserBoss::Render() const
{
  Boss::Render();
  static std::vector< int > hpLookup;
  if (hpLookup.empty()) {
    int hp = 0;
    for (int i = 0; i < 8; i++) {
      hp = 2 * hp + CalculateHP(
          1 + BASE_HP / (fixed::half + HP_REDUCE_POWER_lookup[7 - i]).to_int(),
          _players, _cycle);
      hpLookup.push_back(hp);
    }
  }
  _sharedHp += (_split == 7 ? 0 : 2 * hpLookup[6 - _split]) +
      GetRemainingHP() * 30;
  if (_onScreen) {
    Ship::render_hp_bar(float(_sharedHp) / float(hpLookup[MAX_SPLIT]));
  }
}

int ChaserBoss::GetDamage(int damage, bool magic)
{
  return damage;
}

void ChaserBoss::OnDestroy()
{
  bool last = false;
  if (_split < MAX_SPLIT) {
    for (int i = 0; i < 2; i++) {
      vec2 d;
      d.set_polar(i * fixed::pi + rotation(),
                  8 * ONE_PT_TWO_lookup[MAX_SPLIT - 1 - _split]);
      Ship* s = new ChaserBoss(
          _players, _cycle, _split + 1, position() + d,
          (i + 1) * TIMER / 2,
          z::rand_int(_split + 1 == 1 ? 2 :
                      _split + 1 == 2 ? 4 :
                      _split + 1 == 3 ? 8 : 16));
      spawn(s);
      s->set_rotation(rotation());
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
      SetKilled();
      for (int i = 0; i < Lib::PLAYERS; i++) {
        lib().Rumble(i, 25);
      }
      z0Game::ShipList players = z0().get_players();
      int n = 0;
      for (std::size_t i = 0; i < players.size(); i++) {
        Player* p = (Player*) players[i];
        if (!p->IsKilled()) {
          n++;
        }
      }
      for (std::size_t i = 0; i < players.size(); i++) {
        Player* p = (Player*) players[i];
        if (!p->IsKilled()) {
          p->AddScore(GetScore() / n);
        }
      }
      n = 1;
      for (int i = 0; i < 16; ++i) {
        vec2 v;
        v.set_polar(z::rand_fixed() * (2 * fixed::pi),
                    8 + z::rand_int(64) + z::rand_int(64));

        _fireworks.push_back(
            std::make_pair(n, std::make_pair(position() + v,
                                             get_shape(0).GetColour())));
        n += i;
      }
    }
  }

  for (int i = 0; i < Lib::PLAYERS; i++) {
    lib().Rumble(i, _split < 3 ? 10 : 3);
  }

  explosion();
  explosion(0xffffffff, 12);
  if (_split < 3 || last) {
    explosion(get_shape(0).GetColour(), 24);
  }
  if (_split < 2 || last) {
    explosion(0xffffffff, 36);
  }
  if (_split < 1 || last) {
    explosion(get_shape(0).GetColour(), 48);
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
TractorBoss::TractorBoss(int players, int cycle)
  : Boss(vec2(Lib::WIDTH * (1 + fixed::half), Lib::HEIGHT / 2),
         z0Game::BOSS_2A, BASE_HP, players, cycle)
  , _willAttack(false)
  , _stopped(false)
  , _generating(false)
  , _attacking(false)
  , _continue(false)
  , _genDir(0)
  , _shootType(z::rand_int(2))
  , _sound(true)
  , _timer(0)
  , _attackSize(0)
{
  _s1 = new CompoundShape(vec2(0, -96), 0, DANGEROUS | VULNERABLE, 0xcc33ccff);

  _s1->add_shape(new Polygram(vec2(), 12, 6, 0x882288ff, 0, 0));
  _s1->add_shape(new Polygon(vec2(), 12, 12, 0x882288ff, 0, 0));
  _s1->add_shape(new Polygon(vec2(), 2, 6, 0x882288ff, 0, 0));
  _s1->add_shape(new Polygon(vec2(), 36, 12, 0xcc33ccff, 0, 0));
  _s1->add_shape(new Polygon(vec2(), 34, 12, 0xcc33ccff, 0, 0));
  _s1->add_shape(new Polygon(vec2(), 32, 12, 0xcc33ccff, 0, 0));
  for (int i = 0; i < 8; i++) {
    vec2 d(24, 0);
    d.rotate(i * fixed::pi / 4);
    _s1->add_shape(new Polygram(d, 12, 6, 0xcc33ccff, 0, 0));
  }

  _s2 = new CompoundShape(vec2(0, 96), 0, DANGEROUS | VULNERABLE, 0xcc33ccff);

  _s2->add_shape(new Polygram(vec2(), 12, 6, 0x882288ff, 0, 0));
  _s2->add_shape(new Polygon(vec2(), 12, 12, 0x882288ff, 0, 0));
  _s2->add_shape(new Polygon(vec2(), 2, 6, 0x882288ff, 0, 0));

  _s2->add_shape(new Polygon(vec2(), 36, 12, 0xcc33ccff, 0, 0));
  _s2->add_shape(new Polygon(vec2(), 34, 12, 0xcc33ccff, 0, 0));
  _s2->add_shape(new Polygon(vec2(), 32, 12, 0xcc33ccff, 0, 0));
  for (int i = 0; i < 8; i++) {
    vec2 d(24, 0);
    d.rotate(i * fixed::pi / 4);
    _s2->add_shape(new Polygram(d, 12, 6, 0xcc33ccff, 0, 0));
  }

  add_shape(_s1);
  add_shape(_s2);

  _sAttack = new Polygon(vec2(), 0, 16, 0x993399ff);
  add_shape(_sAttack);

  add_shape(new Line(vec2(), vec2(-2, -96), vec2(-2, 96), 0xcc33ccff, 0));
  add_shape(new Line(vec2(), vec2(0, -96), vec2(0, 96), 0x882288ff, 0));
  add_shape(new Line(vec2(), vec2(2, -96), vec2(2, 96), 0xcc33ccff, 0));

  add_shape(new Polygon(vec2(0, 96), 30, 12, 0, 0, SHIELD));
  add_shape(new Polygon(vec2(0, -96), 30, 12, 0, 0, SHIELD));

  _attackShapes = int(count_shapes());
}

void TractorBoss::Update()
{
  if (position().x <= Lib::WIDTH / 2 &&
      _willAttack && !_stopped && !_continue) {
    _stopped = true;
    _generating = true;
    _genDir = z::rand_int(2) == 0;
    _timer = 0;
  }

  if (position().x < -150) {
    set_position(vec2(Lib::WIDTH + 150, position().y));
    _willAttack = !_willAttack;
    _shootType = z::rand_int(2);
    if (_willAttack) {
      _shootType = 0;
    }
    _continue = false;
    _sound = !_willAttack;
    rotate(z::rand_fixed() * 2 * fixed::pi);
  }

  _timer++;
  if (!_stopped) {
    move(vec2(-1, 0) * SPEED);
    if (!_willAttack && is_on_screen() &&
        _timer % (16 - z0().alive_players() * 2) == 0) {
      if (_shootType == 0 || (IsHPLow() && _shootType == 1)) {
        Player* p = nearest_player();

        vec2 v = _s1->ConvertPoint(position(), rotation(), vec2());
        vec2 d = p->position() - v;
        d.normalise();
        spawn(new SBBossShot(v, d * 5, 0xcc33ccff));
        spawn(new SBBossShot(v, d * -5, 0xcc33ccff));

        v = _s2->ConvertPoint(position(), rotation(), vec2());
        d = p->position() - v;
        d.normalise();
        spawn(new SBBossShot(v, d * 5, 0xcc33ccff));
        spawn(new SBBossShot(v, d * -5, 0xcc33ccff));

        play_sound_random(Lib::SOUND_BOSS_FIRE);
      }
      if (_shootType == 1 || IsHPLow()) {
        Player* p = nearest_player();
        vec2 v = position();

        vec2 d = p->position() - v;
        d.normalise();
        spawn(new SBBossShot(v, d * 5, 0xcc33ccff));
        spawn(new SBBossShot(v, d * -5, 0xcc33ccff));
        play_sound_random(Lib::SOUND_BOSS_FIRE);
      }
    }
    if ((!_willAttack || _continue) && is_on_screen()) {
      if (_sound) {
        play_sound(Lib::SOUND_BOSS_ATTACK);
        _sound = false;
      }
      _targets.clear();
      for (const auto& ship : z0().all_ships(SHIP_PLAYER)) {
        if (((Player*) ship)->IsKilled()) {
          continue;
        }
        vec2 pos = ship->position();
        _targets.push_back(pos);
        vec2 d = position() - pos;
        d.normalise();
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
        _attackSize = 0;
        play_sound(Lib::SOUND_BOSS_ATTACK);
      }

      if (_timer % (10 - 2 * z0().alive_players()) == 0 && _timer < TIMER * 4) {
        Ship* s = new TBossShot(
            _s1->ConvertPoint(position(), rotation(), vec2()),
            _genDir ? rotation() + fixed::pi : rotation());
        spawn(s);

        s = new TBossShot(
            _s2->ConvertPoint(position(), rotation(), vec2()),
            _genDir ? rotation() : rotation() + fixed::pi);
        spawn(s);
        play_sound_random(Lib::SOUND_ENEMY_SPAWN);
      }

      if (IsHPLow() && _timer % (20 - z0().alive_players() * 2) == 0) {
        Player* p = nearest_player();
        vec2 v = position();

        vec2 d = p->position() - v;
        d.normalise();
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
          vec2 v = _s1->ConvertPoint(position(), rotation(), vec2());
          vec2 d;
          d.set_polar(z::rand_fixed() * (2 * fixed::pi), 5);
          spawn(new SBBossShot(v, d, 0xcc33ccff));
          d.rotate(fixed::pi / 2);
          spawn(new SBBossShot(v, d, 0xcc33ccff));
          d.rotate(fixed::pi / 2);
          spawn(new SBBossShot(v, d, 0xcc33ccff));
          d.rotate(fixed::pi / 2);
          spawn(new SBBossShot(v, d, 0xcc33ccff));

          v = _s2->ConvertPoint(position(), rotation(), vec2());
          d.set_polar(z::rand_fixed() * (2 * fixed::pi), 5);
          spawn(new SBBossShot(v, d, 0xcc33ccff));
          d.rotate(fixed::pi / 2);
          spawn(new SBBossShot(v, d, 0xcc33ccff));
          d.rotate(fixed::pi / 2);
          spawn(new SBBossShot(v, d, 0xcc33ccff));
          d.rotate(fixed::pi / 2);
          spawn(new SBBossShot(v, d, 0xcc33ccff));
          play_sound_random(Lib::SOUND_BOSS_FIRE);
        }
        _targets.clear();
        for (const auto& ship : z0().all_ships(SHIP_PLAYER | SHIP_ENEMY)) {
          if (ship == this || ((ship->type() & SHIP_PLAYER) &&
                               ((Player*) ship)->IsKilled())) {
            continue;
          }

          if (ship->type() & SHIP_ENEMY) {
            play_sound_random(Lib::SOUND_BOSS_ATTACK, 0, 0.3f);
          }
          vec2 pos = ship->position();
          _targets.push_back(pos);
          fixed speed = 0;
          if (ship->type() & SHIP_PLAYER) {
            speed = Tractor::TRACTOR_SPEED;
          }
          if (ship->type() & SHIP_ENEMY) {
            speed = 4 + fixed::half;
          }
          vec2 d = position() - pos;
          d.normalise();
          ship->move(d * speed);

          if ((ship->type() & SHIP_ENEMY) &&
              !(ship->type() & SHIP_WALL) &&
              (ship->position() - position()).length() <= 40) {
            ship->destroy();
            _attackSize++;
            _sAttack->SetRadius(_attackSize / (1 + fixed::half));
            add_shape(new Polygon(vec2(), 8, 6, 0xcc33ccff, 0, 0));
          }
        }
      }
      else {
        _timer = 0;
        _stopped = false;
        _continue = true;
        for (int i = 0; i < _attackSize; i++) {
          vec2 d;
          d.set_polar(i * (2 * fixed::pi) / _attackSize, 5);
          spawn(new SBBossShot(position(), d, 0xcc33ccff));
        }
        play_sound(Lib::SOUND_BOSS_FIRE);
        play_sound_random(Lib::SOUND_EXPLOSION);
        _attackSize = 0;
        _sAttack->SetRadius(0);
        while (_attackShapes < int(count_shapes())) {
          destroy_shape(_attackShapes);
        }
      }
    }
  }

  for (unsigned int i = _attackShapes; i < count_shapes(); ++i) {
    vec2 v;
    v.set_polar(z::rand_fixed() * (2 * fixed::pi),
                2 * (z::rand_fixed() - fixed::half) *
                    fixed(_attackSize) / (1 + fixed::half));
    get_shape(i).SetCentre(v);
  }

  fixed r = 0;
  if (!_willAttack || (_stopped && !_generating && !_attacking)) {
    r = fixed::hundredth;
  }
  else if (_stopped && _attacking && !_generating) {
    r = 0;
  }
  else {
    r = fixed::hundredth / 2;
  }
  rotate(r);

  _s1->rotate(fixed::tenth / 2);
  _s2->rotate(-fixed::tenth / 2);
  for (int i = 0; i < 8; i++) {
    _s1->get_shape(i + 4).rotate(-fixed::tenth);
    _s2->get_shape(i + 4).rotate(fixed::tenth);
  }
}

void TractorBoss::Render() const
{
  Boss::Render();
  if ((_stopped && !_generating && !_attacking) ||
      (!_stopped && (_continue || !_willAttack) && is_on_screen())) {
    for (std::size_t i = 0; i < _targets.size(); ++i) {
      if (((_timer + i * 4) / 4) % 2) {
        lib().RenderLine(to_float(position()),
                         to_float(_targets[i]), 0xcc33ccff);
      }
    }
  }
}

int TractorBoss::GetDamage(int damage, bool magic)
{
  return damage;
}

// Ghost boss
//------------------------------
GhostBoss::GhostBoss(int players, int cycle)
  : Boss(vec2(Lib::WIDTH / 2, Lib::HEIGHT / 2),
         z0Game::BOSS_2B, BASE_HP, players, cycle)
  , _visible(false)
  , _vTime(0)
  , _timer(0)
  , _attackTime(ATTACK_TIME)
  , _attack(0)
  , _rDir(false)
  , _startTime(120)
  , _dangerCircle(0)
  , _dangerOffset1(0)
  , _dangerOffset2(0)
  , _dangerOffset3(0)
  , _dangerOffset4(0)
  , _shotType(false)
{
  add_shape(new Polygon(vec2(), 32, 8, 0x9933ccff, 0, 0));
  add_shape(new Polygon(vec2(), 48, 8, 0xcc66ffff, 0, 0));

  for (int i = 0; i < 8; i++) {
    vec2 c;
    c.set_polar(i * fixed::pi / 4, 48);

    CompoundShape* s = new CompoundShape(c, 0, 0);
    for (int j = 0; j < 8; j++) {
      vec2 d;
      d.set_polar(j * fixed::pi / 4, 1);
      s->add_shape(new Line(vec2(), d * 10, d * 10 * 2, 0x9933ccff, 0));
    }
    add_shape(s);
  }

  add_shape(new Polygram(vec2(), 24, 8, 0xcc66ffff, 0, 0));

  for (int n = 0; n < 5; n++) {
    CompoundShape* s = new CompoundShape(vec2(), 0, 0);
    for (int i = 0; i < 16 + n * 6; i++) {
      vec2 d;
      d.set_polar(i * 2 * fixed::pi / (16 + n * 6), fixed(100 + n * 60));
      s->add_shape(new Polystar(d, 16, 8, n ? 0x330066ff : 0x9933ccff));
      s->add_shape(new Polygon(d, 12, 8, n ? 0x330066ff : 0x9933ccff));
    }
    add_shape(s);
  }

  for (int n = 0; n < 5; n++) {
    CompoundShape* s = new CompoundShape(vec2(), 0, DANGEROUS);
    add_shape(s);
  }

  SetIgnoreDamageColourIndex(12);
}

void GhostBoss::Update()
{
  for (int n = 1; n < 5; ++n) {
    CompoundShape* s = (CompoundShape*) &get_shape(11 + n);
    CompoundShape* c = (CompoundShape*) &get_shape(16 + n);
    c->ClearShapes();

    for (int i = 0; i < 16 + n * 6; ++i) {
      Shape& s1 = s->get_shape(i * 2);
      Shape& s2 = s->get_shape(1 + i * 2);
      int t =
          n == 1 ? (i + _dangerOffset1) % 11 :
          n == 2 ? (i + _dangerOffset2) % 7 :
          n == 3 ? (i + _dangerOffset3) % 17 : (i + _dangerOffset4) % 10;

      bool b =
          n == 1 ? (t % 11 == 0 || t % 11 == 5) :
          n == 2 ? (t % 7 == 0) :
          n == 3 ? (t % 17 == 0 || t % 17 == 8) : (t % 10 == 0);

      if (((n == 4 && _attack != 2) ||
           (n + (n == 3)) & _dangerCircle) &&
          _visible && b) {
        s1.SetColour(0xcc66ffff);
        s2.SetColour(0xcc66ffff);
        if (_vTime == 0) {
          vec2 d;
          d.set_polar(i * 2 * fixed::pi / (16 + n * 6), fixed(100 + n * 60));
          c->add_shape(new Polygon(d, 9, 8, 0));
          _warnings.push_back(c->ConvertPoint(position(), rotation(), d));
        }
      }
      else {
        s1.SetColour(0x330066ff);
        s2.SetColour(0x330066ff);
      }
    }
  }
  if (!(_attack == 2 && _attackTime < ATTACK_TIME * 4 && _attackTime)) {
    rotate(-fixed::hundredth * 4);
  }

  for (int i = 0; i < 8; i++) {
    get_shape(i + 2).rotate(fixed::hundredth * 4);
  }
  get_shape(10).rotate(fixed::hundredth * 6);
  for (int n = 0; n < 5; n++) {
    CompoundShape* s = (CompoundShape*) &get_shape(11 + n);
    if (n % 2) {
      s->rotate(fixed::hundredth * 3 + (fixed(3) / 2000) * n);
    }
    else {
      s->rotate(fixed::hundredth * 5 - (fixed(3) / 2000) * n);
    }
    for (std::size_t i = 0; i < s->CountShapes(); i++) {
      s->get_shape(i).rotate(-fixed::tenth);
    }

    s = (CompoundShape*) &get_shape(16 + n);
    if (n % 2) {
      s->rotate(fixed::hundredth * 3 + (fixed(3) / 2000) * n);
    }
    else {
      s->rotate(fixed::hundredth * 4 - (fixed(3) / 2000) * n);
    }
    for (std::size_t i = 0; i < s->CountShapes(); i++) {
      s->get_shape(i).rotate(-fixed::tenth);
    }
  }

  if (_startTime) {
    _startTime--;
    return;
  }

  if (!_attackTime) {
    _timer++;
    if (_timer == 9 * TIMER / 10 ||
        (_timer >= TIMER / 10 && _timer < 9 * TIMER / 10 - 16 &&
         ((_timer % 16 == 0 && _attack == 2) ||
          (_timer % 32 == 0 && _shotType)))) {
      for (int i = 0; i < 8; i++) {
        vec2 d;
        d.set_polar(i * fixed::pi / 4 + rotation(), 5);
        spawn(new SBBossShot(position(), d, 0xcc66ffff));
      }
      play_sound_random(Lib::SOUND_BOSS_FIRE);
    }
    if (_timer != 9 * TIMER / 10 && _timer >= TIMER / 10 &&
        _timer < 9 * TIMER / 10 - 16 && _timer % 16 == 0 &&
        (!_shotType || _attack == 2)) {
      Player* p = nearest_player();
      vec2 d = p->position() - position();
      d.normalise();

      if (d.length() > fixed::half) {
        spawn(new SBBossShot(position(), d * 5, 0xcc66ffff));
        spawn(new SBBossShot(position(), d * -5, 0xcc66ffff));
        play_sound_random(Lib::SOUND_BOSS_FIRE);
      }
    }
  }
  else {
    _attackTime--;

    if (_attack == 2) {
      if (_attackTime >= ATTACK_TIME * 4 && _attackTime % 8 == 0) {
        vec2 pos(z::rand_int(Lib::WIDTH + 1), z::rand_int(Lib::HEIGHT + 1));
        spawn(new GhostMine(pos, this));
        play_sound_random(Lib::SOUND_ENEMY_SPAWN);
        _dangerCircle = 0;
      }
      else if (_attackTime == ATTACK_TIME * 4 - 1) {
        _visible = true;
        _vTime = 60;
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
      else if (_attackTime < ATTACK_TIME * 3) {
        if (_attackTime == ATTACK_TIME * 3 - 1) {
          play_sound(Lib::SOUND_BOSS_ATTACK);
        }
        rotate((_rDir ? 1 : -1) * 2 * fixed::pi / (ATTACK_TIME * 6));
        if (!_attackTime) {
          for (int i = 0; i < 6; i++) {
            destroy_shape(21);
          }
        }
        _dangerOffset1 = z::rand_int(11);
        _dangerOffset2 = z::rand_int(7);
        _dangerOffset3 = z::rand_int(17);
        _dangerOffset3 = z::rand_int(10);
      }
    }

    if (_attack == 0 && _attackTime == ATTACK_TIME * 2) {
      bool r = z::rand_int(2) != 0;
      spawn(new GhostWall(r, true));
      spawn(new GhostWall(!r, false));
    }
    if (_attack == 1 && _attackTime == ATTACK_TIME * 2) {
      _rDir = z::rand_int(2) != 0;
      spawn(new GhostWall(!_rDir, false, 0));
    }
    if (_attack == 1 && _attackTime == ATTACK_TIME * 1) {
      spawn(new GhostWall(_rDir, true, 0));
    }

    if ((_attack == 0 || _attack == 1) && IsHPLow() &&
        _attackTime == ATTACK_TIME * 1) {
      vec2 v;
      int r = z::rand_int(4);
      if (r == 0) {
        v.set(-Lib::WIDTH / 4, Lib::HEIGHT / 2);
      }
      else if (r == 1) {
        v.set(Lib::WIDTH + Lib::WIDTH / 4, Lib::HEIGHT / 2);
      }
      else if (r == 2) {
        v.set(Lib::WIDTH / 2, -Lib::HEIGHT / 4);
      }
      else {
        v.set(Lib::WIDTH / 2, Lib::HEIGHT + Lib::HEIGHT / 4);
      }
      spawn(new BigFollow(v, false));
    }
    if (!_attackTime && !_visible) {
      _visible = true;
      _vTime = 60;
    }
    if (_attackTime == 0 && _attack != 2) {
      int r = z::rand_int(3);
      _dangerCircle |= 1 + (r == 2 ? 3 : r);
      if (z::rand_int(2) || IsHPLow()) {
        r = z::rand_int(3);
        _dangerCircle |= 1 + (r == 2 ? 3 : r);
      }
      if (z::rand_int(2) || IsHPLow()) {
        r = z::rand_int(3);
        _dangerCircle |= 1 + (r == 2 ? 3 : r);
      }
    }
    else {
      _dangerCircle = 0;
    }
  }

  if (_timer >= TIMER) {
    _timer = 0;
    _visible = false;
    _vTime = 60;
    _attack = z::rand_int(3);
    _shotType = z::rand_int(2) == 0;
    get_shape(0).SetCategory(0);
    get_shape(1).SetCategory(0);
    get_shape(11).SetCategory(0);
    if (count_shapes() >= 22) {
      get_shape(21).SetCategory(0);
      get_shape(24).SetCategory(0);
    }
    play_sound(Lib::SOUND_BOSS_ATTACK);

    if (_attack == 0) {
      _attackTime = ATTACK_TIME * 2 + 50;
    }
    if (_attack == 1) {
      _attackTime = ATTACK_TIME * 3;
      for (int i = 0; i < z0().count_players(); i++) {
        spawn(new Powerup(position(), Powerup::SHIELD));
      }
    }
    if (_attack == 2) {
      _attackTime = ATTACK_TIME * 6;
      _rDir = z::rand_int(2) != 0;
    }
  }

  if (_vTime) {
    _vTime--;
    if (!_vTime && _visible) {
      get_shape(0).SetCategory(SHIELD);
      get_shape(1).SetCategory(DANGEROUS | VULNERABLE);
      get_shape(11).SetCategory(DANGEROUS);
      if (count_shapes() >= 22) {
        get_shape(21).SetCategory(DANGEROUS);
        get_shape(24).SetCategory(DANGEROUS);
      }
    }
  }
}

void GhostBoss::Render() const
{
  if ((_startTime / 4) % 2 == 1) {
    render_with_colour(1);
    return;
  }
  if ((_visible && ((_vTime / 4) % 2 == 0)) ||
      (!_visible && ((_vTime / 4) % 2 == 1))) {
    Boss::Render();
    return;
  }

  render_with_colour(0x330066ff);
  if (!_startTime) {
    RenderHPBar();
  }
}

int GhostBoss::GetDamage(int damage, bool magic)
{
  return damage;
}

// Death ray boss
//------------------------------
DeathRayBoss::DeathRayBoss(int players, int cycle)
  : Boss(vec2(Lib::WIDTH * (fixed(3) / 20), -Lib::HEIGHT),
         z0Game::BOSS_2C, BASE_HP, players, cycle)
  , _timer(TIMER * 2)
  , _laser(false)
  , _dir(true)
  , _pos(1)
  , _armTimer(0)
  , _shotTimer(0)
  , _rayAttackTimer(0)
  , _raySrc1()
  , _raySrc2()
{
  add_shape(new Polystar(vec2(), 110, 12, 0x228855ff, fixed::pi / 12, 0));
  add_shape(new Polygram(vec2(), 70, 12, 0x33ff99ff, fixed::pi / 12, 0));
  add_shape(new Polygon(
      vec2(), 120, 12, 0x33ff99ff, fixed::pi / 12, DANGEROUS | VULNERABLE));
  add_shape(new Polygon(vec2(), 115, 12, 0x33ff99ff, fixed::pi / 12, 0));
  add_shape(new Polygon(
      vec2(), 110, 12, 0x33ff99ff, fixed::pi / 12, SHIELD));

  CompoundShape* s1 = new CompoundShape(vec2(), 0, DANGEROUS);
  for (int i = 1; i < 12; i++) {
    CompoundShape* s2 = new CompoundShape(vec2(), i * fixed::pi / 6, 0);
    s2->add_shape(new Box(vec2(130, 0), 10, 24, 0x33ff99ff, 0, 0));
    s2->add_shape(new Box(vec2(130, 0), 8, 22, 0x228855ff, 0, 0));
    s1->add_shape(s2);
  }
  add_shape(s1);

  SetIgnoreDamageColourIndex(5);
  rotate(2 * fixed::pi * fixed(z::rand_int()) / z::rand_max);
}

void DeathRayBoss::Update()
{
  bool positioned = true;
  fixed d =
      _pos == 0 ? 1 * Lib::HEIGHT / 4 - position().y :
      _pos == 1 ? 2 * Lib::HEIGHT / 4 - position().y :
      _pos == 2 ? 3 * Lib::HEIGHT / 4 - position().y :
      _pos == 3 ? 1 * Lib::HEIGHT / 8 - position().y :
      _pos == 4 ? 3 * Lib::HEIGHT / 8 - position().y :
      _pos == 5 ? 5 * Lib::HEIGHT / 8 - position().y :
      7 * Lib::HEIGHT / 8 - position().y;

  if (d.abs() > 3) {
    move(vec2(0, d / d.abs()) * SPEED);
    positioned = false;
  }

  if (_rayAttackTimer) {
    _rayAttackTimer--;
    if (_rayAttackTimer == 40) {
      _rayDest = nearest_player()->position();
    }
    if (_rayAttackTimer < 40) {
      vec2 d = _rayDest - position();
      d.normalise();
      spawn(new SBBossShot(position(), d * 10, 0xccccccff));
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
        spawn(new DeathRay(position() + vec2(100, 0)));
        play_sound_random(Lib::SOUND_BOSS_FIRE);
      }
      if (!_timer) {
        _laser = false;
        _timer = TIMER * (2 + z::rand_int(3));
      }
    }
    else {
      fixed r = rotation();
      if (r == 0) {
        _timer = TIMER * 2;
      }

      if (r < fixed::tenth || r > 2 * fixed::pi - fixed::tenth) {
        set_rotation(0);
      }
      else {
        goingFast = true;
        rotate(_dir ? fixed::tenth : -fixed::tenth);
      }
    }
  }
  else {
    rotate(_dir ? fixed::hundredth * 2 : -fixed::hundredth * 2);
    if (is_on_screen()) {
      _timer--;
      if (_timer % TIMER == 0 && _timer != 0 && !z::rand_int(4)) {
        _dir = z::rand_int(2) != 0;
        _pos = z::rand_int(7);
      }
      if (_timer == TIMER * 2 + 50 && _arms.size() == 2) {
        _rayAttackTimer = RAY_TIMER;
        _raySrc1 = _arms[0]->position();
        _raySrc2 = _arms[1]->position();
        play_sound(Lib::SOUND_ENEMY_SPAWN);
      }
    }
    if (_timer <= 0) {
      _laser = true;
      _timer = 0;
      _pos = z::rand_int(7);
    }
  }

  ++_shotTimer;
  if (_arms.empty() && !IsHPLow()) {
    if (_shotTimer % 8 == 0) {
      _shotQueue.push_back(std::pair<int, int>((_shotTimer / 8) % 12, 16));
      play_sound_random(Lib::SOUND_BOSS_FIRE);
    }
  }
  if (_arms.empty() && IsHPLow()) {
    if (_shotTimer % 48 == 0) {
      for (int i = 1; i < 16; i += 3) {
        _shotQueue.push_back(std::pair<int, int>(i, 16));
      }
      play_sound_random(Lib::SOUND_BOSS_FIRE);
    }
    if (_shotTimer % 48 == 16) {
      for (int i = 2; i < 12; i += 3) {
        _shotQueue.push_back(std::pair<int, int>(i, 16));
      }
      play_sound_random(Lib::SOUND_BOSS_FIRE);
    }
    if (_shotTimer % 48 == 32) {
      for (int i = 3; i < 12; i += 3) {
        _shotQueue.push_back(std::pair<int, int>(i, 16));
      }
      play_sound_random(Lib::SOUND_BOSS_FIRE);
    }
    if (_shotTimer % 128 == 0) {
      _rayAttackTimer = RAY_TIMER;
      vec2 d1, d2;
      d1.set_polar(z::rand_fixed() * 2 * fixed::pi, 110);
      d2.set_polar(z::rand_fixed() * 2 * fixed::pi, 110);
      _raySrc1 = position() + d1;
      _raySrc2 = position() + d2;
      play_sound(Lib::SOUND_ENEMY_SPAWN);
    }
  }
  if (_arms.empty()) {
    _armTimer++;
    if (_armTimer >= ARM_RTIMER) {
      _armTimer = 0;
      if (!IsHPLow()) {
        int32_t players = z0().get_lives() ?
            z0().count_players() : z0().alive_players();
        int hp =
            (ARM_HP * (7 * fixed::tenth + 3 * fixed::tenth * players)).to_int();
        _arms.push_back(new DeathArm(this, true, hp));
        _arms.push_back(new DeathArm(this, false, hp));
        play_sound(Lib::SOUND_ENEMY_SPAWN);
        spawn(_arms[0]);
        spawn(_arms[1]);
      }
    }
  }

  for (std::size_t i = 0; i < _shotQueue.size(); ++i) {
    if (!goingFast || _shotTimer % 2) {
      int n = _shotQueue[i].first;
      vec2 d(1, 0);
      d.rotate(rotation() + n * fixed::pi / 6);
      Ship* s = new SBBossShot(position() + d * 120, d * 5, 0x33ff99ff);
      spawn(s);
    }
    _shotQueue[i].second--;
    if (!_shotQueue[i].second) {
      _shotQueue.erase(_shotQueue.begin() + i);
      --i;
    }
  }
}

void DeathRayBoss::Render() const
{
  Boss::Render();
  for (int i = _rayAttackTimer - 8; i <= _rayAttackTimer + 8; i++) {
    if (i < 40 || i > RAY_TIMER) {
      continue;
    }

    flvec2 pos = to_float(position());
    flvec2 d = to_float(_raySrc1) - pos;
    d *= float(i - 40) / float(RAY_TIMER - 40);
    Polystar s(vec2(), 10, 6, 0x999999ff, 0, 0);
    s.Render(lib(), d + pos, 0);

    d = to_float(_raySrc2) - pos;
    d *= float(i - 40) / float(RAY_TIMER - 40);
    Polystar s2(vec2(), 10, 6, 0x999999ff, 0, 0);
    s2.Render(lib(), d + pos, 0);
  }
}

int DeathRayBoss::GetDamage(int damage, bool magic)
{
  return _arms.size() ? 0 : damage;
}

void DeathRayBoss::OnArmDeath(Ship* arm)
{
  for (unsigned int i = 0; i < _arms.size(); i++) {
    if (_arms[i] == arm) {
      _arms.erase(_arms.begin() + i);
      break;
    }
  }
}

// Super boss
//------------------------------
SuperBossArc::SuperBossArc(
    const vec2& position, int players, int cycle, int i, Ship* boss, int timer)
  : Boss(position, z0Game::BossList(0), SuperBoss::ARC_HP, players, cycle)
  , _boss(boss)
  , _i(i)
  , _timer(timer)
  , _sTimer(0)
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

void SuperBossArc::Update()
{
  rotate(fixed(6) / 1000);
  for (int i = 0; i < 8; ++i) {
    get_shape(7 - i).SetColour(
        z::colour_cycle(IsHPLow() ? 0xff000099 : 0xff0000ff,
                        (i * 32 + 2 * _timer) % 256));
  }
  ++_timer;
  ++_sTimer;
  if (_sTimer == 64) {
    get_shape(0).SetCategory(DANGEROUS | VULNERABLE);
  }
}

void SuperBossArc::Render() const
{
  if (_sTimer >= 64 || _sTimer % 4 < 2) {
    Boss::Render(false);
  }
}

int SuperBossArc::GetDamage(int damage, bool magic)
{
  if (damage >= Player::BOMB_DAMAGE) {
    return damage / 2;
  }
  return damage;
}

void SuperBossArc::OnDestroy()
{
  vec2 d;
  d.set_polar(_i * 2 * fixed::pi / 16 + rotation(), 120);
  move(d);
  explosion();
  explosion(0xffffffff, 12);
  explosion(get_shape(0).GetColour(), 24);
  explosion(0xffffffff, 36);
  explosion(get_shape(0).GetColour(), 48);
  play_sound_random(Lib::SOUND_EXPLOSION);
  ((SuperBoss*) _boss)->_destroyed[_i] = true;
}

SuperBoss::SuperBoss(int players, int cycle)
  : Boss(vec2(Lib::WIDTH / 2, -Lib::HEIGHT / (2 + fixed::half)),
         z0Game::BOSS_3A, BASE_HP, players, cycle)
  , _players(players)
  , _cycle(cycle)
  , _cTimer(0)
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
  for (int i = 0; i < 16; ++i) {
    _destroyed.push_back(false);
  }
  SetIgnoreDamageColourIndex(8);
}

void SuperBoss::Update()
{
  if (_arcs.empty()) {
    for (int i = 0; i < 16; ++i) {
      SuperBossArc* s =
          new SuperBossArc(position(), _players, _cycle, i, this);
      spawn(s);
      _arcs.push_back(s);
    }
  }
  else {
    for (int i = 0; i < 16; ++i) {
      if (_destroyed[i]) {
        continue;
      }
      _arcs[i]->set_position(position());
    }
  }
  vec2 move_vec;
  rotate(fixed(6) / 1000);
  colour c = z::colour_cycle(0xff0000ff, 2 * _cTimer);
  for (int i = 0; i < 8; ++i) {
    get_shape(7 - i).SetColour(
        z::colour_cycle(0xff0000ff, (i * 32 + 2 * _cTimer) % 256));
  }
  ++_cTimer;
  if (position().y < Lib::HEIGHT / 2) {
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
    int r = z::rand_int(6);
    if (r == 0 || r == 1) {
      _snakes = 16;
    }
    else if (r == 2 || r == 3) {
      _state = STATE_ATTACK;
      _timer = 0;
      fixed f = z::rand_fixed() * (2 * fixed::pi);
      fixed rf = d5d1000 * (1 + z::rand_int(2));
      for (int i = 0; i < 32; ++i) {
        vec2 d;
        d.set_polar(f + i * pi2d32, 1);
        if (r == 2) {
          rf = d5d1000 * (1 + z::rand_int(4));
        }
        spawn(new Snake(position() + d * 16, c, d, rf));
        play_sound_random(Lib::SOUND_BOSS_ATTACK);
      }
    }
    else if (r == 5) {
      _state = STATE_ATTACK;
      _timer = 0;
      fixed f = z::rand_fixed() * (2 * fixed::pi);
      for (int i = 0; i < 64; ++i) {
        vec2 d;
        d.set_polar(f + i * pi2d64, 1);
        spawn(new Snake(position() + d * 16, c, d));
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
    std::vector< int > wide3;
    int timer = 0;
    for (int i = 0; i < 16; ++i) {
      if (_destroyed[(i + 15) % 16] &&
          _destroyed[i] && _destroyed[(i + 1) % 16]) {
        wide3.push_back(i);
      }
      if (!_destroyed[i]) {
        timer = _arcs[i]->GetTimer();
      }
    }
    if (!wide3.empty()) {
      int r = z::rand_int(int(wide3.size()));
      SuperBossArc* s = new SuperBossArc(
          position(), _players, _cycle, wide3[r], this, timer);
      s->set_rotation(rotation() - (fixed(6) / 1000));
      spawn(s);
      _arcs[wide3[r]] = s;
      _destroyed[wide3[r]] = false;
      play_sound(Lib::SOUND_ENEMY_SPAWN);
    }
  }
  static const fixed pi2d128 = 2 * fixed::pi / 128;
  if (_state == STATE_IDLE && _timer % 72 == 0) {
    for (int i = 0; i < 128; ++i) {
      vec2 d;
      d.set_polar(i * pi2d128, 1);
      spawn(new RainbowShot(position() + d * 42, move_vec + d * 3, this));
      play_sound_random(Lib::SOUND_BOSS_FIRE);
    }
  }

  if (_snakes) {
    --_snakes;
    vec2 d;
    d.set_polar(z::rand_fixed() * (2 * fixed::pi),
               z::rand_int(32) + z::rand_int(16));
    spawn(new Snake(d + position(), c));
    play_sound_random(Lib::SOUND_ENEMY_SPAWN);
  }
  move(move_vec);
}

int SuperBoss::GetDamage(int damage, bool magic)
{
  return damage;
}

void SuperBoss::OnDestroy()
{
  SetKilled();
  for (const auto& ship : z0().all_ships(SHIP_ENEMY)) {
    if (ship != this) {
      ship->Damage(Player::BOMB_DAMAGE * 100, false, 0);
    }
  }
  explosion();
  explosion(0xffffffff, 12);
  explosion(get_shape(0).GetColour(), 24);
  explosion(0xffffffff, 36);
  explosion(get_shape(0).GetColour(), 48);

  int n = 1;
  for (int i = 0; i < 16; ++i) {
    vec2 v;
    v.set_polar(z::rand_fixed() * (2 * fixed::pi),
                8 + z::rand_int(64) + z::rand_int(64));
    colour c = z::colour_cycle(get_shape(0).GetColour(), n * 2);
    _fireworks.push_back(
        std::make_pair(n, std::make_pair(position() + v, c)));
    n += i;
  }
  for (int i = 0; i < Lib::PLAYERS; i++) {
    lib().Rumble(i, 25);
  }
  play_sound(Lib::SOUND_EXPLOSION);

  z0Game::ShipList players = z0().get_players();

  n = 0;
  for (std::size_t i = 0; i < players.size(); i++) {
    Player* p = (Player*) players[i];
    if (!p->IsKilled()) {
      n++;
    }
  }

  for (std::size_t i = 0; i < players.size(); i++) {
    Player* p = (Player*) players[i];
    if (!p->IsKilled()) {
      p->AddScore(GetScore() / n);
    }
  }

  for (int i = 0; i < 8; ++i) {
    spawn(new Powerup(position(), Powerup::BOMB));
  }
}
