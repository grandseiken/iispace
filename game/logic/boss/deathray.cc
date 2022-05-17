#include "game/logic/boss/deathray.h"
#include "game/logic/player.h"

namespace {
const std::int32_t kDrbBaseHp = 600;
const std::int32_t kDrbArmHp = 100;
const std::int32_t kDrbRayTimer = 100;
const std::int32_t kDrbTimer = 100;
const std::int32_t kDrbArmATimer = 300;
const std::int32_t kDrbArmRTimer = 400;

const fixed kDrbSpeed = 5;
const fixed kDrbArmSpeed = 4;
const fixed kDrbRaySpeed = 10;
}  // namespace

DeathRayBoss::DeathRayBoss(std::int32_t players, std::int32_t cycle)
: Boss(vec2(Lib::kWidth * (fixed(3) / 20), -Lib::kHeight), SimState::BOSS_2C, kDrbBaseHp, players,
       cycle)
, _timer(kDrbTimer * 2)
, _laser(false)
, _dir(true)
, _pos(1)
, _arm_timer(0)
, _shot_timer(0)
, _ray_attack_timer(0)
, _ray_src1()
, _ray_src2() {
  add_shape(new Polygon(vec2(), 110, 12, 0x228855ff, fixed_c::pi / 12, 0, Polygon::T::kPolystar));
  add_shape(new Polygon(vec2(), 70, 12, 0x33ff99ff, fixed_c::pi / 12, 0, Polygon::T::kPolygram));
  add_shape(new Polygon(vec2(), 120, 12, 0x33ff99ff, fixed_c::pi / 12, kDangerous | kVulnerable));
  add_shape(new Polygon(vec2(), 115, 12, 0x33ff99ff, fixed_c::pi / 12, 0));
  add_shape(new Polygon(vec2(), 110, 12, 0x33ff99ff, fixed_c::pi / 12, kShield));

  CompoundShape* s1 = new CompoundShape(vec2(), 0, kDangerous);
  for (std::int32_t i = 1; i < 12; i++) {
    CompoundShape* s2 = new CompoundShape(vec2(), i * fixed_c::pi / 6, 0);
    s2->add_shape(new Box(vec2(130, 0), 10, 24, 0x33ff99ff, 0, 0));
    s2->add_shape(new Box(vec2(130, 0), 8, 22, 0x228855ff, 0, 0));
    s1->add_shape(s2);
  }
  add_shape(s1);

  set_ignore_damage_colour_index(5);
  shape().rotate(2 * fixed_c::pi * fixed(z::rand_int()) / z::rand_max);
}

void DeathRayBoss::update() {
  bool positioned = true;
  fixed d = _pos == 0 ? 1 * Lib::kHeight / 4 - shape().centre.y
      : _pos == 1     ? 2 * Lib::kHeight / 4 - shape().centre.y
      : _pos == 2     ? 3 * Lib::kHeight / 4 - shape().centre.y
      : _pos == 3     ? 1 * Lib::kHeight / 8 - shape().centre.y
      : _pos == 4     ? 3 * Lib::kHeight / 8 - shape().centre.y
      : _pos == 5     ? 5 * Lib::kHeight / 8 - shape().centre.y
                      : 7 * Lib::kHeight / 8 - shape().centre.y;

  if (abs(d) > 3) {
    move(vec2(0, d / abs(d)) * kDrbSpeed);
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
      play_sound_random(Lib::sound::kBossAttack);
      explosion();
    }
  }

  bool going_fast = false;
  if (_laser) {
    if (_timer) {
      if (positioned) {
        _timer--;
      }

      if (_timer < kDrbTimer * 2 && !(_timer % 3)) {
        spawn(new DeathRay(shape().centre + vec2(100, 0)));
        play_sound_random(Lib::sound::kBossFire);
      }
      if (!_timer) {
        _laser = false;
        _timer = kDrbTimer * (2 + z::rand_int(3));
      }
    } else {
      fixed r = shape().rotation();
      if (r == 0) {
        _timer = kDrbTimer * 2;
      }

      if (r < fixed_c::tenth || r > 2 * fixed_c::pi - fixed_c::tenth) {
        shape().set_rotation(0);
      } else {
        going_fast = true;
        shape().rotate(_dir ? fixed_c::tenth : -fixed_c::tenth);
      }
    }
  } else {
    shape().rotate(_dir ? fixed_c::hundredth * 2 : -fixed_c::hundredth * 2);
    if (is_on_screen()) {
      _timer--;
      if (_timer % kDrbTimer == 0 && _timer != 0 && !z::rand_int(4)) {
        _dir = z::rand_int(2) != 0;
        _pos = z::rand_int(7);
      }
      if (_timer == kDrbTimer * 2 + 50 && _arms.size() == 2) {
        _ray_attack_timer = kDrbRayTimer;
        _ray_src1 = _arms[0]->shape().centre;
        _ray_src2 = _arms[1]->shape().centre;
        play_sound(Lib::sound::kEnemySpawn);
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
      _shot_queue.push_back(std::pair<std::int32_t, std::int32_t>((_shot_timer / 8) % 12, 16));
      play_sound_random(Lib::sound::kBossFire);
    }
  }
  if (_arms.empty() && is_hp_low()) {
    if (_shot_timer % 48 == 0) {
      for (std::int32_t i = 1; i < 16; i += 3) {
        _shot_queue.push_back(std::pair<std::int32_t, std::int32_t>(i, 16));
      }
      play_sound_random(Lib::sound::kBossFire);
    }
    if (_shot_timer % 48 == 16) {
      for (std::int32_t i = 2; i < 12; i += 3) {
        _shot_queue.push_back(std::pair<std::int32_t, std::int32_t>(i, 16));
      }
      play_sound_random(Lib::sound::kBossFire);
    }
    if (_shot_timer % 48 == 32) {
      for (std::int32_t i = 3; i < 12; i += 3) {
        _shot_queue.push_back(std::pair<std::int32_t, std::int32_t>(i, 16));
      }
      play_sound_random(Lib::sound::kBossFire);
    }
    if (_shot_timer % 128 == 0) {
      _ray_attack_timer = kDrbRayTimer;
      vec2 d1 = vec2::from_polar(z::rand_fixed() * 2 * fixed_c::pi, 110);
      vec2 d2 = vec2::from_polar(z::rand_fixed() * 2 * fixed_c::pi, 110);
      _ray_src1 = shape().centre + d1;
      _ray_src2 = shape().centre + d2;
      play_sound(Lib::sound::kEnemySpawn);
    }
  }
  if (_arms.empty()) {
    _arm_timer++;
    if (_arm_timer >= kDrbArmRTimer) {
      _arm_timer = 0;
      if (!is_hp_low()) {
        std::int32_t players =
            game().get_lives() ? game().players().size() : game().alive_players();
        std::int32_t hp =
            (kDrbArmHp * (7 * fixed_c::tenth + 3 * fixed_c::tenth * players)).to_int();
        _arms.push_back(new DeathArm(this, true, hp));
        _arms.push_back(new DeathArm(this, false, hp));
        play_sound(Lib::sound::kEnemySpawn);
        spawn(_arms[0]);
        spawn(_arms[1]);
      }
    }
  }

  for (std::size_t i = 0; i < _shot_queue.size(); ++i) {
    if (!going_fast || _shot_timer % 2) {
      std::int32_t n = _shot_queue[i].first;
      vec2 d = vec2(1, 0).rotated(shape().rotation() + n * fixed_c::pi / 6);
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

void DeathRayBoss::render() const {
  Boss::render();
  for (std::int32_t i = _ray_attack_timer - 8; i <= _ray_attack_timer + 8; i++) {
    if (i < 40 || i > kDrbRayTimer) {
      continue;
    }

    fvec2 pos = to_float(shape().centre);
    fvec2 d = to_float(_ray_src1) - pos;
    d *= float(i - 40) / float(kDrbRayTimer - 40);
    Polygon s(vec2(), 10, 6, 0x999999ff, 0, 0, Polygon::T::kPolystar);
    s.render(lib(), d + pos, 0);

    d = to_float(_ray_src2) - pos;
    d *= float(i - 40) / float(kDrbRayTimer - 40);
    Polygon s2(vec2(), 10, 6, 0x999999ff, 0, 0, Polygon::T::kPolystar);
    s2.render(lib(), d + pos, 0);
  }
}

std::int32_t DeathRayBoss::get_damage(std::int32_t damage, bool magic) {
  return _arms.size() ? 0 : damage;
}

void DeathRayBoss::on_arm_death(Ship* arm) {
  for (auto it = _arms.begin(); it != _arms.end(); ++it) {
    if (*it == arm) {
      _arms.erase(it);
      break;
    }
  }
}

DeathRay::DeathRay(const vec2& position) : Enemy(position, kShipNone, 0) {
  add_shape(new Box(vec2(), 10, 48, 0, 0, kDangerous));
  add_shape(new Line(vec2(), vec2(0, -48), vec2(0, 48), 0xffffffff, 0));
  set_bounding_width(48);
}

void DeathRay::update() {
  move(vec2(1, 0) * kDrbRaySpeed);
  if (shape().centre.x > Lib::kWidth + 20) {
    destroy();
  }
}

DeathArm::DeathArm(DeathRayBoss* boss, bool top, std::int32_t hp)
: Enemy(vec2(), kShipNone, hp)
, _boss(boss)
, _top(top)
, _timer(top ? 2 * kDrbArmATimer / 3 : 0)
, _attacking(false)
, _dir()
, _start(30)
, _shots(0) {
  add_shape(new Polygon(vec2(), 60, 4, 0x33ff99ff, 0, 0));
  add_shape(new Polygon(vec2(), 50, 4, 0x228855ff, 0, kVulnerable, Polygon::T::kPolygram));
  add_shape(new Polygon(vec2(), 40, 4, 0, 0, kShield));
  add_shape(new Polygon(vec2(), 20, 4, 0x33ff99ff, 0, 0));
  add_shape(new Polygon(vec2(), 18, 4, 0x228855ff, 0, 0));
  set_bounding_width(60);
  set_destroy_sound(Lib::sound::kPlayerDestroy);
}

void DeathArm::update() {
  if (_timer % (kDrbArmATimer / 2) == kDrbArmATimer / 4) {
    play_sound_random(Lib::sound::kBossFire);
    _target = nearest_player()->shape().centre;
    _shots = 16;
  }
  if (_shots > 0) {
    vec2 d = (_target - shape().centre).normalised() * 5;
    Ship* s = new BossShot(shape().centre, d, 0x33ff99ff);
    spawn(s);
    --_shots;
  }

  shape().rotate(fixed_c::hundredth * 5);
  if (_attacking) {
    _timer++;
    if (_timer < kDrbArmATimer / 4) {
      Player* p = nearest_player();
      vec2 d = p->shape().centre - shape().centre;
      if (d.length() != 0) {
        _dir = d.normalised();
        move(_dir * kDrbArmSpeed);
      }
    } else if (_timer < kDrbArmATimer / 2) {
      move(_dir * kDrbArmSpeed);
    } else if (_timer < kDrbArmATimer) {
      vec2 d = _boss->shape().centre + vec2(80, _top ? 80 : -80) - shape().centre;
      if (d.length() > kDrbArmSpeed) {
        move(d.normalised() * kDrbArmSpeed);
      } else {
        _attacking = false;
        _timer = 0;
      }
    } else if (_timer >= kDrbArmATimer) {
      _attacking = false;
      _timer = 0;
    }
    return;
  }

  _timer++;
  if (_timer >= kDrbArmATimer) {
    _timer = 0;
    _attacking = true;
    _dir = vec2();
    play_sound(Lib::sound::kBossAttack);
  }
  shape().centre = _boss->shape().centre + vec2(80, _top ? 80 : -80);

  if (_start) {
    if (_start == 30) {
      explosion();
      explosion(0xffffffff);
    }
    _start--;
    if (!_start) {
      shapes()[1]->category = kDangerous | kVulnerable;
    }
  }
}

void DeathArm::on_destroy(bool bomb) {
  _boss->on_arm_death(this);
  explosion();
  explosion(0xffffffff, 12);
  explosion(shapes()[0]->colour, 24);
}
