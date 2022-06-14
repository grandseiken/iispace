#include "game/logic/enemy.h"
#include "game/common/functional.h"
#include "game/logic/player.h"
#include "game/logic/ship/shape_v2.h"
#include <algorithm>

namespace {

const std::uint32_t kShielderTimer = 80;
const fixed kShielderSpeed = 2;

const std::uint32_t kTractorTimer = 50;
const fixed kTractorSpeed = 6 * (1_fx / 10);
const fixed kTractorBeamSpeed = 2 + 1_fx / 2;

class Shielder : public Enemy {
public:
  Shielder(ii::SimInterface& sim, const vec2& position, bool power = false);
  void update() override;

private:
  vec2 dir_{0};
  std::uint32_t timer_ = 0;
  bool rotate_ = false;
  bool rdir_ = false;
  bool power_ = false;
};

class Tractor : public Enemy {
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

Shielder::Shielder(ii::SimInterface& sim, const vec2& position, bool power)
: Enemy{sim, position, ii::ship_flag::kNone}, dir_{0, 1}, power_{power} {
  auto c0 = colour_hue360(150, .2f);
  auto c1 = colour_hue360(160, .5f, .6f);
  add_new_shape<ii::Polygon>(vec2{24, 0}, 8, 6, c0, 0, ii::shape_flag::kVulnShield,
                             ii::Polygon::T::kPolystar);
  add_new_shape<ii::Polygon>(vec2{-24, 0}, 8, 6, c0, 0, ii::shape_flag::kVulnShield,
                             ii::Polygon::T::kPolystar);
  add_new_shape<ii::Polygon>(vec2{0, 24}, 8, 6, c0, fixed_c::pi / 2, ii::shape_flag::kVulnShield,
                             ii::Polygon::T::kPolystar);
  add_new_shape<ii::Polygon>(vec2{0, -24}, 8, 6, c0, fixed_c::pi / 2, ii::shape_flag::kVulnShield,
                             ii::Polygon::T::kPolystar);
  add_new_shape<ii::Polygon>(vec2{24, 0}, 8, 6, c1, 0);
  add_new_shape<ii::Polygon>(vec2{-24, 0}, 8, 6, c1, 0);
  add_new_shape<ii::Polygon>(vec2{0, 24}, 8, 6, c1, fixed_c::pi / 2);
  add_new_shape<ii::Polygon>(vec2{0, -24}, 8, 6, c1, fixed_c::pi / 2);

  add_new_shape<ii::Polygon>(vec2{0, 0}, 24, 4, c0, 0, ii::shape_flag::kNone,
                             ii::Polygon::T::kPolystar);
  add_new_shape<ii::Polygon>(vec2{0, 0}, 14, 8, power ? c1 : c0, 0,
                             ii::shape_flag::kDangerous | ii::shape_flag::kVulnerable);
}

void Shielder::update() {
  fixed s = power_ ? fixed_c::hundredth * 12 : fixed_c::hundredth * 4;
  shape().rotate(s);
  shapes()[9]->rotate(-2 * s);
  for (std::uint32_t i = 0; i < 8; ++i) {
    shapes()[i]->rotate(-s);
  }

  bool on_screen = false;
  dir_ = shape().centre.x < 0                   ? vec2{1, 0}
      : shape().centre.x > ii::kSimDimensions.x ? vec2{-1, 0}
      : shape().centre.y < 0                    ? vec2{0, 1}
      : shape().centre.y > ii::kSimDimensions.y ? vec2{0, -1}
                                                : (on_screen = true, dir_);

  if (!on_screen && rotate_) {
    timer_ = 0;
    rotate_ = false;
  }

  fixed speed = kShielderSpeed +
      (power_ ? fixed_c::tenth * 3 : fixed_c::tenth * 2) * (16 - handle().get<ii::Health>()->hp);
  if (rotate_) {
    auto d = rotate(
        dir_, (rdir_ ? 1 : -1) * (kShielderTimer - timer_) * fixed_c::pi / (2 * kShielderTimer));
    if (!--timer_) {
      rotate_ = false;
      dir_ = rotate(dir_, (rdir_ ? 1 : -1) * fixed_c::pi / 2);
    }
    move(d * speed);
  } else {
    ++timer_;
    if (timer_ > kShielderTimer * 2) {
      timer_ = kShielderTimer;
      rotate_ = true;
      rdir_ = sim().random(2) != 0;
    }
    if (is_on_screen() && power_ && timer_ % kShielderTimer == kShielderTimer / 2) {
      ii::spawn_boss_shot(sim(), shape().centre, 3 * sim().nearest_player_direction(shape().centre),
                          colour_hue360(160, .5f, .6f));
      play_sound_random(ii::sound::kBossFire);
    }
    move(dir_ * speed);
  }
  dir_ = normalise(dir_);
}

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
      if (!((Player*)ship)->is_killed()) {
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
  Enemy::render();
  if (spinning_) {
    for (std::size_t i = 0; i < players_.size(); ++i) {
      if (((timer_ + i * 4) / 4) % 2 && !((Player*)players_[i])->is_killed()) {
        sim().render_line(to_float(shape().centre), to_float(players_[i]->position()),
                          colour_hue360(300, .5f, .6f));
      }
    }
  }
}

}  // namespace

namespace ii {
namespace {

template <shape::ShapeNode S>
using standard_transform = shape::translate_p<0, shape::rotate_p<1, S>>;
std::tuple<vec2, fixed> transform_parameters(ecs::const_handle h) {
  auto& transform = *h.get<Transform>();
  return {transform.centre, transform.rotation};
}

template <shape::ShapeNode S>
void render_enemy(const SimInterface& sim, ecs::const_handle h) {
  auto c_override = h.get<Render>()->colour_override;
  if (!c_override && h.has<Health>() && h.get<Health>()->hit_timer) {
    c_override = glm::vec4{1.f};
  }
  S::iterate(shape::iterate_lines, transform_parameters(h), {},
             [&](const vec2& a, const vec2& b, const glm::vec4& c) {
               sim.render_line(to_float(a), to_float(b), c_override ? *c_override : c);
             });
}

template <shape::ShapeNode S>
void explode_on_destroy(SimInterface& sim, ecs::const_handle h, damage_type) {
  S::iterate(shape::iterate_centres, transform_parameters(h), {},
             [&](const vec2& v, const glm::vec4& c) { sim.explosion(to_float(v), c); });
}

template <shape::ShapeNode S>
bool ship_check_point(ecs::const_handle h, const vec2& v, shape_flag mask) {
  return S::check_point(transform_parameters(h), v, mask);
}

template <ecs::Component C>
void update_function(SimInterface& sim, ecs::handle h) {
  h.get<C>()->update(sim, h);
}

template <ecs::Component Logic>
ecs::handle create_enemy(SimInterface& sim, std::uint32_t threat_value, std::uint32_t score_reward,
                         ship_flag flags = ship_flag::kNone) {
  auto h = sim.index().create();
  h.add(ShipFlag{.flags = ship_flag::kEnemy | flags});
  h.add(LegacyShip{.ship = std::make_unique<ShipForwarder>(sim, h)});
  h.add(Update{.update = &update_function<Logic>});
  h.add(Enemy{.threat_value = threat_value, .score_reward = score_reward});
  return h;
}

template <shape::ShapeNode Shape>
void add_enemy_shape(ecs::handle h, std::uint32_t bounding_width, const vec2& position,
                     fixed rotation = 0) {
  h.add(Transform{.centre = position, .rotation = rotation});
  h.add(Collision{.bounding_width = bounding_width, .check = &ship_check_point<Shape>});
  h.add(Render{.render = &render_enemy<Shape>});
}

}  // namespace

namespace {
using follow_shape =
    standard_transform<shape::ngon<{.radius = 10,
                                    .sides = 4,
                                    .colour = colour_hue360(270, .6f),
                                    .flags = shape_flag::kDangerous | shape_flag::kVulnerable}>>;

using big_follow_shape =
    standard_transform<shape::ngon<{.radius = 20,
                                    .sides = 4,
                                    .colour = colour_hue360(270, .6f),
                                    .flags = shape_flag::kDangerous | shape_flag::kVulnerable}>>;

struct Follow : ecs::component {
  static constexpr std::uint32_t kTime = 90;
  static constexpr fixed kSpeed = 2;

  std::uint32_t timer = 0;
  IShip* target = nullptr;

  void update(SimInterface& sim, ecs::handle h) {
    auto& transform = *h.get<Transform>();
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
};

void big_follow_on_destroy(SimInterface& sim, ecs::const_handle h, damage_type type) {
  if (type == damage_type::kBomb) {
    return;
  }
  auto& transform = *h.get<Transform>();
  vec2 d = rotate(vec2{10, 0}, transform.rotation);
  for (std::uint32_t i = 0; i < 3; ++i) {
    spawn_follow(sim, transform.centre + d, h.get<Enemy>()->score_reward != 0);
    d = rotate(d, 2 * fixed_c::pi / 3);
  }
}
}  // namespace

void spawn_follow(SimInterface& sim, const vec2& position, bool has_score, fixed rotation) {
  auto h = create_enemy<Follow>(sim, 1, has_score ? 15u : 0);
  add_enemy_shape<follow_shape>(h, 10, position, rotation);
  h.add(Follow{});
  h.add(Health{.hp = 1,
               .destroy_sound = ii::sound::kEnemyShatter,
               .on_destroy = &explode_on_destroy<follow_shape>});
}

void spawn_big_follow(SimInterface& sim, const vec2& position, bool has_score) {
  auto h = create_enemy<Follow>(sim, 3, has_score ? 20u : 0);
  add_enemy_shape<big_follow_shape>(h, 10, position);
  h.add(Follow{});
  h.add(Health{.hp = 3,
               .destroy_sound = ii::sound::kPlayerDestroy,
               .on_destroy =
                   sequence<&explode_on_destroy<big_follow_shape>, &big_follow_on_destroy>});
}

namespace {
using chaser_shape =
    standard_transform<shape::ngon<{.radius = 10,
                                    .sides = 4,
                                    .colour = colour_hue360(210, .6f),
                                    .style = shape::ngon_style::kPolygram,
                                    .flags = shape_flag::kDangerous | shape_flag::kVulnerable}>>;

struct Chaser : ecs::component {
  static constexpr std::uint32_t kTime = 60;
  static constexpr fixed kSpeed = 4;

  bool move = false;
  std::uint32_t timer = kTime;
  vec2 dir{0};

  void update(SimInterface& sim, ecs::handle h) {
    auto& transform = *h.get<Transform>();
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
  auto h = create_enemy<Chaser>(sim, 2, 30);
  add_enemy_shape<chaser_shape>(h, 10, position);
  h.add(Chaser{});
  h.add(Health{.hp = 2,
               .destroy_sound = ii::sound::kEnemyShatter,
               .on_destroy = &explode_on_destroy<chaser_shape>});
}

namespace {
using square_shape =
    standard_transform<shape::box<{.width = 10,
                                   .height = 10,
                                   .colour = colour_hue360(120, .6f),
                                   .flags = shape_flag::kDangerous | shape_flag::kVulnerable}>>;

struct Square : ecs::component {
  static constexpr fixed kSpeed = 2 + 1_fx / 4;

  vec2 dir{0};
  std::uint32_t timer = 0;

  Square(SimInterface& sim, fixed dir_angle)
  : dir{from_polar(dir_angle, 1_fx)}, timer{sim.random(80) + 40} {}

  void update(SimInterface& sim, ecs::handle h) {
    auto& transform = *h.get<Transform>();
    if (sim.is_on_screen(transform.centre) && !sim.get_non_wall_count()) {
      if (timer) {
        --timer;
      }
    } else {
      timer = sim.random(80) + 40;
    }

    if (!timer) {
      h.get<Health>()->damage(sim, h, 4, damage_type::kNone, std::nullopt);
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
      h.get<Render>()->colour_override = colour_hue(0.f, .2f, 0.f);
    } else {
      h.get<Render>()->colour_override.reset();
    }
  }
};
}  // namespace

void spawn_square(SimInterface& sim, const vec2& position, fixed dir_angle) {
  auto h = create_enemy<Square>(sim, 2, 25, ship_flag::kWall);
  add_enemy_shape<square_shape>(h, 15, position);
  h.add(Square{sim, dir_angle});
  h.add(Health{.hp = 4, .on_destroy = &explode_on_destroy<square_shape>});
}

namespace {
using wall_shape =
    standard_transform<shape::box<{.width = 10,
                                   .height = 40,
                                   .colour = colour_hue360(120, .5f, .6f),
                                   .flags = shape_flag::kDangerous | shape_flag::kVulnerable}>>;

struct Wall : ecs::component {
  static constexpr std::uint32_t kTimer = 80;
  static constexpr fixed kSpeed = 1 + 1_fx / 4;

  vec2 dir{0, 1};
  std::uint32_t timer = 0;
  bool is_rotating = false;
  bool rdir = false;

  Wall(bool rdir) : rdir{rdir} {}

  void update(SimInterface& sim, ecs::handle h) {
    if (!sim.get_non_wall_count() && timer % 8 < 2) {
      auto& health = *h.get<Health>();
      if (health.hp > 2) {
        sim.play_sound(sound::kEnemySpawn, 1.f, 0.f);
      }
      health.damage(sim, h, std::max(2u, health.hp) - 2, damage_type::kNone, std::nullopt);
    }

    auto& transform = *h.get<Transform>();
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
};

void wall_on_destroy(SimInterface& sim, ecs::const_handle h, damage_type type) {
  if (type == damage_type::kBomb) {
    return;
  }

  const auto& transform = *h.get<Transform>();
  auto d = rotate(h.get<Wall>()->dir, fixed_c::pi / 2);
  auto v = transform.centre + d * 10 * 3;
  if (sim.is_on_screen(v)) {
    spawn_square(sim, v, transform.rotation);
  }
  v = transform.centre - d * 10 * 3;
  if (sim.is_on_screen(v)) {
    spawn_square(sim, v, transform.rotation);
  }
}
}  // namespace

void spawn_wall(SimInterface& sim, const vec2& position, bool rdir) {
  auto h = create_enemy<Wall>(sim, 4, 20, ship_flag::kWall);
  add_enemy_shape<wall_shape>(h, 50, position);
  h.add(Wall{rdir});
  h.add(
      Health{.hp = 10, .on_destroy = sequence<&explode_on_destroy<wall_shape>, &wall_on_destroy>});
}

namespace {
constexpr glm::vec4 follow_hub_colour = colour_hue360(240, .7f);
template <shape::ngon_data S>
using r_pi4_ngon = shape::rotate<fixed_c::pi / 4, shape::ngon<S>>;
using fh_centre = r_pi4_ngon<{.radius = 16,
                              .sides = 4,
                              .colour = follow_hub_colour,
                              .style = shape::ngon_style::kPolygram,
                              .flags = shape_flag::kDangerous | shape_flag::kVulnerable}>;
using fh_spoke = r_pi4_ngon<{
    .radius = 8, .sides = 4, .colour = follow_hub_colour, .style = shape::ngon_style::kPolygon}>;
using fh_power_spoke = r_pi4_ngon<{
    .radius = 8, .sides = 4, .colour = follow_hub_colour, .style = shape::ngon_style::kPolystar}>;
template <typename T>
using fh_arrange = shape::compound<shape::translate<16, 0, T>, shape::translate<-16, 0, T>,
                                   shape::translate<0, 16, T>, shape::translate<0, -16, T>>;
using follow_hub_shape = shape::translate_p<0, fh_centre, shape::rotate_p<1, fh_arrange<fh_spoke>>>;
using follow_hub_power_shape =
    shape::translate_p<0, fh_centre,
                       shape::rotate_p<1, fh_arrange<fh_spoke>, fh_arrange<fh_power_spoke>>>;

struct FollowHub : ecs::component {
  static constexpr std::uint32_t kTimer = 170;
  static constexpr fixed kSpeed = 1;

  std::uint32_t timer = 0;
  vec2 dir{0};
  std::uint32_t count = 0;
  bool power_a = false;
  bool power_b = false;

  FollowHub(bool power_a, bool power_b) : power_a{power_a}, power_b{power_b} {}

  void update(SimInterface& sim, ecs::handle h) {
    auto& transform = *h.get<Transform>();
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
};

void follow_hub_on_destroy(SimInterface& sim, ecs::const_handle h, damage_type type) {
  if (type == damage_type::kBomb) {
    return;
  }
  const auto& c = *h.get<FollowHub>();
  const auto& transform = *h.get<Transform>();
  if (c.power_b) {
    spawn_big_follow(sim, transform.centre, true);
  }
  spawn_chaser(sim, transform.centre);
}
}  // namespace

void spawn_follow_hub(SimInterface& sim, const vec2& position, bool power_a, bool power_b) {
  auto h = create_enemy<FollowHub>(sim, 6u + 2 * power_a + 2 * power_b,
                                   50u + power_a * 10 + power_b * 10);
  h.add(FollowHub{power_a, power_b});
  if (power_b) {
    add_enemy_shape<follow_hub_power_shape>(h, 16, position);
    h.add(
        Health{.hp = 14,
               .destroy_sound = ii::sound::kPlayerDestroy,
               .on_destroy =
                   sequence<&explode_on_destroy<follow_hub_power_shape>, &follow_hub_on_destroy>});
  } else {
    add_enemy_shape<follow_hub_shape>(h, 16, position);
    h.add(Health{.hp = 14,
                 .destroy_sound = ii::sound::kPlayerDestroy,
                 .on_destroy =
                     sequence<&explode_on_destroy<follow_hub_shape>, &follow_hub_on_destroy>});
  }
}

void legacy_enemy_on_destroy(SimInterface&, ecs::handle h, damage_type type) {
  auto enemy = static_cast<::Enemy*>(h.get<LegacyShip>()->ship.get());
  enemy->explosion();
  enemy->on_destroy(type == damage_type::kBomb);
}

void spawn_shielder(SimInterface& sim, const vec2& position, bool power) {
  auto h = sim.create_legacy(std::make_unique<Shielder>(sim, position, power));
  h.add(legacy_collision(/* bounding width */ 36));
  h.add(Enemy{.threat_value = 8u + 2 * power, .score_reward = 60u + power * 40});
  h.add(Health{.hp = 16,
               .destroy_sound = ii::sound::kPlayerDestroy,
               .on_destroy = &legacy_enemy_on_destroy});
}

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