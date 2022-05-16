#include "game/logic/boss/tractor.h"
#include "game/logic/player.h"

static const int32_t TB_BASE_HP = 900;
static const int32_t TB_TIMER = 100;
static const fixed TB_SPEED = 2;

TractorBoss::TractorBoss(int32_t players, int32_t cycle)
: Boss(vec2(Lib::WIDTH * (1 + fixed_c::half), Lib::HEIGHT / 2), SimState::BOSS_2A, TB_BASE_HP,
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
  _s1 = new CompoundShape(vec2(0, -96), 0, DANGEROUS | VULNERABLE);

  _s1->add_shape(new Polygon(vec2(), 12, 6, 0x882288ff, 0, 0, Polygon::T::POLYGRAM));
  _s1->add_shape(new Polygon(vec2(), 12, 12, 0x882288ff, 0, 0));
  _s1->add_shape(new Polygon(vec2(), 2, 6, 0x882288ff, 0, 0));
  _s1->add_shape(new Polygon(vec2(), 36, 12, 0xcc33ccff, 0, 0));
  _s1->add_shape(new Polygon(vec2(), 34, 12, 0xcc33ccff, 0, 0));
  _s1->add_shape(new Polygon(vec2(), 32, 12, 0xcc33ccff, 0, 0));
  for (int32_t i = 0; i < 8; i++) {
    vec2 d = vec2(24, 0).rotated(i * fixed_c::pi / 4);
    _s1->add_shape(new Polygon(d, 12, 6, 0xcc33ccff, 0, 0, Polygon::T::POLYGRAM));
  }

  _s2 = new CompoundShape(vec2(0, 96), 0, DANGEROUS | VULNERABLE);

  _s2->add_shape(new Polygon(vec2(), 12, 6, 0x882288ff, 0, 0, Polygon::T::POLYGRAM));
  _s2->add_shape(new Polygon(vec2(), 12, 12, 0x882288ff, 0, 0));
  _s2->add_shape(new Polygon(vec2(), 2, 6, 0x882288ff, 0, 0));

  _s2->add_shape(new Polygon(vec2(), 36, 12, 0xcc33ccff, 0, 0));
  _s2->add_shape(new Polygon(vec2(), 34, 12, 0xcc33ccff, 0, 0));
  _s2->add_shape(new Polygon(vec2(), 32, 12, 0xcc33ccff, 0, 0));
  for (int32_t i = 0; i < 8; i++) {
    vec2 d = vec2(24, 0).rotated(i * fixed_c::pi / 4);
    _s2->add_shape(new Polygon(d, 12, 6, 0xcc33ccff, 0, 0, Polygon::T::POLYGRAM));
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

void TractorBoss::update() {
  if (shape().centre.x <= Lib::WIDTH / 2 && _will_attack && !_stopped && !_continue) {
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
    shape().rotate(z::rand_fixed() * 2 * fixed_c::pi);
  }

  _timer++;
  if (!_stopped) {
    move(TB_SPEED * vec2(-1, 0));
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

        play_sound_random(Lib::SOUND_BOSS_FIRE);
      }
      if (_shoot_type == 1 || is_hp_low()) {
        Player* p = nearest_player();
        vec2 v = shape().centre;

        vec2 d = (p->shape().centre - v).normalised();
        spawn(new BossShot(v, d * 5, 0xcc33ccff));
        spawn(new BossShot(v, d * -5, 0xcc33ccff));
        play_sound_random(Lib::SOUND_BOSS_FIRE);
      }
    }
    if ((!_will_attack || _continue) && is_on_screen()) {
      if (_sound) {
        play_sound(Lib::SOUND_BOSS_ATTACK);
        _sound = false;
      }
      _targets.clear();
      for (const auto& ship : game().all_ships(SHIP_PLAYER)) {
        if (((Player*)ship)->is_killed()) {
          continue;
        }
        vec2 pos = ship->shape().centre;
        _targets.push_back(pos);
        vec2 d = (shape().centre - pos).normalised();
        ship->move(d * Tractor::TRACTOR_BEAM_SPEED);
      }
    }
  } else {
    if (_generating) {
      if (_timer >= TB_TIMER * 5) {
        _timer = 0;
        _generating = false;
        _attacking = false;
        _attack_size = 0;
        play_sound(Lib::SOUND_BOSS_ATTACK);
      }

      if (_timer < TB_TIMER * 4 && _timer % (10 - 2 * game().alive_players()) == 0) {
        Ship* s = new TBossShot(_s1->convert_point(shape().centre, shape().rotation(), vec2()),
                                _gen_dir ? shape().rotation() + fixed_c::pi : shape().rotation());
        spawn(s);

        s = new TBossShot(_s2->convert_point(shape().centre, shape().rotation(), vec2()),
                          shape().rotation() + (_gen_dir ? 0 : fixed_c::pi));
        spawn(s);
        play_sound_random(Lib::SOUND_ENEMY_SPAWN);
      }

      if (is_hp_low() && _timer % (20 - game().alive_players() * 2) == 0) {
        Player* p = nearest_player();
        vec2 v = shape().centre;

        vec2 d = (p->shape().centre - v).normalised();
        spawn(new BossShot(v, d * 5, 0xcc33ccff));
        spawn(new BossShot(v, d * -5, 0xcc33ccff));
        play_sound_random(Lib::SOUND_BOSS_FIRE);
      }
    } else {
      if (!_attacking) {
        if (_timer >= TB_TIMER * 4) {
          _timer = 0;
          _attacking = true;
        }
        if (_timer % (TB_TIMER / (1 + fixed_c::half)).to_int() == TB_TIMER / 8) {
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
          play_sound_random(Lib::SOUND_BOSS_FIRE);
        }
        _targets.clear();
        for (const auto& ship : game().all_ships(SHIP_PLAYER | SHIP_ENEMY)) {
          if (ship == this || ((ship->type() & SHIP_PLAYER) && ((Player*)ship)->is_killed())) {
            continue;
          }

          if (ship->type() & SHIP_ENEMY) {
            play_sound_random(Lib::SOUND_BOSS_ATTACK, 0, 0.3f);
          }
          vec2 pos = ship->shape().centre;
          _targets.push_back(pos);
          fixed speed = 0;
          if (ship->type() & SHIP_PLAYER) {
            speed = Tractor::TRACTOR_BEAM_SPEED;
          }
          if (ship->type() & SHIP_ENEMY) {
            speed = 4 + fixed_c::half;
          }
          vec2 d = (shape().centre - pos).normalised();
          ship->move(d * speed);

          if ((ship->type() & SHIP_ENEMY) && !(ship->type() & SHIP_WALL) &&
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
        for (int32_t i = 0; i < _attack_size; i++) {
          vec2 d = vec2::from_polar(i * (2 * fixed_c::pi) / _attack_size, 5);
          spawn(new BossShot(shape().centre, d, 0xcc33ccff));
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

int32_t TractorBoss::get_damage(int32_t damage, bool magic) {
  return damage;
}

TBossShot::TBossShot(const vec2& position, fixed angle) : Enemy(position, SHIP_NONE, 1) {
  add_shape(new Polygon(vec2(), 8, 6, 0xcc33ccff, 0, DANGEROUS | VULNERABLE));
  _dir = vec2::from_polar(angle, 3);
  set_bounding_width(8);
  set_score(0);
  set_destroy_sound(Lib::SOUND_ENEMY_SHATTER);
}

void TBossShot::update() {
  if ((shape().centre.x > Lib::WIDTH && _dir.x > 0) || (shape().centre.x < 0 && _dir.x < 0)) {
    _dir.x = -_dir.x;
  }
  if ((shape().centre.y > Lib::HEIGHT && _dir.y > 0) || (shape().centre.y < 0 && _dir.y < 0)) {
    _dir.y = -_dir.y;
  }

  move(_dir);
}