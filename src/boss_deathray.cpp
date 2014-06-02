#include "boss_deathray.h"
#include "player.h"

static const int32_t DRB_BASE_HP = 600;
static const int32_t DRB_ARM_HP = 100;
static const int32_t DRB_RAY_TIMER = 100;
static const int32_t DRB_TIMER = 100;
static const int32_t DRB_ARM_ATIMER = 300;
static const int32_t DRB_ARM_RTIMER = 400;

static const fixed DRB_SPEED = 5;
static const fixed DRB_ARM_SPEED = 4;
static const fixed DRB_RAY_SPEED = 10;

DeathRayBoss::DeathRayBoss(int32_t players, int32_t cycle)
  : Boss(vec2(Lib::WIDTH * (fixed(3) / 20), -Lib::HEIGHT),
         GameModal::BOSS_2C, DRB_BASE_HP, players, cycle)
  , _timer(DRB_TIMER * 2)
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
    move(vec2(0, d / d.abs()) * DRB_SPEED);
    positioned = false;
  }

  if (_ray_attack_timer) {
    _ray_attack_timer--;
    if (_ray_attack_timer == 40) {
      _ray_dest = nearest_player()->shape().centre;
    }
    if (_ray_attack_timer < 40) {
      vec2 d = (_ray_dest - shape().centre).normalised();
      spawn(new BossShot(shape().centre, d * 10, 0xccccccff));
      play_sound_random(Lib::SOUND_BOSS_ATTACK);
      explosion();
    }
  }

  bool going_fast = false;
  if (_laser) {
    if (_timer) {
      if (positioned) {
        _timer--;
      }

      if (_timer < DRB_TIMER * 2 && !(_timer % 3)) {
        spawn(new DeathRay(shape().centre + vec2(100, 0)));
        play_sound_random(Lib::SOUND_BOSS_FIRE);
      }
      if (!_timer) {
        _laser = false;
        _timer = DRB_TIMER * (2 + z::rand_int(3));
      }
    }
    else {
      fixed r = shape().rotation();
      if (r == 0) {
        _timer = DRB_TIMER * 2;
      }

      if (r < fixed::tenth || r > 2 * fixed::pi - fixed::tenth) {
        shape().set_rotation(0);
      }
      else {
        going_fast = true;
        shape().rotate(_dir ? fixed::tenth : -fixed::tenth);
      }
    }
  }
  else {
    shape().rotate(_dir ? fixed::hundredth * 2 : -fixed::hundredth * 2);
    if (is_on_screen()) {
      _timer--;
      if (_timer % DRB_TIMER == 0 && _timer != 0 && !z::rand_int(4)) {
        _dir = z::rand_int(2) != 0;
        _pos = z::rand_int(7);
      }
      if (_timer == DRB_TIMER * 2 + 50 && _arms.size() == 2) {
        _ray_attack_timer = DRB_RAY_TIMER;
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
      _ray_attack_timer = DRB_RAY_TIMER;
      vec2 d1 = vec2::from_polar(z::rand_fixed() * 2 * fixed::pi, 110);
      vec2 d2 = vec2::from_polar(z::rand_fixed() * 2 * fixed::pi, 110);
      _ray_src1 = shape().centre + d1;
      _ray_src2 = shape().centre + d2;
      play_sound(Lib::SOUND_ENEMY_SPAWN);
    }
  }
  if (_arms.empty()) {
    _arm_timer++;
    if (_arm_timer >= DRB_ARM_RTIMER) {
      _arm_timer = 0;
      if (!is_hp_low()) {
        int32_t players = game().get_lives() ?
            game().players().size() : game().alive_players();
        int32_t hp = (DRB_ARM_HP *
                      (7 * fixed::tenth + 3 * fixed::tenth * players)).to_int();
        _arms.push_back(new DeathArm(this, true, hp));
        _arms.push_back(new DeathArm(this, false, hp));
        play_sound(Lib::SOUND_ENEMY_SPAWN);
        spawn(_arms[0]);
        spawn(_arms[1]);
      }
    }
  }

  for (std::size_t i = 0; i < _shot_queue.size(); ++i) {
    if (!going_fast || _shot_timer % 2) {
      int32_t n = _shot_queue[i].first;
      vec2 d = vec2(1, 0).rotated(shape().rotation() + n * fixed::pi / 6);
      Ship* s = new BossShot(shape().centre + d * 120, d * 5, 0x33ff99ff);
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
    if (i < 40 || i > DRB_RAY_TIMER) {
      continue;
    }

    flvec2 pos = to_float(shape().centre);
    flvec2 d = to_float(_ray_src1) - pos;
    d *= float(i - 40) / float(DRB_RAY_TIMER - 40);
    Polygon s(vec2(), 10, 6, 0x999999ff, 0, 0, Polygon::T::POLYSTAR);
    s.render(lib(), d + pos, 0);

    d = to_float(_ray_src2) - pos;
    d *= float(i - 40) / float(DRB_RAY_TIMER - 40);
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


DeathRay::DeathRay(const vec2& position)
  : Enemy(position, SHIP_NONE, 0)
{
  add_shape(new Box(vec2(), 10, 48, 0, 0, DANGEROUS));
  add_shape(new Line(vec2(), vec2(0, -48), vec2(0, 48), 0xffffffff, 0));
  set_bounding_width(48);
}

void DeathRay::update()
{
  move(vec2(1, 0) * DRB_RAY_SPEED);
  if (shape().centre.x > Lib::WIDTH + 20) {
    destroy();
  }
}

DeathArm::DeathArm(DeathRayBoss* boss, bool top, int32_t hp)
  : Enemy(vec2(), SHIP_NONE, hp)
  , _boss(boss)
  , _top(top)
  , _timer(top ? 2 * DRB_ARM_ATIMER / 3 : 0)
  , _attacking(false)
  , _dir()
  , _start(30)
  , _shots(0)
{
  add_shape(new Polygon(vec2(), 60, 4, 0x33ff99ff, 0, 0));
  add_shape(new Polygon(vec2(), 50, 4, 0x228855ff,
                        0, VULNERABLE, Polygon::T::POLYGRAM));
  add_shape(new Polygon(vec2(), 40, 4, 0, 0, SHIELD));
  add_shape(new Polygon(vec2(), 20, 4, 0x33ff99ff, 0, 0));
  add_shape(new Polygon(vec2(), 18, 4, 0x228855ff, 0, 0));
  set_bounding_width(60);
  set_destroy_sound(Lib::SOUND_PLAYER_DESTROY);
}

void DeathArm::update()
{
  if (_timer % (DRB_ARM_ATIMER / 2) == DRB_ARM_ATIMER / 4) {
    play_sound_random(Lib::SOUND_BOSS_FIRE);
    _target = nearest_player()->shape().centre;
    _shots = 16;
  }
  if (_shots > 0) {
    vec2 d = (_target - shape().centre).normalised() * 5;
    Ship* s = new BossShot(shape().centre, d, 0x33ff99ff);
    spawn(s);
    --_shots;
  }

  shape().rotate(fixed::hundredth * 5);
  if (_attacking) {
    _timer++;
    if (_timer < DRB_ARM_ATIMER / 4) {
      Player* p = nearest_player();
      vec2 d = p->shape().centre - shape().centre;
      if (d.length() != 0) {
        _dir = d.normalised();
        move(_dir * DRB_ARM_SPEED);
      }
    }
    else if (_timer < DRB_ARM_ATIMER / 2) {
      move(_dir * DRB_ARM_SPEED);
    }
    else if (_timer < DRB_ARM_ATIMER) {
      vec2 d = _boss->shape().centre +
          vec2(80, _top ? 80 : -80) - shape().centre;
      if (d.length() > DRB_ARM_SPEED) {
        move(d.normalised() * DRB_ARM_SPEED);
      }
      else {
        _attacking = false;
        _timer = 0;
      }
    }
    else if (_timer >= DRB_ARM_ATIMER) {
      _attacking = false;
      _timer = 0;
    }
    return;
  }

  _timer++;
  if (_timer >= DRB_ARM_ATIMER) {
    _timer = 0;
    _attacking = true;
    _dir = vec2();
    play_sound(Lib::SOUND_BOSS_ATTACK);
  }
  shape().centre = _boss->shape().centre + vec2(80, _top ? 80 : -80);

  if (_start) {
    if (_start == 30) {
      explosion();
      explosion(0xffffffff);
    }
    _start--;
    if (!_start) {
      shapes()[1]->category = DANGEROUS | VULNERABLE;
    }
  }
}

void DeathArm::on_destroy(bool bomb)
{
  _boss->on_arm_death(this);
  explosion();
  explosion(0xffffffff, 12);
  explosion(shapes()[0]->colour, 24);
}
