#include "game/logic/player/player.h"
#include "game/logic/ship/enums.h"
#include "game/logic/ship/geometry.h"
#include "game/logic/ship/ship_template.h"
#include <algorithm>
#include <utility>

namespace ii {
namespace {
vec2 get_centre(const Transform* transform, const LegacyShip* legacy_ship) {
  if (transform) {
    return transform->centre;
  }
  return legacy_ship->ship->position();
}

struct Shot : ecs::component {
  static constexpr fixed kSpeed = 10;

  using shape = standard_transform<geom::box_shape<2, 2, glm::vec4{1.f}>,
                                   geom::box_shape<1, 1, glm::vec4{.2f}>,
                                   geom::box_shape<3, 3, glm::vec4{.2f}>>;

  ecs::handle player;
  vec2 velocity{0};
  bool magic = false;

  Shot(ecs::handle player, const vec2& direction, bool magic)
  : player{std::move(player)}, velocity{normalise(direction) * kSpeed}, magic{magic} {}

  void update(ecs::handle h, Transform& transform, Render& render, SimInterface& sim) const {
    render.colour_override = magic && sim.random(2)
        ? glm::vec4{1.f}
        : SimInterface::player_colour(player.get<Player>()->player_number);
    if (sim.conditions().mode == game_mode::kWhat) {
      render.colour_override = glm::vec4{0.f};
    }
    transform.move(velocity);
    bool on_screen = all(greaterThanEqual(transform.centre, vec2{-4, -4})) &&
        all(lessThan(transform.centre, vec2{4 + kSimDimensions.x, 4 + kSimDimensions.y}));
    if (!on_screen) {
      h.emplace<Destroy>();
      return;
    }

    for (const auto& ship : sim.collision_list(transform.centre, shape_flag::kVulnerable)) {
      ecs::call_if<&Health::damage>(ship->handle(), sim, 1,
                                    magic ? damage_type::kMagic : damage_type::kNone, player.id());
      if (!magic) {
        h.emplace<Destroy>();
      }
    }

    if (sim.any_collision(transform.centre, shape_flag::kShield) ||
        (!magic && sim.any_collision(transform.centre, shape_flag::kWeakShield))) {
      h.emplace<Destroy>();
    }
  }
};

void spawn_shot(SimInterface& sim, const vec2& position, ecs::handle player, const vec2& direction,
                bool magic) {
  create_ship<Shot>(sim, position).add(Shot{player, direction, magic});
}

struct Powerup : ecs::component {
  static constexpr fixed kSpeed = 1;
  static constexpr std::uint32_t kRotateTime = 100;

  powerup_type type = powerup_type::kExtraLife;
  std::uint32_t frame = 0;
  vec2 dir = {0, 1};
  bool rotate = false;
  bool first_frame = true;

  Powerup(powerup_type type) : type{type} {}

  void update(ecs::handle h, Transform& transform, SimInterface& sim) {
    ++frame;

    if (!sim.is_on_screen(transform.centre)) {
      dir = kSimDimensions / 2_fx - transform.centre;
    } else {
      if (first_frame) {
        dir = from_polar(sim.random_fixed() * 2 * fixed_c::pi, 1_fx);
      }

      dir = ::rotate(dir, 2 * fixed_c::hundredth * (rotate ? 1 : -1));
      rotate = sim.random(kRotateTime) ? rotate : !rotate;
    }
    first_frame = false;

    auto ph = sim.nearest_player(transform.centre);
    auto pv = ph.get<Transform>()->centre - transform.centre;
    bool is_killed = ph.get<Player>()->is_killed();
    if (length(pv) <= 40 && !is_killed) {
      dir = pv;
    }
    dir = normalise(dir);

    transform.move(dir * kSpeed * ((length(pv) <= 40) ? 3 : 1));
    if (length(pv) <= 10 && !is_killed) {
      collect(h, transform, sim, ph);
    }
  }

  void
  collect(ecs::handle h, const Transform& transform, SimInterface& sim, ecs::handle source) const {
    auto* player = static_cast<::Player*>(source.get<LegacyShip>()->ship.get());
    switch (type) {
    case ii::powerup_type::kExtraLife:
      sim.add_life();
      break;

    case ii::powerup_type::kMagicShots:
      player->activate_magic_shots();
      break;

    case ii::powerup_type::kShield:
      player->activate_magic_shield();
      break;

    case ii::powerup_type::kBomb:
      player->activate_bomb();
      break;
    }
    sim.play_sound(
        type == ii::powerup_type::kExtraLife ? ii::sound::kPowerupLife : ii::sound::kPowerupOther,
        transform.centre);
    sim.rumble(source.get<Player>()->player_number, 6);

    auto r = 5 + sim.random(5);
    for (std::uint32_t i = 0; i < r; ++i) {
      vec2 dir = from_polar(sim.random_fixed() * 2 * fixed_c::pi, 6_fx);
      sim.add_particle(
          {to_float(transform.centre), glm::vec4{1.f}, to_float(dir), 4 + sim.random(8)});
    }
    h.emplace<Destroy>();
  }

  void render(const Transform& transform, const SimInterface& sim) const {
    // TODO: really need a better way of doing dynamic shape-type parameters.
    using out0 = standard_transform<geom::ngon_shape<13, 5, glm::vec4{1.f}>>;
    using out1 = standard_transform<geom::ngon_shape<9, 5, glm::vec4{1.f}>>;
    using extra_life = standard_transform<geom::ngon_shape<8, 3, glm::vec4{1.f}>>;
    using magic_shots = geom::translate_p<0, geom::box_shape<3, 3, glm::vec4{1.f}>>;
    using shield = standard_transform<geom::ngon_shape<11, 5, glm::vec4{1.f}>>;
    using bomb =
        standard_transform<geom::ngon_shape<11, 10, glm::vec4{1.f}, geom::ngon_style::kPolystar>>;

    auto c0 = SimInterface::player_colour((frame % (2 * kMaxPlayers)) / 2);
    auto c1 = SimInterface::player_colour(((frame + 1) % (2 * kMaxPlayers)) / 2);
    std::tuple parameters{transform.centre, fixed_c::pi / 2};

    render_shape<out0>(sim, parameters, c0);
    render_shape<out1>(sim, parameters, c1);
    switch (type) {
    case powerup_type::kExtraLife:
      render_shape<extra_life>(sim, parameters);
      break;
    case powerup_type::kMagicShots:
      render_shape<magic_shots>(sim, parameters);
      break;
    case powerup_type::kShield:
      render_shape<shield>(sim, parameters);
      break;
    case powerup_type::kBomb:
      render_shape<bomb>(sim, parameters);
      break;
    }
  }
};

}  // namespace

void spawn_powerup(SimInterface& sim, const vec2& position, powerup_type type) {
  auto h = sim.index().create();
  h.add(LegacyShip{.ship = std::make_unique<ShipForwarder>(sim, h)});
  h.add(Update{.update = ecs::call<&Powerup::update>});
  h.add(Render{.render = ecs::call<&Powerup::render>});
  h.add(Transform{.centre = position});
  h.add(Powerup{type});
  h.add(PowerupTag{});
}

}  // namespace ii

namespace {
const fixed kPlayerSpeed = 5;
const fixed kBombRadius = 180;
const fixed kBombBossRadius = 280;

const std::uint32_t kReviveTime = 100;
const std::uint32_t kShieldTime = 50;
const std::uint32_t kShotTimer = 4;
const std::uint32_t kMagicShotCount = 120;
}  // namespace

namespace ii {
void spawn_player(SimInterface& sim, const vec2& position, std::uint32_t player_number) {
  auto h = sim.create_legacy(std::make_unique<::Player>(sim, position, player_number));
  h.add(Player{.player_number = player_number, .invulnerability_timer = kReviveTime});
  h.add(Transform{.centre = position});
}
}  // namespace ii

ii::SimInterface::ship_list Player::kill_queue_;

Player::Player(ii::SimInterface& sim, const vec2& position, std::uint32_t player_number)
: ii::Ship{sim, position} {
  auto c = ii::SimInterface::player_colour(player_number);
  auto c_dark = c;
  c_dark.a = .2f;
  add_new_shape<ii::Polygon>(vec2{0}, 16, 3, c);
  add_new_shape<ii::Fill>(vec2{8, 0}, 2, 2, c);
  add_new_shape<ii::Fill>(vec2{8, 0}, 1, 1, c_dark);
  add_new_shape<ii::Fill>(vec2{8, 0}, 3, 3, c_dark);
  add_new_shape<ii::Polygon>(vec2{0}, 8, 3, c, fixed_c::pi);
  kill_queue_.clear();
}

void Player::update() {
  auto& pc = *handle().get<ii::Player>();
  auto& transform = *handle().get<ii::Transform>();
  auto input = sim().input(pc.player_number);
  if (input.target_absolute) {
    pc.fire_target = *input.target_absolute;
  } else if (input.target_relative) {
    pc.fire_target = transform.centre + *input.target_relative;
  }
  pc.fire_timer = (pc.fire_timer + 1) % kShotTimer;

  // Temporary death.
  if (pc.kill_timer > 1 && --pc.kill_timer) {
    return;
  }

  if (pc.kill_timer) {
    if (sim().get_lives() && kill_queue_[0] == this) {
      sim().sub_life();
      pc.kill_timer = 0;
      kill_queue_.erase(kill_queue_.begin());
      pc.invulnerability_timer = kReviveTime;
      shape().centre = transform.centre = {
          (1 + pc.player_number) * ii::kSimDimensions.x / (1 + sim().player_count()),
          ii::kSimDimensions.y / 2};
      sim().rumble(pc.player_number, 10);
      play_sound(ii::sound::kPlayerRespawn);
    }
    return;
  }
  pc.invulnerability_timer && --pc.invulnerability_timer;

  // Movement.
  if (length(input.velocity) > fixed_c::hundredth) {
    auto v = length(input.velocity) > 1 ? normalise(input.velocity) : input.velocity;
    transform.set_rotation(angle(v));
    shape().set_rotation(angle(v));
    shape().centre = transform.centre = max(
        vec2{0},
        min(vec2{ii::kSimDimensions.x, ii::kSimDimensions.y}, kPlayerSpeed * v + transform.centre));
  }

  // Bombs.
  if (pc.has_bomb && input.keys & ii::input_frame::kBomb) {
    auto c = ii::SimInterface::player_colour(pc.player_number);
    pc.has_bomb = false;
    destroy_shape(5);

    explosion(glm::vec4{1.f}, 16);
    explosion(c, 32);
    explosion(glm::vec4{1.f}, 48);

    vec2 t = shape().centre;
    for (std::uint32_t i = 0; i < 64; ++i) {
      shape().centre = t + from_polar(2 * i * fixed_c::pi / 64, kBombRadius);
      explosion((i % 2) ? c : glm::vec4{1.f}, 8 + sim().random(8) + sim().random(8), true,
                to_float(t));
    }
    shape().centre = t;

    sim().rumble(pc.player_number, 10);
    play_sound(ii::sound::kExplosion);

    sim().index().iterate_dispatch_if<ii::Enemy>(
        [&](ii::ecs::handle h, const ii::Enemy& e, ii::Health& health) {
          if (!h.has<ii::LegacyShip>() && !h.has<ii::Transform>()) {
            return;
          }
          auto centre = ii::ecs::call<&ii::get_centre>(h);
          if (length(centre - transform.centre) > kBombBossRadius) {
            return;
          }
          if (h.has<ii::Boss>() || length(centre - transform.centre) <= kBombRadius) {
            health.damage(h, sim(), kBombDamage, ii::damage_type::kBomb, std::nullopt);
          }
          if (!h.has<ii::Boss>() && e.score_reward) {
            pc.add_score(sim(), 0);
          }
        },
        /* include_new */ false);
  }

  // Shots.
  auto shot = pc.fire_target - transform.centre;
  if (length(shot) > 0 && !pc.fire_timer && input.keys & ii::input_frame::kFire) {
    ii::spawn_shot(sim(), transform.centre, handle(), shot, pc.magic_shot_count != 0);
    pc.magic_shot_count && --pc.magic_shot_count;

    float volume = .5f * sim().random_fixed().to_float() + .5f;
    float pitch = (sim().random_fixed().to_float() - 1.f) / 12.f;
    float pan = 2.f * transform.centre.x.to_float() / ii::kSimDimensions.x - 1.f;
    sim().play_sound(ii::sound::kPlayerFire, volume, pan, pitch);
  }

  // Damage.
  if (sim().any_collision(transform.centre, ii::shape_flag::kDangerous)) {
    damage();
  }
}

void Player::render() const {
  const auto& pc = *handle().get<ii::Player>();
  auto c = ii::SimInterface::player_colour(pc.player_number);
  if (!pc.kill_timer &&
      (sim().conditions().mode != ii::game_mode::kWhat || pc.invulnerability_timer > 0)) {
    auto t = to_float(pc.fire_target);
    sim().render_line(t + glm::vec2{0, 9}, t - glm::vec2{0, 8}, c);
    sim().render_line(t + glm::vec2{9, 1}, t - glm::vec2{8, -1}, c);
    if (pc.invulnerability_timer % 2) {
      render_with_colour(glm::vec4{1.f});
    } else {
      Ship::render();
    }
  }

  if (sim().conditions().mode != ii::game_mode::kBoss) {
    auto& pc = *handle().get<ii::Player>();
    sim().render_player_info(pc.player_number, c, pc.score, pc.multiplier,
                             static_cast<float>(pc.magic_shot_count) / kMagicShotCount);
  }
}

void Player::damage() {
  auto& pc = *handle().get<ii::Player>();
  if (pc.kill_timer || pc.invulnerability_timer) {
    return;
  }

  if (pc.has_shield) {
    sim().rumble(pc.player_number, 10);
    play_sound(ii::sound::kPlayerShield);
    destroy_shape(5);
    pc.has_shield = false;
    pc.invulnerability_timer = kShieldTime;
    return;
  }

  explosion();
  explosion(glm::vec4{1.f}, 14);
  explosion(std::nullopt, 20);

  pc.magic_shot_count = 0;
  pc.multiplier = 1;
  pc.multiplier_count = 0;
  pc.kill_timer = kReviveTime;
  ++pc.death_count;
  if (pc.has_shield || pc.has_bomb) {
    destroy_shape(5);
    pc.has_shield = false;
    pc.has_bomb = false;
  }
  kill_queue_.push_back(this);
  sim().rumble(pc.player_number, 25);
  play_sound(ii::sound::kPlayerDestroy);
}

void Player::activate_magic_shots() {
  handle().get<ii::Player>()->magic_shot_count = kMagicShotCount;
}

void Player::activate_magic_shield() {
  auto& pc = *handle().get<ii::Player>();
  if (pc.has_shield) {
    return;
  }

  if (pc.has_bomb) {
    destroy_shape(5);
    pc.has_bomb = false;
  }
  pc.has_shield = true;
  add_new_shape<ii::Polygon>(vec2{0}, 16, 10, glm::vec4{1.f});
}

void Player::activate_bomb() {
  auto& pc = *handle().get<ii::Player>();
  if (pc.has_bomb) {
    return;
  }

  if (pc.has_shield) {
    destroy_shape(5);
    pc.has_shield = false;
  }
  pc.has_bomb = true;
  add_new_shape<ii::Polygon>(vec2{-8, 0}, 6, 5, glm::vec4{1.f}, fixed_c::pi, ii::shape_flag::kNone,
                             ii::Polygon::T::kPolystar);
}
