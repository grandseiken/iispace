#include "game/logic/player/player.h"
#include "game/logic/ship/enums.h"
#include "game/logic/ship/geometry.h"
#include "game/logic/ship/ship_template.h"
#include <algorithm>
#include <utility>

namespace ii {
namespace {
constexpr std::uint32_t kMagicShotCount = 120;

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
        : player_colour(player.get<Player>()->player_number);
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
    auto& pc = *source.get<Player>();
    switch (type) {
    case powerup_type::kExtraLife:
      sim.add_life();
      break;

    case powerup_type::kMagicShots:
      pc.magic_shot_count = kMagicShotCount;
      break;

    case powerup_type::kShield:
      pc.has_shield = true;
      pc.has_bomb = false;
      break;

    case powerup_type::kBomb:
      pc.has_bomb = true;
      pc.has_shield = false;
      break;
    }
    sim.play_sound(type == powerup_type::kExtraLife ? sound::kPowerupLife : sound::kPowerupOther,
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
    // TODO: really need a better way of doing dynamic shape-type parameters (specifically colours).
    using out0 = standard_transform<geom::ngon_shape<13, 5, glm::vec4{1.f}>>;
    using out1 = standard_transform<geom::ngon_shape<9, 5, glm::vec4{1.f}>>;
    using extra_life = standard_transform<geom::ngon_shape<8, 3, glm::vec4{1.f}>>;
    using magic_shots = geom::translate_p<0, geom::box_shape<3, 3, glm::vec4{1.f}>>;
    using shield = standard_transform<geom::ngon_shape<11, 5, glm::vec4{1.f}>>;
    using bomb =
        standard_transform<geom::ngon_shape<11, 10, glm::vec4{1.f}, geom::ngon_style::kPolystar>>;

    auto c0 = player_colour((frame % (2 * kMaxPlayers)) / 2);
    auto c1 = player_colour(((frame + 1) % (2 * kMaxPlayers)) / 2);
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

struct PlayerLogic : ecs::component {
  static constexpr fixed kPlayerSpeed = 5;
  static constexpr fixed kBombRadius = 180;
  static constexpr fixed kBombBossRadius = 280;

  static constexpr std::uint32_t kReviveTime = 100;
  static constexpr std::uint32_t kShieldTime = 50;
  static constexpr std::uint32_t kShotTimer = 4;

  std::uint32_t invulnerability_timer = kReviveTime;
  vec2 fire_target{0};
  std::uint32_t fire_timer = 0;

  void update(ecs::handle h, Player& pc, Transform& transform, SimInterface& sim) {
    auto input = sim.input(pc.player_number);
    if (input.target_absolute) {
      fire_target = *input.target_absolute;
    } else if (input.target_relative) {
      fire_target = transform.centre + *input.target_relative;
    }
    fire_timer = (fire_timer + 1) % kShotTimer;

    // Temporary death.
    if (pc.kill_timer > 1 && --pc.kill_timer) {
      return;
    }

    auto& kill_queue = Player::kill_queue;
    if (pc.kill_timer) {
      if (sim.get_lives() && !kill_queue.empty() && kill_queue.front() == pc.player_number) {
        sim.sub_life();
        pc.kill_timer = 0;
        kill_queue.erase(kill_queue.begin());
        invulnerability_timer = kReviveTime;
        transform.centre = {(1 + pc.player_number) * kSimDimensions.x / (1 + sim.player_count()),
                            kSimDimensions.y / 2};
        sim.rumble(pc.player_number, 10);
        sim.play_sound(sound::kPlayerRespawn, transform.centre);
      }
      return;
    }
    invulnerability_timer && --invulnerability_timer;

    // Movement.
    if (length(input.velocity) > fixed_c::hundredth) {
      auto v = length(input.velocity) > 1 ? normalise(input.velocity) : input.velocity;
      transform.set_rotation(angle(v));
      transform.centre =
          max(vec2{0},
              min(vec2{kSimDimensions.x, kSimDimensions.y}, kPlayerSpeed * v + transform.centre));
    }

    // Bombs.
    if (pc.has_bomb && input.keys & input_frame::kBomb) {
      auto c = player_colour(pc.player_number);
      pc.has_bomb = false;
      // destroy_shape(5);

      explosion(pc, transform, sim, glm::vec4{1.f}, 16);
      explosion(pc, transform, sim, c, 32);
      explosion(pc, transform, sim, glm::vec4{1.f}, 48);

      Transform bomb_explode;
      for (std::uint32_t i = 0; i < 64; ++i) {
        bomb_explode.centre = transform.centre + from_polar(2 * i * fixed_c::pi / 64, kBombRadius);
        explosion(pc, bomb_explode, sim, (i % 2) ? c : glm::vec4{1.f},
                  8 + sim.random(8) + sim.random(8), to_float(transform.centre));
      }

      sim.rumble(pc.player_number, 10);
      sim.play_sound(sound::kExplosion, transform.centre);

      sim.index().iterate_dispatch_if<Enemy>(
          [&](ecs::handle h, const Enemy& e, Health& health) {
            if (!h.has<LegacyShip>() && !h.has<Transform>()) {
              return;
            }
            auto centre = ecs::call<&get_centre>(h);
            if (length(centre - transform.centre) > kBombBossRadius) {
              return;
            }
            if (h.has<Boss>() || length(centre - transform.centre) <= kBombRadius) {
              health.damage(h, sim, Player::kBombDamage, damage_type::kBomb, std::nullopt);
            }
            if (!h.has<Boss>() && e.score_reward) {
              pc.add_score(sim, 0);
            }
          },
          /* include_new */ false);
    }

    // Shots.
    auto shot = fire_target - transform.centre;
    if (length(shot) > 0 && !fire_timer && input.keys & input_frame::kFire) {
      spawn_shot(sim, transform.centre, h, shot, pc.magic_shot_count != 0);
      pc.magic_shot_count && --pc.magic_shot_count;

      float volume = .5f * sim.random_fixed().to_float() + .5f;
      float pitch = (sim.random_fixed().to_float() - 1.f) / 12.f;
      float pan = 2.f * transform.centre.x.to_float() / kSimDimensions.x - 1.f;
      sim.play_sound(sound::kPlayerFire, volume, pan, pitch);
    }

    // Damage.
    if (sim.any_collision(transform.centre, shape_flag::kDangerous)) {
      damage(pc, transform, sim);
    }
  }

  void damage(Player& pc, Transform& transform, SimInterface& sim) {
    if (pc.kill_timer || invulnerability_timer) {
      return;
    }

    if (pc.has_shield) {
      sim.rumble(pc.player_number, 10);
      sim.play_sound(sound::kPlayerShield, transform.centre);
      // destroy_shape(5);
      pc.has_shield = false;
      invulnerability_timer = kShieldTime;
      return;
    }

    explosion(pc, transform, sim);
    explosion(pc, transform, sim, glm::vec4{1.f}, 14);
    explosion(pc, transform, sim, std::nullopt, 20);

    pc.magic_shot_count = 0;
    pc.multiplier = 1;
    pc.multiplier_count = 0;
    pc.kill_timer = kReviveTime;
    ++pc.death_count;
    if (pc.has_shield || pc.has_bomb) {
      // destroy_shape(5);
      pc.has_shield = false;
      pc.has_bomb = false;
    }
    Player::kill_queue.push_back(pc.player_number);
    sim.rumble(pc.player_number, 25);
    sim.play_sound(sound::kPlayerDestroy, transform.centre);
  }

  void explosion(const Player& pc, const Transform& transform, SimInterface& sim,
                 std::optional<glm::vec4> colour = std::nullopt, std::uint32_t time = 8,
                 std::optional<glm::vec2> towards = std::nullopt) {
    glm::vec2 v{transform.centre.x.to_float(), transform.centre.y.to_float()};
    auto c = colour.value_or(player_colour(pc.player_number));
    auto c_dark = c;
    c_dark.a = .5f;
    sim.explosion(v, c, time, towards);
    sim.explosion(v + glm::vec2{8.f, 0.f}, c, time, towards);
    sim.explosion(v + glm::vec2{0.f, 8.f}, c, time, towards);
    sim.explosion(v + glm::vec2{8.f, 0.f}, c_dark, time, towards);
    sim.explosion(v + glm::vec2{0.f, 8.f}, c_dark, time, towards);
    if (pc.has_bomb || pc.has_shield) {
      sim.explosion(v, colour.value_or(glm::vec4{1.f}), time, towards);
    }
  }

  void render(const Player& pc, const Transform& transform, const SimInterface& sim) const {
    // TODO: really need a better way of doing dynamic shape-type parameters (specifically colours).
    static constexpr glm::vec4 c_dark{1.f, 1.f, 1.f, .2f};
    using box_shapes =
        geom::translate<8, 0,
                        geom::rotate_eval<geom::negate_p<1>, geom::box_shape<2, 2, glm::vec4{1.f}>>,
                        geom::rotate_eval<geom::negate_p<1>, geom::box_shape<1, 1, c_dark>>,
                        geom::rotate_eval<geom::negate_p<1>, geom::box_shape<3, 3, c_dark>>>;
    using shape =
        standard_transform<geom::ngon_shape<16, 3, glm::vec4{1.f}>,
                           geom::rotate<fixed_c::pi, geom::ngon_shape<8, 3, glm::vec4{1.f}>>,
                           box_shapes>;
    using shield = standard_transform<geom::ngon_shape<16, 10, glm::vec4{1.f}>>;
    using bomb = standard_transform<geom::translate<
        -8, 0,
        geom::rotate<fixed_c::pi,
                     geom::ngon_shape<6, 5, glm::vec4{1.f}, geom::ngon_style::kPolystar>>>>;

    std::tuple parameters{transform.centre, transform.rotation};
    render_shape<shape>(sim, parameters, player_colour(pc.player_number));
    if (pc.has_shield) {
      render_shape<shield>(sim, parameters);
    }
    if (pc.has_bomb) {
      render_shape<bomb>(sim, parameters);
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

void spawn_player(SimInterface& sim, const vec2& position, std::uint32_t player_number) {
  auto h = sim.index().create();
  h.add(LegacyShip{.ship = std::make_unique<ShipForwarder>(sim, h)});
  h.add(Update{.update = ecs::call<&PlayerLogic::update>});
  h.add(Render{.render = ecs::call<&PlayerLogic::render>});
  h.add(Player{.player_number = player_number});
  h.add(PlayerLogic{});
  h.add(Transform{.centre = position});
}

}  // namespace ii
