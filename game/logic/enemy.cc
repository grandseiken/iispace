#include "game/logic/enemy.h"
#include "game/common/functional.h"
#include "game/logic/player.h"
#include "game/logic/ship/ecs_call.h"
#include "game/logic/ship/shape_v2.h"
#include <algorithm>

namespace ii {
namespace {

template <geom::ShapeNode S>
using standard_transform = geom::translate_p<0, geom::rotate_p<1, S>>;

template <ecs::Component Logic>
auto transform_parameters(ecs::const_handle h) {
  if constexpr (requires { &Logic::shape_parameters; }) {
    return ecs::call<&Logic::shape_parameters>(h);
  } else {
    auto& transform = *h.get<Transform>();
    return std::tuple<vec2, fixed>{transform.centre, transform.rotation};
  }
}

template <ecs::Component Logic, geom::ShapeNode S>
void render_enemy(ecs::const_handle h, const Render& render, const Health* health,
                  const SimInterface& sim) {
  auto c_override = render.colour_override;
  if (!c_override && health && health->hit_timer) {
    c_override = glm::vec4{1.f};
  }

  S::iterate(geom::iterate_lines, transform_parameters<Logic>(h), {},
             [&](const vec2& a, const vec2& b, const glm::vec4& c) {
               sim.render_line(to_float(a), to_float(b), c_override ? *c_override : c);
             });
}

template <ecs::Component Logic, geom::ShapeNode S>
void explode_on_destroy(ecs::const_handle h, SimInterface& sim, damage_type) {
  S::iterate(geom::iterate_centres, transform_parameters<Logic>(h), {},
             [&](const vec2& v, const glm::vec4& c) { sim.explosion(to_float(v), c); });
}

template <ecs::Component Logic, geom::ShapeNode S>
bool ship_check_point(ecs::const_handle h, const vec2& v, shape_flag mask) {
  return S::check_point(transform_parameters<Logic>(h), v, mask);
}

template <ecs::Component Logic, geom::ShapeNode S = typename Logic::shape>
ecs::handle create_enemy(SimInterface& sim, std::uint32_t threat_value, std::uint32_t score_reward,
                         std::uint32_t hp, const vec2& position, fixed rotation = 0,
                         std::optional<sound> destroy_sound = std::nullopt) {
  auto h = sim.index().create();

  ship_flag flags = ship_flag::kEnemy;
  if constexpr (requires { Logic::kShipFlags; }) {
    flags |= Logic::kShipFlags;
  }
  h.add(ShipFlag{.flags = flags});

  h.add(LegacyShip{.ship = std::make_unique<ShipForwarder>(sim, h)});
  h.add(Update{.update = ecs::call<&Logic::update>});
  h.add(Enemy{.threat_value = threat_value, .score_reward = score_reward});
  h.add(Render{.render = ecs::call<&render_enemy<Logic, S>>});
  h.add(Transform{.centre = position, .rotation = rotation});
  h.add(Collision{.bounding_width = Logic::kBoundingWidth, .check = &ship_check_point<Logic, S>});

  using on_destroy_t = void(ecs::const_handle, SimInterface & sim, damage_type);
  auto on_destroy = &explode_on_destroy<Logic, S>;
  if constexpr (requires { &Logic::on_destroy; }) {
    on_destroy =
        sequence<&explode_on_destroy<Logic, S>, cast<on_destroy_t, ecs::call<&Logic::on_destroy>>>;
  }
  h.add(Health{.hp = hp,
               .destroy_sound = destroy_sound ? *destroy_sound : Logic::kDestroySound,
               .on_destroy = on_destroy});
  return h;
}

}  // namespace

namespace {
struct Follow : ecs::component {
  static constexpr std::uint32_t kBoundingWidth = 10;
  static constexpr sound kDestroySound = sound::kEnemyShatter;

  static constexpr std::uint32_t kTime = 90;
  static constexpr fixed kSpeed = 2;

  using small_shape =
      geom::shape<geom::ngon{10, 4, colour_hue360(270, .6f), geom::ngon_style::kPolygon,
                             shape_flag::kDangerous | shape_flag::kVulnerable}>;
  using big_shape =
      geom::shape<geom::ngon{20, 4, colour_hue360(270, .6f), geom::ngon_style::kPolygon,
                             shape_flag::kDangerous | shape_flag::kVulnerable}>;
  using shape = standard_transform<geom::conditional_p<2, big_shape, small_shape>>;

  std::tuple<vec2, fixed, bool> shape_parameters(const Transform& transform) const {
    return {transform.centre, transform.rotation, is_big_follow};
  }

  Follow(bool is_big_follow) : is_big_follow{is_big_follow} {}
  std::uint32_t timer = 0;
  IShip* target = nullptr;
  bool is_big_follow = false;

  void update(Transform& transform, SimInterface& sim) {
    transform.rotate(fixed_c::tenth);
    if (!sim.alive_players()) {
      return;
    }

    ++timer;
    if (!target || timer > kTime) {
      target = sim.nearest_player(transform.centre);
      timer = 0;
    }
    auto d = target->position() - transform.centre;
    transform.move(normalise(d) * kSpeed);
  }

  void on_destroy(const Enemy& enemy, const Transform& transform, SimInterface& sim,
                  damage_type type) const {
    if (!is_big_follow || type == damage_type::kBomb) {
      return;
    }
    vec2 d = rotate(vec2{10, 0}, transform.rotation);
    for (std::uint32_t i = 0; i < 3; ++i) {
      spawn_follow(sim, transform.centre + d, enemy.score_reward != 0);
      d = rotate(d, 2 * fixed_c::pi / 3);
    }
  }
};
}  // namespace

void spawn_follow(SimInterface& sim, const vec2& position, bool has_score, fixed rotation) {
  auto h = create_enemy<Follow>(sim, 1, has_score ? 15u : 0, 1, position, rotation);
  h.add(Follow{false});
}

void spawn_big_follow(SimInterface& sim, const vec2& position, bool has_score) {
  auto h =
      create_enemy<Follow>(sim, 3, has_score ? 20u : 0, 3, position, 0, ii::sound::kPlayerDestroy);
  h.add(Follow{true});
}

namespace {
struct Chaser : ecs::component {
  static constexpr std::uint32_t kBoundingWidth = 10;
  static constexpr sound kDestroySound = sound::kEnemyShatter;

  static constexpr std::uint32_t kTime = 60;
  static constexpr fixed kSpeed = 4;
  using shape = standard_transform<
      geom::shape<geom::ngon{10, 4, colour_hue360(210, .6f), geom::ngon_style::kPolygram,
                             shape_flag::kDangerous | shape_flag::kVulnerable}>>;

  bool move = false;
  std::uint32_t timer = kTime;
  vec2 dir{0};

  void update(Transform& transform, SimInterface& sim) {
    bool before = sim.is_on_screen(transform.centre);

    if (timer) {
      --timer;
    }
    if (!timer) {
      timer = kTime * (move + 1);
      move = !move;
      if (move) {
        dir = kSpeed * sim.nearest_player_direction(transform.centre);
      }
    }
    if (move) {
      transform.move(dir);
      if (!before && sim.is_on_screen(transform.centre)) {
        move = false;
      }
    } else {
      transform.rotate(fixed_c::tenth);
    }
  }
};
}  // namespace

void spawn_chaser(SimInterface& sim, const vec2& position) {
  auto h = create_enemy<Chaser>(sim, 2, 30, 2, position);
  h.add(Chaser{});
}

namespace {
struct Square : ecs::component {
  static constexpr std::uint32_t kBoundingWidth = 15;
  static constexpr ship_flag kShipFlags = ship_flag::kWall;
  static constexpr sound kDestroySound = sound::kEnemyDestroy;
  static constexpr fixed kSpeed = 2 + 1_fx / 4;
  using shape = standard_transform<geom::shape<geom::box{
      {10, 10}, colour_hue360(120, .6f), shape_flag::kDangerous | shape_flag::kVulnerable}>>;

  vec2 dir{0};
  std::uint32_t timer = 0;

  Square(SimInterface& sim, fixed dir_angle)
  : dir{from_polar(dir_angle, 1_fx)}, timer{sim.random(80) + 40} {}

  void
  update(ecs::handle h, Transform& transform, Health& health, Render& render, SimInterface& sim) {
    if (sim.is_on_screen(transform.centre) && !sim.get_non_wall_count()) {
      if (timer) {
        --timer;
      }
    } else {
      timer = sim.random(80) + 40;
    }

    if (!timer) {
      health.damage(h, sim, 4, damage_type::kNone, std::nullopt);
    }

    const vec2& v = transform.centre;
    if (v.x < 0 && dir.x <= 0) {
      dir.x = -dir.x;
      if (dir.x <= 0) {
        dir.x = 1;
      }
    }
    if (v.y < 0 && dir.y <= 0) {
      dir.y = -dir.y;
      if (dir.y <= 0) {
        dir.y = 1;
      }
    }
    if (v.x > ii::kSimDimensions.x && dir.x >= 0) {
      dir.x = -dir.x;
      if (dir.x >= 0) {
        dir.x = -1;
      }
    }
    if (v.y > ii::kSimDimensions.y && dir.y >= 0) {
      dir.y = -dir.y;
      if (dir.y >= 0) {
        dir.y = -1;
      }
    }
    dir = normalise(dir);
    transform.move(dir * kSpeed);
    transform.set_rotation(angle(dir));

    if ((timer % 4 == 1 || timer % 4 == 2) && !sim.get_non_wall_count()) {
      render.colour_override = colour_hue(0.f, .2f, 0.f);
    } else {
      render.colour_override.reset();
    }
  }
};
}  // namespace

void spawn_square(SimInterface& sim, const vec2& position, fixed dir_angle) {
  auto h = create_enemy<Square>(sim, 2, 25, 4, position);
  h.add(Square{sim, dir_angle});
}

namespace {
struct Wall : ecs::component {
  static constexpr std::uint32_t kBoundingWidth = 50;
  static constexpr ship_flag kShipFlags = ship_flag::kWall;
  static constexpr sound kDestroySound = sound::kEnemyDestroy;

  static constexpr std::uint32_t kTimer = 80;
  static constexpr fixed kSpeed = 1 + 1_fx / 4;
  using shape = standard_transform<geom::shape<geom::box{
      {10, 40}, colour_hue360(120, .5f, .6f), shape_flag::kDangerous | shape_flag::kVulnerable}>>;

  vec2 dir{0, 1};
  std::uint32_t timer = 0;
  bool is_rotating = false;
  bool rdir = false;

  Wall(bool rdir) : rdir{rdir} {}

  void update(ecs::handle h, Transform& transform, Health& health, SimInterface& sim) {
    if (!sim.get_non_wall_count() && timer % 8 < 2) {
      if (health.hp > 2) {
        sim.play_sound(sound::kEnemySpawn, 1.f, 0.f);
      }
      health.damage(h, sim, std::max(2u, health.hp) - 2, damage_type::kNone, std::nullopt);
    }

    if (is_rotating) {
      auto d = rotate(dir, (rdir ? -1 : 1) * (kTimer - timer) * fixed_c::pi / (4 * kTimer));

      transform.set_rotation(angle(d));
      if (!--timer) {
        is_rotating = false;
        dir = rotate(dir, rdir ? -fixed_c::pi / 4 : fixed_c::pi / 4);
      }
      return;
    }
    if (++timer > kTimer * 6) {
      if (sim.is_on_screen(transform.centre)) {
        timer = kTimer;
        is_rotating = true;
      } else {
        timer = 0;
      }
    }

    const auto& v = transform.centre;
    if ((v.x < 0 && dir.x < -fixed_c::hundredth) || (v.y < 0 && dir.y < -fixed_c::hundredth) ||
        (v.x > ii::kSimDimensions.x && dir.x > fixed_c::hundredth) ||
        (v.y > ii::kSimDimensions.y && dir.y > fixed_c::hundredth)) {
      dir = -normalise(dir);
    }

    transform.move(dir * kSpeed);
    transform.set_rotation(angle(dir));
  }

  void on_destroy(const Transform& transform, SimInterface& sim, damage_type type) const {
    if (type == damage_type::kBomb) {
      return;
    }
    auto d = rotate(dir, fixed_c::pi / 2);
    auto v = transform.centre + d * 10 * 3;
    if (sim.is_on_screen(v)) {
      spawn_square(sim, v, transform.rotation);
    }
    v = transform.centre - d * 10 * 3;
    if (sim.is_on_screen(v)) {
      spawn_square(sim, v, transform.rotation);
    }
  }
};
}  // namespace

void spawn_wall(SimInterface& sim, const vec2& position, bool rdir) {
  auto h = create_enemy<Wall>(sim, 4, 20, 10, position);
  h.add(Wall{rdir});
}

namespace {
struct FollowHub : ecs::component {
  static constexpr std::uint32_t kBoundingWidth = 16;
  static constexpr sound kDestroySound = sound::kPlayerDestroy;

  static constexpr std::uint32_t kTimer = 170;
  static constexpr fixed kSpeed = 1;
  static constexpr auto kColour = colour_hue360(240, .7f);

  template <geom::ShapeNode S>
  using fh_arrange = geom::compound<geom::translate<16, 0, S>, geom::translate<-16, 0, S>,
                                    geom::translate<0, 16, S>, geom::translate<0, -16, S>>;
  template <geom::ngon S>
  using r_pi4_ngon = geom::rotate<fixed_c::pi / 4, geom::shape<S>>;
  using fh_centre = r_pi4_ngon<geom::ngon{16, 4, kColour, geom::ngon_style::kPolygram,
                                          shape_flag::kDangerous | shape_flag::kVulnerable}>;
  using fh_spoke = r_pi4_ngon<geom::ngon{8, 4, kColour, geom::ngon_style::kPolygon}>;
  using fh_power_spoke = r_pi4_ngon<geom::ngon{8, 4, kColour, geom::ngon_style::kPolystar}>;
  using shape = geom::translate_p<
      0, fh_centre,
      geom::rotate_p<1, fh_arrange<fh_spoke>, geom::if_p<2, fh_arrange<fh_power_spoke>>>>;

  std::tuple<vec2, fixed, bool> shape_parameters(const Transform& transform) const {
    return {transform.centre, transform.rotation, power_b};
  }

  std::uint32_t timer = 0;
  vec2 dir{0};
  std::uint32_t count = 0;
  bool power_a = false;
  bool power_b = false;

  FollowHub(bool power_a, bool power_b) : power_a{power_a}, power_b{power_b} {}

  void update(Transform& transform, SimInterface& sim) {
    ++timer;
    if (timer > (power_a ? kTimer / 2 : kTimer)) {
      timer = 0;
      ++count;
      if (sim.is_on_screen(transform.centre)) {
        if (power_b) {
          spawn_chaser(sim, transform.centre);
        } else {
          spawn_follow(sim, transform.centre);
        }
        sim.play_sound(sound::kEnemySpawn, transform.centre, true);
      }
    }

    dir = transform.centre.x < 0                    ? vec2{1, 0}
        : transform.centre.x > ii::kSimDimensions.x ? vec2{-1, 0}
        : transform.centre.y < 0                    ? vec2{0, 1}
        : transform.centre.y > ii::kSimDimensions.y ? vec2{0, -1}
        : count > 3                                 ? (count = 0, rotate(dir, -fixed_c::pi / 2))
                                                    : dir;

    auto s = power_a ? fixed_c::hundredth * 5 + fixed_c::tenth : fixed_c::hundredth * 5;
    transform.rotate(s);
    transform.move(dir * kSpeed);
  }

  void on_destroy(const Transform& transform, SimInterface& sim, damage_type type) const {
    if (type == damage_type::kBomb) {
      return;
    }
    if (power_b) {
      spawn_big_follow(sim, transform.centre, true);
    }
    spawn_chaser(sim, transform.centre);
  }
};
}  // namespace

void spawn_follow_hub(SimInterface& sim, const vec2& position, bool power_a, bool power_b) {
  auto h = create_enemy<FollowHub>(sim, 6u + 2 * power_a + 2 * power_b,
                                   50u + power_a * 10 + power_b * 10, 14, position);
  h.add(FollowHub{power_a, power_b});
}

namespace {

struct Shielder : ecs::component {
  static constexpr std::uint32_t kBoundingWidth = 36;
  static constexpr sound kDestroySound = sound::kPlayerDestroy;

  static constexpr std::uint32_t kTimer = 80;
  static constexpr fixed kSpeed = 2;

  static constexpr auto kColour0 = colour_hue360(150, .2f);
  static constexpr auto kColour1 = colour_hue360(160, .5f, .6f);
  template <geom::ShapeNode S>
  using s_arrange = geom::compound<geom::translate<24, 0, S>, geom::translate<-24, 0, S>,
                                   geom::translate<0, 24, geom::rotate<fixed_c::pi / 2, S>>,
                                   geom::translate<0, -24, geom::rotate<fixed_c::pi / 2, S>>>;
  // TODO: better way to swap colours.
  using s_centre = geom::conditional_p<
      3,
      geom::shape<geom::ngon{14, 8, kColour1, geom::ngon_style::kPolygon,
                             ii::shape_flag::kDangerous | ii::shape_flag::kVulnerable}>,
      geom::shape<geom::ngon{14, 8, kColour0, geom::ngon_style::kPolygon,
                             ii::shape_flag::kDangerous | ii::shape_flag::kVulnerable}>>;
  using s_shield0 =
      geom::shape<geom::ngon{8, 6, kColour0, geom::ngon_style::kPolystar, shape_flag::kVulnShield}>;
  using s_shield1 = geom::shape<geom::ngon{8, 6, kColour1}>;
  using s_spokes = geom::shape<geom::ngon{24, 4, kColour0, geom::ngon_style::kPolystar}>;
  using shape = geom::translate_p<0,
                                  geom::rotate_p<1, s_arrange<geom::rotate_p<2, s_shield0>>,
                                                 s_arrange<geom::rotate_p<2, s_shield1>>, s_spokes>,
                                  geom::rotate_p<2, s_centre>>;

  std::tuple<vec2, fixed, fixed, bool> shape_parameters(const Transform& transform) const {
    return {transform.centre, transform.rotation, -transform.rotation, power};
  }

  vec2 dir{0, 1};
  std::uint32_t timer = 0;
  bool rotate = false;
  bool rdir = false;
  bool power = false;

  Shielder(bool power) : power{power} {}

  void update(Transform& transform, const Health& health, SimInterface& sim) {
    fixed s = power ? fixed_c::hundredth * 12 : fixed_c::hundredth * 4;
    transform.rotate(s);

    bool on_screen = false;
    dir = transform.centre.x < 0                    ? vec2{1, 0}
        : transform.centre.x > ii::kSimDimensions.x ? vec2{-1, 0}
        : transform.centre.y < 0                    ? vec2{0, 1}
        : transform.centre.y > ii::kSimDimensions.y ? vec2{0, -1}
                                                    : (on_screen = true, dir);

    if (!on_screen && rotate) {
      timer = 0;
      rotate = false;
    }

    fixed speed = kSpeed + (power ? fixed_c::tenth * 3 : fixed_c::tenth * 2) * (16 - health.hp);
    if (rotate) {
      auto d = ::rotate(dir, (rdir ? 1 : -1) * (kTimer - timer) * fixed_c::pi / (2 * kTimer));
      if (!--timer) {
        rotate = false;
        dir = ::rotate(dir, (rdir ? 1 : -1) * fixed_c::pi / 2);
      }
      transform.move(d * speed);
    } else {
      ++timer;
      if (timer > kTimer * 2) {
        timer = kTimer;
        rotate = true;
        rdir = sim.random(2) != 0;
      }
      if (sim.is_on_screen(transform.centre) && power && timer % kTimer == kTimer / 2) {
        ii::spawn_boss_shot(sim, transform.centre,
                            3 * sim.nearest_player_direction(transform.centre),
                            colour_hue360(160, .5f, .6f));
        sim.play_sound(ii::sound::kBossFire, transform.centre, true);
      }
      transform.move(dir * speed);
    }
    dir = normalise(dir);
  }
};

}  // namespace

void spawn_shielder(SimInterface& sim, const vec2& position, bool power) {
  auto h = create_enemy<Shielder>(sim, 8u + 2 * power, 60u + power * 40, 16, position);
  h.add(Shielder{power});
}

namespace {

const std::uint32_t kTractorTimer = 50;
const fixed kTractorSpeed = 6 * (1_fx / 10);
const fixed kTractorBeamSpeed = 2 + 1_fx / 2;

class Tractor : public ::Enemy {
public:
  Tractor(ii::SimInterface& sim, const vec2& position, bool power = false);
  void update() override;
  void render() const override;

private:
  std::uint32_t timer_ = 0;
  vec2 dir_{0};
  bool power_ = false;

  bool ready_ = false;
  bool spinning_ = false;
  ii::SimInterface::ship_list players_;
};

Tractor::Tractor(ii::SimInterface& sim, const vec2& position, bool power)
: Enemy{sim, position, ii::ship_flag::kNone}, timer_{kTractorTimer * 4}, power_{power} {
  auto c = colour_hue360(300, .5f, .6f);
  add_new_shape<ii::Polygon>(vec2{24, 0}, 12, 6, c, 0,
                             ii::shape_flag::kDangerous | ii::shape_flag::kVulnerable,
                             ii::Polygon::T::kPolygram);
  add_new_shape<ii::Polygon>(vec2{-24, 0}, 12, 6, c, 0,
                             ii::shape_flag::kDangerous | ii::shape_flag::kVulnerable,
                             ii::Polygon::T::kPolygram);
  add_new_shape<ii::Line>(vec2{0, 0}, vec2{24, 0}, vec2{-24, 0}, c);
  if (power) {
    add_new_shape<ii::Polygon>(vec2{24, 0}, 16, 6, c, 0, ii::shape_flag::kNone,
                               ii::Polygon::T::kPolystar);
    add_new_shape<ii::Polygon>(vec2{-24, 0}, 16, 6, c, 0, ii::shape_flag::kNone,
                               ii::Polygon::T::kPolystar);
  }
}

void Tractor::update() {
  shapes()[0]->rotate(fixed_c::hundredth * 5);
  shapes()[1]->rotate(-fixed_c::hundredth * 5);
  if (power_) {
    shapes()[3]->rotate(-fixed_c::hundredth * 8);
    shapes()[4]->rotate(fixed_c::hundredth * 8);
  }

  if (shape().centre.x < 0) {
    dir_ = vec2{1, 0};
  } else if (shape().centre.x > ii::kSimDimensions.x) {
    dir_ = vec2{-1, 0};
  } else if (shape().centre.y < 0) {
    dir_ = vec2{0, 1};
  } else if (shape().centre.y > ii::kSimDimensions.y) {
    dir_ = vec2{0, -1};
  } else {
    ++timer_;
  }

  if (!ready_ && !spinning_) {
    move(dir_ * kTractorSpeed * (is_on_screen() ? 1 : 2 + fixed_c::half));

    if (timer_ > kTractorTimer * 8) {
      ready_ = true;
      timer_ = 0;
    }
  } else if (ready_) {
    if (timer_ > kTractorTimer) {
      ready_ = false;
      spinning_ = true;
      timer_ = 0;
      players_ = sim().players();
      play_sound(ii::sound::kBossFire);
    }
  } else if (spinning_) {
    shape().rotate(fixed_c::tenth * 3);
    for (const auto& ship : players_) {
      if (!((::Player*)ship)->is_killed()) {
        auto d = normalise(shape().centre - ship->position());
        ship->position() += d * kTractorBeamSpeed;
      }
    }

    if (timer_ % (kTractorTimer / 2) == 0 && is_on_screen() && power_) {
      ii::spawn_boss_shot(sim(), shape().centre, 4 * sim().nearest_player_direction(shape().centre),
                          colour_hue360(300, .5f, .6f));
      play_sound_random(ii::sound::kBossFire);
    }

    if (timer_ > kTractorTimer * 5) {
      spinning_ = false;
      timer_ = 0;
    }
  }
}

void Tractor::render() const {
  ::Enemy::render();
  if (spinning_) {
    for (std::size_t i = 0; i < players_.size(); ++i) {
      if (((timer_ + i * 4) / 4) % 2 && !((::Player*)players_[i])->is_killed()) {
        sim().render_line(to_float(shape().centre), to_float(players_[i]->position()),
                          colour_hue360(300, .5f, .6f));
      }
    }
  }
}

}  // namespace

void spawn_tractor(SimInterface& sim, const vec2& position, bool power) {
  auto h = sim.create_legacy(std::make_unique<Tractor>(sim, position, power));
  h.add(legacy_collision(/* bounding width */ 36));
  h.add(Enemy{.threat_value = 10u + 2 * power, .score_reward = 85u + 40 * power});
  h.add(Health{.hp = 50,
               .destroy_sound = ii::sound::kPlayerDestroy,
               .on_destroy = &legacy_enemy_on_destroy});
}

void spawn_boss_shot(SimInterface& sim, const vec2& position, const vec2& velocity,
                     const glm::vec4& c) {
  auto h = sim.create_legacy(std::make_unique<BossShot>(sim, position, velocity, c));
  h.add(legacy_collision(/* bounding width */ 12));
  h.add(Enemy{.threat_value = 1});
  h.add(Health{.hp = 0, .on_destroy = &legacy_enemy_on_destroy});
}

}  // namespace ii

Enemy::Enemy(ii::SimInterface& sim, const vec2& position, ii::ship_flag type)
: ii::Ship{sim, position, type | ii::ship_flag::kEnemy} {}

void Enemy::render() const {
  if (auto c = handle().get<ii::Health>(); c && c->hit_timer) {
    for (std::size_t i = 0; i < shapes().size(); ++i) {
      bool hit_flash = !c->hit_flash_ignore_index || i < *c->hit_flash_ignore_index;
      shapes()[i]->render(sim(), to_float(shape().centre), shape().rotation().to_float(),
                          hit_flash ? std::make_optional(glm::vec4{1.f}) : std::nullopt);
    }
    return;
  }
  Ship::render();
}

BossShot::BossShot(ii::SimInterface& sim, const vec2& position, const vec2& velocity,
                   const glm::vec4& c)
: Enemy{sim, position, ii::ship_flag::kWall}, dir_{velocity} {
  add_new_shape<ii::Polygon>(vec2{0}, 16, 8, c, 0, ii::shape_flag::kNone,
                             ii::Polygon::T::kPolystar);
  add_new_shape<ii::Polygon>(vec2{0}, 10, 8, c, 0);
  add_new_shape<ii::Polygon>(vec2{0}, 12, 8, glm::vec4{0.f}, 0, ii::shape_flag::kDangerous);
}

void BossShot::update() {
  move(dir_);
  vec2 p = shape().centre;
  if ((p.x < -10 && dir_.x < 0) || (p.x > ii::kSimDimensions.x + 10 && dir_.x > 0) ||
      (p.y < -10 && dir_.y < 0) || (p.y > ii::kSimDimensions.y + 10 && dir_.y > 0)) {
    destroy();
  }
  shape().set_rotation(shape().rotation() + fixed_c::hundredth * 2);
}