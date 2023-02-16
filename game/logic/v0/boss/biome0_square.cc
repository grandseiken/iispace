#include "game/common/colour.h"
#include "game/common/easing.h"
#include "game/geometry/node.h"
#include "game/geometry/shapes/ngon.h"
#include "game/logic/v0/boss/boss.h"
#include "game/logic/v0/boss/boss_template.h"
#include "game/logic/v0/enemy/enemy.h"
#include "game/logic/v0/lib/ship_template.h"
#include "game/logic/v0/lib/waypoint.h"
#include <algorithm>
#include <array>

namespace ii::v0 {
namespace {
using namespace geom;

// TODO: special attack: zoom vertically from one side to another.
struct SquareBoss : public ecs::component {
  static constexpr std::uint32_t kBaseHp = 3250;
  static constexpr std::uint32_t kBiomeHp = 750;
  static constexpr std::uint32_t kTimer = 120;
  static constexpr std::uint32_t kSpawnTimerBase = 90;
  static constexpr std::uint32_t kSpecialAttackTime = 120;
  static constexpr fixed kSpeed = 2_fx + 1_fx / 4;
  static constexpr fixed kSpecialAttackRadius = 145;
  static constexpr fixed kSpecialAttackRadiusExtra = 35;
  static constexpr fixed kBoundingWidth = 200;
  static constexpr fixed kTurningRadius = 72;
  static constexpr fixed kMarginX = 1_fx / 5;
  static constexpr fixed kMarginY = 1_fx / 4;

  static constexpr auto z = colour::kZEnemyBoss;
  static constexpr auto c0 = colour::kNewPurple;
  static constexpr auto c1 = colour::kSolarizedDarkCyan;
  static constexpr auto c2 = colour::kSolarizedDarkViolet;
  static constexpr auto c3 = colour::kSolarizedDarkMagenta;
  static constexpr auto outline = nline(colour::kOutline, colour::kZOutline, 2.5f);
  static constexpr auto m_outline = nline(colour::kOutline, colour::kZOutline, -2.5f);

  template <fixed C, ShapeNode... Nodes>
  using rotate_s = rotate_eval<multiply_p<C, 1>, pack<Nodes...>>;
  template <fixed R, std::uint32_t T, render::flag RFlags>
  using ngon = compound<
      ngon_eval<constant<nd2(50 + 25 * R, 50, 4)>,
                set_colour2<nline(colour::kZero, z, 2.f), parameter_i<2, T>, parameter_i<2, 5>>,
                set_colour2_alpha<sfill(colour::kZero, z), parameter_i<2, T>, parameter_i<2, 5>,
                                  constant<cvec4{colour::kFillAlpha2}>>,
                constant<RFlags>>,
      ngon_eval<constant<nd(44 + 25 * R, 4)>,
                set_colour<nline(colour::kZero, z, 4.f), parameter_i<2, T>>, constant<sfill()>,
                constant<RFlags>>,
      ngon<nd(52 + 25 * R, 4), outline>, ngon<nd(48, 4), m_outline>>;
  template <fixed C, fixed R, std::uint32_t T, render::flag RFlags = render::flag::kNone>
  using rotate_ngon = rotate_s<C, ngon<R, T, RFlags>>;
  template <fixed C, fixed R, std::uint32_t T, shape_flag Flags,
            render::flag RFlags = render::flag::kNone>
  using rotate_ngon_c =
      rotate_s<C, ngon<R, T, RFlags>, ngon_collider<nd2(50 + 25 * R, 50, 4), Flags>>;
  using shape =
      translate_p<0, rotate_ngon_c<1, 6, 0, shape_flag::kBombVulnerable, render::flag::kNoFlash>,
                  rotate_ngon<3_fx / 2, 5, 1, render::flag::kNoFlash>,
                  rotate_ngon_c<2, 4, 2, shape_flag::kDangerous, render::flag::kNoFlash>,
                  rotate_ngon<5_fx / 2, 3, 3>, rotate_ngon_c<3, 2, 4, shape_flag::kVulnerable>,
                  rotate_ngon_c<7_fx / 2, 1, 5, shape_flag::kShield>>;

  cvec4 c_mix(float t) const { return colour::perceptual_mix(c0, c1, t); }

  std::tuple<vec2, fixed, std::array<cvec4, 6>> shape_parameters(const Transform& transform) const {
    std::array<cvec4, 6> colours = {c_mix(0.f),     c_mix(1.f / 5), c_mix(2.f / 5),
                                    c_mix(3.f / 5), c_mix(4.f / 5), c_mix(1.f)};
    return {transform.centre, transform.rotation, colours};
  }

  struct corner_tag {
    std::uint32_t index = 0;
    bool anti = false;
  };

  SquareBoss(std::uint32_t biome_index, std::uint32_t corner_index)
  : biome_index{biome_index}
  , waypoints{kTurningRadius}
  , start{corner_index, corner_index % 2 == 0} {}

  std::uint32_t biome_index = 0;
  std::uint32_t attack_colour = 0;
  std::uint32_t timer = kTimer * 6;
  std::uint32_t spawn_timer = 0;
  std::uint32_t spawn_timer_extra = 0;

  WaypointFollower<corner_tag> waypoints;
  corner_tag start;
  fixed rotation = 0;
  fixed speed = 2 * kSpeed;

  struct special_t {
    std::vector<ecs::entity_id> players;
    std::uint32_t timer = 0;
    bool rotate = false;
  };
  std::uint32_t special_counter = 0;
  std::optional<special_t> special_attack;

  void update(ecs::handle h, Transform& transform, const Health& health, SimInterface& sim) {
    auto dim = sim.dimensions();
    std::array<vec2, 4> corners = {
        vec2{dim.x * kMarginX, dim.y * kMarginY},
        vec2{dim.x * (1 - kMarginX), dim.y * kMarginY},
        vec2{dim.x * (1 - kMarginX), dim.y * (1 - kMarginY)},
        vec2{dim.x * kMarginX, dim.y * (1 - kMarginY)},
    };
    if (waypoints.empty()) {
      waypoints.add(corners[start.index], start);
    }
    while (waypoints.needs_add()) {
      const auto& t = waypoints.back().tag;
      auto index = (t.index + (t.anti ? 3u : 1u)) % 4u;
      waypoints.add(corners[index], corner_tag{index, t.anti});
    }
    auto& random = sim.random(h);

    auto threat = health.threat_level(6u);
    if (special_attack) {
      special_attack->timer && --special_attack->timer;
      std::erase_if(special_attack->players, [&](ecs::entity_id id) {
        auto ph = sim.index().get(id);
        return !ph || ph->get<Player>()->is_killed;
      });
      if (!special_attack->timer) {
        auto e = sim.emit(resolve_key::predicted());
        trigger_special(sim, e);
        if (random.rbool()) {
          waypoints.reverse();
          for (auto& wp : waypoints.waypoints) {
            wp.tag.anti = !wp.tag.anti;
          }
        }
        special_attack.reset();
      }
    } else if (sim.is_on_screen(transform.centre)) {
      timer && --timer;
      if (!timer && waypoints.size() <= 3u) {
        timer = (random.uint(10) + 2) * kTimer;
        const auto& t = waypoints.back().tag;
        auto index = (t.index + 2u) % 4u;
        waypoints.add(corners[index], corner_tag{index, !t.anti});
      }

      spawn_timer += 2 + threat + 4 * sim.alive_players() + 3 * biome_index;
      ++special_counter;
      if (spawn_timer >= (kSpawnTimerBase + spawn_timer_extra) * 5) {
        spawn_timer -= (kSpawnTimerBase + spawn_timer_extra) * 5;
        auto e = sim.emit(resolve_key::predicted());
        spawn_timer_extra = trigger_attack(sim, transform, random, threat, e);
        if (special_counter >= 8 * kSpawnTimerBase && !random.uint(3)) {
          special_counter = 0;
          special_attack = {
              {.timer = kSpecialAttackTime - 10 * biome_index, .rotate = random.rbool()}};
          assign_special_targets(sim, random, threat, e);
        }
      }
    }

    fixed target_rotation = special_attack ? 0_fx : 1_fx / 50;
    fixed target_speed = special_attack ? 0_fx : kSpeed;
    rotation = rc_smooth(rotation, target_rotation, 47_fx / 48);
    speed = rc_smooth(speed, target_speed, 47_fx / 48);
    transform.rotate(rotation);
    waypoints.update(transform.centre, speed);
  }

  std::uint32_t trigger_attack(SimInterface& sim, const Transform& transform, RandomEngine& random,
                               std::uint32_t threat, EmitHandle& e) {
    auto r = biome_index > 0 ? random.uint(2u) + 1u : random.uint(3u);
    if (biome_index > 1) {
      if (!random.uint(2)) {
        ++r;
      }
      if (threat > 2 && !random.uint(2)) {
        ++r;
      }
    } else {
      if (threat > 1 && !random.uint(2)) {
        ++r;
      }
      if (threat > 3 && !random.uint(2)) {
        ++r;
      }
    }
    ++attack_colour;
    attack_colour %= 3;
    auto c = attack_colour == 0 ? c0 : attack_colour == 1 ? c1 : c2;
    switch (r) {
    case 0:
    case 1:
      if (auto h = sim.random_alive_player(random); h) {
        shoot_follow(sim, transform, 1u, 5_fx, h->get<Transform>()->centre - transform.centre, c);
      }
      e.play_random(sound::kBossFire, transform.centre);
      return 0;
    case 2:
      if (auto h = sim.random_alive_player(random); h) {
        auto d = perpendicular(h->get<Transform>()->centre - transform.centre);
        shoot_follow(sim, transform, 1u, 7_fx, d, c);
        shoot_follow(sim, transform, 1u, 7_fx, -d, c);
      }
      e.play_random(sound::kBossFire, transform.centre);
      return 80;
    case 3:
      for (std::uint32_t i = 0; i < 24; ++i) {
        float f = i / 12.f;
        f = f > 1.f ? 2.f - f : f;
        shoot_follow(sim, transform, 0u, 8_fx, from_polar(i * pi<fixed> / 12, 1_fx), c);
      }
      e.play_random(sound::kBossFire, transform.centre);
      return 160;
    case 4:
      for (std::uint32_t i = 0; i < 4; ++i) {
        shoot_follow(sim, transform, 1u, 6_fx,
                     from_polar(transform.rotation + i * pi<fixed> / 2, 1_fx), c);
      }
      e.play_random(sound::kBossFire, transform.centre);
      return 260;
    }
    return 0;
  }

  void shoot_follow(SimInterface& sim, const Transform& transform, std::size_t size, fixed speed,
                    const vec2& direction, const cvec4& colour) {
    std::optional<vec2> d;
    std::optional<ecs::handle> h;
    if (speed && direction != vec2{0}) {
      d = speed * normalise(direction);
    }
    switch (size) {
    case 0:
      h = spawn_follow(sim, transform.centre);
      break;
    case 1:
      h = spawn_big_follow(sim, transform.centre);
      break;
    case 2:
      h = spawn_huge_follow(sim, transform.centre);
      break;
    }
    if (!h) {
      return;
    }
    if (auto* physics = h->get<Physics>(); physics) {
      if (d) {
        physics->apply_impulse(*d * physics->mass);
      }
      if (waypoints.direction) {
        physics->apply_impulse(speed * *waypoints.direction * physics->mass / 2);
      }
    }
    h->add(ColourOverride{.colour = colour});
  }

  void assign_special_targets(SimInterface& sim, RandomEngine& random, std::uint32_t threat,
                              EmitHandle& e) {
    auto max_targets = 1u + threat / 2u + biome_index;
    auto handles = sim.alive_player_handles();
    if (handles.size() >= max_targets) {
      random.shuffle(handles.begin(), handles.end());
      handles.erase(handles.begin() + max_targets, handles.end());
    }
    special_attack->players.clear();
    for (const auto& h : handles) {
      e.play(sound::kBossAttack, h.get<Transform>()->centre);
      special_attack->players.emplace_back(h.id());
    }
  }

  void trigger_special(SimInterface& sim, EmitHandle& e) const {
    for (const auto& id : special_attack->players) {
      const auto& p_transform = *sim.index().get(id)->get<Transform>();
      vec2 d{kSpecialAttackRadius, 0};
      if (special_attack->rotate) {
        d = ::rotate(d, pi<fixed> / 2);
      }
      for (std::uint32_t i = 0; i < 6; ++i) {
        spawn_follow(sim, p_transform.centre + d, std::nullopt, /* drop */ false)
            .add(ColourOverride{.colour = c3});
        d = ::rotate(d, 2 * pi<fixed> / 6);
      }
      e.play(sound::kEnemySpawn, p_transform.centre);
    }
  }

  void render(std::vector<render::shape>& output, const SimInterface& sim) const {
    static constexpr std::uint32_t kSmallWidth = 11;
    static constexpr auto cf = colour::alpha(c3, colour::kFillAlpha0);
    static constexpr float z = colour::kZEnemySmall;
    using small_follow_shape = standard_transform<
        ngon_colour_p<nd(kSmallWidth + 2, 4), 2, outline>,
        ngon_colour_p2<nd(kSmallWidth, 4), 3, 4, nline(c3, z, 1.5f), sfill(cf, z)>>;
    if (!special_attack) {
      return;
    }
    auto t = fixed{special_attack->timer} / kSpecialAttackTime;
    auto ta = ((2_fx + sin(4 * pi<fixed> * t)) / 3).to_float();
    auto c0 = colour::alpha(colour::kOutline, ta);
    auto c1 = colour::alpha(c3, ta);
    auto c2 = colour::alpha(cf, ta);

    for (const auto& id : special_attack->players) {
      auto ph = sim.index().get(id);
      if (!ph || ph->get<Player>()->is_killed) {
        continue;
      }
      const auto& p_transform = *ph->get<Transform>();
      vec2 d{kSpecialAttackRadius + ease_in_cubic(t) * kSpecialAttackRadiusExtra, 0};
      if (special_attack->rotate) {
        d = ::rotate(d, pi<fixed> / 2);
      }
      for (std::uint32_t i = 0; i < 6; ++i) {
        auto v = p_transform.centre + d;
        auto& r = local_resolve();
        resolve_shape<small_follow_shape>(
            std::tuple{v, 4 * pi<fixed> * ease_out_cubic(1_fx - t), c0, c1, c2}, r);
        render_shape(output, r);
        d = ::rotate(d, 2 * pi<fixed> / 6);
      }
    }
  }
};
DEBUG_STRUCT_TUPLE(SquareBoss::special_t, players, timer, rotate);
DEBUG_STRUCT_TUPLE(SquareBoss::corner_tag, index, anti);
DEBUG_STRUCT_TUPLE(SquareBoss, waypoints, start, rotation, speed, timer, spawn_timer,
                   spawn_timer_extra, special_counter, special_attack);

}  // namespace

void spawn_biome0_square_boss(SimInterface& sim, std::uint32_t biome_index) {
  auto& r = sim.random(random_source::kGameSequence);
  auto dim = sim.dimensions();
  auto corner_index = r.uint(4u);
  vec2 position{0};
  switch (corner_index) {
  default:
  case 0:
    position = vec2{dim.x * SquareBoss::kMarginX, -dim.y / 2};
    break;
  case 1:
    position = vec2{dim.x * (1 - SquareBoss::kMarginX), -dim.y / 2};
    break;
  case 2:
    position = vec2{dim.x * (1 - SquareBoss::kMarginX), 3 * dim.y / 2};
    break;
  case 3:
    position = vec2{dim.x * SquareBoss::kMarginX, 3 * dim.y / 2};
    break;
  }

  auto h = create_ship_default<SquareBoss>(sim, position);
  add_boss_data<SquareBoss>(h, ustring::ascii("Squall"),
                            SquareBoss::kBaseHp + SquareBoss::kBiomeHp * biome_index);
  h.add(Physics{.mass = 12u});
  h.add(SquareBoss{biome_index, corner_index});
}

}  // namespace ii::v0
