#include "boss_ghost.h"
#include "player.h"

static const int32_t GB_BASE_HP = 700;
static const int32_t GB_TIMER = 250;
static const int32_t GB_ATTACK_TIME = 100;
static const fixed GB_WALL_SPEED = 3 + fixed(1) / 2;

GhostBoss::GhostBoss(int32_t players, int32_t cycle)
: Boss(vec2(Lib::WIDTH / 2, Lib::HEIGHT / 2), GameModal::BOSS_2B, GB_BASE_HP, players, cycle)
, _visible(false)
, _vtime(0)
, _timer(0)
, _attack_time(GB_ATTACK_TIME)
, _attack(0)
, _rDir(false)
, _start_time(120)
, _danger_circle(0)
, _danger_offset1(0)
, _danger_offset2(0)
, _danger_offset3(0)
, _danger_offset4(0)
, _shot_type(false) {
  add_shape(new Polygon(vec2(), 32, 8, 0x9933ccff, 0, 0));
  add_shape(new Polygon(vec2(), 48, 8, 0xcc66ffff, 0, 0));

  for (int32_t i = 0; i < 8; i++) {
    vec2 c = vec2::from_polar(i * fixed_c::pi / 4, 48);

    CompoundShape* s = new CompoundShape(c, 0, 0);
    for (int32_t j = 0; j < 8; j++) {
      vec2 d = vec2::from_polar(j * fixed_c::pi / 4, 1);
      s->add_shape(new Line(vec2(), d * 10, d * 10 * 2, 0x9933ccff, 0));
    }
    add_shape(s);
  }

  add_shape(new Polygon(vec2(), 24, 8, 0xcc66ffff, 0, 0, Polygon::T::POLYGRAM));

  for (int32_t n = 0; n < 5; n++) {
    CompoundShape* s = new CompoundShape(vec2(), 0, 0);
    for (int32_t i = 0; i < 16 + n * 6; i++) {
      vec2 d = vec2::from_polar(i * 2 * fixed_c::pi / (16 + n * 6), 100 + n * 60);
      s->add_shape(new Polygon(d, 16, 8, n ? 0x330066ff : 0x9933ccff, 0, 0, Polygon::T::POLYSTAR));
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

void GhostBoss::update() {
  for (int32_t n = 1; n < 5; ++n) {
    CompoundShape* s = (CompoundShape*) shapes()[11 + n].get();
    CompoundShape* c = (CompoundShape*) shapes()[16 + n].get();
    c->clear_shapes();

    for (int32_t i = 0; i < 16 + n * 6; ++i) {
      Shape& s1 = *s->shapes()[2 * i];
      Shape& s2 = *s->shapes()[1 + 2 * i];
      int32_t t = n == 1 ? (i + _danger_offset1) % 11 : n == 2 ? (i + _danger_offset2) % 7 : n == 3
                  ? (i + _danger_offset3) % 17
                  : (i + _danger_offset4) % 10;

      bool b = n == 1 ? (t % 11 == 0 || t % 11 == 5) : n == 2 ? (t % 7 == 0) : n == 3
                  ? (t % 17 == 0 || t % 17 == 8)
                  : (t % 10 == 0);

      if (((n == 4 && _attack != 2) || (n + (n == 3)) & _danger_circle) && _visible && b) {
        s1.colour = 0xcc66ffff;
        s2.colour = 0xcc66ffff;
        if (_vtime == 0) {
          vec2 d = vec2::from_polar(i * 2 * fixed_c::pi / (16 + n * 6), 100 + n * 60);
          c->add_shape(new Polygon(d, 9, 8, 0));
          _warnings.push_back(c->convert_point(shape().centre, shape().rotation(), d));
        }
      } else {
        s1.colour = 0x330066ff;
        s2.colour = 0x330066ff;
      }
    }
  }
  if (!(_attack == 2 && _attack_time < GB_ATTACK_TIME * 4 && _attack_time)) {
    shape().rotate(-fixed_c::hundredth * 4);
  }

  for (std::size_t i = 0; i < 8; i++) {
    shapes()[i + 2]->rotate(fixed_c::hundredth * 4);
  }
  shapes()[10]->rotate(fixed_c::hundredth * 6);
  for (int32_t n = 0; n < 5; n++) {
    CompoundShape* s = (CompoundShape*) shapes()[11 + n].get();
    if (n % 2) {
      s->rotate(fixed_c::hundredth * 3 + (fixed(3) / 2000) * n);
    } else {
      s->rotate(fixed_c::hundredth * 5 - (fixed(3) / 2000) * n);
    }
    for (const auto& t : s->shapes()) {
      t->rotate(-fixed_c::tenth);
    }

    s = (CompoundShape*) shapes()[16 + n].get();
    if (n % 2) {
      s->rotate(fixed_c::hundredth * 3 + (fixed(3) / 2000) * n);
    } else {
      s->rotate(fixed_c::hundredth * 4 - (fixed(3) / 2000) * n);
    }
    for (const auto& t : s->shapes()) {
      t->rotate(-fixed_c::tenth);
    }
  }

  if (_start_time) {
    _start_time--;
    return;
  }

  if (!_attack_time) {
    _timer++;
    if (_timer == 9 * GB_TIMER / 10 ||
        (_timer >= GB_TIMER / 10 && _timer < 9 * GB_TIMER / 10 - 16 &&
         ((_timer % 16 == 0 && _attack == 2) || (_timer % 32 == 0 && _shot_type)))) {
      for (int32_t i = 0; i < 8; i++) {
        vec2 d = vec2::from_polar(i * fixed_c::pi / 4 + shape().rotation(), 5);
        spawn(new BossShot(shape().centre, d, 0xcc66ffff));
      }
      play_sound_random(Lib::SOUND_BOSS_FIRE);
    }
    if (_timer != 9 * GB_TIMER / 10 && _timer >= GB_TIMER / 10 && _timer < 9 * GB_TIMER / 10 - 16 &&
        _timer % 16 == 0 && (!_shot_type || _attack == 2)) {
      Player* p = nearest_player();
      vec2 d = (p->shape().centre - shape().centre).normalised();

      if (d.length() > fixed_c::half) {
        spawn(new BossShot(shape().centre, d * 5, 0xcc66ffff));
        spawn(new BossShot(shape().centre, d * -5, 0xcc66ffff));
        play_sound_random(Lib::SOUND_BOSS_FIRE);
      }
    }
  } else {
    _attack_time--;

    if (_attack == 2) {
      if (_attack_time >= GB_ATTACK_TIME * 4 && _attack_time % 8 == 0) {
        vec2 pos(z::rand_int(Lib::WIDTH + 1), z::rand_int(Lib::HEIGHT + 1));
        spawn(new GhostMine(pos, this));
        play_sound_random(Lib::SOUND_ENEMY_SPAWN);
        _danger_circle = 0;
      } else if (_attack_time == GB_ATTACK_TIME * 4 - 1) {
        _visible = true;
        _vtime = 60;
        add_shape(new Box(vec2(Lib::WIDTH / 2 + 32, 0), Lib::WIDTH / 2, 10, 0xcc66ffff, 0, 0));
        add_shape(new Box(vec2(Lib::WIDTH / 2 + 32, 0), Lib::WIDTH / 2, 7, 0xcc66ffff, 0, 0));
        add_shape(new Box(vec2(Lib::WIDTH / 2 + 32, 0), Lib::WIDTH / 2, 4, 0xcc66ffff, 0, 0));

        add_shape(new Box(vec2(-Lib::WIDTH / 2 - 32, 0), Lib::WIDTH / 2, 10, 0xcc66ffff, 0, 0));
        add_shape(new Box(vec2(-Lib::WIDTH / 2 - 32, 0), Lib::WIDTH / 2, 7, 0xcc66ffff, 0, 0));
        add_shape(new Box(vec2(-Lib::WIDTH / 2 - 32, 0), Lib::WIDTH / 2, 4, 0xcc66ffff, 0, 0));
        play_sound(Lib::SOUND_BOSS_FIRE);
      } else if (_attack_time < GB_ATTACK_TIME * 3) {
        if (_attack_time == GB_ATTACK_TIME * 3 - 1) {
          play_sound(Lib::SOUND_BOSS_ATTACK);
        }
        shape().rotate((_rDir ? 1 : -1) * 2 * fixed_c::pi / (GB_ATTACK_TIME * 6));
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

    if (_attack == 0 && _attack_time == GB_ATTACK_TIME * 2) {
      bool r = z::rand_int(2) != 0;
      spawn(new GhostWall(r, true));
      spawn(new GhostWall(!r, false));
    }
    if (_attack == 1 && _attack_time == GB_ATTACK_TIME * 2) {
      _rDir = z::rand_int(2) != 0;
      spawn(new GhostWall(!_rDir, false, 0));
    }
    if (_attack == 1 && _attack_time == GB_ATTACK_TIME * 1) {
      spawn(new GhostWall(_rDir, true, 0));
    }

    if ((_attack == 0 || _attack == 1) && is_hp_low() && _attack_time == GB_ATTACK_TIME * 1) {
      int32_t r = z::rand_int(4);
      vec2 v = r == 0 ? vec2(-Lib::WIDTH / 4, Lib::HEIGHT / 2) : r == 1
              ? vec2(Lib::WIDTH + Lib::WIDTH / 4, Lib::HEIGHT / 2)
              : r == 2 ? vec2(Lib::WIDTH / 2, -Lib::HEIGHT / 4)
                       : vec2(Lib::WIDTH / 2, Lib::HEIGHT + Lib::HEIGHT / 4);
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
    } else {
      _danger_circle = 0;
    }
  }

  if (_timer >= GB_TIMER) {
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
      _attack_time = GB_ATTACK_TIME * 2 + 50;
    }
    if (_attack == 1) {
      _attack_time = GB_ATTACK_TIME * 3;
      for (std::size_t i = 0; i < game().players().size(); i++) {
        spawn(new Powerup(shape().centre, Powerup::SHIELD));
      }
    }
    if (_attack == 2) {
      _attack_time = GB_ATTACK_TIME * 6;
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

void GhostBoss::render() const {
  if ((_start_time / 4) % 2 == 1) {
    render_with_colour(1);
    return;
  }
  if ((_visible && ((_vtime / 4) % 2 == 0)) || (!_visible && ((_vtime / 4) % 2 == 1))) {
    Boss::render();
    return;
  }

  render_with_colour(0x330066ff);
  if (!_start_time) {
    render_hp_bar();
  }
}

int32_t GhostBoss::get_damage(int32_t damage, bool magic) {
  return damage;
}

GhostWall::GhostWall(bool swap, bool no_gap, bool ignored)
: Enemy(vec2(Lib::WIDTH / 2, swap ? -10 : 10 + Lib::HEIGHT), SHIP_NONE, 0), _dir(0, swap ? 1 : -1) {
  if (no_gap) {
    add_shape(new Box(vec2(), fixed(Lib::WIDTH), 10, 0xcc66ffff, 0, DANGEROUS | SHIELD));
    add_shape(new Box(vec2(), fixed(Lib::WIDTH), 7, 0xcc66ffff, 0, 0));
    add_shape(new Box(vec2(), fixed(Lib::WIDTH), 4, 0xcc66ffff, 0, 0));
  } else {
    add_shape(new Box(vec2(), Lib::HEIGHT / 2, 10, 0xcc66ffff, 0, DANGEROUS | SHIELD));
    add_shape(new Box(vec2(), Lib::HEIGHT / 2, 7, 0xcc66ffff, 0, DANGEROUS | SHIELD));
    add_shape(new Box(vec2(), Lib::HEIGHT / 2, 4, 0xcc66ffff, 0, DANGEROUS | SHIELD));
  }
  set_bounding_width(fixed(Lib::WIDTH));
}

GhostWall::GhostWall(bool swap, bool swap_gap)
: Enemy(vec2(swap ? -10 : 10 + Lib::WIDTH, Lib::HEIGHT / 2), SHIP_NONE, 0), _dir(swap ? 1 : -1, 0) {
  set_bounding_width(fixed(Lib::HEIGHT));
  fixed off = swap_gap ? -100 : 100;

  add_shape(new Box(vec2(0, off - 20 - Lib::HEIGHT / 2), 10, Lib::HEIGHT / 2, 0xcc66ffff, 0,
                    DANGEROUS | SHIELD));
  add_shape(new Box(vec2(0, off + 20 + Lib::HEIGHT / 2), 10, Lib::HEIGHT / 2, 0xcc66ffff, 0,
                    DANGEROUS | SHIELD));

  add_shape(new Box(vec2(0, off - 20 - Lib::HEIGHT / 2), 7, Lib::HEIGHT / 2, 0xcc66ffff, 0, 0));
  add_shape(new Box(vec2(0, off + 20 + Lib::HEIGHT / 2), 7, Lib::HEIGHT / 2, 0xcc66ffff, 0, 0));

  add_shape(new Box(vec2(0, off - 20 - Lib::HEIGHT / 2), 4, Lib::HEIGHT / 2, 0xcc66ffff, 0, 0));
  add_shape(new Box(vec2(0, off + 20 + Lib::HEIGHT / 2), 4, Lib::HEIGHT / 2, 0xcc66ffff, 0, 0));
}

void GhostWall::update() {
  if ((_dir.x > 0 && shape().centre.x < 32) || (_dir.y > 0 && shape().centre.y < 16) ||
      (_dir.x < 0 && shape().centre.x >= Lib::WIDTH - 32) ||
      (_dir.y < 0 && shape().centre.y >= Lib::HEIGHT - 16)) {
    move(_dir * GB_WALL_SPEED / 2);
  } else {
    move(_dir * GB_WALL_SPEED);
  }

  if ((_dir.x > 0 && shape().centre.x > Lib::WIDTH + 10) ||
      (_dir.y > 0 && shape().centre.y > Lib::HEIGHT + 10) ||
      (_dir.x < 0 && shape().centre.x < -10) || (_dir.y < 0 && shape().centre.y < -10)) {
    destroy();
  }
}

GhostMine::GhostMine(const vec2& position, Boss* ghost)
: Enemy(position, SHIP_NONE, 0), _timer(80), _ghost(ghost) {
  add_shape(new Polygon(vec2(), 24, 8, 0x9933ccff, 0, 0));
  add_shape(new Polygon(vec2(), 20, 8, 0x9933ccff, 0, 0));
  set_bounding_width(24);
  set_score(0);
}

void GhostMine::update() {
  if (_timer == 80) {
    explosion();
    shape().set_rotation(z::rand_fixed() * 2 * fixed_c::pi);
  }
  if (_timer) {
    _timer--;
    if (!_timer) {
      shapes()[0]->category = DANGEROUS | SHIELD | VULNSHIELD;
    }
  }
  for (const auto& ship : game().collision_list(shape().centre, DANGEROUS)) {
    if (ship == _ghost) {
      Enemy* e = z::rand_int(6) == 0 || (_ghost->is_hp_low() && z::rand_int(5) == 0)
          ? new BigFollow(shape().centre, false)
          : new Follow(shape().centre);
      e->set_score(0);
      spawn(e);
      damage(1, false, 0);
      break;
    }
  }
}

void GhostMine::render() const {
  if (!((_timer / 4) % 2)) {
    Enemy::render();
  }
}
