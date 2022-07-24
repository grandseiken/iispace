#include "game/logic/player/player.h"
#include "game/logic/geometry/enums.h"
#include "game/logic/geometry/shapes/shapes.h"
#include "game/logic/player/ai_player.h"
#include "game/logic/ship/ship_template.h"
#include <algorithm>
#include <utility>

namespace ii {
namespace {
constexpr std::uint32_t kMagicShotCount = 120;

struct Shot : ecs::component {
  static constexpr fixed kSpeed = 10;

  using shape = standard_transform<geom::box_colour_p<2, 2, 2>, geom::box_colour_p<1, 1, 3>,
                                   geom::box_colour_p<3, 3, 3>>;

  std::tuple<vec2, fixed, glm::vec4, glm::vec4> shape_parameters(const Transform& transform) const {
    auto c_dark = colour;
    c_dark.a = .2f;
    return {transform.centre, transform.rotation, colour, c_dark};
  }

  ecs::entity_id player;
  std::uint32_t player_number = 0;
  vec2 velocity{0};
  bool magic = false;
  glm::vec4 colour{0.f};

  Shot(ecs::entity_id player, std::uint32_t player_number, const vec2& direction, bool magic)
  : player{player}
  , player_number{player_number}
  , velocity{normalise(direction) * kSpeed}
  , magic{magic} {}

  void update(ecs::handle h, Transform& transform, SimInterface& sim) {
    if (sim.conditions().mode == game_mode::kWhat) {
      colour = glm::vec4{0.f};
    } else {
      colour = magic && sim.random(2) ? glm::vec4{1.f} : player_colour(player_number);
    }
    transform.move(velocity);
    bool on_screen = all(greaterThanEqual(transform.centre, vec2{-4, -4})) &&
        all(lessThan(transform.centre, vec2{4 + kSimDimensions.x, 4 + kSimDimensions.y}));
    if (!on_screen) {
      h.emplace<Destroy>();
      return;
    }

    bool destroy = false;
    auto generation = sim.index().generation();
    auto collision = sim.collision_list(
        transform.centre, shape_flag::kVulnerable | shape_flag::kShield | shape_flag::kWeakShield);
    for (const auto& e : collision) {
      if (+(e.hit_mask & shape_flag::kVulnerable)) {
        ecs::call_if<&Health::damage>(e.h, sim, 1, magic ? damage_type::kMagic : damage_type::kNone,
                                      player);
        if (!magic) {
          destroy = true;
        }
      }
    }

    if (!destroy) {
      // Compatibility: need to rerun the collision check if new entities might have spawned.
      if (generation != sim.index().generation()) {
        collision =
            sim.collision_list(transform.centre, shape_flag::kShield | shape_flag::kWeakShield);
      }
      for (const auto& e : collision) {
        if (!e.h.has<Destroy>() &&
            (+(e.hit_mask & shape_flag::kShield) ||
             (!magic && +(e.hit_mask & shape_flag::kWeakShield)))) {
          destroy = true;
        }
      }
    }
    if (destroy) {
      h.emplace<Destroy>();
    }
  }
};

void spawn_shot(SimInterface& sim, const vec2& position, ecs::handle player, const vec2& direction,
                bool magic) {
  create_ship<Shot>(sim, position)
      .add(Shot{player.id(), player.get<Player>()->player_number, direction, magic});
}

struct Powerup : ecs::component {
  static constexpr fixed kSpeed = 1;
  static constexpr std::uint32_t kRotateTime = 100;
  static constexpr glm::vec4 cw{1.f};

  using out0 = geom::ngon_colour_p<13, 5, 2>;
  using out1 = geom::ngon_colour_p<9, 5, 3>;
  using extra_life = geom::switch_entry<powerup_type::kExtraLife, geom::ngon<8, 3, cw>>;
  using magic_shots = geom::switch_entry<powerup_type::kMagicShots, geom::box<3, 3, cw>>;
  using shield = geom::switch_entry<powerup_type::kShield, geom::ngon<11, 5, cw>>;
  using bomb = geom::switch_entry<powerup_type::kBomb, geom::polystar<11, 10, cw>>;
  using shape =
      standard_transform<out0, out1, geom::switch_p<4, extra_life, magic_shots, shield, bomb>>;

  std::tuple<vec2, fixed, glm::vec4, glm::vec4, powerup_type>
  shape_parameters(const Transform& transform) const {
    auto c0 = player_colour((frame % (2 * kMaxPlayers)) / 2);
    auto c1 = player_colour(((frame + 1) % (2 * kMaxPlayers)) / 2);
    return {transform.centre, fixed_c::pi / 2, c0, c1, type};
  }

  Powerup(powerup_type type) : type{type} {}
  powerup_type type = powerup_type::kExtraLife;
  std::uint32_t frame = 0;
  vec2 dir = {0, 1};
  bool rotate = false;
  bool first_frame = true;

  void update(ecs::handle h, Transform& transform, SimInterface& sim) {
    ++frame;

    if (!sim.is_on_screen(transform.centre)) {
      dir = kSimDimensions / 2_fx - transform.centre;
    } else {
      if (first_frame) {
        dir = from_polar(sim.random_fixed() * 2 * fixed_c::pi, 1_fx);
      }

      dir = sim.rotate_compatibility(dir, 2 * fixed_c::hundredth * (rotate ? 1 : -1));
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
      ++sim.global_entity().get<GlobalData>()->lives;
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
};

struct PlayerLogic : ecs::component {
  static constexpr fixed kPlayerSpeed = 5;
  static constexpr fixed kBombRadius = 180;
  static constexpr fixed kBombBossRadius = 280;

  static constexpr std::uint32_t kReviveTime = 100;
  static constexpr std::uint32_t kShieldTime = 50;
  static constexpr std::uint32_t kShotTimer = 4;

  using shield = geom::if_p<2, geom::ngon_colour_p<16, 10, 6>>;
  using bomb = geom::if_p<
      3, geom::translate<-8, 0, geom::rotate<fixed_c::pi, geom::polystar_colour_p<6, 5, 6>>>>;
  using box_shapes =
      geom::translate<8, 0, geom::rotate_eval<geom::negate_p<1>, geom::box_colour_p<2, 2, 4>>,
                      geom::rotate_eval<geom::negate_p<1>, geom::box_colour_p<1, 1, 5>>,
                      geom::rotate_eval<geom::negate_p<1>, geom::box_colour_p<3, 3, 5>>>;
  using shape = standard_transform<geom::ngon_colour_p<16, 3, 4>,
                                   geom::rotate<fixed_c::pi, geom::ngon_colour_p<8, 3, 4>>,
                                   box_shapes, shield, bomb>;

  std::tuple<vec2, fixed, bool, bool, glm::vec4, glm::vec4, glm::vec4>
  shape_parameters(const Player& pc, const Transform& transform) const {
    auto colour = !should_render(pc) ? glm::vec4{0.f}
        : invulnerability_timer % 2  ? glm::vec4{1.f}
                                     : player_colour(pc.player_number);
    auto c_dark = colour;
    c_dark.a = std::min(c_dark.a, .2f);
    auto powerup_colour = !should_render(pc) ? glm::vec4{0.f} : glm::vec4{1.f};
    return {transform.centre, transform.rotation, pc.has_shield, pc.has_bomb, colour,
            c_dark,           powerup_colour};
  };

  PlayerLogic(const SimInterface& sim) : is_what_mode{sim.conditions().mode == game_mode::kWhat} {}
  bool is_what_mode = false;
  std::uint32_t invulnerability_timer = kReviveTime;
  std::uint32_t fire_timer = 0;
  vec2 fire_target{0};

  void update(ecs::handle h, Player& pc, Transform& transform, SimInterface& sim) {
    if (auto* ai = h.get<AiPlayer>(); ai) {
      sim.input(pc.player_number) = ecs::call<&AiPlayer::think>(h, sim);
    }
    const auto& input = sim.input(pc.player_number);
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

    auto& global_data = *sim.global_entity().get<GlobalData>();
    auto& kill_queue = global_data.player_kill_queue;
    if (pc.kill_timer) {
      if (global_data.lives && !kill_queue.empty() && kill_queue.front() == pc.player_number) {
        --global_data.lives;
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

      explosion(h, std::nullopt, sim, glm::vec4{1.f}, 16);
      explosion(h, std::nullopt, sim, c, 32);
      explosion(h, std::nullopt, sim, glm::vec4{1.f}, 48);

      for (std::uint32_t i = 0; i < 64; ++i) {
        auto v = transform.centre + from_polar(2 * i * fixed_c::pi / 64, kBombRadius);
        explosion(h, v, sim, (i % 2) ? c : glm::vec4{1.f}, 8 + sim.random(8) + sim.random(8),
                  transform.centre);
      }

      sim.rumble(pc.player_number, 10);
      sim.play_sound(sound::kExplosion, transform.centre);

      sim.index().iterate_dispatch_if<Enemy>(
          [&](ecs::handle eh, const Enemy& e, const Transform& e_transform, Health& health,
              Boss* boss) {
            if (length(e_transform.centre - transform.centre) > kBombBossRadius) {
              return;
            }
            if (boss || length(e_transform.centre - transform.centre) <= kBombRadius) {
              health.damage(eh, sim, Player::kBombDamage, damage_type::kBomb, std::nullopt);
            }
            if (!boss && e.score_reward) {
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
      damage(h, pc, transform, sim);
    }
  }

  void damage(ecs::handle h, Player& pc, Transform& transform, SimInterface& sim) {
    if (pc.kill_timer || invulnerability_timer) {
      return;
    }

    if (pc.has_shield) {
      sim.rumble(pc.player_number, 10);
      sim.play_sound(sound::kPlayerShield, transform.centre);
      pc.has_shield = false;
      invulnerability_timer = kShieldTime;
      return;
    }

    explosion(h, std::nullopt, sim);
    explosion(h, std::nullopt, sim, glm::vec4{1.f}, 14);
    explosion(h, std::nullopt, sim, std::nullopt, 20);

    pc.magic_shot_count = 0;
    pc.multiplier = 1;
    pc.multiplier_count = 0;
    pc.kill_timer = kReviveTime;
    ++pc.death_count;
    if (pc.has_shield || pc.has_bomb) {
      pc.has_shield = false;
      pc.has_bomb = false;
    }
    sim.global_entity().get<GlobalData>()->player_kill_queue.push_back(pc.player_number);
    sim.rumble(pc.player_number, 25);
    sim.play_sound(sound::kPlayerDestroy, transform.centre);
  }

  void explosion(ecs::handle h, const std::optional<vec2>& position_override, SimInterface& sim,
                 const std::optional<glm::vec4>& colour = std::nullopt, std::uint32_t time = 8,
                 const std::optional<vec2>& towards = std::nullopt) const {
    auto parameters = get_shape_parameters<PlayerLogic>(h);
    if (position_override) {
      std::get<0>(parameters) = *position_override;
    }
    explode_shapes<shape>(sim, parameters, colour, time, towards);
  }

  void render(const Player& pc, const SimInterface& sim) const {
    auto c = player_colour(pc.player_number);
    if (should_render(pc)) {
      auto t = to_float(fire_target);
      sim.render_line(t + glm::vec2{0, 9}, t - glm::vec2{0, 8}, c);
      sim.render_line(t + glm::vec2{9, 1}, t - glm::vec2{8, -1}, c);
    }

    if (sim.conditions().mode != game_mode::kBoss) {
      sim.render_player_info(pc.player_number, c, pc.score, pc.multiplier,
                             static_cast<float>(pc.magic_shot_count) / kMagicShotCount);
    }
  }

  bool should_render(const Player& pc) const {
    return !pc.kill_timer && (!is_what_mode || invulnerability_timer);
  }
};

}  // namespace

void spawn_powerup(SimInterface& sim, const vec2& position, powerup_type type) {
  auto h = create_ship<Powerup>(sim, position);
  h.add(Powerup{type});
  h.add(PowerupTag{.type = type});
}

void spawn_player(SimInterface& sim, const vec2& position, std::uint32_t player_number,
                  bool is_ai) {
  auto h = create_ship<PlayerLogic>(sim, position);
  h.add(Player{.player_number = player_number});
  h.add(PlayerLogic{sim});
  if (is_ai) {
    h.add(AiPlayer{});
  }
}

}  // namespace ii
