#include "game/logic/player.h"
#include "game/core/lib.h"
#include "game/core/replay.h"
#include "game/core/z0_game.h"
#include "game/logic/enemy.h"
#include "game/logic/player_input.h"
#include <algorithm>

namespace {
const fixed kPlayerSpeed = 5;
const fixed kShotSpeed = 10;
const fixed kBombRadius = 180;
const fixed kBombBossRadius = 280;
const fixed kPowerupSpeed = 1;

const std::int32_t kReviveTime = 100;
const std::int32_t kShieldTime = 50;
const std::int32_t kShotTimer = 4;
const std::int32_t kMagicShotCount = 120;
const std::int32_t kPowerupRotateTime = 100;
}  // namespace

SimState::ship_list Player::kill_queue_;
SimState::ship_list Player::shot_sound_queue_;
std::int32_t Player::fire_timer_;

Player::Player(PlayerInput& input, const vec2& position, std::int32_t player_number)
: Ship{position, kShipPlayer}
, input_{input}
, player_number_{player_number}
, revive_timer_{kReviveTime}
, fire_target_{get_screen_centre()} {
  add_new_shape<Polygon>(vec2{}, 16, 3, colour());
  add_new_shape<Fill>(vec2{8, 0}, 2, 2, colour());
  add_new_shape<Fill>(vec2{8, 0}, 1, 1, colour() & 0xffffff33);
  add_new_shape<Fill>(vec2{8, 0}, 3, 3, colour() & 0xffffff33);
  add_new_shape<Polygon>(vec2{}, 8, 3, colour(), fixed_c::pi);
  kill_queue_.clear();
  shot_sound_queue_.clear();
  fire_timer_ = 0;
}

Player::~Player() {}

void Player::update() {
  vec2 velocity;
  std::int32_t keys = 0;
  input_.get(*this, velocity, fire_target_, keys);

  for (auto it = shot_sound_queue_.begin(); it != shot_sound_queue_.end(); ++it) {
    if ((!(keys & 1) || kill_timer_) && *it == this) {
      shot_sound_queue_.erase(it);
      break;
    }
  }

  // Temporary death.
  if (kill_timer_ > 1) {
    --kill_timer_;
    return;
  } else if (kill_timer_) {
    if (game().get_lives() && kill_queue_[0] == this) {
      game().sub_life();
      kill_timer_ = 0;
      kill_queue_.erase(kill_queue_.begin());
      revive_timer_ = kReviveTime;
      shape().centre = {(1 + player_number_) * Lib::kWidth / (1 + game().players().size()),
                        Lib::kHeight / 2};
      lib().rumble(player_number_, 10);
      play_sound(Lib::sound::kPlayerRespawn);
    }
    return;
  }
  if (revive_timer_) {
    --revive_timer_;
  }

  // Movement.
  if (velocity.length() > fixed_c::hundredth) {
    vec2 v = velocity.length() > 1 ? velocity.normalised() : velocity;
    shape().set_rotation(v.angle());
    shape().centre = std::max(
        vec2{}, std::min(vec2{Lib::kWidth, Lib::kHeight}, kPlayerSpeed * v + shape().centre));
  }

  // Bombs.
  if (bomb_ && keys & 2) {
    bomb_ = false;
    destroy_shape(5);

    explosion(0xffffffff, 16);
    explosion(colour(), 32);
    explosion(0xffffffff, 48);

    vec2 t = shape().centre;
    for (std::int32_t i = 0; i < 64; ++i) {
      shape().centre = t + vec2::from_polar(2 * i * fixed_c::pi / 64, kBombRadius);
      explosion((i % 2) ? colour() : 0xffffffff, 8 + z::rand_int(8) + z::rand_int(8), true,
                to_float(t));
    }
    shape().centre = t;

    lib().rumble(player_number_, 10);
    play_sound(Lib::sound::kExplosion);

    const auto& list = game().ships_in_radius(shape().centre, kBombBossRadius, kShipEnemy);
    for (const auto& ship : list) {
      if ((ship->type() & kShipBoss) ||
          (ship->shape().centre - shape().centre).length() <= kBombRadius) {
        ship->damage(kBombDamage, false, 0);
      }
      if (!(ship->type() & kShipBoss) && ((Enemy*)ship)->get_score() > 0) {
        add_score(0);
      }
    }
  }

  // Shots
  vec2 shot = fire_target_ - shape().centre;
  if (shot.length() > 0 && !fire_timer_ && keys & 1) {
    spawn_new<Shot>(shape().centre, this, shot, magic_shot_timer_ != 0);
    if (magic_shot_timer_) {
      --magic_shot_timer_;
    }

    bool could_play = false;
    // Avoid randomness errors due to sound timings.
    float volume = .5f * z::rand_fixed().to_float() + .5f;
    float pitch = (z::rand_fixed().to_float() - 1.f) / 12.f;
    if (shot_sound_queue_.empty() || shot_sound_queue_[0] == this) {
      could_play = lib().play_sound(Lib::sound::kPlayerFire, volume,
                                    2.f * shape().centre.x.to_float() / Lib::kWidth - 1.f, pitch);
      if (could_play && !shot_sound_queue_.empty()) {
        shot_sound_queue_.erase(shot_sound_queue_.begin());
      }
    }
    if (!could_play) {
      bool in_queue = false;
      for (const auto& ship : shot_sound_queue_) {
        if (ship == this) {
          in_queue = true;
        }
      }
      if (!in_queue) {
        shot_sound_queue_.push_back(this);
      }
    }
  }

  // Damage
  if (game().any_collision(shape().centre, kDangerous)) {
    damage();
  }
}

void Player::render() const {
  if (!kill_timer_ && (game().mode() != game_mode::kWhat || revive_timer_ > 0)) {
    auto t = to_float(fire_target_);
    if (t >= fvec2{} &&
        t <= fvec2{static_cast<float>(Lib::kWidth), static_cast<float>(Lib::kHeight)}) {
      lib().render_line(t + fvec2{0, 9}, t - fvec2{0, 8}, colour());
      lib().render_line(t + fvec2{9, 1}, t - fvec2{8, -1}, colour());
    }
    if (revive_timer_ % 2) {
      render_with_colour(0xffffffff);
    } else {
      Ship::render();
    }
  }

  if (game().mode() == game_mode::kBoss) {
    return;
  }

  std::stringstream ss;
  ss << multiplier_ << "X";
  std::string s = ss.str();
  std::int32_t n = player_number_;
  fvec2 v = n == 1 ? fvec2{Lib::kWidth / Lib::kTextWidth - 1.f - s.length(), 1.f}
      : n == 2     ? fvec2{1.f, Lib::kHeight / Lib::kTextHeight - 2.f}
      : n == 3     ? fvec2{Lib::kWidth / Lib::kTextWidth - 1.f - s.length(),
                       Lib::kHeight / Lib::kTextHeight - 2.f}
                   : fvec2{1.f, 1.f};
  lib().render_text(v, s, z0Game::kPanelText);

  ss.str("");
  n % 2 ? ss << score_ << "   " : ss << "   " << score_;
  lib().render_text(
      v - (n % 2 ? fvec2{static_cast<float>(ss.str().length() - s.length()), 0} : fvec2{}),
      ss.str(), colour());

  if (magic_shot_timer_ != 0) {
    v.x += n % 2 ? -1 : ss.str().length();
    v *= 16;
    lib().render_rect(v + fvec2{5.f, 11.f - (10 * magic_shot_timer_) / kMagicShotCount},
                      v + fvec2{9.f, 13.f}, 0xffffffff, 2);
  }
}

void Player::damage() {
  if (kill_timer_ || revive_timer_) {
    return;
  }
  for (const auto& player : kill_queue_) {
    if (player == this) {
      return;
    }
  }

  if (shield_) {
    lib().rumble(player_number_, 10);
    play_sound(Lib::sound::kPlayerShield);
    destroy_shape(5);
    shield_ = false;
    revive_timer_ = kShieldTime;
    return;
  }

  explosion();
  explosion(0xffffffff, 14);
  explosion(0, 20);

  magic_shot_timer_ = 0;
  multiplier_ = 1;
  multiplier_count_ = 0;
  kill_timer_ = kReviveTime;
  ++death_count_;
  if (shield_ || bomb_) {
    destroy_shape(5);
    shield_ = false;
    bomb_ = false;
  }
  kill_queue_.push_back(this);
  lib().rumble(player_number_, 25);
  play_sound(Lib::sound::kPlayerDestroy);
}

void Player::add_score(std::int64_t score) {
  static const std::int32_t multiplier_lookup[24] = {
      1,    2,    4,     8,     16,    32,     64,     128,    256,     512,     1024,    2048,
      4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288, 1048576, 2097152, 4194304, 8388608,
  };

  lib().rumble(player_number_, 3);
  score_ += score * multiplier_;
  ++multiplier_count_;
  if (multiplier_lookup[std::min(multiplier_ + 3, 23)] <= multiplier_count_) {
    multiplier_count_ = 0;
    ++multiplier_;
  }
}

colour_t Player::player_colour(std::size_t player_number) {
  return player_number == 0 ? 0xff0000ff
      : player_number == 1  ? 0xff5500ff
      : player_number == 2  ? 0xffaa00ff
      : player_number == 3  ? 0xffff00ff
                            : 0x00ff00ff;
}

void Player::activate_magic_shots() {
  magic_shot_timer_ = kMagicShotCount;
}

void Player::activate_magic_shield() {
  if (shield_) {
    return;
  }

  if (bomb_) {
    destroy_shape(5);
    bomb_ = false;
  }
  shield_ = true;
  add_new_shape<Polygon>(vec2{}, 16, 10, 0xffffffff);
}

void Player::activate_bomb() {
  if (bomb_) {
    return;
  }

  if (shield_) {
    destroy_shape(5);
    shield_ = false;
  }
  bomb_ = true;
  add_new_shape<Polygon>(vec2{-8, 0}, 6, 5, 0xffffffff, fixed_c::pi, 0, Polygon::T::kPolystar);
}

void Player::update_fire_timer() {
  fire_timer_ = (fire_timer_ + 1) % kShotTimer;
}

Shot::Shot(const vec2& position, Player* player, const vec2& direction, bool magic)
: Ship{position, kShipNone}, player_{player}, velocity_{direction}, magic_{magic} {
  velocity_ = velocity_.normalised() * kShotSpeed;
  add_new_shape<Fill>(vec2{}, 2, 2, player_->colour());
  add_new_shape<Fill>(vec2{}, 1, 1, player_->colour() & 0xffffff33);
  add_new_shape<Fill>(vec2{}, 3, 3, player_->colour() & 0xffffff33);
}

void Shot::render() const {
  if (game().mode() == game_mode::kWhat) {
    return;
  }
  if (flash_) {
    render_with_colour(0xffffffff);
  } else {
    Ship::render();
  }
}

void Shot::update() {
  flash_ = magic_ ? z::rand_int(2) != 0 : false;
  move(velocity_);
  bool on_screen =
      shape().centre >= vec2{-4, -4} && shape().centre < vec2{4 + Lib::kWidth, 4 + Lib::kHeight};
  if (!on_screen) {
    destroy();
    return;
  }

  for (const auto& ship : game().collision_list(shape().centre, kVulnerable)) {
    ship->damage(1, magic_, player_);
    if (!magic_) {
      destroy();
    }
  }

  if (game().any_collision(shape().centre, kShield) ||
      (!magic_ && game().any_collision(shape().centre, kVulnShield))) {
    destroy();
  }
}

Powerup::Powerup(const vec2& position, type t)
: Ship{position, kShipPowerup}, type_{t}, dir_{0, 1} {
  add_new_shape<Polygon>(vec2{}, 13, 5, 0, fixed_c::pi / 2, 0);
  add_new_shape<Polygon>(vec2{}, 9, 5, 0, fixed_c::pi / 2, 0);

  switch (type_) {
  case type::kExtraLife:
    add_new_shape<Polygon>(vec2{}, 8, 3, 0xffffffff, fixed_c::pi / 2);
    break;

  case type::kMagicShots:
    add_new_shape<Fill>(vec2{}, 3, 3, 0xffffffff);
    break;

  case type::kShield:
    add_new_shape<Polygon>(vec2{}, 11, 5, 0xffffffff, fixed_c::pi / 2);
    break;

  case type::kBomb:
    add_new_shape<Polygon>(vec2{}, 11, 10, 0xffffffff, fixed_c::pi / 2, 0, Polygon::T::kPolystar);
    break;
  }
}

void Powerup::update() {
  shapes()[0]->colour = Player::player_colour(frame_ / 2);
  frame_ = (frame_ + 1) % (kPlayers * 2);
  shapes()[1]->colour = Player::player_colour(frame_ / 2);

  if (!is_on_screen()) {
    dir_ = get_screen_centre() - shape().centre;
  } else {
    if (first_frame_) {
      dir_ = vec2::from_polar(z::rand_fixed() * 2 * fixed_c::pi, 1);
    }

    dir_ = dir_.rotated(2 * fixed_c::hundredth * (rotate_ ? 1 : -1));
    rotate_ = z::rand_int(kPowerupRotateTime) ? rotate_ : !rotate_;
  }
  first_frame_ = false;

  Player* p = nearest_player();
  vec2 pv = p->shape().centre - shape().centre;
  if (pv.length() <= 40 && !p->is_killed()) {
    dir_ = pv;
  }
  dir_ = dir_.normalised();

  move(dir_ * kPowerupSpeed * ((pv.length() <= 40) ? 3 : 1));
  if (pv.length() <= 10 && !p->is_killed()) {
    damage(1, false, p);
  }
}

void Powerup::damage(std::int32_t damage, bool magic, Player* source) {
  if (source) {
    switch (type_) {
    case type::kExtraLife:
      game().add_life();
      break;

    case type::kMagicShots:
      source->activate_magic_shots();
      break;

    case type::kShield:
      source->activate_magic_shield();
      break;

    case type::kBomb:
      source->activate_bomb();
      break;
    }
    play_sound(type_ == type::kExtraLife ? Lib::sound::kPowerupLife : Lib::sound::kPowerupOther);
    lib().rumble(source->player_number(), 6);
  }

  std::int32_t r = 5 + z::rand_int(5);
  for (std::int32_t i = 0; i < r; ++i) {
    vec2 dir = vec2::from_polar(z::rand_fixed() * 2 * fixed_c::pi, 6);
    spawn(Particle{to_float(shape().centre), 0xffffffff, to_float(dir), 4 + z::rand_int(8)});
  }
  destroy();
}
