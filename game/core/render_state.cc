#include "game/core/render_state.h"
#include "game/core/io_input_adapter.h"
#include "game/logic/sim/io/output.h"
#include "game/logic/sim/sim_state.h"
#include "game/mixer/mixer.h"
#include "game/render/gl_renderer.h"
#include <glm/gtc/constants.hpp>
#include <unordered_map>

namespace ii {
namespace {
const std::uint32_t kStarTimer = 500;
}  // namespace

void RenderState::handle_output(ISimState& state, Mixer* mixer, IoInputAdapter* input) {
  struct sound_average {
    std::size_t count = 0;
    float volume = 0.f;
    float pan = 0.f;
    float pitch = 0.f;
  };

  std::unordered_map<std::uint32_t, std::uint32_t> rumble;
  std::unordered_map<sound, sound_average> sound_map;

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
      for (const auto& pair : e.rumble_map) {
        rumble[pair.first] = std::max(rumble[pair.first], pair.second);
      }
      for (std::uint32_t i = 0; e.global_rumble && i < input->player_count(); ++i) {
        rumble[i] = std::max(rumble[i], e.global_rumble);
      }
    }
    e.rumble_map.clear();  // Always clear rumble, since either handling it now or never.
    e.global_rumble = 0;

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

    if (!e.background_fx && e.particles.empty() && e.rumble_map.empty() && !e.global_rumble &&
        e.sounds.empty()) {
      it = output.entries.erase(it);
    } else {
      ++it;
    }
  }

  // Final resolution.
  for (const auto& pair : rumble) {
    // Rumble currently defined in ticks. TODO: plumb through lf/hf.
    auto duration_ms = static_cast<std::uint32_t>(pair.second * 16);
    input->rumble(pair.first, 0x7fff, 0x7fff, duration_ms);
  }

  for (auto& pair : sound_map) {
    auto& e = pair.second;
    e.volume = std::max(0.f, std::min(1.f, e.volume));
    e.pan = e.pan / static_cast<float>(pair.second.count);
    e.pitch = std::pow(2.f, e.pitch);
    mixer->play(static_cast<ii::Mixer::audio_handle_t>(pair.first), e.volume, e.pan, e.pitch);
  }
}

void RenderState::update() {
  for (auto& particle : particles_) {
    if (!particle.timer) {
      continue;
    }
    --particle.timer;
    if (auto* p = std::get_if<dot_particle>(&particle.data)) {
      p->position += p->velocity;
    } else if (auto* p = std::get_if<line_particle>(&particle.data)) {
      p->position += p->velocity;
      p->rotation = normalise_angle(p->rotation + p->angular_velocity);
    }
  }
  std::erase_if(particles_, [](const particle& p) { return !p.timer; });

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
  auto render_box = [&](const glm::vec2& v, const glm::vec2& d, const glm::vec4& c, float lw) {
    render::box box;
    box.origin = v;
    box.dimensions = d;
    box.colour = c;
    box.line_width = lw;
    shapes.emplace_back(render::shape::from(box));
  };

  for (const auto& particle : particles_) {
    if (const auto* p = std::get_if<dot_particle>(&particle.data)) {
      render_box(p->position, glm::vec2{1.5f, 1.5f}, p->colour, .5f);
    } else if (const auto* p = std::get_if<line_particle>(&particle.data)) {
      auto v = from_polar(p->rotation, p->radius);
      render::line line;
      line.colour = p->colour;
      line.a = p->position + v;
      line.b = p->position - v;
      shapes.emplace_back(render::shape::from(line));
    }
  }

  for (const auto& star : stars_) {
    switch (star.type) {
    case star_type::kDotStar:
    case star_type::kFarStar:
      render_box(star.position, glm::vec2{1, 1}, star.colour, 1.f);
      break;

    case star_type::kBigStar:
      render_box(star.position, glm::vec2{2, 2}, star.colour, 1.f);
      break;

    case star_type::kPlanet:
      render::ngon ngon;
      ngon.sides = 8;
      ngon.radius = star.size;
      ngon.colour = star.colour;
      ngon.origin = star.position;
      shapes.emplace_back(render::shape::from(ngon));
      break;
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

}  // namespace ii