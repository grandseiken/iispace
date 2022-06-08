#include "game/logic/player.h"
#include "game/logic/enemy.h"
#include <algorithm>

namespace {
const fixed kPlayerSpeed = 5;
const fixed kShotSpeed = 10;
const fixed kBombRadius = 180;
const fixed kBombBossRadius = 280;
const fixed kPowerupSpeed = 1;

const std::uint32_t kReviveTime = 100;
const std::uint32_t kShieldTime = 50;
const std::uint32_t kShotTimer = 4;
const std::uint32_t kMagicShotCount = 120;
const std::uint32_t kPowerupRotateTime = 100;

class Shot : public ii::Ship {
public:
  Shot(ii::SimInterface& sim, const vec2& position, Player* player, const vec2& direction,
       bool magic = false);
  ~Shot() override {}

  void update() override;
  void render() const override;

private:
  Player* player_ = nullptr;
  vec2 velocity_;
  bool magic_ = false;
  bool flash_ = false;
};

class Powerup : public ii::Ship {
public:
  Powerup(ii::SimInterface& sim, const vec2& position, ii::powerup_type t);
  void update() override;
  void damage(std::uint32_t damage, bool magic, Player* source) override;

private:
  ii::powerup_type type_ = ii::powerup_type::kExtraLife;
  std::uint32_t frame_ = 0;
  vec2 dir_ = {0, 1};
  bool rotate_ = false;
  bool first_frame_ = true;
};

Shot::Shot(ii::SimInterface& sim, const vec2& position, Player* player, const vec2& direction,
           bool magic)
: ii::Ship{sim, position, kShipNone}, player_{player}, velocity_{direction}, magic_{magic} {
  velocity_ = normalise(velocity_) * kShotSpeed;
  auto c_dark = player_->colour();
  c_dark.a = .2f;
  add_new_shape<ii::Fill>(vec2{0}, 2, 2, player_->colour());
  add_new_shape<ii::Fill>(vec2{0}, 1, 1, c_dark);
  add_new_shape<ii::Fill>(vec2{0}, 3, 3, c_dark);
}

void Shot::render() const {
  if (sim().conditions().mode == ii::game_mode::kWhat) {
    return;
  }
  if (flash_) {
    render_with_colour(glm::vec4{1.f});
  } else {
    ii::Ship::render();
  }
}

void Shot::update() {
  flash_ = magic_ ? sim().random(2) != 0 : false;
  move(velocity_);
  bool on_screen = all(greaterThanEqual(shape().centre, vec2{-4, -4})) &&
      all(lessThan(shape().centre, vec2{4 + ii::kSimDimensions.x, 4 + ii::kSimDimensions.y}));
  if (!on_screen) {
    destroy();
    return;
  }

  for (const auto& ship : sim().collision_list(shape().centre, kVulnerable)) {
    ship->damage(1, magic_, player_);
    if (!magic_) {
      destroy();
    }
  }

  if (sim().any_collision(shape().centre, kShield) ||
      (!magic_ && sim().any_collision(shape().centre, kVulnShield))) {
    destroy();
  }
}

Powerup::Powerup(ii::SimInterface& sim, const vec2& position, ii::powerup_type t)
: ii::Ship{sim, position, kShipPowerup}, type_{t}, dir_{0, 1} {
  add_new_shape<ii::Polygon>(vec2{0}, 13, 5, glm::vec4{0.f}, fixed_c::pi / 2, 0);
  add_new_shape<ii::Polygon>(vec2{0}, 9, 5, glm::vec4{0.f}, fixed_c::pi / 2, 0);

  switch (type_) {
  case ii::powerup_type::kExtraLife:
    add_new_shape<ii::Polygon>(vec2{0}, 8, 3, glm::vec4{1.f}, fixed_c::pi / 2);
    break;

  case ii::powerup_type::kMagicShots:
    add_new_shape<ii::Fill>(vec2{0}, 3, 3, glm::vec4{1.f});
    break;

  case ii::powerup_type::kShield:
    add_new_shape<ii::Polygon>(vec2{0}, 11, 5, glm::vec4{1.f}, fixed_c::pi / 2);
    break;

  case ii::powerup_type::kBomb:
    add_new_shape<ii::Polygon>(vec2{0}, 11, 10, glm::vec4{1.f}, fixed_c::pi / 2, 0,
                               ii::Polygon::T::kPolystar);
    break;
  }
}

void Powerup::update() {
  shapes()[0]->colour = ii::SimInterface::player_colour(frame_ / 2);
  frame_ = (frame_ + 1) % (ii::kMaxPlayers * 2);
  shapes()[1]->colour = ii::SimInterface::player_colour(frame_ / 2);

  if (!is_on_screen()) {
    dir_ = get_screen_centre() - shape().centre;
  } else {
    if (first_frame_) {
      dir_ = from_polar(sim().random_fixed() * 2 * fixed_c::pi, 1_fx);
    }

    dir_ = rotate(dir_, 2 * fixed_c::hundredth * (rotate_ ? 1 : -1));
    rotate_ = sim().random(kPowerupRotateTime) ? rotate_ : !rotate_;
  }
  first_frame_ = false;

  Player* p = nearest_player();
  auto pv = p->shape().centre - shape().centre;
  if (length(pv) <= 40 && !p->is_killed()) {
    dir_ = pv;
  }
  dir_ = normalise(dir_);

  move(dir_ * kPowerupSpeed * ((length(pv) <= 40) ? 3 : 1));
  if (length(pv) <= 10 && !p->is_killed()) {
    damage(1, false, p);
  }
}

void Powerup::damage(std::uint32_t damage, bool magic, Player* source) {
  if (source) {
    switch (type_) {
    case ii::powerup_type::kExtraLife:
      sim().add_life();
      break;

    case ii::powerup_type::kMagicShots:
      source->activate_magic_shots();
      break;

    case ii::powerup_type::kShield:
      source->activate_magic_shield();
      break;

    case ii::powerup_type::kBomb:
      source->activate_bomb();
      break;
    }
    play_sound(type_ == ii::powerup_type::kExtraLife ? ii::sound::kPowerupLife
                                                     : ii::sound::kPowerupOther);
    sim().rumble(source->player_number(), 6);
  }

  auto r = 5 + sim().random(5);
  for (std::uint32_t i = 0; i < r; ++i) {
    vec2 dir = from_polar(sim().random_fixed() * 2 * fixed_c::pi, 6_fx);
    spawn(
        ii::particle{to_float(shape().centre), glm::vec4{1.f}, to_float(dir), 4 + sim().random(8)});
  }
  destroy();
}

void spawn_shot(ii::SimInterface& sim, const vec2& position, Player* player, const vec2& direction,
                bool magic) {
  sim.add_new_ship<Shot>(position, player, direction, magic);
}

}  // namespace

namespace ii {
Player* spawn_player(SimInterface& sim, const vec2& position, std::uint32_t player_number) {
  return sim.add_new_ship<Player>(position, player_number);
}

void spawn_powerup(SimInterface& sim, const vec2& position, powerup_type type) {
  sim.add_new_ship<Powerup>(position, type);
}
}  // namespace ii

ii::SimInterface::ship_list Player::kill_queue_;
std::uint32_t Player::fire_timer_;

Player::Player(ii::SimInterface& sim, const vec2& position, std::uint32_t player_number)
: ii::Ship{sim, position, kShipPlayer}
, player_number_{player_number}
, revive_timer_{kReviveTime}
, fire_target_{get_screen_centre()} {
  auto c_dark = colour();
  c_dark.a = .2f;
  add_new_shape<ii::Polygon>(vec2{0}, 16, 3, colour());
  add_new_shape<ii::Fill>(vec2{8, 0}, 2, 2, colour());
  add_new_shape<ii::Fill>(vec2{8, 0}, 1, 1, c_dark);
  add_new_shape<ii::Fill>(vec2{8, 0}, 3, 3, c_dark);
  add_new_shape<ii::Polygon>(vec2{0}, 8, 3, colour(), fixed_c::pi);
  kill_queue_.clear();
  fire_timer_ = 0;
}

Player::~Player() {}

void Player::update() {
  auto input = sim().input(player_number_);
  if (input.target_absolute) {
    fire_target_ = *input.target_absolute;
  } else if (input.target_relative) {
    fire_target_ = shape().centre + *input.target_relative;
  }

  // Temporary death.
  if (kill_timer_ > 1) {
    --kill_timer_;
    return;
  } else if (kill_timer_) {
    if (sim().get_lives() && kill_queue_[0] == this) {
      sim().sub_life();
      kill_timer_ = 0;
      kill_queue_.erase(kill_queue_.begin());
      revive_timer_ = kReviveTime;
      shape().centre = {(1 + player_number_) * ii::kSimDimensions.x / (1 + sim().player_count()),
                        ii::kSimDimensions.y / 2};
      sim().rumble(player_number_, 10);
      play_sound(ii::sound::kPlayerRespawn);
    }
    return;
  }
  if (revive_timer_) {
    --revive_timer_;
  }

  // Movement.
  if (length(input.velocity) > fixed_c::hundredth) {
    auto v = length(input.velocity) > 1 ? normalise(input.velocity) : input.velocity;
    shape().set_rotation(angle(v));
    shape().centre = max(
        vec2{0},
        min(vec2{ii::kSimDimensions.x, ii::kSimDimensions.y}, kPlayerSpeed * v + shape().centre));
  }

  // Bombs.
  if (bomb_ && input.keys & ii::input_frame::kBomb) {
    bomb_ = false;
    destroy_shape(5);

    explosion(glm::vec4{1.f}, 16);
    explosion(colour(), 32);
    explosion(glm::vec4{1.f}, 48);

    vec2 t = shape().centre;
    for (std::uint32_t i = 0; i < 64; ++i) {
      shape().centre = t + from_polar(2 * i * fixed_c::pi / 64, kBombRadius);
      explosion((i % 2) ? colour() : glm::vec4{1.f}, 8 + sim().random(8) + sim().random(8), true,
                to_float(t));
    }
    shape().centre = t;

    sim().rumble(player_number_, 10);
    play_sound(ii::sound::kExplosion);

    const auto& list = sim().ships_in_radius(shape().centre, kBombBossRadius, kShipEnemy);
    for (const auto& ship : list) {
      if ((ship->type() & kShipBoss) ||
          length(ship->shape().centre - shape().centre) <= kBombRadius) {
        ship->damage(kBombDamage, false, 0);
      }
      if (!(ship->type() & kShipBoss) && ((Enemy*)ship)->get_score() > 0) {
        add_score(0);
      }
    }
  }

  // Shots.
  auto shot = fire_target_ - shape().centre;
  if (length(shot) > 0 && !fire_timer_ && input.keys & ii::input_frame::kFire) {
    spawn_shot(sim(), shape().centre, this, shot, magic_shot_timer_ != 0);
    if (magic_shot_timer_) {
      --magic_shot_timer_;
    }

    float volume = .5f * sim().random_fixed().to_float() + .5f;
    float pitch = (sim().random_fixed().to_float() - 1.f) / 12.f;
    float pan = 2.f * shape().centre.x.to_float() / ii::kSimDimensions.x - 1.f;
    sim().play_sound(ii::sound::kPlayerFire, volume, pan, pitch);
  }

  // Damage.
  if (sim().any_collision(shape().centre, kDangerous)) {
    damage();
  }
}

void Player::render() const {
  if (!kill_timer_ && (sim().conditions().mode != ii::game_mode::kWhat || revive_timer_ > 0)) {
    auto t = to_float(fire_target_);
    sim().render_line(t + glm::vec2{0, 9}, t - glm::vec2{0, 8}, colour());
    sim().render_line(t + glm::vec2{9, 1}, t - glm::vec2{8, -1}, colour());
    if (revive_timer_ % 2) {
      render_with_colour(glm::vec4{1.f});
    } else {
      Ship::render();
    }
  }

  if (sim().conditions().mode != ii::game_mode::kBoss) {
    sim().render_player_info(player_number_, colour(), score_, multiplier_,
                             static_cast<float>(magic_shot_timer_) / kMagicShotCount);
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
    sim().rumble(player_number_, 10);
    play_sound(ii::sound::kPlayerShield);
    destroy_shape(5);
    shield_ = false;
    revive_timer_ = kShieldTime;
    return;
  }

  explosion();
  explosion(glm::vec4{1.f}, 14);
  explosion(std::nullopt, 20);

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
  sim().rumble(player_number_, 25);
  play_sound(ii::sound::kPlayerDestroy);
}

void Player::add_score(std::uint64_t score) {
  static const std::uint32_t multiplier_lookup[24] = {
      1,    2,    4,     8,     16,    32,     64,     128,    256,     512,     1024,    2048,
      4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288, 1048576, 2097152, 4194304, 8388608,
  };

  sim().rumble(player_number_, 3);
  score_ += score * multiplier_;
  ++multiplier_count_;
  if (multiplier_lookup[std::min(multiplier_ + 3, 23u)] <= multiplier_count_) {
    multiplier_count_ = 0;
    ++multiplier_;
  }
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
  add_new_shape<ii::Polygon>(vec2{0}, 16, 10, glm::vec4{1.f});
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
  add_new_shape<ii::Polygon>(vec2{-8, 0}, 6, 5, glm::vec4{1.f}, fixed_c::pi, 0,
                             ii::Polygon::T::kPolystar);
}

void Player::update_fire_timer() {
  fire_timer_ = (fire_timer_ + 1) % kShotTimer;
}
