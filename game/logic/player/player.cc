#include "game/logic/player/player.h"
#include "game/logic/ship/geometry.h"
#include "game/logic/ship/ship_template.h"
#include <algorithm>

namespace ii {
namespace {

struct Shot : ecs::component {
  static constexpr fixed kSpeed = 10;

  Shot(::Player* player, const vec2& direction, bool magic)
  : player{player}, velocity{normalise(direction) * kSpeed}, magic{magic} {}
  ::Player* player = nullptr;
  vec2 velocity{0};
  bool magic = false;
  bool flash = false;

  void update(ecs::handle h, Transform& transform, SimInterface& sim) {
    flash = magic && sim.random(2);
    transform.move(velocity);
    bool on_screen = all(greaterThanEqual(transform.centre, vec2{-4, -4})) &&
        all(lessThan(transform.centre, vec2{4 + ii::kSimDimensions.x, 4 + ii::kSimDimensions.y}));
    if (!on_screen) {
      h.emplace<Destroy>();
      return;
    }

    for (const auto& ship : sim.collision_list(transform.centre, ii::shape_flag::kVulnerable)) {
      ship->damage(1, magic, player);
      if (!magic) {
        h.emplace<Destroy>();
      }
    }

    if (sim.any_collision(transform.centre, ii::shape_flag::kShield) ||
        (!magic && sim.any_collision(transform.centre, ii::shape_flag::kWeakShield))) {
      h.emplace<Destroy>();
    }
  }

  void render(const Transform& transform, const SimInterface& sim) const {
    if (sim.conditions().mode == ii::game_mode::kWhat) {
      return;
    }
    using shape = geom::translate_p<0, geom::box_shape<2, 2, glm::vec4{1.f}>,
                                    geom::box_shape<1, 1, glm::vec4{.2f}>,
                                    geom::box_shape<3, 3, glm::vec4{.2f}>>;
    geom::iterate(shape{}, geom::iterate_lines, std::tuple{transform.centre}, {},
                  [&](const vec2& a, const vec2& b, const glm::vec4& c) {
                    auto colour = flash ? glm::vec4{1.f} : player->colour();
                    colour.a = c.a;
                    sim.render_line(to_float(a), to_float(b), colour);
                  });
  }
};

void spawn_shot(ii::SimInterface& sim, const vec2& position, ::Player* player,
                const vec2& direction, bool magic) {
  auto h = sim.index().create();
  h.add(ShipFlags{.flags = ship_flag{0}});
  h.add(LegacyShip{.ship = std::make_unique<ShipForwarder>(sim, h)});
  h.add(Update{.update = ecs::call<&Shot::update>});
  h.add(Transform{.centre = position});
  h.add(Render{.render = ecs::call<&Shot::render>});
  h.add(Shot{player, direction, magic});
}

}  // namespace
}  // namespace ii

namespace {
const fixed kPlayerSpeed = 5;
const fixed kBombRadius = 180;
const fixed kBombBossRadius = 280;
const fixed kPowerupSpeed = 1;

const std::uint32_t kReviveTime = 100;
const std::uint32_t kShieldTime = 50;
const std::uint32_t kShotTimer = 4;
const std::uint32_t kMagicShotCount = 120;
const std::uint32_t kPowerupRotateTime = 100;

class Powerup : public ii::Ship {
public:
  Powerup(ii::SimInterface& sim, const vec2& position, ii::powerup_type t);
  void update() override;
  void damage(std::uint32_t damage, bool magic, IShip* source) override;

private:
  ii::powerup_type type_ = ii::powerup_type::kExtraLife;
  std::uint32_t frame_ = 0;
  vec2 dir_ = {0, 1};
  bool rotate_ = false;
  bool first_frame_ = true;
};

Powerup::Powerup(ii::SimInterface& sim, const vec2& position, ii::powerup_type t)
: ii::Ship{sim, position, ii::ship_flag::kPowerup}, type_{t}, dir_{0, 1} {
  add_new_shape<ii::Polygon>(vec2{0}, 13, 5, glm::vec4{0.f}, fixed_c::pi / 2);
  add_new_shape<ii::Polygon>(vec2{0}, 9, 5, glm::vec4{0.f}, fixed_c::pi / 2);

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
    add_new_shape<ii::Polygon>(vec2{0}, 11, 10, glm::vec4{1.f}, fixed_c::pi / 2,
                               ii::shape_flag::kNone, ii::Polygon::T::kPolystar);
    break;
  }
}

void Powerup::update() {
  shapes()[0]->colour = ii::SimInterface::player_colour(frame_ / 2);
  frame_ = (frame_ + 1) % (ii::kMaxPlayers * 2);
  shapes()[1]->colour = ii::SimInterface::player_colour(frame_ / 2);

  if (!is_on_screen()) {
    dir_ = ii::kSimDimensions / 2_fx - shape().centre;
  } else {
    if (first_frame_) {
      dir_ = from_polar(sim().random_fixed() * 2 * fixed_c::pi, 1_fx);
    }

    dir_ = rotate(dir_, 2 * fixed_c::hundredth * (rotate_ ? 1 : -1));
    rotate_ = sim().random(kPowerupRotateTime) ? rotate_ : !rotate_;
  }
  first_frame_ = false;

  auto* p = sim().nearest_player(shape().centre);
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

void Powerup::damage(std::uint32_t damage, bool magic, IShip* source) {
  if (source) {
    auto p = static_cast<Player*>(source->handle().get<ii::LegacyShip>()->ship.get());
    switch (type_) {
    case ii::powerup_type::kExtraLife:
      sim().add_life();
      break;

    case ii::powerup_type::kMagicShots:
      p->activate_magic_shots();
      break;

    case ii::powerup_type::kShield:
      p->activate_magic_shield();
      break;

    case ii::powerup_type::kBomb:
      p->activate_bomb();
      break;
    }
    play_sound(type_ == ii::powerup_type::kExtraLife ? ii::sound::kPowerupLife
                                                     : ii::sound::kPowerupOther);
    sim().rumble(p->player_number(), 6);
  }

  auto r = 5 + sim().random(5);
  for (std::uint32_t i = 0; i < r; ++i) {
    vec2 dir = from_polar(sim().random_fixed() * 2 * fixed_c::pi, 6_fx);
    spawn(
        ii::particle{to_float(shape().centre), glm::vec4{1.f}, to_float(dir), 4 + sim().random(8)});
  }
  destroy();
}

}  // namespace

namespace ii {
void Player::add_score(SimInterface& sim, std::uint64_t s) {
  static const std::uint32_t multiplier_lookup[24] = {
      1,    2,    4,     8,     16,    32,     64,     128,    256,     512,     1024,    2048,
      4096, 8192, 16384, 32768, 65536, 131072, 262144, 524288, 1048576, 2097152, 4194304, 8388608,
  };

  sim.rumble(player_number, 3);
  score += s * multiplier;
  ++multiplier_count;
  if (multiplier_lookup[std::min(multiplier + 3, 23u)] <= multiplier_count) {
    multiplier_count = 0;
    ++multiplier;
  }
}

void spawn_player(SimInterface& sim, const vec2& position, std::uint32_t player_number) {
  auto h = sim.create_legacy(std::make_unique<::Player>(sim, position, player_number));
  h.add(Player{.player_number = player_number});
}

void spawn_powerup(SimInterface& sim, const vec2& position, powerup_type type) {
  sim.create_legacy(std::make_unique<Powerup>(sim, position, type));
}
}  // namespace ii

ii::SimInterface::ship_list Player::kill_queue_;
std::uint32_t Player::fire_timer_;

Player::Player(ii::SimInterface& sim, const vec2& position, std::uint32_t player_number)
: ii::Ship{sim, position, ii::ship_flag::kPlayer}
, player_number_{player_number}
, revive_timer_{kReviveTime}
, fire_target_{ii::kSimDimensions / 2_fx} {
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
  auto& pc = *handle().get<ii::Player>();
  auto input = sim().input(player_number_);
  if (input.target_absolute) {
    fire_target_ = *input.target_absolute;
  } else if (input.target_relative) {
    fire_target_ = shape().centre + *input.target_relative;
  }

  // Temporary death.
  if (pc.kill_timer > 1) {
    --pc.kill_timer;
    return;
  } else if (pc.kill_timer) {
    if (sim().get_lives() && kill_queue_[0] == this) {
      sim().sub_life();
      pc.kill_timer = 0;
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

    const auto& list =
        sim().ships_in_radius(shape().centre, kBombBossRadius, ii::ship_flag::kEnemy);
    for (const auto& ship : list) {
      if (+(ship->type() & ii::ship_flag::kBoss) ||
          length(ship->position() - shape().centre) <= kBombRadius) {
        ship->damage(kBombDamage, false, 0);
      }
      const auto* enemy = ship->handle().get<ii::Enemy>();
      if (!(ship->type() & ii::ship_flag::kBoss) && enemy && enemy->score_reward) {
        pc.add_score(sim(), 0);
      }
    }
  }

  // Shots.
  auto shot = fire_target_ - shape().centre;
  if (length(shot) > 0 && !fire_timer_ && input.keys & ii::input_frame::kFire) {
    ii::spawn_shot(sim(), shape().centre, this, shot, magic_shot_timer_ != 0);
    if (magic_shot_timer_) {
      --magic_shot_timer_;
    }

    float volume = .5f * sim().random_fixed().to_float() + .5f;
    float pitch = (sim().random_fixed().to_float() - 1.f) / 12.f;
    float pan = 2.f * shape().centre.x.to_float() / ii::kSimDimensions.x - 1.f;
    sim().play_sound(ii::sound::kPlayerFire, volume, pan, pitch);
  }

  // Damage.
  if (sim().any_collision(shape().centre, ii::shape_flag::kDangerous)) {
    damage();
  }
}

void Player::render() const {
  const auto& pc = *handle().get<ii::Player>();
  if (!pc.kill_timer && (sim().conditions().mode != ii::game_mode::kWhat || revive_timer_ > 0)) {
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
    auto& pc = *handle().get<ii::Player>();
    sim().render_player_info(player_number_, colour(), pc.score, pc.multiplier,
                             static_cast<float>(magic_shot_timer_) / kMagicShotCount);
  }
}

void Player::damage() {
  auto& pc = *handle().get<ii::Player>();
  if (pc.kill_timer || revive_timer_) {
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
  pc.multiplier = 1;
  pc.multiplier_count = 0;
  pc.kill_timer = kReviveTime;
  ++pc.death_count;
  if (shield_ || bomb_) {
    destroy_shape(5);
    shield_ = false;
    bomb_ = false;
  }
  kill_queue_.push_back(this);
  sim().rumble(player_number_, 25);
  play_sound(ii::sound::kPlayerDestroy);
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
  add_new_shape<ii::Polygon>(vec2{-8, 0}, 6, 5, glm::vec4{1.f}, fixed_c::pi, ii::shape_flag::kNone,
                             ii::Polygon::T::kPolystar);
}

void Player::update_fire_timer() {
  fire_timer_ = (fire_timer_ + 1) % kShotTimer;
}
