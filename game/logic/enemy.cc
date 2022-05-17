#include "game/logic/enemy.h"
#include "game/logic/player.h"
#include <algorithm>

namespace {
const std::int32_t kFollowTime = 90;
const fixed kFollowSpeed = 2;

const std::int32_t kChaserTime = 60;
const fixed kChaserSpeed = 4;

const fixed kSquareSpeed = 2 + fixed(1) / 4;

const std::int32_t kWallTimer = 80;
const fixed kWallSpeed = 1 + fixed(1) / 4;

const std::int32_t kFollowHubTimer = 170;
const fixed kFollowHubSpeed = 1;

const std::int32_t kShielderTimer = 80;
const fixed kShielderSpeed = 2;

const std::int32_t kTractorTimer = 50;
const fixed kTractorSpeed = 6 * (fixed(1) / 10);
}  // namespace
const fixed Tractor::kTractorBeamSpeed = 2 + fixed(1) / 2;

Enemy::Enemy(const vec2& position, Ship::ship_category type, std::int32_t hp)
: Ship(position, Ship::ship_category(type | kShipEnemy))
, _hp(hp)
, _score(0)
, _damaged(false)
, _destroy_sound(Lib::sound::kEnemyDestroy) {}

void Enemy::damage(std::int32_t damage, bool magic, Player* source) {
  _hp -= std::max(damage, 0);
  if (damage > 0) {
    play_sound_random(Lib::sound::kEnemyHit);
  }

  if (_hp <= 0 && !is_destroyed()) {
    play_sound_random(_destroy_sound);
    if (source && get_score() > 0) {
      source->add_score(get_score());
    }
    explosion();
    on_destroy(damage >= Player::kBombDamage);
    destroy();
  } else if (!is_destroyed()) {
    if (damage > 0) {
      play_sound_random(Lib::sound::kEnemyHit);
    }
    _damaged = damage >= Player::kBombDamage ? 25 : 1;
  }
}

void Enemy::render() const {
  if (!_damaged) {
    Ship::render();
    return;
  }
  render_with_colour(0xffffffff);
  --_damaged;
}

Follow::Follow(const vec2& position, fixed radius, std::int32_t hp)
: Enemy(position, kShipNone, hp), _timer(0), _target(0) {
  add_shape(new Polygon(vec2(), radius, 4, 0x9933ffff, 0, kDangerous | kVulnerable));
  set_score(15);
  set_bounding_width(10);
  set_destroy_sound(Lib::sound::kEnemyShatter);
  set_enemy_value(1);
}

void Follow::update() {
  shape().rotate(fixed_c::tenth);
  if (!game().alive_players()) {
    return;
  }

  _timer++;
  if (!_target || _timer > kFollowTime) {
    _target = nearest_player();
    _timer = 0;
  }
  vec2 d = _target->shape().centre - shape().centre;
  move(d.normalised() * kFollowSpeed);
}

BigFollow::BigFollow(const vec2& position, bool has_score)
: Follow(position, 20, 3), _has_score(has_score) {
  set_score(has_score ? 20 : 0);
  set_destroy_sound(Lib::sound::kPlayerDestroy);
  set_enemy_value(3);
}

void BigFollow::on_destroy(bool bomb) {
  if (bomb) {
    return;
  }

  vec2 d = vec2(10, 0).rotated(shape().rotation());
  for (std::int32_t i = 0; i < 3; i++) {
    Follow* s = new Follow(shape().centre + d);
    if (!_has_score) {
      s->set_score(0);
    }
    spawn(s);
    d = d.rotated(2 * fixed_c::pi / 3);
  }
}

Chaser::Chaser(const vec2& position)
: Enemy(position, kShipNone, 2), _move(false), _timer(kChaserTime), _dir() {
  add_shape(
      new Polygon(vec2(), 10, 4, 0x3399ffff, 0, kDangerous | kVulnerable, Polygon::T::kPolygram));
  set_score(30);
  set_bounding_width(10);
  set_destroy_sound(Lib::sound::kEnemyShatter);
  set_enemy_value(2);
}

void Chaser::update() {
  bool before = is_on_screen();

  if (_timer) {
    _timer--;
  }
  if (_timer <= 0) {
    _timer = kChaserTime * (_move + 1);
    _move = !_move;
    if (_move) {
      _dir = kChaserSpeed * (nearest_player()->shape().centre - shape().centre).normalised();
    }
  }
  if (_move) {
    move(_dir);
    if (!before && is_on_screen()) {
      _move = false;
    }
  } else {
    shape().rotate(fixed_c::tenth);
  }
}

Square::Square(const vec2& position, fixed rotation)
: Enemy(position, kShipWall, 4), _dir(), _timer(z::rand_int(80) + 40) {
  add_shape(new Box(vec2(), 10, 10, 0x33ff33ff, 0, kDangerous | kVulnerable));
  _dir = vec2::from_polar(rotation, 1);
  set_score(25);
  set_bounding_width(15);
  set_enemy_value(2);
}

void Square::update() {
  if (is_on_screen() && game().get_non_wall_count() == 0) {
    _timer--;
  } else {
    _timer = z::rand_int(80) + 40;
  }

  if (_timer == 0) {
    damage(4, false, 0);
  }

  const vec2& pos = shape().centre;
  if (pos.x < 0 && _dir.x <= 0) {
    _dir.x = -_dir.x;
    if (_dir.x <= 0) {
      _dir.x = 1;
    }
  }
  if (pos.y < 0 && _dir.y <= 0) {
    _dir.y = -_dir.y;
    if (_dir.y <= 0) {
      _dir.y = 1;
    }
  }

  if (pos.x > Lib::kWidth && _dir.x >= 0) {
    _dir.x = -_dir.x;
    if (_dir.x >= 0) {
      _dir.x = -1;
    }
  }
  if (pos.y > Lib::kHeight && _dir.y >= 0) {
    _dir.y = -_dir.y;
    if (_dir.y >= 0) {
      _dir.y = -1;
    }
  }
  _dir = _dir.normalised();
  move(_dir * kSquareSpeed);
  shape().set_rotation(_dir.angle());
}

void Square::render() const {
  if (game().get_non_wall_count() == 0 && (_timer % 4 == 1 || _timer % 4 == 2)) {
    render_with_colour(0x33333300);
  } else {
    Enemy::render();
  }
}

Wall::Wall(const vec2& position, bool rdir)
: Enemy(position, kShipWall, 10), _dir(0, 1), _timer(0), _rotate(false), _rdir(rdir) {
  add_shape(new Box(vec2(), 10, 40, 0x33cc33ff, 0, kDangerous | kVulnerable));
  set_score(20);
  set_bounding_width(50);
  set_enemy_value(4);
}

void Wall::update() {
  if (game().get_non_wall_count() == 0 && _timer % 8 < 2) {
    if (get_hp() > 2) {
      play_sound(Lib::sound::kEnemySpawn);
    }
    damage(get_hp() - 2, false, 0);
  }

  if (_rotate) {
    vec2 d =
        _dir.rotated((_rdir ? 1 : -1) * (_timer - kWallTimer) * fixed_c::pi / (4 * kWallTimer));

    shape().set_rotation(d.angle());
    _timer--;
    if (_timer <= 0) {
      _timer = 0;
      _rotate = false;
      _dir = _dir.rotated(_rdir ? -fixed_c::pi / 4 : fixed_c::pi / 4);
    }
    return;
  } else {
    _timer++;
    if (_timer > kWallTimer * 6) {
      if (is_on_screen()) {
        _timer = kWallTimer;
        _rotate = true;
      } else {
        _timer = 0;
      }
    }
  }

  vec2 pos = shape().centre;
  if ((pos.x < 0 && _dir.x < -fixed_c::hundredth) || (pos.y < 0 && _dir.y < -fixed_c::hundredth) ||
      (pos.x > Lib::kWidth && _dir.x > fixed_c::hundredth) ||
      (pos.y > Lib::kHeight && _dir.y > fixed_c::hundredth)) {
    _dir = -_dir.normalised();
  }

  move(_dir * kWallSpeed);
  shape().set_rotation(_dir.angle());
}

void Wall::on_destroy(bool bomb) {
  if (bomb) {
    return;
  }
  vec2 d = _dir.rotated(fixed_c::pi / 2);

  vec2 v = shape().centre + d * 10 * 3;
  if (v.x >= 0 && v.x <= Lib::kWidth && v.y >= 0 && v.y <= Lib::kHeight) {
    spawn(new Square(v, shape().rotation()));
  }

  v = shape().centre - d * 10 * 3;
  if (v.x >= 0 && v.x <= Lib::kWidth && v.y >= 0 && v.y <= Lib::kHeight) {
    spawn(new Square(v, shape().rotation()));
  }
}

FollowHub::FollowHub(const vec2& position, bool powera, bool powerb)
: Enemy(position, kShipNone, 14)
, _timer(0)
, _dir(0, 0)
, _count(0)
, _powera(powera)
, _powerb(powerb) {
  add_shape(new Polygon(vec2(), 16, 4, 0x6666ffff, fixed_c::pi / 4, kDangerous | kVulnerable,
                        Polygon::T::kPolygram));
  if (_powerb) {
    add_shape(
        new Polygon(vec2(16, 0), 8, 4, 0x6666ffff, fixed_c::pi / 4, 0, Polygon::T::kPolystar));
    add_shape(
        new Polygon(vec2(-16, 0), 8, 4, 0x6666ffff, fixed_c::pi / 4, 0, Polygon::T::kPolystar));
    add_shape(
        new Polygon(vec2(0, 16), 8, 4, 0x6666ffff, fixed_c::pi / 4, 0, Polygon::T::kPolystar));
    add_shape(
        new Polygon(vec2(0, -16), 8, 4, 0x6666ffff, fixed_c::pi / 4, 0, Polygon::T::kPolystar));
  }

  add_shape(new Polygon(vec2(16, 0), 8, 4, 0x6666ffff, fixed_c::pi / 4));
  add_shape(new Polygon(vec2(-16, 0), 8, 4, 0x6666ffff, fixed_c::pi / 4));
  add_shape(new Polygon(vec2(0, 16), 8, 4, 0x6666ffff, fixed_c::pi / 4));
  add_shape(new Polygon(vec2(0, -16), 8, 4, 0x6666ffff, fixed_c::pi / 4));
  set_score(50 + _powera * 10 + _powerb * 10);
  set_bounding_width(16);
  set_destroy_sound(Lib::sound::kPlayerDestroy);
  set_enemy_value(6 + (powera ? 2 : 0) + (powerb ? 2 : 0));
}

void FollowHub::update() {
  _timer++;
  if (_timer > (_powera ? kFollowHubTimer / 2 : kFollowHubTimer)) {
    _timer = 0;
    _count++;
    if (is_on_screen()) {
      if (_powerb) {
        spawn(new Chaser(shape().centre));
      } else {
        spawn(new Follow(shape().centre));
      }
      play_sound_random(Lib::sound::kEnemySpawn);
    }
  }

  _dir = shape().centre.x < 0           ? vec2(1, 0)
      : shape().centre.x > Lib::kWidth  ? vec2(-1, 0)
      : shape().centre.y < 0            ? vec2(0, 1)
      : shape().centre.y > Lib::kHeight ? vec2(0, -1)
      : _count > 3                      ? (_count = 0, _dir.rotated(-fixed_c::pi / 2))
                                        : _dir;

  fixed s = _powera ? fixed_c::hundredth * 5 + fixed_c::tenth : fixed_c::hundredth * 5;
  shape().rotate(s);
  shapes()[0]->rotate(-s);

  move(_dir * kFollowHubSpeed);
}

void FollowHub::on_destroy(bool bomb) {
  if (bomb) {
    return;
  }
  if (_powerb) {
    spawn(new BigFollow(shape().centre, true));
  }
  spawn(new Chaser(shape().centre));
}

Shielder::Shielder(const vec2& position, bool power)
: Enemy(position, kShipNone, 16)
, _dir(0, 1)
, _timer(0)
, _rotate(false)
, _rdir(false)
, _power(power) {
  add_shape(new Polygon(vec2(24, 0), 8, 6, 0x006633ff, 0, kVulnShield, Polygon::T::kPolystar));
  add_shape(new Polygon(vec2(-24, 0), 8, 6, 0x006633ff, 0, kVulnShield, Polygon::T::kPolystar));
  add_shape(new Polygon(vec2(0, 24), 8, 6, 0x006633ff, fixed_c::pi / 2, kVulnShield,
                        Polygon::T::kPolystar));
  add_shape(new Polygon(vec2(0, -24), 8, 6, 0x006633ff, fixed_c::pi / 2, kVulnShield,
                        Polygon::T::kPolystar));
  add_shape(new Polygon(vec2(24, 0), 8, 6, 0x33cc99ff, 0, 0));
  add_shape(new Polygon(vec2(-24, 0), 8, 6, 0x33cc99ff, 0, 0));
  add_shape(new Polygon(vec2(0, 24), 8, 6, 0x33cc99ff, fixed_c::pi / 2, 0));
  add_shape(new Polygon(vec2(0, -24), 8, 6, 0x33cc99ff, fixed_c::pi / 2, 0));

  add_shape(new Polygon(vec2(0, 0), 24, 4, 0x006633ff, 0, 0, Polygon::T::kPolystar));
  add_shape(
      new Polygon(vec2(0, 0), 14, 8, power ? 0x33cc99ff : 0x006633ff, 0, kDangerous | kVulnerable));
  set_score(60 + _power * 40);
  set_bounding_width(36);
  set_destroy_sound(Lib::sound::kPlayerDestroy);
  set_enemy_value(8 + (power ? 2 : 0));
}

void Shielder::update() {
  fixed s = _power ? fixed_c::hundredth * 12 : fixed_c::hundredth * 4;
  shape().rotate(s);
  shapes()[9]->rotate(-2 * s);
  for (std::int32_t i = 0; i < 8; i++) {
    shapes()[i]->rotate(-s);
  }

  bool on_screen = false;
  _dir = shape().centre.x < 0           ? vec2(1, 0)
      : shape().centre.x > Lib::kWidth  ? vec2(-1, 0)
      : shape().centre.y < 0            ? vec2(0, 1)
      : shape().centre.y > Lib::kHeight ? vec2(0, -1)
                                        : (on_screen = true, _dir);

  if (!on_screen && _rotate) {
    _timer = 0;
    _rotate = false;
  }

  fixed speed =
      kShielderSpeed + (_power ? fixed_c::tenth * 3 : fixed_c::tenth * 2) * (16 - get_hp());
  if (_rotate) {
    vec2 d = _dir.rotated((_rdir ? 1 : -1) * (kShielderTimer - _timer) * fixed_c::pi /
                          (2 * kShielderTimer));
    _timer--;
    if (_timer <= 0) {
      _timer = 0;
      _rotate = false;
      _dir = _dir.rotated((_rdir ? 1 : -1) * fixed_c::pi / 2);
    }
    move(d * speed);
  } else {
    _timer++;
    if (_timer > kShielderTimer * 2) {
      _timer = kShielderTimer;
      _rotate = true;
      _rdir = z::rand_int(2) != 0;
    }
    if (is_on_screen() && _power && _timer % kShielderTimer == kShielderTimer / 2) {
      Player* p = nearest_player();
      vec2 v = shape().centre;

      vec2 d = (p->shape().centre - v).normalised();
      spawn(new BossShot(v, d * 3, 0x33cc99ff));
      play_sound_random(Lib::sound::kBossFire);
    }
    move(_dir * speed);
  }
  _dir = _dir.normalised();
}

Tractor::Tractor(const vec2& position, bool power)
: Enemy(position, kShipNone, 50)
, _timer(kTractorTimer * 4)
, _dir(0, 0)
, _power(power)
, _ready(false)
, _spinning(false) {
  add_shape(new Polygon(vec2(24, 0), 12, 6, 0xcc33ccff, 0, kDangerous | kVulnerable,
                        Polygon::T::kPolygram));
  add_shape(new Polygon(vec2(-24, 0), 12, 6, 0xcc33ccff, 0, kDangerous | kVulnerable,
                        Polygon::T::kPolygram));
  add_shape(new Line(vec2(0, 0), vec2(24, 0), vec2(-24, 0), 0xcc33ccff));
  if (power) {
    add_shape(new Polygon(vec2(24, 0), 16, 6, 0xcc33ccff, 0, 0, Polygon::T::kPolystar));
    add_shape(new Polygon(vec2(-24, 0), 16, 6, 0xcc33ccff, 0, 0, Polygon::T::kPolystar));
  }
  set_score(85 + power * 40);
  set_bounding_width(36);
  set_destroy_sound(Lib::sound::kPlayerDestroy);
  set_enemy_value(10 + (power ? 2 : 0));
}

void Tractor::update() {
  shapes()[0]->rotate(fixed_c::hundredth * 5);
  shapes()[1]->rotate(-fixed_c::hundredth * 5);
  if (_power) {
    shapes()[3]->rotate(-fixed_c::hundredth * 8);
    shapes()[4]->rotate(fixed_c::hundredth * 8);
  }

  if (shape().centre.x < 0) {
    _dir = vec2(1, 0);
  } else if (shape().centre.x > Lib::kWidth) {
    _dir = vec2(-1, 0);
  } else if (shape().centre.y < 0) {
    _dir = vec2(0, 1);
  } else if (shape().centre.y > Lib::kHeight) {
    _dir = vec2(0, -1);
  } else {
    _timer++;
  }

  if (!_ready && !_spinning) {
    move(_dir * kTractorSpeed * (is_on_screen() ? 1 : 2 + fixed_c::half));

    if (_timer > kTractorTimer * 8) {
      _ready = true;
      _timer = 0;
    }
  } else if (_ready) {
    if (_timer > kTractorTimer) {
      _ready = false;
      _spinning = true;
      _timer = 0;
      _players = game().players();
      play_sound(Lib::sound::kBossFire);
    }
  } else if (_spinning) {
    shape().rotate(fixed_c::tenth * 3);
    for (const auto& ship : _players) {
      if (!((Player*)ship)->is_killed()) {
        vec2 d = (shape().centre - ship->shape().centre).normalised();
        ship->move(d * kTractorBeamSpeed);
      }
    }

    if (_timer % (kTractorTimer / 2) == 0 && is_on_screen() && _power) {
      Player* p = nearest_player();
      vec2 d = (p->shape().centre - shape().centre).normalised();
      spawn(new BossShot(shape().centre, d * 4, 0xcc33ccff));
      play_sound_random(Lib::sound::kBossFire);
    }

    if (_timer > kTractorTimer * 5) {
      _spinning = false;
      _timer = 0;
    }
  }
}

void Tractor::render() const {
  Enemy::render();
  if (_spinning) {
    for (std::size_t i = 0; i < _players.size(); ++i) {
      if (((_timer + i * 4) / 4) % 2 && !((Player*)_players[i])->is_killed()) {
        lib().render_line(to_float(shape().centre), to_float(_players[i]->shape().centre),
                          0xcc33ccff);
      }
    }
  }
}

BossShot::BossShot(const vec2& position, const vec2& velocity, colour_t c)
: Enemy(position, kShipWall, 0), _dir(velocity) {
  add_shape(new Polygon(vec2(), 16, 8, c, 0, 0, Polygon::T::kPolystar));
  add_shape(new Polygon(vec2(), 10, 8, c, 0, 0));
  add_shape(new Polygon(vec2(), 12, 8, 0, 0, kDangerous));
  set_bounding_width(12);
  set_score(0);
  set_enemy_value(1);
}

void BossShot::update() {
  move(_dir);
  vec2 p = shape().centre;
  if ((p.x < -10 && _dir.x < 0) || (p.x > Lib::kWidth + 10 && _dir.x > 0) ||
      (p.y < -10 && _dir.y < 0) || (p.y > Lib::kHeight + 10 && _dir.y > 0)) {
    destroy();
  }
  shape().set_rotation(shape().rotation() + fixed_c::hundredth * 2);
}