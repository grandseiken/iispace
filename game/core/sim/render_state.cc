#include "game/core/sim/render_state.h"
#include "game/common/colour.h"
#include "game/core/sim/input_adapter.h"
#include "game/logic/sim/io/output.h"
#include "game/logic/sim/sim_state.h"
#include "game/mixer/mixer.h"
#include "game/render/gl_renderer.h"
#include <glm/gtc/constants.hpp>
#include <algorithm>
#include <unordered_map>

namespace ii {
namespace {
const std::uint32_t kStarTimer = 500;

std::uint32_t ticks_to_ms(std::uint32_t ticks) {
  // Rumble currently defined in ticks, slight overestimate is fine.
  return ticks * 17u;
}
}  // namespace

void RenderState::handle_output(ISimState& state, Mixer* mixer, SimInputAdapter* input) {
  struct sound_average {
    std::size_t count = 0;
    float volume = 0.f;
    float pan = 0.f;
    float pitch = 0.f;
  };

  std::unordered_map<sound, sound_average> sound_map;
  std::vector<bool> rumbled;
  if (input) {
    rumbled.resize(input->player_count());
    rumble_.resize(input->player_count());
  }

  auto& output = state.output();
  for (auto it = output.entries.begin(); it != output.entries.end();) {
    auto& e = it->e;

    if (e.background_fx) {
      handle_background_fx(*e.background_fx);
      e.background_fx.reset();
    }

    // Particles.
    particles_.insert(particles_.end(), e.particles.begin(), e.particles.end());
    e.particles.clear();

    // Rumble.
    if (input) {
      for (const auto& rumble : e.rumble) {
        auto freq_to_u16 = [](float f) {
          return static_cast<std::uint16_t>(std::clamp(f, 0.f, 1.f) * static_cast<float>(0xffff));
        };
        if (rumble.player_id < input->player_count()) {
          rumble_t r{rumble.time_ticks, freq_to_u16(rumble.lf), freq_to_u16(rumble.hf)};
          rumble_[rumble.player_id].emplace_back(r);
          rumbled[rumble.player_id] = true;
        }
      }
    }
    e.rumble.clear();  // Always clear rumble, since either handling it now or never.

    // Sounds.
    if (mixer) {
      for (const auto& entry : e.sounds) {
        auto& s = sound_map[entry.sound_id];
        ++s.count;
        s.volume += entry.volume;
        s.pan += entry.pan;
        s.pitch = entry.pitch;
      }
      e.sounds.clear();
    }

    if (!e.background_fx && e.particles.empty() && e.sounds.empty() && e.rumble.empty()) {
      it = output.entries.erase(it);
    } else {
      ++it;
    }
  }

  // Final resolution.
  for (std::uint32_t i = 0; input && i < input->player_count(); ++i) {
    if (rumbled[i]) {
      // TODO: screen shake?
      auto r = resolve_rumble(i);
      auto ms = static_cast<std::uint32_t>(
          .5f + ticks_to_ms(r.time_ticks) * static_cast<float>(state.fps()) / 50.f);
      input->rumble(i, r.lf, r.hf, ms);
    }
  }

  for (auto& pair : sound_map) {
    auto& e = pair.second;
    e.volume = std::max(0.f, std::min(1.f, e.volume));
    e.pan = e.pan / static_cast<float>(pair.second.count);
    e.pitch = std::pow(2.f, e.pitch);
    mixer->play(static_cast<ii::Mixer::audio_handle_t>(pair.first), e.volume, e.pan, e.pitch);
  }
}

void RenderState::update(SimInputAdapter* input) {
  if (input) {
    for (std::uint32_t i = 0; i < rumble_.size(); ++i) {
      bool rumbled = false;
      for (auto& r : rumble_[i]) {
        r.time_ticks && --r.time_ticks;
        rumbled |= !r.time_ticks;
      }
      if (rumbled) {
        std::erase_if(rumble_[i], [](const rumble_t& r) { return !r.time_ticks; });
        auto r = resolve_rumble(i);
        input->rumble(i, r.lf, r.hf, ticks_to_ms(r.time_ticks));
      }
    }
  }

  std::vector<particle> new_particles;
  for (auto& p : particles_) {
    if (p.time == p.end_time) {
      continue;
    }
    ++p.time;
    if (std::holds_alternative<dot_particle>(p.data)) {
      p.position += p.velocity;
    } else if (auto* d = std::get_if<line_particle>(&p.data)) {
      if (p.time >= p.end_time / 3 && p.time > 4 && d->radius > 2 * d->width &&
          !engine_.uint(50u - std::min(48u, static_cast<std::uint32_t>(d->radius)))) {
        auto v = from_polar(d->rotation, d->radius);

        d->radius /= 2.f;
        line_particle d0 = *d;
        d->angular_velocity -= glm::pi<float>() / 64.f;
        d0.angular_velocity += glm::pi<float>() / 64.f;
        d0.rotation = d->rotation + d0.angular_velocity;

        p.time = std::max(4u, p.time - 4u);
        --p.end_time;
        p.flash_time = 0;
        particle p0 = p;
        p0.data = d0;

        p.velocity -= normalize(v);
        p.position -= v / 2.f;
        p0.velocity += normalize(v);
        p0.position += v / 2.f + p0.velocity;
        new_particles.emplace_back(p0);
      }
      p.position += p.velocity;
      d->rotation = normalise_angle(d->rotation + d->angular_velocity);
    }
  }
  particles_.insert(particles_.end(), new_particles.begin(), new_particles.end());
  std::erase_if(particles_, [](const particle& p) { return p.time == p.end_time; });

  auto create_star = [&] {
    auto r = engine_.uint(12);
    if (r <= 0 && engine_.uint(4)) {
      return;
    }

    auto t = r <= 0 ? star_type::kPlanet
        : r <= 3    ? star_type::kBigStar
        : r <= 7    ? star_type::kFarStar
                    : star_type::kDotStar;
    float speed = t == star_type::kDotStar ? 18.f : t == star_type::kBigStar ? 14.f : 10.f;

    star_data star;
    star.timer = kStarTimer;
    star.type = t;
    star.speed = speed;

    auto edge = engine_.uint(4);
    float ratio = engine_.fixed().to_float();

    star.position.x = edge < 2 ? ratio * dimensions_.x : edge == 2 ? -16 : 16 + dimensions_.x;
    star.position.y = edge >= 2 ? ratio * dimensions_.y : edge == 0 ? -16 : 16 + dimensions_.y;

    auto c0 = colour::hue(0.f, .15f, 0.f);
    auto c1 = colour::hue(0.f, .25f, 0.f);
    auto c2 = colour::hue(0.f, .35f, 0.f);
    star.colour = t == star_type::kDotStar ? (engine_.rbool() ? c1 : c2)
        : t == star_type::kFarStar         ? (engine_.rbool() ? c1 : c0)
        : t == star_type::kBigStar         ? (engine_.rbool() ? c0 : c1)
                                           : c0;
    if (t == star_type::kPlanet) {
      star.size = 4.f + engine_.uint(4);
    }
    stars_.emplace_back(star);
  };

  bool destroy = false;
  for (auto& star : stars_) {
    star.position += star_direction_ * star.speed;
    destroy |= !--star.timer;
  }
  if (destroy) {
    std::erase_if(stars_, [](const auto& s) { return !s.timer; });
  }

  auto r = star_rate_ > 1 ? engine_.uint(star_rate_) : 0u;
  for (std::uint32_t i = 0; i < r; ++i) {
    create_star();
  }
}

void RenderState::render(render::GlRenderer& r, std::uint64_t tick_count,
                         std::vector<render::shape>& shapes) const {
  switch (bgfx_type_) {
  case background_fx_type::kNone:
  case background_fx_type::kLegacy_Stars:
    break;
  case background_fx_type::kBiome0: {
    auto c = colour::kSolarizedDarkBase03;
    c.z /= 1.5f;
    r.render_background(tick_count, c);
  } break;
  }

  auto render_box = [&](const glm::vec2& v, const glm::vec2& vv, const glm::vec2& d,
                        const glm::vec4& c, float lw, float z) {
    shapes.emplace_back(render::shape{
        .origin = v,
        .colour = c,
        .z_index = z,
        .trail = render::motion_trail{.prev_origin = v - vv, .prev_colour = c},
        .data = render::box{.dimensions = d, .line_width = lw},
    });
  };

  for (const auto& star : stars_) {
    switch (star.type) {
    case star_type::kDotStar:
    case star_type::kFarStar:
      render_box(star.position, star_direction_ * star.speed, glm::vec2{1, 1}, star.colour, 1.f,
                 colour::kZParticle);
      break;

    case star_type::kBigStar:
      render_box(star.position, star_direction_ * star.speed, glm::vec2{2, 2}, star.colour, 1.f,
                 colour::kZParticle);
      break;

    case star_type::kPlanet:
      shapes.emplace_back(render::shape{
          .origin = star.position,
          .colour = star.colour,
          .z_index = colour::kZParticle,
          .trail = render::motion_trail{.prev_origin = star.position - star_direction_ * star.speed,
                                        .prev_colour = star.colour},
          .data = render::ngon{.radius = star.size, .sides = 8},
      });
      break;
    }
  }

  for (const auto& p : particles_) {
    float a = p.time <= p.flash_time || !p.fade ? 1.f
                                                : .5f *
            (1.f - static_cast<float>(p.time - p.flash_time - 1) / (p.end_time - p.flash_time - 1));
    glm::vec4 colour{p.colour.x, p.colour.y, p.time <= p.flash_time ? 1.f : p.colour.z, a};

    if (const auto* d = std::get_if<dot_particle>(&p.data)) {
      render_box(p.position, p.velocity, glm::vec2{d->radius, d->radius}, colour, d->line_width,
                 colour::kZParticle);
    } else if (const auto* d = std::get_if<line_particle>(&p.data)) {
      float t = std::max(0.f, (17.f - p.time) / 16.f);
      shapes.emplace_back(render::shape{
          .origin = p.position,
          .rotation = d->rotation,
          .colour = colour,
          .z_index = colour::kZParticle,
          .trail = render::motion_trail{.prev_origin = p.position - p.velocity,
                                        .prev_rotation = d->rotation - d->angular_velocity,
                                        .prev_colour = colour},
          .data = render::line{.radius = d->radius,
                               .line_width = d->width * glm::mix(1.f, 2.f, t),
                               .sides = 3 + static_cast<std::uint32_t>(.5f + d->width)},
      });
    }
  }
}

void RenderState::handle_background_fx(const background_fx_change& change) {
  bgfx_type_ = change.type;
  switch (bgfx_type_) {
  case background_fx_type::kNone:
  case background_fx_type::kBiome0:
    break;
  case background_fx_type::kLegacy_Stars:
    star_direction_ =
        rotate(star_direction_, (engine_.fixed().to_float() - .5f) * glm::pi<float>());
    for (auto& star : stars_) {
      star.timer = kStarTimer;
    }
    star_rate_ = engine_.uint(3) + 2;
    break;
  }
}

auto RenderState::resolve_rumble(std::uint32_t player) const -> rumble_t {
  rumble_t result;
  if (player < rumble_.size()) {
    for (const auto& r : rumble_[player]) {
      bool gt = r.lf > result.lf || r.hf > result.hf;
      bool ge = r.lf >= result.lf && r.hf >= result.hf;
      if (gt && ge) {
        result.time_ticks = r.time_ticks;
      } else if (ge) {
        result.time_ticks = std::max(result.time_ticks, r.time_ticks);
      } else if (gt) {
        result.time_ticks = std::min(result.time_ticks, r.time_ticks);
      }
      result.lf = std::max(result.lf, r.lf);
      result.hf = std::max(result.hf, r.hf);
    }
  }
  return result;
}

}  // namespace ii
