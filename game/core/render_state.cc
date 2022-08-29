#include "game/core/render_state.h"
#include "game/core/io_input_adapter.h"
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

void RenderState::handle_output(ISimState& state, Mixer* mixer, IoInputAdapter* input) {
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
      input->rumble(i, r.lf, r.hf, ticks_to_ms(r.time_ticks));
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

void RenderState::update(IoInputAdapter* input) {
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
  for (auto& particle : particles_) {
    if (particle.time == particle.end_time) {
      continue;
    }
    ++particle.time;
    if (auto* p = std::get_if<dot_particle>(&particle.data)) {
      p->position += p->velocity;
    } else if (auto* p = std::get_if<line_particle>(&particle.data)) {
      if (particle.time >= particle.end_time / 3 && particle.time > 4 &&
          !engine_.uint(50u - std::min(48u, static_cast<std::uint32_t>(p->radius)))) {
        auto v = from_polar(p->rotation, p->radius);
        line_particle pn;
        pn.colour = p->colour;
        pn.velocity = p->velocity + normalise(v);
        pn.radius = p->radius / 2.f;
        pn.angular_velocity = p->angular_velocity + glm::pi<float>() / 64.f;
        pn.position = p->position + v / 2.f + pn.velocity;
        pn.rotation = p->rotation + pn.angular_velocity;

        ii::particle ppn;
        ppn.data = pn;
        ppn.time = std::max(4u, particle.time - 4u);
        ppn.end_time = particle.end_time - 1;
        ppn.flash_time = 0;
        ppn.fade = particle.fade;
        new_particles.emplace_back(ppn);

        p->velocity -= normalise(v);
        p->position -= v / 2.f;
        p->angular_velocity -= glm::pi<float>() / 64.f;
        p->radius /= 2.f;
        particle.time = ppn.time;
        --particle.end_time;
      }
      p->position += p->velocity;
      p->rotation = normalise_angle(p->rotation + p->angular_velocity);
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

    auto c0 = colour_hue(0.f, .15f, 0.f);
    auto c1 = colour_hue(0.f, .25f, 0.f);
    auto c2 = colour_hue(0.f, .35f, 0.f);
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

void RenderState::render(render::GlRenderer& r) const {
  std::vector<render::shape> shapes;
  auto render_box = [&](const glm::vec2& v, const glm::vec2& d, const glm::vec4& c, float lw,
                        float z) {
    render::box box;
    box.origin = v;
    box.dimensions = d;
    box.colour = c;
    box.line_width = lw;
    shapes.emplace_back(render::shape::from(box, z));
  };

  for (const auto& star : stars_) {
    switch (star.type) {
    case star_type::kDotStar:
    case star_type::kFarStar:
      render_box(star.position, glm::vec2{1, 1}, star.colour, 1.f, -96.f);
      break;

    case star_type::kBigStar:
      render_box(star.position, glm::vec2{2, 2}, star.colour, 1.f, -96.f);
      break;

    case star_type::kPlanet:
      render::ngon ngon;
      ngon.sides = 8;
      ngon.radius = star.size;
      ngon.colour = star.colour;
      ngon.origin = star.position;
      shapes.emplace_back(render::shape::from(ngon, -96.f));
      break;
    }
  }

  for (const auto& particle : particles_) {
    auto get_colour = [&](glm::vec4 c) {
      float a = particle.time <= particle.flash_time || !particle.fade ? 1.f
                                                                       : .5f *
              (1.f -
               static_cast<float>(particle.time - particle.flash_time - 1) /
                   (particle.end_time - particle.flash_time - 1));
      return glm::vec4{c.x, c.y, particle.time <= particle.flash_time ? 1.f : c.z, a};
    };
    if (const auto* p = std::get_if<dot_particle>(&particle.data)) {
      render_box(p->position, glm::vec2{p->radius, p->radius}, get_colour(p->colour), p->line_width,
                 -64.f);
    } else if (const auto* p = std::get_if<line_particle>(&particle.data)) {
      auto v = from_polar(p->rotation, p->radius);
      float t = std::max(0.f, (17.f - particle.time) / 16.f);
      render::line line;
      line.colour = get_colour(p->colour);
      line.a = p->position + v;
      line.b = p->position - v;
      line.line_width = glm::mix(1.f, 2.f, t);
      shapes.emplace_back(render::shape::from(line, -64.f));
    }
  }

  r.render_shapes(shapes);
}

void RenderState::handle_background_fx(const background_fx_change& change) {
  switch (change.type) {
  case background_fx_type::kStars:
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