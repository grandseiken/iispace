#include "game/logic/boss/tractor.h"
#include "game/logic/player.h"

namespace {
const std::int32_t kTbBaseHp = 900;
const std::int32_t kTbTimer = 100;
const fixed kTbSpeed = 2;
}  // namespace

TractorBoss::TractorBoss(std::int32_t players, std::int32_t cycle)
: Boss(vec2(Lib::kWidth * (1 + fixed_c::half), Lib::kHeight / 2), SimState::BOSS_2A, kTbBaseHp,
       players, cycle)
, _will_attack(false)
, _stopped(false)
, _generating(false)
, _attacking(false)
, _continue(false)
, _gen_dir(0)
, _shoot_type(z::rand_int(2))
, _sound(true)
, _timer(0)
, _attack_size(0) {
  _s1 = new CompoundShape(vec2(0, -96), 0, kDangerous | kVulnerable);

  _s1->add_shape(new Polygon(vec2(), 12, 6, 0x882288ff, 0, 0, Polygon::T::kPolygram));
  _s1->add_shape(new Polygon(vec2(), 12, 12, 0x882288ff, 0, 0));
  _s1->add_shape(new Polygon(vec2(), 2, 6, 0x882288ff, 0, 0));
  _s1->add_shape(new Polygon(vec2(), 36, 12, 0xcc33ccff, 0, 0));
  _s1->add_shape(new Polygon(vec2(), 34, 12, 0xcc33ccff, 0, 0));
  _s1->add_shape(new Polygon(vec2(), 32, 12, 0xcc33ccff, 0, 0));
  for (std::int32_t i = 0; i < 8; i++) {
    vec2 d = vec2(24, 0).rotated(i * fixed_c::pi / 4);
    _s1->add_shape(new Polygon(d, 12, 6, 0xcc33ccff, 0, 0, Polygon::T::kPolygram));
  }

  _s2 = new CompoundShape(vec2(0, 96), 0, kDangerous | kVulnerable);

  _s2->add_shape(new Polygon(vec2(), 12, 6, 0x882288ff, 0, 0, Polygon::T::kPolygram));
  _s2->add_shape(new Polygon(vec2(), 12, 12, 0x882288ff, 0, 0));
  _s2->add_shape(new Polygon(vec2(), 2, 6, 0x882288ff, 0, 0));

  _s2->add_shape(new Polygon(vec2(), 36, 12, 0xcc33ccff, 0, 0));
  _s2->add_shape(new Polygon(vec2(), 34, 12, 0xcc33ccff, 0, 0));
  _s2->add_shape(new Polygon(vec2(), 32, 12, 0xcc33ccff, 0, 0));
  for (std::int32_t i = 0; i < 8; i++) {
    vec2 d = vec2(24, 0).rotated(i * fixed_c::pi / 4);
    _s2->add_shape(new Polygon(d, 12, 6, 0xcc33ccff, 0, 0, Polygon::T::kPolygram));
  }

  add_shape(_s1);
  add_shape(_s2);

  _sattack = new Polygon(vec2(), 0, 16, 0x993399ff);
  add_shape(_sattack);

  add_shape(new Line(vec2(), vec2(-2, -96), vec2(-2, 96), 0xcc33ccff, 0));
  add_shape(new Line(vec2(), vec2(0, -96), vec2(0, 96), 0x882288ff, 0));
  add_shape(new Line(vec2(), vec2(2, -96), vec2(2, 96), 0xcc33ccff, 0));

  add_shape(new Polygon(vec2(0, 96), 30, 12, 0, 0, kShield));
  add_shape(new Polygon(vec2(0, -96), 30, 12, 0, 0, kShield));

  _attack_shapes = shapes().size();
}

void TractorBoss::update() {
  if (shape().centre.x <= Lib::kWidth / 2 && _will_attack && !_stopped && !_continue) {
    _stopped = true;
    _generating = true;
    _gen_dir = z::rand_int(2) == 0;
    _timer = 0;
  }

  if (shape().centre.x < -150) {
    shape().centre.x = Lib::kWidth + 150;
    _will_attack = !_will_attack;
    _shoot_type = z::rand_int(2);
    if (_will_attack) {
      _shoot_type = 0;
    }
    _continue = false;
    _sound = !_will_attack;
    shape().rotate(z::rand_fixed() * 2 * fixed_c::pi);
  }

  _timer++;
  if (!_stopped) {
    move(kTbSpeed * vec2(-1, 0));
    if (!_will_attack && is_on_screen() && _timer % (16 - game().alive_players() * 2) == 0) {
      if (_shoot_type == 0 || (is_hp_low() && _shoot_type == 1)) {
        Player* p = nearest_player();

        vec2 v = _s1->convert_point(shape().centre, shape().rotation(), vec2());
        vec2 d = (p->shape().centre - v).normalised();
        spawn(new BossShot(v, d * 5, 0xcc33ccff));
        spawn(new BossShot(v, d * -5, 0xcc33ccff));

        v = _s2->convert_point(shape().centre, shape().rotation(), vec2());
        d = (p->shape().centre - v).normalised();
        spawn(new BossShot(v, d * 5, 0xcc33ccff));
        spawn(new BossShot(v, d * -5, 0xcc33ccff));

        play_sound_random(Lib::sound::kBossFire);
      }
      if (_shoot_type == 1 || is_hp_low()) {
        Player* p = nearest_player();
        vec2 v = shape().centre;

        vec2 d = (p->shape().centre - v).normalised();
        spawn(new BossShot(v, d * 5, 0xcc33ccff));
        spawn(new BossShot(v, d * -5, 0xcc33ccff));
        play_sound_random(Lib::sound::kBossFire);
      }
    }
    if ((!_will_attack || _continue) && is_on_screen()) {
      if (_sound) {
        play_sound(Lib::sound::kBossAttack);
        _sound = false;
      }
      _targets.clear();
      for (const auto& ship : game().all_ships(kShipPlayer)) {
        if (((Player*)ship)->is_killed()) {
          continue;
        }
        vec2 pos = ship->shape().centre;
        _targets.push_back(pos);
        vec2 d = (shape().centre - pos).normalised();
        ship->move(d * Tractor::kTractorBeamSpeed);
      }
    }
  } else {
    if (_generating) {
      if (_timer >= kTbTimer * 5) {
        _timer = 0;
        _generating = false;
        _attacking = false;
        _attack_size = 0;
        play_sound(Lib::sound::kBossAttack);
      }

      if (_timer < kTbTimer * 4 && _timer % (10 - 2 * game().alive_players()) == 0) {
        Ship* s = new TBossShot(_s1->convert_point(shape().centre, shape().rotation(), vec2()),
                                _gen_dir ? shape().rotation() + fixed_c::pi : shape().rotation());
        spawn(s);

        s = new TBossShot(_s2->convert_point(shape().centre, shape().rotation(), vec2()),
                          shape().rotation() + (_gen_dir ? 0 : fixed_c::pi));
        spawn(s);
        play_sound_random(Lib::sound::kEnemySpawn);
      }

      if (is_hp_low() && _timer % (20 - game().alive_players() * 2) == 0) {
        Player* p = nearest_player();
        vec2 v = shape().centre;

        vec2 d = (p->shape().centre - v).normalised();
        spawn(new BossShot(v, d * 5, 0xcc33ccff));
        spawn(new BossShot(v, d * -5, 0xcc33ccff));
        play_sound_random(Lib::sound::kBossFire);
      }
    } else {
      if (!_attacking) {
        if (_timer >= kTbTimer * 4) {
          _timer = 0;
          _attacking = true;
        }
        if (_timer % (kTbTimer / (1 + fixed_c::half)).to_int() == kTbTimer / 8) {
          vec2 v = _s1->convert_point(shape().centre, shape().rotation(), vec2());
          vec2 d = vec2::from_polar(z::rand_fixed() * (2 * fixed_c::pi), 5);
          spawn(new BossShot(v, d, 0xcc33ccff));
          d = d.rotated(fixed_c::pi / 2);
          spawn(new BossShot(v, d, 0xcc33ccff));
          d = d.rotated(fixed_c::pi / 2);
          spawn(new BossShot(v, d, 0xcc33ccff));
          d = d.rotated(fixed_c::pi / 2);
          spawn(new BossShot(v, d, 0xcc33ccff));

          v = _s2->convert_point(shape().centre, shape().rotation(), vec2());
          d = vec2::from_polar(z::rand_fixed() * (2 * fixed_c::pi), 5);
          spawn(new BossShot(v, d, 0xcc33ccff));
          d = d.rotated(fixed_c::pi / 2);
          spawn(new BossShot(v, d, 0xcc33ccff));
          d = d.rotated(fixed_c::pi / 2);
          spawn(new BossShot(v, d, 0xcc33ccff));
          d = d.rotated(fixed_c::pi / 2);
          spawn(new BossShot(v, d, 0xcc33ccff));
          play_sound_random(Lib::sound::kBossFire);
        }
        _targets.clear();
        for (const auto& ship : game().all_ships(kShipPlayer | kShipEnemy)) {
          if (ship == this || ((ship->type() & kShipPlayer) && ((Player*)ship)->is_killed())) {
            continue;
          }

          if (ship->type() & kShipEnemy) {
            play_sound_random(Lib::sound::kBossAttack, 0, 0.3f);
          }
          vec2 pos = ship->shape().centre;
          _targets.push_back(pos);
          fixed speed = 0;
          if (ship->type() & kShipPlayer) {
            speed = Tractor::kTractorBeamSpeed;
          }
          if (ship->type() & kShipEnemy) {
            speed = 4 + fixed_c::half;
          }
          vec2 d = (shape().centre - pos).normalised();
          ship->move(d * speed);

          if ((ship->type() & kShipEnemy) && !(ship->type() & kShipWall) &&
              (ship->shape().centre - shape().centre).length() <= 40) {
            ship->destroy();
            _attack_size++;
            _sattack->radius = _attack_size / (1 + fixed_c::half);
            add_shape(new Polygon(vec2(), 8, 6, 0xcc33ccff, 0, 0));
          }
        }
      } else {
        _timer = 0;
        _stopped = false;
        _continue = true;
        for (std::int32_t i = 0; i < _attack_size; i++) {
          vec2 d = vec2::from_polar(i * (2 * fixed_c::pi) / _attack_size, 5);
          spawn(new BossShot(shape().centre, d, 0xcc33ccff));
        }
        play_sound(Lib::sound::kBossFire);
        play_sound_random(Lib::sound::kExplosion);
        _attack_size = 0;
        _sattack->radius = 0;
        while (_attack_shapes < shapes().size()) {
          destroy_shape(_attack_shapes);
        }
      }
    }
  }

  for (std::size_t i = _attack_shapes; i < shapes().size(); ++i) {
    vec2 v = vec2::from_polar(
        z::rand_fixed() * (2 * fixed_c::pi),
        2 * (z::rand_fixed() - fixed_c::half) * fixed(_attack_size) / (1 + fixed_c::half));
    shapes()[i]->centre = v;
  }

  fixed r = 0;
  if (!_will_attack || (_stopped && !_generating && !_attacking)) {
    r = fixed_c::hundredth;
  } else if (_stopped && _attacking && !_generating) {
    r = 0;
  } else {
    r = fixed_c::hundredth / 2;
  }
  shape().rotate(r);

  _s1->rotate(fixed_c::tenth / 2);
  _s2->rotate(-fixed_c::tenth / 2);
  for (std::size_t i = 0; i < 8; i++) {
    _s1->shapes()[4 + i]->rotate(-fixed_c::tenth);
    _s2->shapes()[4 + i]->rotate(fixed_c::tenth);
  }
}

void TractorBoss::render() const {
  Boss::render();
  if ((_stopped && !_generating && !_attacking) ||
      (!_stopped && (_continue || !_will_attack) && is_on_screen())) {
    for (std::size_t i = 0; i < _targets.size(); ++i) {
      if (((_timer + i * 4) / 4) % 2) {
        lib().render_line(to_float(shape().centre), to_float(_targets[i]), 0xcc33ccff);
      }
    }
  }
}

std::int32_t TractorBoss::get_damage(std::int32_t damage, bool magic) {
  return damage;
}

TBossShot::TBossShot(const vec2& position, fixed angle) : Enemy(position, kShipNone, 1) {
  add_shape(new Polygon(vec2(), 8, 6, 0xcc33ccff, 0, kDangerous | kVulnerable));
  _dir = vec2::from_polar(angle, 3);
  set_bounding_width(8);
  set_score(0);
  set_destroy_sound(Lib::sound::kEnemyShatter);
}

void TBossShot::update() {
  if ((shape().centre.x > Lib::kWidth && _dir.x > 0) || (shape().centre.x < 0 && _dir.x < 0)) {
    _dir.x = -_dir.x;
  }
  if ((shape().centre.y > Lib::kHeight && _dir.y > 0) || (shape().centre.y < 0 && _dir.y < 0)) {
    _dir.y = -_dir.y;
  }

  move(_dir);
}